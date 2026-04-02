// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/AutomatonState.h"
#include "peano4/grid/VertexLookupTable.h"
#include "tarch/la/Vector.h"
#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class VertexLookupTableTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::VertexLookupTableTest final: public tarch::tests::TestCase {
private:
  void testCreateVertexLookupTableMatchesPeanoCurve();
  void testCreateVertexLookupTableCachesByStructuralState();
  void validateLookupTableMatchesPeanoCurve(
    const peano4::grid::AutomatonState&       state,
    const tarch::la::Vector<DIMENSIONS, int>& relativePosition,
    const peano4::grid::VertexLookupTable&    lut
  );

public:
  VertexLookupTableTest();
  ~VertexLookupTableTest() override final = default;

  void run() override final;
};
