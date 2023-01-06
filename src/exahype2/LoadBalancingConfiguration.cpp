#include "LoadBalancingConfiguration.h"


exahype2::LoadBalancingConfiguration::LoadBalancingConfiguration( double loadBalancingQuality, int minSizeOfTree, int maxNumberOfTrees, int maxSizeOfDeployedRemoteMPITree ):
  _loadBalancingQuality(loadBalancingQuality),
  _minSizeOfTree(minSizeOfTree),
  _maxNumberOfTrees(maxNumberOfTrees),
  _maxSizeOfDeployedRemoteMPITree(maxSizeOfDeployedRemoteMPITree) {
}


bool exahype2::LoadBalancingConfiguration::makeSplitDependOnMemory(toolbox::loadbalancing::RecursiveSubdivision::State state) {
  return false;
}


int exahype2::LoadBalancingConfiguration::getMaxLocalTreesPerRank(toolbox::loadbalancing::RecursiveSubdivision::State state) {
  if (
    state==toolbox::loadbalancing::RecursiveSubdivision::State::IntraRankDistribution
    or
    state==toolbox::loadbalancing::RecursiveSubdivision::State::APosterioriBalancingPerRank
  ) {
    return _maxNumberOfTrees;
  }
  else {
    return std::numeric_limits<int>::max();
  }
}


double exahype2::LoadBalancingConfiguration::getWorstCaseBalancingRatio(toolbox::loadbalancing::RecursiveSubdivision::State state) {
  return _loadBalancingQuality;
}


int exahype2::LoadBalancingConfiguration::getMinTreeSize(toolbox::loadbalancing::RecursiveSubdivision::State state) {
  if (
    state==toolbox::loadbalancing::RecursiveSubdivision::State::InterRankBalancing
    or
    state==toolbox::loadbalancing::RecursiveSubdivision::State::APosterioriBalancingPerRank
  ) {
    return 0;
  }
  else {
    return _minSizeOfTree;
  }
}



int exahype2::LoadBalancingConfiguration::getMaxTreeSize(toolbox::loadbalancing::RecursiveSubdivision::State state) {
  if (
    state==toolbox::loadbalancing::RecursiveSubdivision::State::InterRankDistribution
    or
    state==toolbox::loadbalancing::RecursiveSubdivision::State::IntraRankDistribution
  ) {
    return _maxSizeOfDeployedRemoteMPITree;
  }
  else {
    return std::numeric_limits<int>::max();
  }
}

