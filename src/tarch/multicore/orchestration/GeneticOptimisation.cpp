#include "GeneticOptimisation.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/Core.h"
#include <limits>


tarch::multicore::orchestration::GeneticOptimisation::Key::Key(
  int  numberOfTasksToHoldBack_,
  std::pair<int,int> tasksToFuseAndTargetDevice_,
  bool fuseTasksImmediatelyWhenSpawned_,
  bool processPendingTasksWhileWaiting_
):
  numberOfTasksToHoldBack(numberOfTasksToHoldBack_),
  tasksToFuseAndTargetDevice(tasksToFuseAndTargetDevice_),
  fuseTasksImmediatelyWhenSpawned(fuseTasksImmediatelyWhenSpawned_),
  processPendingTasksWhileWaiting(processPendingTasksWhileWaiting_) {
}


bool tarch::multicore::orchestration::GeneticOptimisation::Key::operator<(const Key& other) const {
  return numberOfTasksToHoldBack<other.numberOfTasksToHoldBack
      or (numberOfTasksToHoldBack==other.numberOfTasksToHoldBack and tasksToFuseAndTargetDevice.first<other.tasksToFuseAndTargetDevice.first)
      or (numberOfTasksToHoldBack==other.numberOfTasksToHoldBack and tasksToFuseAndTargetDevice.first==other.tasksToFuseAndTargetDevice.first and tasksToFuseAndTargetDevice.second<other.tasksToFuseAndTargetDevice.second)
      or (numberOfTasksToHoldBack==other.numberOfTasksToHoldBack and tasksToFuseAndTargetDevice.first==other.tasksToFuseAndTargetDevice.first and tasksToFuseAndTargetDevice.second==other.tasksToFuseAndTargetDevice.second and fuseTasksImmediatelyWhenSpawned<other.fuseTasksImmediatelyWhenSpawned)
      or (numberOfTasksToHoldBack==other.numberOfTasksToHoldBack and tasksToFuseAndTargetDevice.first==other.tasksToFuseAndTargetDevice.first and tasksToFuseAndTargetDevice.second==other.tasksToFuseAndTargetDevice.second and fuseTasksImmediatelyWhenSpawned==other.fuseTasksImmediatelyWhenSpawned and processPendingTasksWhileWaiting<other.processPendingTasksWhileWaiting);
}


std::string tarch::multicore::orchestration::GeneticOptimisation::Key::toString() const {
  std::ostringstream msg;
  msg << "(hold-back=" << numberOfTasksToHoldBack
      << ",fuse=" << tasksToFuseAndTargetDevice.first
      << ",device=" << tasksToFuseAndTargetDevice.second
      << ",fuse-immediately=" << fuseTasksImmediatelyWhenSpawned
      << ",process-pending=" << processPendingTasksWhileWaiting
      << ")";
  return msg.str();
}


tarch::multicore::orchestration::GeneticOptimisation::Value::Value(double probability_):
  probability(probability_) {
}


tarch::logging::Log tarch::multicore::orchestration::GeneticOptimisation::_log( "tarch::multicore::orchestration::GeneticOptimisation" );


tarch::multicore::orchestration::GeneticOptimisation::GeneticOptimisation():
  _activeKey(
    0, std::pair<int,int>(0,0), false, false
  ),
  _currentWatch( "tarch::multicore::orchestration::GeneticOptimisation", "GeneticOptimisation", false, false ),
  _lastDuration(-1.0) {
  createInitialConfigurations();
  normalise();
}


void tarch::multicore::orchestration::GeneticOptimisation::createInitialConfigurations() {
  const int MaxInitialRage = 16;
  for (int numberOfTasksToHoldBack = 0; numberOfTasksToHoldBack<=MaxInitialRage; numberOfTasksToHoldBack++)
  for (int tasksToFuse = 0; tasksToFuse<=MaxInitialRage; tasksToFuse++)
  for (int targetDevice = -1; targetDevice<=tarch::multicore::Core::getInstance().getNumberOfGPUs(); targetDevice++)
  for (int fuseTasksImmediatelyWhenSpawned = 0; fuseTasksImmediatelyWhenSpawned<2; fuseTasksImmediatelyWhenSpawned++)
  for (int processPendingTasksWhileWaiting = 0; processPendingTasksWhileWaiting<2; processPendingTasksWhileWaiting++) {
    _configurationVariants.insert(std::pair<Key,Value>(
      Key(
        numberOfTasksToHoldBack,
        std::pair<int,int>(tasksToFuse,targetDevice),
        fuseTasksImmediatelyWhenSpawned==0,
        processPendingTasksWhileWaiting==0
      ),
      Value(1.0)
    ));
  }
}


void tarch::multicore::orchestration::GeneticOptimisation::normalise() {
  double sumOfProbabilities = 0.0;
  for (auto& p: _configurationVariants) {
    sumOfProbabilities += p.second.probability;
  }
  logInfo( "normalise()", "have " << _configurationVariants.size() << " potential configurations. Renormalise total total probability of " << sumOfProbabilities );
  for (auto& p: _configurationVariants) {
    p.second.probability /= sumOfProbabilities;
  }
}


void tarch::multicore::orchestration::GeneticOptimisation::startBSPSection() {
  _currentWatch.start();
}


void tarch::multicore::orchestration::GeneticOptimisation::endBSPSection() {
  _currentWatch.stop();
  double newValue = _currentWatch.getCalendarTime();

  constexpr double Min = 1e-8;
  if (newValue>Min) {
    // update stats
    _configurationVariants.at(_activeKey).measurement.setValue(newValue);

    // update probabilities. Increase local ones if appropriate and then re-normalise
    if ( _lastDuration>Min and _lastDuration>_configurationVariants.at(_activeKey).measurement.getValue() ) {
      increaseProbabilityOfActiveConfiguration();
      removeUnreasonableProbabilities();
      normalise();
    }
    _lastDuration = _configurationVariants.at(_activeKey).measurement.getValue();

    pickNewActiveConfigurationRandomly();
  }
}


void tarch::multicore::orchestration::GeneticOptimisation::increaseProbabilityOfActiveConfiguration() {
  Key left(
    _activeKey.numberOfTasksToHoldBack-1,
    std::pair<int,int>( _activeKey.tasksToFuseAndTargetDevice.first, _activeKey.tasksToFuseAndTargetDevice.second),
    _activeKey.fuseTasksImmediatelyWhenSpawned,
    _activeKey.processPendingTasksWhileWaiting
  );
  Key right(
    _activeKey.numberOfTasksToHoldBack-1,
    std::pair<int,int>( _activeKey.tasksToFuseAndTargetDevice.first, _activeKey.tasksToFuseAndTargetDevice.second),
    _activeKey.fuseTasksImmediatelyWhenSpawned,
    _activeKey.processPendingTasksWhileWaiting
  );
  Key down(
    _activeKey.numberOfTasksToHoldBack,
    std::pair<int,int>( _activeKey.tasksToFuseAndTargetDevice.first-1, _activeKey.tasksToFuseAndTargetDevice.second),
    _activeKey.fuseTasksImmediatelyWhenSpawned,
    _activeKey.processPendingTasksWhileWaiting
  );
  Key up(
    _activeKey.numberOfTasksToHoldBack,
    std::pair<int,int>( _activeKey.tasksToFuseAndTargetDevice.first+1, _activeKey.tasksToFuseAndTargetDevice.second),
    _activeKey.fuseTasksImmediatelyWhenSpawned,
    _activeKey.processPendingTasksWhileWaiting
  );
  if ( _configurationVariants.count(left)==0 ) {
    _configurationVariants.insert( std::pair<Key, Value>(left,Value(_configurationVariants.at(_activeKey).probability)) );
    logInfo( "increaseProbabilityOfActiveConfiguration()", "insert new potential configuration " << left.toString() );
  }
  if ( _configurationVariants.count(right)==0 ) {
    _configurationVariants.insert( std::pair<Key, Value>(right,Value(_configurationVariants.at(_activeKey).probability)) );
    logInfo( "increaseProbabilityOfActiveConfiguration()", "insert new potential configuration " << right.toString() );
  }
  if ( _configurationVariants.count(down)==0 ) {
    _configurationVariants.insert( std::pair<Key, Value>(down,Value(_configurationVariants.at(_activeKey).probability)) );
    logInfo( "increaseProbabilityOfActiveConfiguration()", "insert new potential configuration " << down.toString() );
  }
  if ( _configurationVariants.count(up)==0 ) {
    _configurationVariants.insert( std::pair<Key, Value>(up,Value(_configurationVariants.at(_activeKey).probability)) );
    logInfo( "increaseProbabilityOfActiveConfiguration()", "insert new potential configuration " << up.toString() );
  }

  _configurationVariants.at(_activeKey).probability *= 1.1;
}


void tarch::multicore::orchestration::GeneticOptimisation::removeUnreasonableProbabilities() {
  constexpr double MinValue       = 1e-8;

  double MinProbability = 0.1 / _configurationVariants.size();

  std::map< Key, Value >::iterator p = _configurationVariants.begin();
  while (p!=_configurationVariants.end()) {
    if (p->second.measurement.getValue() > MinProbability and p->second.probability <= MinProbability ) {
      logInfo( "removeUnreasonableProbabilities()", "remove option " << p->first.toString() << " as it has low probability (" << p->second.probability << ")" );
      p = _configurationVariants.erase(p);
    }
    else {
      p++;
    }
  }
}


void  tarch::multicore::orchestration::GeneticOptimisation::pickNewActiveConfigurationRandomly() {
  double targetIntegrand  = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
  double currentIntegrand = 0.0;

  std::map< Key, Value >::iterator p = _configurationVariants.begin();
  currentIntegrand += p->second.probability;
  while (currentIntegrand<targetIntegrand) {
    currentIntegrand += p->second.probability;
    p++;
  }
  if (p==_configurationVariants.end()) {
    p = _configurationVariants.begin();
  }

  _activeKey = p->first;
  logInfo( "pickNewActiveConfigurationRandomly()", "picked key " << _activeKey.toString() );
}


int tarch::multicore::orchestration::GeneticOptimisation::getNumberOfTasksToHoldBack() {
  return _activeKey.numberOfTasksToHoldBack;
}


std::pair<int,int> tarch::multicore::orchestration::GeneticOptimisation::getNumberOfTasksToFuseAndTargetDevice() {
  return _activeKey.tasksToFuseAndTargetDevice;
}


bool tarch::multicore::orchestration::GeneticOptimisation::fuseTasksImmediatelyWhenSpawned() {
  return _activeKey.fuseTasksImmediatelyWhenSpawned;
}


bool tarch::multicore::orchestration::GeneticOptimisation::processPendingTasksWhileWaitingInBSPSection() {
  return _activeKey.processPendingTasksWhileWaiting;
}
