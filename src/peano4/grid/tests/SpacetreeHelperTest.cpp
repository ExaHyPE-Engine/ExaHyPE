// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeHelperTest.h"

#include "peano4/grid/GridVertex.h"
#include "peano4/grid/SpacetreeHelper.h"
#include "tarch/la/Vector.h"

namespace {
  void initVertices(peano4::grid::GridVertex vertices[TWO_POWER_D], int defaultRank) {
    for (int v = 0; v < TWO_POWER_D; ++v) {
      vertices[v].setState(peano4::grid::GridVertex::State::Unrefined);
      for (int i = 0; i < TWO_POWER_D; ++i) {
        vertices[v].adjacentRanks[i]         = defaultRank;
        vertices[v].backupOfAdjacentRanks[i] = defaultRank;
      }
    }
  }
} // namespace

peano4::grid::tests::SpacetreeHelperTest::SpacetreeHelperTest():
  TestCase("peano4::grid::tests::SpacetreeHelperTest") {}

void peano4::grid::tests::SpacetreeHelperTest::run() {
  testMethod(testGetAdjacentRanksOfFace_SendVsReceiveAndStateFiltering);
  testMethod(testAreFacesLocal_HangingVertexRules);
  testMethod(testParallelDomainBoundaryAdjacencyFlags);
  testMethod(testPatch3x3IndexTable);
  testMethod(testFaceAdjacencyTables);
  testMethod(testAreAllVerticesNonHanging);
  testMethod(testIsSpacetreeNodeLocal);
  testMethod(testGetTreeOwningSpacetreeNode);
  testMethod(testIsSpacetreeNodeSplitCandidate);
}

void peano4::grid::tests::SpacetreeHelperTest::testGetAdjacentRanksOfFace_SendVsReceiveAndStateFiltering() {
#if DIMENSIONS == 2
  GridVertex vertices[TWO_POWER_D];
  initVertices(vertices, /*defaultRank*/ 1);

  const auto& T = SpacetreeHelper::getFaceAdjTables();

  const int faceNumber = 0;
  const int normal     = faceNumber % DIMENSIONS;
  const int v0         = T.faceVerts[faceNumber][0];
  const int v1         = T.faceVerts[faceNumber][1];

  // Make one vertex invalid for sending (Delete) and one invalid for receiving (New).
  vertices[v0].setState(GridVertex::State::Delete);
  vertices[v1].setState(GridVertex::State::New);

  const int i0_v0 = T.adjIdx0[normal][v0];
  const int i1_v0 = T.adjIdx1[normal][v0];
  const int i0_v1 = T.adjIdx0[normal][v1];
  const int i1_v1 = T.adjIdx1[normal][v1];

  int outSend[TWO_POWER_D];
  int outRecv[TWO_POWER_D];
  SpacetreeHelper::getAdjacentRanksOfFace(vertices, faceNumber, /*calledByReceivingProcess*/ false, outSend);
  SpacetreeHelper::getAdjacentRanksOfFace(vertices, faceNumber, /*calledByReceivingProcess*/ true, outRecv);

  // Sending: Delete is ignored, New is used (from adjacentRanks).
  validateEquals(outSend[0], -1);
  validateEquals(outSend[1], -1);
  validateEquals(outSend[2], vertices[v1].adjacentRanks[i0_v1]);
  validateEquals(outSend[3], vertices[v1].adjacentRanks[i1_v1]);

  // Receiving: New is ignored, Delete is used (from backupOfAdjacentRanks).
  validateEquals(outRecv[0], vertices[v0].backupOfAdjacentRanks[i0_v0]);
  validateEquals(outRecv[1], vertices[v0].backupOfAdjacentRanks[i1_v0]);
  validateEquals(outRecv[2], -1);
  validateEquals(outRecv[3], -1);
#endif

#if DIMENSIONS == 3
  GridVertex vertices[TWO_POWER_D];
  for (int v = 0; v < TWO_POWER_D; ++v) {
    vertices[v].setState(GridVertex::State::Unrefined);
    for (int j = 0; j < TWO_POWER_D; ++j) {
      vertices[v].adjacentRanks[j]         = 10 * v + j;
      vertices[v].backupOfAdjacentRanks[j] = 100 + 10 * v + j;
    }
  }

  const auto& T          = SpacetreeHelper::getFaceAdjTables();
  const int   faceNumber = 0;
  const int   normal     = faceNumber % DIMENSIONS;

  // In 3D, each face has 4 vertices; the routine outputs 2 adjacency ranks per face vertex -> 8 outputs.
  const int vNew    = T.faceVerts[faceNumber][0];
  const int vDelete = T.faceVerts[faceNumber][1];
  const int vHang   = T.faceVerts[faceNumber][2];
  // T.faceVerts[faceNumber][3] stays Unrefined (valid for both).

  vertices[vNew].setState(GridVertex::State::New);            // sending: valid, receiving: invalid
  vertices[vDelete].setState(GridVertex::State::Delete);      // sending: invalid, receiving: valid
  vertices[vHang].setState(GridVertex::State::HangingVertex); // invalid for both

  auto computeExpected = [&](bool receiving, int expected[TWO_POWER_D]) {
    int c = 0;
    for (int k = 0; k < TWO_POWER_D / 2; ++k) {
      const int  vIdx = T.faceVerts[faceNumber][k];
      const auto st   = vertices[vIdx].getState();

      const bool invalid = receiving ? (st == GridVertex::State::HangingVertex) or (st == GridVertex::State::New)
                                     : (st == GridVertex::State::HangingVertex) or (st == GridVertex::State::Delete);

      const int i0 = T.adjIdx0[normal][vIdx];
      const int i1 = T.adjIdx1[normal][vIdx];

      if (receiving) {
        expected[c++] = invalid ? -1 : vertices[vIdx].backupOfAdjacentRanks[i0];
        expected[c++] = invalid ? -1 : vertices[vIdx].backupOfAdjacentRanks[i1];
      } else {
        expected[c++] = invalid ? -1 : vertices[vIdx].adjacentRanks[i0];
        expected[c++] = invalid ? -1 : vertices[vIdx].adjacentRanks[i1];
      }
    }
  };

  int outSend[TWO_POWER_D];
  int outRecv[TWO_POWER_D];
  int expSend[TWO_POWER_D];
  int expRecv[TWO_POWER_D];

  SpacetreeHelper::getAdjacentRanksOfFace(vertices, faceNumber, /*calledByReceivingProcess*/ false, outSend);
  SpacetreeHelper::getAdjacentRanksOfFace(vertices, faceNumber, /*calledByReceivingProcess*/ true, outRecv);

  computeExpected(/*receiving*/ false, expSend);
  computeExpected(/*receiving*/ true, expRecv);

  for (int i = 0; i < TWO_POWER_D; ++i) {
    validateEqualsWithParams3(outSend[i], expSend[i], i, outSend[i], expSend[i]);
    validateEqualsWithParams3(outRecv[i], expRecv[i], i, outRecv[i], expRecv[i]);
  }
#endif
}

void peano4::grid::tests::SpacetreeHelperTest::testAreFacesLocal_HangingVertexRules() {
#if DIMENSIONS == 2
  const int                treeId = 1;
  const SplitSpecification splitTriggered;
  const std::set<int>      splitting;
  const std::set<int>      joinTriggered;
  const std::set<int>      joining;

  const auto& T = SpacetreeHelper::getFaceAdjTables();

  // Lower face in x-direction (faceNumber=0): for Hanging vertices on a lower face, only i1 is considered.
  {
    GridVertex vertices[TWO_POWER_D];
    initVertices(vertices, /*defaultRank*/ 7);

    const int faceNumber = 0;
    const int normal     = faceNumber % DIMENSIONS;

    const int v0 = T.faceVerts[faceNumber][0];
    const int v1 = T.faceVerts[faceNumber][1];

    vertices[v0].setState(GridVertex::State::HangingVertex);
    vertices[v1].setState(GridVertex::State::HangingVertex);

    const int i0_v0 = T.adjIdx0[normal][v0];
    const int i1_v0 = T.adjIdx1[normal][v0];
    const int i0_v1 = T.adjIdx0[normal][v1];
    const int i1_v1 = T.adjIdx1[normal][v1];

    // Put local id only into i0 (which should be ignored for Hanging vertices on a lower face).
    vertices[v0].adjacentRanks[i0_v0] = treeId;
    vertices[v1].adjacentRanks[i0_v1] = treeId;
    vertices[v0].adjacentRanks[i1_v0] = 7;
    vertices[v1].adjacentRanks[i1_v1] = 7;

    const auto facesLocal = SpacetreeHelper::
      areFacesLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining);
    validate(not facesLocal.test(faceNumber));

    // Now put local into i1 as well; face becomes local.
    vertices[v0].adjacentRanks[i1_v0] = treeId;
    const auto facesLocal2            = SpacetreeHelper::
      areFacesLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining);
    validate(facesLocal2.test(faceNumber));
  }

  // Upper face in x-direction (faceNumber=DIMENSIONS): for Hanging vertices on an upper face, only i0 is considered.
  {
    GridVertex vertices[TWO_POWER_D];
    initVertices(vertices, /*defaultRank*/ 7);

    const int faceNumber = DIMENSIONS;
    const int normal     = faceNumber % DIMENSIONS;

    const int v0 = T.faceVerts[faceNumber][0];

    vertices[v0].setState(GridVertex::State::HangingVertex);

    const int i0_v0 = T.adjIdx0[normal][v0];
    const int i1_v0 = T.adjIdx1[normal][v0];

    // Put local id only into i1 (which should be ignored for Hanging vertices on an upper face).
    vertices[v0].adjacentRanks[i0_v0] = 7;
    vertices[v0].adjacentRanks[i1_v0] = treeId;

    const auto facesLocal = SpacetreeHelper::
      areFacesLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining);
    validate(not facesLocal.test(faceNumber));

    vertices[v0].adjacentRanks[i0_v0] = treeId;
    const auto facesLocal2            = SpacetreeHelper::
      areFacesLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining);
    validate(facesLocal2.test(faceNumber));
  }
#endif
}

void peano4::grid::tests::SpacetreeHelperTest::testParallelDomainBoundaryAdjacencyFlags() {
#if DIMENSIONS == 2
  const int                treeId = 1;
  const SplitSpecification splitTriggered;
  const std::set<int>      splitting;
  const std::set<int>      joinTriggered;
  const std::set<int>      joining;

  GridVertex vertices[TWO_POWER_D];
  for (int v = 0; v < TWO_POWER_D; ++v) {
    vertices[v].setState(GridVertex::State::Unrefined);
    for (int j = 0; j < TWO_POWER_D; ++j) {
      vertices[v].adjacentRanks[j]         = treeId;
      vertices[v].backupOfAdjacentRanks[j] = treeId;
    }
  }

  // One vertex has both local and remote neighbours in its backup adjacency list.
  vertices[0].backupOfAdjacentRanks[0] = 2;

  const auto vertexFlags = SpacetreeHelper::
    areVerticesAdjacentToParallelDomainBoundary(vertices, treeId, splitTriggered, splitting, joinTriggered, joining);
  validate(vertexFlags.test(0));
  validate(not vertexFlags.test(1));

  // For a face, inject a remote rank into a backup entry that is guaranteed to contribute to this face.
  const auto& T                             = SpacetreeHelper::getFaceAdjTables();
  const int   faceNumber                    = 0;
  const int   normal                        = faceNumber % DIMENSIONS;
  const int   v0                            = T.faceVerts[faceNumber][0];
  const int   i0_v0                         = T.adjIdx0[normal][v0];
  vertices[v0].backupOfAdjacentRanks[i0_v0] = 2;

  const auto faceFlags = SpacetreeHelper::areFacesAdjacentToParallelDomainBoundary(
    vertices,
    treeId,
    splitTriggered,
    splitting,
    joinTriggered,
    joining,
    /*calledByLeaveCell*/ true
  );
  validate(faceFlags.test(faceNumber));
#endif
}

void peano4::grid::tests::SpacetreeHelperTest::testPatch3x3IndexTable() {
  const auto& tbl = SpacetreeHelper::getPatch3x3IndexTable().idx;

  // linear = sum_d ( (bit(k,d) + bit(j,d)) * 3^d ).
  for (int k = 0; k < TWO_POWER_D; ++k) {
    for (int j = 0; j < TWO_POWER_D; ++j) {
      int expected = 0;
      int stride   = 1;
      for (int d = 0; d < DIMENSIONS; ++d) {
        const int kd = (k >> d) & 1;
        const int jd = (j >> d) & 1;
        expected += (kd + jd) * stride;
        stride *= 3;
      }
      validateEqualsWithParams2(tbl[k][j], expected, k, j);
    }
  }
}

void peano4::grid::tests::SpacetreeHelperTest::testFaceAdjacencyTables() {
#if DIMENSIONS == 2
  const auto& T = SpacetreeHelper::getFaceAdjTables();

  // Face membership in 2D:
  // face 0 (dim0, bit=0): vertices with x-bit 0 -> 0,2
  // face 2 (dim0, bit=1): vertices with x-bit 1 -> 1,3
  // face 1 (dim1, bit=0): vertices with y-bit 0 -> 0,1
  // face 3 (dim1, bit=1): vertices with y-bit 1 -> 2,3
  for (int face = 0; face < TWO_TIMES_D; ++face) {
    bool      expectedOnFace[TWO_POWER_D] = {false, false, false, false};
    const int normal                      = face % DIMENSIONS;
    const int bit                         = face / DIMENSIONS;

    for (int v = 0; v < TWO_POWER_D; ++v) {
      const bool on     = ((v & (1 << normal)) != 0) == (bit == 1);
      expectedOnFace[v] = on;
    }

    // Table stores the two vertices on each face (order not relevant here).
    for (int k = 0; k < TWO_POWER_D / 2; ++k) {
      const int vIdx = T.faceVerts[face][k];
      validateWithParams2(expectedOnFace[vIdx], face, vIdx);
      expectedOnFace[vIdx] = false; // mark as seen
    }
    for (int v = 0; v < TWO_POWER_D; ++v) {
      validateWithParams2(not expectedOnFace[v], face, v);
    }
  }

  // Adjacency index formula (from implementation): base=(TWO_POWER_D-1)-v
  // adjIdx0 = base & ~(1<<n), adjIdx1 = base | (1<<n)
  for (int n = 0; n < DIMENSIONS; ++n) {
    for (int v = 0; v < TWO_POWER_D; ++v) {
      const int base = (TWO_POWER_D - 1) - v;
      validateEqualsWithParams3(T.adjIdx0[n][v], (base & ~(1 << n)), n, v, base);
      validateEqualsWithParams3(T.adjIdx1[n][v], (base | (1 << n)), n, v, base);
    }
  }
#endif
}

void peano4::grid::tests::SpacetreeHelperTest::testAreAllVerticesNonHanging() {
#if DIMENSIONS == 2
  GridVertex vertices[TWO_POWER_D];
  initVertices(vertices, /*defaultRank*/ 1);

  validate(SpacetreeHelper::areAllVerticesNonHanging(vertices));

  vertices[2].setState(GridVertex::State::HangingVertex);
  validate(not SpacetreeHelper::areAllVerticesNonHanging(vertices));
#endif
#if DIMENSIONS == 3
  GridVertex vertices[TWO_POWER_D];
  for (int v = 0; v < TWO_POWER_D; ++v) {
    vertices[v].setState(GridVertex::State::Unrefined);
  }

  validate(SpacetreeHelper::areAllVerticesNonHanging(vertices));

  vertices[0].setState(GridVertex::State::HangingVertex);
  validate(not SpacetreeHelper::areAllVerticesNonHanging(vertices));

  vertices[0].setState(GridVertex::State::Unrefined);
  validate(SpacetreeHelper::areAllVerticesNonHanging(vertices));

  vertices[TWO_POWER_D - 1].setState(GridVertex::State::HangingVertex);
  validate(not SpacetreeHelper::areAllVerticesNonHanging(vertices));
#endif
}

void peano4::grid::tests::SpacetreeHelperTest::testIsSpacetreeNodeLocal() {
#if DIMENSIONS == 2
  GridVertex vertices[TWO_POWER_D];
  initVertices(vertices, /*defaultRank*/ 1);

  const int           treeId = 1;
  SplitSpecification  splitTriggered;
  std::set<int>       splitting;
  const std::set<int> joinTriggered;
  const std::set<int> joining;

  // Owning-slot for vertex k is (TWO_POWER_D - k - 1).
  validate(
    SpacetreeHelper::
      isSpacetreeNodeLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining, false, false)
  );

  // Make one vertex owned by a remote rank -> non-local.
  vertices[0].adjacentRanks[TWO_POWER_D - 0 - 1] = 2;
  validate(
    not SpacetreeHelper::
      isSpacetreeNodeLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining, false, false)
  );

  // If that vertex is hanging, it is ignored for locality.
  vertices[0].setState(GridVertex::State::HangingVertex);
  validate(
    SpacetreeHelper::
      isSpacetreeNodeLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining, false, false)
  );

  // splitTriggered ranks are always treated as local.
  vertices[0].setState(GridVertex::State::Unrefined);
  vertices[0].adjacentRanks[TWO_POWER_D - 0 - 1] = 4;
  splitTriggered[4]                              = SplitInstruction{0, SplitInstruction::Mode::BottomUp};
  validate(
    SpacetreeHelper::
      isSpacetreeNodeLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining, false, false)
  );

  // Splitting ranks are treated as local only if splittingIsConsideredLocal is true.
  splitTriggered.clear();
  splitting.insert(5);
  vertices[0].adjacentRanks[TWO_POWER_D - 0 - 1] = 5;
  validate(
    not SpacetreeHelper::
      isSpacetreeNodeLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining, false, false)
  );
  validate(
    SpacetreeHelper::
      isSpacetreeNodeLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining, true, false)
  );
#elif DIMENSIONS == 3
  const int treeId = 1;

  GridVertex vertices[TWO_POWER_D];

  SplitSpecification  splitTriggered;
  std::set<int>       splitting;
  const std::set<int> joinTriggered;
  std::set<int>       joining;

  const int v          = 3;                   // any vertex 0..7 is fine in 3D
  const int owningSlot = TWO_POWER_D - v - 1; // matches isSpacetreeNodeLocal()

  // All local => local
  initVertices(vertices, treeId);
  validate(
    SpacetreeHelper::
      isSpacetreeNodeLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining, false, false)
  );

  // One non-hanging vertex owned by remote rank => non-local
  initVertices(vertices, treeId);
  splitTriggered.clear();
  splitting.clear();
  joining.clear();
  vertices[v].adjacentRanks[owningSlot] = 2;
  validate(
    not SpacetreeHelper::
      isSpacetreeNodeLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining, false, false)
  );

  // Hanging vertices are ignored for locality checks
  vertices[v].setState(GridVertex::State::HangingVertex);
  validate(
    SpacetreeHelper::
      isSpacetreeNodeLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining, false, false)
  );

  // splitTriggered ranks are treated as local
  initVertices(vertices, treeId);
  splitTriggered.clear();
  splitting.clear();
  joining.clear();
  vertices[v].adjacentRanks[owningSlot] = 4;
  splitTriggered[4]                     = SplitInstruction{0, SplitInstruction::Mode::BottomUp};
  validate(
    SpacetreeHelper::
      isSpacetreeNodeLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining, false, false)
  );

  // Splitting ranks are treated as local only if splittingIsConsideredLocal==true
  initVertices(vertices, treeId);
  splitTriggered.clear();
  splitting.clear();
  joining.clear();
  vertices[v].adjacentRanks[owningSlot] = 5;
  splitting.insert(5);
  validate(
    not SpacetreeHelper::
      isSpacetreeNodeLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining, false, false)
  );
  validate(
    SpacetreeHelper::
      isSpacetreeNodeLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining, true, false)
  );

  // Joining ranks are treated as local only if joiningIsConsideredLocal==true
  initVertices(vertices, treeId);
  splitTriggered.clear();
  splitting.clear();
  joining.clear();
  vertices[v].adjacentRanks[owningSlot] = 6;
  joining.insert(6);
  validate(
    !SpacetreeHelper::
      isSpacetreeNodeLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining, false, false)
  );
  validate(
    SpacetreeHelper::
      isSpacetreeNodeLocal(vertices, treeId, splitTriggered, splitting, joinTriggered, joining, false, true)
  );
#endif
}

void peano4::grid::tests::SpacetreeHelperTest::testGetTreeOwningSpacetreeNode() {
#if DIMENSIONS == 2
  GridVertex vertices[TWO_POWER_D];
  initVertices(vertices, /*defaultRank*/ 1);

  const int                treeId = 1;
  const SplitSpecification splitTriggered;
  const std::set<int>      splitting;
  const std::set<int>      joinTriggered;
  const std::set<int>      joining;

  // Fully local: owner is the local tree id.
  validateEquals(
    SpacetreeHelper::getTreeOwningSpacetreeNode(vertices, treeId, splitTriggered, splitting, joinTriggered, joining),
    treeId
  );

  // Fully remote: no vertex adjacent to local; should fall back to weakId (remote owner).
  initVertices(vertices, /*defaultRank*/ 2);
  validateEquals(
    SpacetreeHelper::getTreeOwningSpacetreeNode(vertices, treeId, splitTriggered, splitting, joinTriggered, joining),
    2
  );

  // splitTriggered counts as local adjacency and can define ownership.
  SplitSpecification splitTriggered2;
  splitTriggered2[3] = SplitInstruction{0, SplitInstruction::Mode::BottomUp};
  initVertices(vertices, /*defaultRank*/ 3);
  validateEquals(
    SpacetreeHelper::getTreeOwningSpacetreeNode(vertices, treeId, splitTriggered2, splitting, joinTriggered, joining),
    3
  );
#endif
}

void peano4::grid::tests::SpacetreeHelperTest::testIsSpacetreeNodeSplitCandidate() {
#if DIMENSIONS == 2
  const int treeId = 1;

  GridVertex coarseGridVertices[TWO_POWER_D];
  GridVertex fineGridVertices[TWO_POWER_D];
  initVertices(coarseGridVertices, treeId);
  initVertices(fineGridVertices, treeId);

  SplitSpecification splitTriggered;
  std::set<int>      splitting;
  std::set<int>      joinTriggered;
  std::set<int>      joining;

  for (int v = 0; v < TWO_POWER_D; ++v) {
    fineGridVertices[v].setFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep, false);
  }

  validate(
    SpacetreeHelper::isSpacetreeNodeSplitCandidate(
      coarseGridVertices,
      fineGridVertices,
      treeId,
      splitTriggered,
      splitting,
      joinTriggered,
      joining
    )
  );

  // Periodic BC rank in any adjacency entry disqualifies the candidate.
  fineGridVertices[0].adjacentRanks[0] = SpacetreeHelper::RankOfPeriodicBoundaryCondition;
  validate(not SpacetreeHelper::isSpacetreeNodeSplitCandidate(
    coarseGridVertices,
    fineGridVertices,
    treeId,
    splitTriggered,
    splitting,
    joinTriggered,
    joining
  ));

  // Parent-of-subtree flag disqualifies the candidate.
  initVertices(fineGridVertices, treeId);
  fineGridVertices[1].setFlag(GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep, true);
  validate(not SpacetreeHelper::isSpacetreeNodeSplitCandidate(
    coarseGridVertices,
    fineGridVertices,
    treeId,
    splitTriggered,
    splitting,
    joinTriggered,
    joining
  ));
#endif
}
