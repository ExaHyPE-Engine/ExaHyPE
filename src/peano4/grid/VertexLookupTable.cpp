// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VertexLookupTable.h"

#include "PeanoCurve.h"
#include "peano4/utils/Linearised.h"

#include <bitset>

inline tarch::la::Vector<DIMENSIONS, int> convertToIntegerVector(const std::bitset<DIMENSIONS>& in) {
  tarch::la::Vector<DIMENSIONS, int> result;
  for (int d = 0; d < DIMENSIONS; d++) {
    result[d] = in[d] ? 1 : 0;
  }
  return result;
}

peano4::grid::VertexLookupTable peano4::grid::createVertexLookupTable(
  const AutomatonState&                     fineGridState,
  const tarch::la::Vector<DIMENSIONS, int>& cellPositionWithin3x3Patch
) {
  VertexLookupTable lut;

  const std::bitset<DIMENSIONS> first = PeanoCurve::getFirstVertexIndex(fineGridState);

  for (int i = 0; i < TWO_POWER_D; ++i) {
    const auto vIndex = (first ^ std::bitset<DIMENSIONS>(i));
    const int  vIdx   = static_cast<int>(vIndex.to_ulong());
    const int  lin    = peano4::utils::dLinearised(vIndex);

    lut.vIdx[i] = vIdx;
    lut.lin[i]  = lin;
    lut.vPos[i] = cellPositionWithin3x3Patch + convertToIntegerVector(vIndex);

    const int readStack  = PeanoCurve::getVertexReadStackNumber(fineGridState, vIndex);
    const int writeStack = PeanoCurve::getVertexWriteStackNumber(fineGridState, vIndex);
    lut.readStack[i]     = readStack;
    lut.writeStack[i]    = writeStack;
    lut.inOutRead[i]     = PeanoCurve::isInOutStack(readStack);
    lut.inOutWrite[i]    = PeanoCurve::isInOutStack(writeStack);
  }

  return lut;
}
