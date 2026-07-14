// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AutomatonState.h"
#include "peano4/Globals.h"
#include "tarch/la/Vector.h"

namespace peano4::grid {
  struct VertexLookupTable {
    std::array<int, TWO_POWER_D> vIdx; // Vertex index; also the linearised fine-grid index
    std::array<tarch::la::Vector<DIMENSIONS, int>, TWO_POWER_D> vPos;       // Offset within 3x3 patch
    std::array<int, TWO_POWER_D>                                readStack;  // Stack numbers for load
    std::array<int, TWO_POWER_D>                                writeStack; // Stack numbers for store
    std::array<bool, TWO_POWER_D>                               inOutRead;  // In/out mask for load
    std::array<bool, TWO_POWER_D>                               inOutWrite; // In/out mask for store
    std::array<int, TWO_TIMES_D> faceIndex;      // getFaceNumberAlongCurve(state, i) for i=0..2D-1
    std::array<int, TWO_TIMES_D> faceReadStack;  // getFaceReadStackNumber(state, faceIndex[i])
    std::array<int, TWO_TIMES_D> faceWriteStack; // getFaceWriteStackNumber(state, faceIndex[i])
    int                          cellReadStack;  // getCellReadStackNumber(state)
    int                          cellWriteStack; // getCellWriteStackNumber(state)
  };

  const VertexLookupTable& createVertexLookupTable(
    const AutomatonState&                     fineGridState,
    const tarch::la::Vector<DIMENSIONS, int>& cellPositionWithin3x3Patch
  );
} // namespace peano4::grid
