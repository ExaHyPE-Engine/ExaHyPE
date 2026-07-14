// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VertexTypeTest.h"

#include "peano4/grid/GridVertex.h"
#include "peano4/grid/VertexType.h"
#include "peano4/Loop.h"

peano4::grid::tests::VertexTypeTest::VertexTypeTest():
  TestCase("peano4::grid::tests::VertexTypeTest") {}

void peano4::grid::tests::VertexTypeTest::run() {
  testMethod(testGetVertexTypes);
  testMethod(testGetVertexTypesFromPreviousTreeSweep);
}

void peano4::grid::tests::VertexTypeTest::testGetVertexTypes() {
  GridVertex coarseGridVertices[TWO_POWER_D];
  VertexType result[TWO_POWER_D];

  auto cornerIndex = [&](int cornerBits) -> int {
    std::bitset<DIMENSIONS>            bits(cornerBits);
    tarch::la::Vector<DIMENSIONS, int> coords(bits);
    return peano4::dLinearised(coords, 2);
  };

  auto setAll = [&](GridVertex::State state) {
    for (int i = 0; i < TWO_POWER_D; ++i) {
      coarseGridVertices[i].setState(state);
      coarseGridVertices[i].setFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep, false);
    }
  };

  auto filled = [&](int value) { return tarch::la::Vector<DIMENSIONS, int>(value); };

  // All refined -> all persistent everywhere.
  setAll(GridVertex::State::Refined);
  getVertexTypes(coarseGridVertices, filled(0), result);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    validateEquals(VertexType::Persistent, result[i]);
  }
  getVertexTypes(coarseGridVertices, filled(2), result);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    validateEquals(VertexType::Persistent, result[i]);
  }

  // One hanging coarse corner at (0,0,...,0).
  setAll(GridVertex::State::Refined);
  coarseGridVertices[cornerIndex(0)].setState(GridVertex::State::RefinementTriggered);
  getVertexTypes(coarseGridVertices, filled(0), result);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    const VertexType expected = (i == 0) ? VertexType::Hanging : VertexType::Persistent;
    validateEquals(expected, result[i]);
  }

  // New dominates on an edge: c000=New, c100=Hanging, all others Persistent.
  setAll(GridVertex::State::Refined);
  coarseGridVertices[cornerIndex(0)].setState(GridVertex::State::Refining);
  coarseGridVertices[cornerIndex(1)].setState(GridVertex::State::RefinementTriggered);
  getVertexTypes(coarseGridVertices, filled(0), result);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    const VertexType expected = (i == 0 || i == 1) ? VertexType::New : VertexType::Persistent;
    validateEquals(expected, result[i]);
  }

  // Delete dominates on an edge: c000=Delete, c100=Hanging, all others Persistent.
  setAll(GridVertex::State::Refined);
  coarseGridVertices[cornerIndex(0)].setState(GridVertex::State::Erasing);
  coarseGridVertices[cornerIndex(1)].setState(GridVertex::State::RefinementTriggered);
  getVertexTypes(coarseGridVertices, filled(0), result);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    const VertexType expected = (i == 0 || i == 1) ? VertexType::Delete : VertexType::Persistent;
    validateEquals(expected, result[i]);
  }

  // Opposite corner at (1,1,...,1) hanging, queried through positions (2/3).
  setAll(GridVertex::State::Refined);
  coarseGridVertices[cornerIndex(TWO_POWER_D - 1)].setState(GridVertex::State::RefinementTriggered);
  getVertexTypes(coarseGridVertices, filled(2), result);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    const VertexType expected = (i == TWO_POWER_D - 1) ? VertexType::Hanging : VertexType::Persistent;
    validateEquals(expected, result[i]);
  }

  // getVertexTypes must not depend on the "previous sweep" flag.
  setAll(GridVertex::State::Refined);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    coarseGridVertices[i].setFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep, true);
  }
  getVertexTypes(coarseGridVertices, filled(0), result);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    validateEquals(VertexType::Persistent, result[i]);
  }
}

void peano4::grid::tests::VertexTypeTest::testGetVertexTypesFromPreviousTreeSweep() {
  GridVertex coarseGridVertices[TWO_POWER_D];
  VertexType result[TWO_POWER_D];

  auto cornerIndex = [&](int cornerBits) -> int {
    std::bitset<DIMENSIONS>            bits(cornerBits);
    tarch::la::Vector<DIMENSIONS, int> coords(bits);
    return peano4::dLinearised(coords, 2);
  };

  auto setAll = [&](GridVertex::State state, bool prevHangingFlag) {
    for (int i = 0; i < TWO_POWER_D; ++i) {
      coarseGridVertices[i].setState(state);
      coarseGridVertices[i].setFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep, prevHangingFlag);
    }
  };

  auto filled = [&](int value) { return tarch::la::Vector<DIMENSIONS, int>(value); };

  // No corner was hanging in the previous sweep -> all persistent.
  setAll(GridVertex::State::Erasing, false); // use a non-trivial state to verify it's ignored
  getVertexTypesFromPreviousTreeSweep(coarseGridVertices, filled(0), result);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    validateEquals(VertexType::Persistent, result[i]);
  }

  // One hanging corner at (0,0,...,0) in previous sweep.
  setAll(GridVertex::State::Refined, false);
  coarseGridVertices[cornerIndex(0)].setFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep, true);
  getVertexTypesFromPreviousTreeSweep(coarseGridVertices, filled(0), result);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    const VertexType expected = (i == 0) ? VertexType::Hanging : VertexType::Persistent;
    validateEquals(expected, result[i]);
  }

  // Two adjacent hanging corners along x -> the edge point becomes hanging.
  setAll(GridVertex::State::Refined, false);
  coarseGridVertices[cornerIndex(0)].setFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep, true);
  coarseGridVertices[cornerIndex(1)].setFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep, true);
  getVertexTypesFromPreviousTreeSweep(coarseGridVertices, filled(0), result);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    const VertexType expected = (i == 0 || i == 1) ? VertexType::Hanging : VertexType::Persistent;
    validateEquals(expected, result[i]);
  }

  // All corners hanging in previous sweep -> everything is hanging.
  setAll(GridVertex::State::Refined, true);
  getVertexTypesFromPreviousTreeSweep(coarseGridVertices, filled(0), result);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    validateEquals(VertexType::Hanging, result[i]);
  }

  // Opposite corner at (1,1,...,1) hanging, queried through positions (2/3).
  setAll(GridVertex::State::Refined, false);
  coarseGridVertices[cornerIndex(TWO_POWER_D - 1)]
    .setFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep, true);
  getVertexTypesFromPreviousTreeSweep(coarseGridVertices, filled(2), result);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    const VertexType expected = (i == TWO_POWER_D - 1) ? VertexType::Hanging : VertexType::Persistent;
    validateEquals(expected, result[i]);
  }
}
