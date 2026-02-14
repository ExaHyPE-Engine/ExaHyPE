// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadBalancingConfiguration.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"

#include <cmath>

exahype2::LoadBalancingConfiguration::LoadBalancingConfiguration(
  double                               loadBalancingQuality,
  int                                  minSizeOfTree,
  int                                  maxNumberOfTrees,
  peano4::grid::SplitInstruction::Mode mode
):
  _loadBalancingQuality(loadBalancingQuality),
  _minSizeOfTree(minSizeOfTree),
  _maxNumberOfTrees(maxNumberOfTrees),
  _mode(mode) {}

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

int exahype2::LoadBalancingConfiguration::getMaxLocalTreesPerRank(toolbox::loadbalancing::State state) const {
  return translateSetMaxNumberOfTreesIntoRealNumberOfTrees(_maxNumberOfTrees);
}

double exahype2::LoadBalancingConfiguration::getWorstCaseBalancingRatio(toolbox::loadbalancing::State) const {
  return _loadBalancingQuality;
}

int exahype2::LoadBalancingConfiguration::getMinTreeSize(toolbox::loadbalancing::State) const { return _minSizeOfTree; }

peano4::grid::SplitInstruction::Mode exahype2::LoadBalancingConfiguration::getMode(
  toolbox::loadbalancing::State
) const {
  return _mode;
}
