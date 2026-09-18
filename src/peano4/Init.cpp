// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Init.h"

#include "peano4/communication/CommunicationManager.h"
#include "peano4/topology/TopologyManager.h"

void peano4::init(
  const tarch::la::Vector<DIMENSIONS, double>& offset,
  const tarch::la::Vector<DIMENSIONS, double>& width,
  const std::bitset<DIMENSIONS>&               periodicBoundaries
) {
  peano4::communication::CommunicationManager::getInstance().init();
  peano4::topology::TopologyManager::getInstance().init(offset, width, periodicBoundaries);
}

void peano4::shutdown() {
  peano4::topology::TopologyManager::getInstance().shutdown();
  peano4::communication::CommunicationManager::getInstance().shutdown();
}
