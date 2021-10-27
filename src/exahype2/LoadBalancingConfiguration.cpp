#include "LoadBalancingConfiguration.h"


exahype2::LoadBalancingConfiguration::LoadBalancingConfiguration( double loadBalancingQuality, int minSizeOfTree, int maxNumberOfTrees, int maxSizeOfDeployedRemoteMPITree ):
  _loadBalancingQuality(loadBalancingQuality),
  _minSizeOfTree(minSizeOfTree),
  _maxNumberOfTrees(maxNumberOfTrees),
  _maxSizeOfDeployedRemoteMPITree(maxSizeOfDeployedRemoteMPITree) {
}


bool exahype2::LoadBalancingConfiguration::makeSplitDependOnMemory(toolbox::loadbalancing::RecursiveSubdivision::Configuration::Phase phase) {
  return false;
}


int exahype2::LoadBalancingConfiguration::getMaxLocalTreesPerRank(toolbox::loadbalancing::RecursiveSubdivision::Configuration::Phase phase) {
  if (
    phase==Configuration::Phase::InitialIntraRankDistribution
    or
    phase==Configuration::Phase::APosterioriBalancingPerRank
  ) {
    return _maxNumberOfTrees;
  }
  else {
    return std::numeric_limits<int>::max();
  }
}


double exahype2::LoadBalancingConfiguration::getWorstCaseBalancingRatio(toolbox::loadbalancing::RecursiveSubdivision::Configuration::Phase phase) {
  return _loadBalancingQuality;
}


int exahype2::LoadBalancingConfiguration::getMinTreeSize(toolbox::loadbalancing::RecursiveSubdivision::Configuration::Phase phase) {
  if (
    phase==Configuration::Phase::InterRankBalancing
    or
    phase==Configuration::Phase::APosterioriBalancingPerRank
  ) {
    return 0;
  }
  else {
    return _minSizeOfTree;
  }
}



int exahype2::LoadBalancingConfiguration::getMaxTreeSize(toolbox::loadbalancing::RecursiveSubdivision::Configuration::Phase phase) {
  if (
    phase==Configuration::Phase::InitialInterRankDistribution
    or
    phase==Configuration::Phase::InitialIntraRankDistribution
  ) {
    return _maxSizeOfDeployedRemoteMPITree;
  }
  else {
    return std::numeric_limits<int>::max();
  }
}

