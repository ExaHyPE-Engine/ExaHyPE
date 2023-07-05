#include "SamplingMulticoreOrchestration.h"

#include <limits>

#include "tarch/multicore/Core.h"
#include "tarch/multicore/Tasks.h"

tarch::multicore::orchestration::GeneticOptimisation::Key::Key(
  int             numberOfTasksToHoldBack_,
  FuseInstruction fusionInstruction_,
  bool            fuseTasksImmediatelyWhenSpawned_,
  bool            processPendingTasksWhileWaiting_
):
  numberOfTasksToHoldBack(numberOfTasksToHoldBack_),
  fusionInstruction(fusionInstruction_),
  fuseTasksImmediatelyWhenSpawned(fuseTasksImmediatelyWhenSpawned_),
  processPendingTasksWhileWaiting(processPendingTasksWhileWaiting_) {}

bool tarch::multicore::orchestration::GeneticOptimisation::Key::operator<(const Key& other) const {
  return numberOfTasksToHoldBack<other.numberOfTasksToHoldBack
      or (numberOfTasksToHoldBack==other.numberOfTasksToHoldBack and fusionInstruction.device<other.fusionInstruction.device)
      or (numberOfTasksToHoldBack==other.numberOfTasksToHoldBack and fusionInstruction.device==other.fusionInstruction.device and fusionInstruction.minTasks<other.fusionInstruction.minTasks)
      or (numberOfTasksToHoldBack==other.numberOfTasksToHoldBack and fusionInstruction.device==other.fusionInstruction.device and fusionInstruction.minTasks==other.fusionInstruction.minTasks and fusionInstruction.maxTasks<other.fusionInstruction.maxTasks)
      or (numberOfTasksToHoldBack==other.numberOfTasksToHoldBack and fusionInstruction.device==other.fusionInstruction.device and fusionInstruction.minTasks==other.fusionInstruction.minTasks and fusionInstruction.maxTasks==other.fusionInstruction.maxTasks and fuseTasksImmediatelyWhenSpawned<other.fuseTasksImmediatelyWhenSpawned)
      or (numberOfTasksToHoldBack==other.numberOfTasksToHoldBack and fusionInstruction.device==other.fusionInstruction.device and fusionInstruction.minTasks==other.fusionInstruction.minTasks and fusionInstruction.maxTasks==other.fusionInstruction.maxTasks and fuseTasksImmediatelyWhenSpawned==other.fuseTasksImmediatelyWhenSpawned and processPendingTasksWhileWaiting<other.processPendingTasksWhileWaiting);
}

std::string tarch::multicore::orchestration::GeneticOptimisation::Key::toString() const {
  std::ostringstream msg;
  msg
    << "(hold-back=" << numberOfTasksToHoldBack << ",device=" << fusionInstruction.device << ",min-tasks="
    << fusionInstruction.minTasks << ",max-tasks=" << fusionInstruction.maxTasks << ",fuse-immediately="
    << fuseTasksImmediatelyWhenSpawned << ",process-pending=" << processPendingTasksWhileWaiting << ")";
  return msg.str();
}

tarch::multicore::orchestration::GeneticOptimisation::Value::Value(double probability_):
  probability(probability_) {}

tarch::logging::Log tarch::multicore::orchestration::GeneticOptimisation::_log("tarch::multicore::orchestration::"
                                                                               "GeneticOptimisation");

tarch::multicore::orchestration::GeneticOptimisation::GeneticOptimisation():
  _activeKey(0, FuseInstruction(-1, std::numeric_limits<int>::max(), 0), false, false),
  _currentWatch("tarch::multicore::orchestration::GeneticOptimisation", "GeneticOptimisation", false, false),
  _lastDuration(-1.0),
  _bspRunThroughsWithActiveKey(0),
  _currentDevice(0) {
  createInitialConfigurations();
  normalise();
}

void tarch::multicore::orchestration::GeneticOptimisation::createInitialConfigurations() {
  const int MaxInitialRage = 16;
  for (int numberOfTasksToHoldBack = 0; numberOfTasksToHoldBack <= MaxInitialRage; numberOfTasksToHoldBack++)
    for (int maxTasksToFuse = 0; maxTasksToFuse <= MaxInitialRage; maxTasksToFuse++)
      for (int minTasksToFuse = 0; minTasksToFuse <= maxTasksToFuse; minTasksToFuse++)
        for (int targetDevice = -1; targetDevice <= tarch::multicore::Core::getInstance().getNumberOfGPUs();
             targetDevice++)
          for (int fuseTasksImmediatelyWhenSpawned = 0; fuseTasksImmediatelyWhenSpawned < 2;
               fuseTasksImmediatelyWhenSpawned++)
            for (int processPendingTasksWhileWaiting = 0; processPendingTasksWhileWaiting < 2;
                 processPendingTasksWhileWaiting++) {
              _configurationVariants.insert(std::pair<Key, Value>(
                Key(
                  numberOfTasksToHoldBack,
                  FuseInstruction(targetDevice, minTasksToFuse, maxTasksToFuse),
                  fuseTasksImmediatelyWhenSpawned == 0,
                  processPendingTasksWhileWaiting == 0
                ),
                Value(1.0)
              ));
            }
}

void tarch::multicore::orchestration::GeneticOptimisation::normalise() {
  double sumOfProbabilities = 0.0;
  for (auto& p : _configurationVariants) {
    sumOfProbabilities += p.second.probability;
  }
  logInfo(
    "normalise()",
    "have "
      << _configurationVariants.size() << " potential configurations. Renormalise total total probability of "
      << sumOfProbabilities
  );
  for (auto& p : _configurationVariants) {
    p.second.probability /= sumOfProbabilities;
  }
}

void tarch::multicore::orchestration::GeneticOptimisation::startBSPSection(int nestedParallelismLevel) {
  if (nestedParallelismLevel <= 1 and _bspRunThroughsWithActiveKey == 0) {
    _currentWatch.start();
  }
}

void tarch::multicore::orchestration::GeneticOptimisation::endBSPSection(int nestedParallelismLevel) {
  if (nestedParallelismLevel <= 1) {
    const int BSPRunThroughsWithActiveKey = 16;
    _bspRunThroughsWithActiveKey++;

    if (_bspRunThroughsWithActiveKey >= BSPRunThroughsWithActiveKey) {
      _bspRunThroughsWithActiveKey = 0;

      _currentWatch.stop();
      double newValue = _currentWatch.getCalendarTime();

      constexpr double Min = 1e-8;
      if (newValue > Min) {
        // update stats
        _configurationVariants.at(_activeKey).measurement.setValue(newValue);

        // update probabilities. Increase local ones if appropriate and then re-normalise
        if (_lastDuration > Min and _lastDuration > _configurationVariants.at(_activeKey).measurement.getValue()) {
          increaseProbabilityOfActiveConfiguration();
          removeUnreasonableProbabilities();
          normalise();
        }
        _lastDuration = _configurationVariants.at(_activeKey).measurement.getValue();

        pickNewActiveConfigurationRandomly();
      }
    }
  }
}

void tarch::multicore::orchestration::GeneticOptimisation::increaseProbabilityOfActiveConfiguration() {
  Key left(
    _activeKey.numberOfTasksToHoldBack - 1,
    _activeKey.fusionInstruction,
    _activeKey.fuseTasksImmediatelyWhenSpawned,
    _activeKey.processPendingTasksWhileWaiting
  );
  Key right(
    _activeKey.numberOfTasksToHoldBack - 1,
    _activeKey.fusionInstruction,
    _activeKey.fuseTasksImmediatelyWhenSpawned,
    _activeKey.processPendingTasksWhileWaiting
  );
  Key down(
    _activeKey.numberOfTasksToHoldBack,
    FuseInstruction(
      _activeKey.fusionInstruction.device,
      _activeKey.fusionInstruction.minTasks,
      _activeKey.fusionInstruction.maxTasks + 1
    ),
    _activeKey.fuseTasksImmediatelyWhenSpawned,
    _activeKey.processPendingTasksWhileWaiting
  );
  Key up(
    _activeKey.numberOfTasksToHoldBack,
    FuseInstruction(
      _activeKey.fusionInstruction.device,
      _activeKey.fusionInstruction.minTasks + 1,
      _activeKey.fusionInstruction.maxTasks + 1
    ),
    _activeKey.fuseTasksImmediatelyWhenSpawned,
    _activeKey.processPendingTasksWhileWaiting
  );
  if (_configurationVariants.count(left) == 0) {
    _configurationVariants.insert(std::pair<Key, Value>(left, Value(_configurationVariants.at(_activeKey).probability))
    );
    logInfo("increaseProbabilityOfActiveConfiguration()", "insert new potential configuration " << left.toString());
  }
  if (_configurationVariants.count(right) == 0) {
    _configurationVariants.insert(std::pair<Key, Value>(right, Value(_configurationVariants.at(_activeKey).probability))
    );
    logInfo("increaseProbabilityOfActiveConfiguration()", "insert new potential configuration " << right.toString());
  }
  if (_configurationVariants.count(down) == 0) {
    _configurationVariants.insert(std::pair<Key, Value>(down, Value(_configurationVariants.at(_activeKey).probability))
    );
    logInfo("increaseProbabilityOfActiveConfiguration()", "insert new potential configuration " << down.toString());
  }
  if (_configurationVariants.count(up) == 0) {
    _configurationVariants.insert(std::pair<Key, Value>(up, Value(_configurationVariants.at(_activeKey).probability)));
    logInfo("increaseProbabilityOfActiveConfiguration()", "insert new potential configuration " << up.toString());
  }

  _configurationVariants.at(_activeKey).probability *= 1.1;
}

void tarch::multicore::orchestration::GeneticOptimisation::removeUnreasonableProbabilities() {
  constexpr double MinValue = 1e-8;

  double MinProbability = 0.1 / _configurationVariants.size();

  std::map<Key, Value>::iterator p = _configurationVariants.begin();
  while (p != _configurationVariants.end()) {
    if (p->second.measurement.getValue() > MinProbability and p->second.probability <= MinProbability) {
      logInfo(
        "removeUnreasonableProbabilities()",
        "remove option " << p->first.toString() << " as it has low probability (" << p->second.probability << ")"
      );
      p = _configurationVariants.erase(p);
    } else {
      p++;
    }
  }
}

void tarch::multicore::orchestration::GeneticOptimisation::pickNewActiveConfigurationRandomly() {
  double targetIntegrand  = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);
  double currentIntegrand = 0.0;

  std::map<Key, Value>::iterator p = _configurationVariants.begin();
  currentIntegrand += p->second.probability;
  while (currentIntegrand < targetIntegrand) {
    currentIntegrand += p->second.probability;
    p++;
  }
  if (p == _configurationVariants.end()) {
    p = _configurationVariants.begin();
  }

  _activeKey = p->first;
  logInfo("pickNewActiveConfigurationRandomly()", "picked key " << _activeKey.toString());
}

int tarch::multicore::orchestration::GeneticOptimisation::getNumberOfTasksToHoldBack() {
  return _activeKey.numberOfTasksToHoldBack;
}

tarch::multicore::orchestration::GeneticOptimisation::FuseInstruction tarch::multicore::orchestration::
  GeneticOptimisation::getNumberOfTasksToFuseAndTargetDevice() {
  _currentDevice++;
  _currentDevice = _currentDevice % tarch::multicore::Core::getInstance().getNumberOfGPUs();
  auto result    = _activeKey.fusionInstruction;
  result.device  = _currentDevice;
  return result;
}

bool tarch::multicore::orchestration::GeneticOptimisation::fuseTasksImmediatelyWhenSpawned() {
  return _activeKey.fuseTasksImmediatelyWhenSpawned;
}

bool tarch::multicore::orchestration::GeneticOptimisation::processPendingTasksWhileWaitingInBSPSection() {
  return _activeKey.processPendingTasksWhileWaiting;
}

tarch::multicore::orchestration::Strategy::ExecutionPolicy tarch::multicore::orchestration::GeneticOptimisation::
  paralleliseForkJoinSection(int nestedParallelismLevel, int numberOfTasks, int codeLocationIdentifier) {
  return _activeKey.processPendingTasksWhileWaiting
           ? tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallel
           : tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunSerially;
}
