// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Configuration.h"

#include "tarch/multicore/Core.h"

int toolbox::loadbalancing::DefaultConfiguration::getMaxLocalTreesPerRank(toolbox::loadbalancing::State) const {
  return tarch::multicore::Core::getInstance().getNumberOfThreads();
}

double toolbox::loadbalancing::DefaultConfiguration::getWorstCaseBalancingRatio(toolbox::loadbalancing::State) const {
  return 0.9;
}

peano4::grid::SplitInstruction::Mode
toolbox::loadbalancing::DefaultConfiguration::getMode(toolbox::loadbalancing::State) const {
  return peano4::grid::SplitInstruction::Mode::BottomUp;
}
