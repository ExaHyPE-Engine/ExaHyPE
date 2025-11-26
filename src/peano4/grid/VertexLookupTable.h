// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AutomatonState.h"
#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"

#include <array>

namespace peano4::grid {
  struct VertexLookupTable {
    std::array<int, TWO_POWER_D>                                vIdx;       // Vertex index as unsigned int
    std::array<int, TWO_POWER_D>                                lin;        // Linearised index for fine grid vertices
    std::array<tarch::la::Vector<DIMENSIONS, int>, TWO_POWER_D> vPos;       // Offset within 3x3 patch
    std::array<int, TWO_POWER_D>                                readStack;  // Stack numbers for load
    std::array<int, TWO_POWER_D>                                writeStack; // Stack numbers for store
    std::array<bool, TWO_POWER_D>                               inOutRead;  // In/out mask for load
    std::array<bool, TWO_POWER_D>                               inOutWrite; // In/out mask for store
  };

  VertexLookupTable createVertexLookupTable(
    const AutomatonState&                     fineGridState,
    const tarch::la::Vector<DIMENSIONS, int>& cellPositionWithin3x3Patch
  );
} // namespace peano4::grid
