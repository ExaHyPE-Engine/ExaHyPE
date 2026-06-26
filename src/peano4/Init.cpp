// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Init.h"

#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"

void peano4::init(
  const tarch::la::Vector<DIMENSIONS, double>& offset,
  const tarch::la::Vector<DIMENSIONS, double>& width,
  const std::bitset<DIMENSIONS>&               periodicBC
) {
  peano4::parallel::Node::initMPIDatatypes();
  peano4::parallel::Node::getInstance().init();
  peano4::parallel::SpacetreeSet::getInstance().init(offset, width, periodicBC);
}

void peano4::shutdown() {
  peano4::parallel::Node::getInstance().shutdown();
  peano4::parallel::Node::shutdownMPIDatatypes();
  peano4::parallel::SpacetreeSet::getInstance().shutdown();
}
