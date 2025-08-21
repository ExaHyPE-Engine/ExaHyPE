// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridVertexTest.h"

#include "peano4/grid/GridVertex.h"

peano4::grid::tests::GridVertexTest::GridVertexTest():
  TestCase("peano4::grid::tests::GridVertexTest") {}

void peano4::grid::tests::GridVertexTest::run() {
  testMethod([this]() { testCreateGridVertex(); });
}

void peano4::grid::tests::GridVertexTest::testCreateGridVertex() {
  std::array<int, TWO_POWER_D> adj{};
  adj.fill(7);
  GridVertex v = createVertex(3, GridVertex::State::Refined, adj, false);

  validateEquals(static_cast<int>(v.getState()), static_cast<int>(GridVertex::State::Refined));
  validateEquals(v.level, 3);

  // Adjacent ranks & backup equal to input
  for (int i = 0; i < TWO_POWER_D; ++i) {
    validateEquals(v.adjacentRanks[i], 7);
    validateEquals(v.backupOfAdjacentRanks[i], 7);
  }

  validateTrue (v.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep));
  validateTrue (v.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep));
  validateFalse(v.getFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep));
  validateFalse(v.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep));

  validateEquals(v.numberOfAdjacentRefinedLocalCells, 0);

  v.setFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep, true);
  v.setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, false);
  v.setFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep, v.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep));
  v.setFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep,     v.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep));
  v.clearFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);
  v.clearFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep);

  validateFalse(v.getFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep));
  validateTrue (v.getFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep));
  validateFalse(v.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep));
  validateFalse(v.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep));
}
