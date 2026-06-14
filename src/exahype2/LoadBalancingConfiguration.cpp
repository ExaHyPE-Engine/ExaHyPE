// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadBalancingConfiguration.h"

#include "RuntimeParameters.h"
#include "peano4/Globals.h"
#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"

#include <algorithm>

exahype2::LoadBalancingConfiguration::LoadBalancingConfiguration():
  LoadBalancingConfiguration(false) {}

exahype2::LoadBalancingConfiguration::LoadBalancingConfiguration(bool hasPeriodicBC):
  _hasPeriodicBC(hasPeriodicBC) {}

int exahype2::LoadBalancingConfiguration::translateSetMaxNumberOfTreesIntoRealNumberOfTrees(int value) const {
  int result = -1;
  if (value <= 0) {
    const int threads = tarch::multicore::Core::getInstance().getNumberOfThreads();
    result            = threads;
  } else {
    assertion(value > 0);
    result = value;
  }
  return result;
}

int exahype2::LoadBalancingConfiguration::getMaxLocalTreesPerRank(toolbox::loadbalancing::State) const {
  return translateSetMaxNumberOfTreesIntoRealNumberOfTrees(
    RuntimeParameters::getInstance().getMaxNumberOfTreesPerRank());
}

double exahype2::LoadBalancingConfiguration::getWorstCaseBalancingRatio(toolbox::loadbalancing::State) const {
  return RuntimeParameters::getInstance().getLoadBalancingQuality();
}

int exahype2::LoadBalancingConfiguration::getMinTreeSize(toolbox::loadbalancing::State) const {
  const int minSizeOfTree = RuntimeParameters::getInstance().getMinSizeOfTree();
  return _hasPeriodicBC ? std::max(minSizeOfTree, static_cast<int>(THREE_POWER_D)) : minSizeOfTree;
}

peano4::grid::SplitInstruction::Mode
exahype2::LoadBalancingConfiguration::getMode(toolbox::loadbalancing::State) const {
  return peano4::grid::SplitInstruction::Mode::BottomUp;
}
