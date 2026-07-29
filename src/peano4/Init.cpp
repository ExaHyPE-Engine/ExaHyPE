// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Init.h"

#include "peano4/parallel/ExchangeChannels.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "peano4/parallel/TreeIdMapping.h"

void peano4::init(
  const tarch::la::Vector<DIMENSIONS, double>& offset,
  const tarch::la::Vector<DIMENSIONS, double>& width,
  const std::bitset<DIMENSIONS>&               periodicBC
) {
  peano4::parallel::ExchangeChannels::getInstance().init();
  peano4::parallel::TreeIdMapping::getInstance().init();
  peano4::parallel::SpacetreeSet::getInstance().init(offset, width, periodicBC);
}

void peano4::shutdown() {
  peano4::parallel::ExchangeChannels::getInstance().shutdown();
  peano4::parallel::SpacetreeSet::getInstance().shutdown();
}
