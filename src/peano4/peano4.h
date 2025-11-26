// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace peano4 {
  void writeCopyrightMessage();

  void init(
    int*                                         argc,
    char***                                      argv,
    const tarch::la::Vector<DIMENSIONS, double>& offset,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    const std::bitset<DIMENSIONS>&               periodicBC = 0
  );

  void shutdown();
} // namespace peano4
