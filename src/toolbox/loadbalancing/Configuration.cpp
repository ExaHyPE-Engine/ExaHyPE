// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Configuration.h"

#include "tarch/multicore/Core.h"

int toolbox::loadbalancing::DefaultConfiguration::getMaxLocalTreesPerRank(State) {
  return tarch::multicore::Core::getInstance().getNumberOfThreads();
}

double toolbox::loadbalancing::DefaultConfiguration::getWorstCaseBalancingRatio(State) { return 0.9; }

int toolbox::loadbalancing::DefaultConfiguration::getMinTreeSize(State) { return 1; }

peano4::SplitInstruction::Mode toolbox::loadbalancing::DefaultConfiguration::getMode(State) {
  return peano4::SplitInstruction::Mode::BottomUp;
}
