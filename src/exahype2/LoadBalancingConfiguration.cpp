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
    // const double exp      = std::ceil(std::log(threads) / std::log(3.0));
    // const int    nextPow3 = static_cast<int>(std::pow(3.0, exp));
    // assertion(nextPow3 > 0);
    // result = nextPow3;
    result = threads;
  } else {
    assertion(value > 0);
    result = value;
  }
  return result;
}

int exahype2::LoadBalancingConfiguration::getMaxLocalTreesPerRank(toolbox::loadbalancing::State state) {
  return translateSetMaxNumberOfTreesIntoRealNumberOfTrees(_maxNumberOfTrees);
}

double exahype2::LoadBalancingConfiguration::getWorstCaseBalancingRatio(toolbox::loadbalancing::State) {
  return _loadBalancingQuality;
}

int exahype2::LoadBalancingConfiguration::getMinTreeSize(toolbox::loadbalancing::State) { return _minSizeOfTree; }

peano4::grid::SplitInstruction::Mode exahype2::LoadBalancingConfiguration::getMode(toolbox::loadbalancing::State) {
  return _mode;
}
