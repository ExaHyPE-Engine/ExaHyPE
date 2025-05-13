// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "config.h"
#include "tarch/la/Vector.h"

namespace peano4 {
  void init(
    int*                                         argc,
    char***                                      argv,
    const tarch::la::Vector<Dimensions, double>& offset,
    const tarch::la::Vector<Dimensions, double>& width,
    const std::bitset<Dimensions>&               periodicBC = 0
  );

  void shutdown();
} // namespace peano4
