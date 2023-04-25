#include "LoadBalancingConfiguration.h"


#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"


tarch::logging::Log  exahype2::LoadBalancingConfiguration::_log( "exahype2::LoadBalancingConfiguration" );


exahype2::LoadBalancingConfiguration::LoadBalancingConfiguration(
  double loadBalancingQuality,
  int minSizeOfTree,
  int maxNumberOfTreesThroughoutInitialDistribution,
  int maxNumberOfTrees
):
  _loadBalancingQuality(loadBalancingQuality),
  _minSizeOfTree(minSizeOfTree),
  _maxNumberOfTreesThroughoutInitialDistribution( maxNumberOfTreesThroughoutInitialDistribution ),
  _maxNumberOfTrees( maxNumberOfTrees ) {
  assertion2(
    (_maxNumberOfTrees>=_maxNumberOfTreesThroughoutInitialDistribution)
    or
    _maxNumberOfTrees<0
    or
    _maxNumberOfTreesThroughoutInitialDistribution<0,
    _maxNumberOfTrees, _maxNumberOfTreesThroughoutInitialDistribution
  );
}


bool exahype2::LoadBalancingConfiguration::makeSplitDependOnMemory(toolbox::loadbalancing::State state) {
  return false;
}


int exahype2::LoadBalancingConfiguration::translateSetMaxNumberOfTreesIntoRealNumberOfTrees(int value) const {
  int result = -1;
  if (value==UseNumberOfThreads) {
    result = tarch::multicore::Core::getInstance().getNumberOfThreads();
  }
  else if (value==UseTwiceTheNumberOfThreads) {
    result = tarch::multicore::Core::getInstance().getNumberOfThreads() * 2;
  }
  else {
    assertion(value>0);
    result = value;
  }

  assertion(result>=0);
  return result;
}


int exahype2::LoadBalancingConfiguration::getMaxLocalTreesPerRank(toolbox::loadbalancing::State state) {
  if (
    state==toolbox::loadbalancing::State::IntraRankDistribution
  ) {
    return translateSetMaxNumberOfTreesIntoRealNumberOfTrees(_maxNumberOfTreesThroughoutInitialDistribution);
  }
  else {
    return translateSetMaxNumberOfTreesIntoRealNumberOfTrees(_maxNumberOfTrees);
  }
}


double exahype2::LoadBalancingConfiguration::getWorstCaseBalancingRatio(toolbox::loadbalancing::State state) {
  return _loadBalancingQuality;
}


int exahype2::LoadBalancingConfiguration::getMinTreeSize(toolbox::loadbalancing::State state) {
  if (
    state==toolbox::loadbalancing::State::InterRankDistribution
    or
    state==toolbox::loadbalancing::State::IntraRankDistribution
  ) {
    return 0;
  }
  else {
    return _minSizeOfTree;
  }
}


std::string exahype2::LoadBalancingConfiguration::toString() const {
  std::ostringstream msg;
  msg << "("
      << "lb-quality=" << _loadBalancingQuality
      << ",min-size-of-tree=" << _minSizeOfTree
      << ",max-no-of-trees-throughout-distribution=" << _maxNumberOfTreesThroughoutInitialDistribution
      << ",max-no-of-trees=" << _maxNumberOfTrees
      << ")";
  return msg.str();
}


