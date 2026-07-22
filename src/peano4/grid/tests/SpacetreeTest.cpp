// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeTest.h"

#include "peano4/Globals.h"
#include "peano4/Loop.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/grid/SpacetreeHelper.h"
#include "tarch/la/Vector.h"

namespace {
  struct CountingTraversalObserver final : public peano4::grid::GridTraversalObserver {
    int               loadNodeDataCalls  = 0;
    int               enterNodeCalls     = 0;
    int               leaveNodeCalls     = 0;
    int               storeNodeDataCalls = 0;
    std::vector<int>  enteredLevels;
    std::vector<bool> enteredCellIsLocal;
    std::vector<int>  enteredVertexBoundaryMasks;
    std::vector<int>  enteredVertexLocalMasks;

    void loadNodeData(const peano4::grid::GridTraversalEvent&) override { ++loadNodeDataCalls; }

    void enterNode(const peano4::grid::GridTraversalEvent& event) override {
      ++enterNodeCalls;
      enteredLevels.push_back(event.level);
      enteredCellIsLocal.push_back(event.getIsCellLocal());
      enteredVertexBoundaryMasks.push_back(event.getIsVertexAdjacentToParallelDomainBoundary());
      enteredVertexLocalMasks.push_back(event.getIsVertexLocal());
    }

    void leaveNode(const peano4::grid::GridTraversalEvent&) override { ++leaveNodeCalls; }

    void storeNodeData(const peano4::grid::GridTraversalEvent&) override { ++storeNodeDataCalls; }

    peano4::grid::GridTraversalObserver* clone(int) override { return new CountingTraversalObserver(*this); }

    std::vector<peano4::grid::GridControlEvent> getGridControlEvents() const override { return {}; }

    void beginTraversal(
      const tarch::la::Vector<DIMENSIONS, double>&,
      const tarch::la::Vector<DIMENSIONS, double>&
    ) override {}

    void
    endTraversal(const tarch::la::Vector<DIMENSIONS, double>&, const tarch::la::Vector<DIMENSIONS, double>&) override {}
  };

  std::string vectorToString(const std::vector<int>& v) {
    std::ostringstream out;
    out << "[";
    for (std::size_t i = 0; i < v.size(); ++i) {
      if (i) {
        out << ",";
      }
      out << v[i];
    }
    out << "]";
    return out.str();
  }

  peano4::grid::GridVertex makeVertex(
    peano4::grid::GridVertex::State     state,
    const std::array<int, TWO_POWER_D>& adjacency,
    std::uint8_t                        flags = 0
  ) {
    peano4::grid::GridVertex vertex;
    vertex.setState(state);
    vertex.adjacentRanks         = adjacency;
    vertex.backupOfAdjacentRanks = adjacency;
    vertex.flags                 = flags;
    vertex.level                 = 4;
    return vertex;
  }

  peano4::grid::Spacetree
  createUnitTree(const int treeId = 0, const std::bitset<DIMENSIONS>& periodicBC = std::bitset<DIMENSIONS>{}) {
#if DIMENSIONS == 2
    return peano4::grid::Spacetree(treeId, {0.0, 0.0}, {1.0, 1.0}, periodicBC);
#else
    return peano4::grid::Spacetree(treeId, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, periodicBC);
#endif
  }

  void createRootVertices(
    int                             treeId,
    peano4::grid::GridVertex::State state,
    peano4::grid::GridVertex        vertices[TWO_POWER_D]
  ) {
    peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, int kScalar) {
      std::array<int, TWO_POWER_D>          adjacentRanks;
      tarch::la::Vector<DIMENSIONS, double> x(0.0);
      for (int d = 0; d < DIMENSIONS; ++d) {
        x(d) = static_cast<double>(k(d));
      }
      adjacentRanks.fill(-1);
      adjacentRanks[TWO_POWER_D - 1 - kScalar] = treeId;
      vertices[kScalar]                        = peano4::grid::GridVertex::create(0, state, x, adjacentRanks, false);
      vertices[kScalar].setFlag(peano4::grid::GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep, true);
    });
  }

  peano4::grid::GridControlEvent createFarAwayRefineEvent() {
    peano4::grid::GridControlEvent result;
    result.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Refine);
    for (int d = 0; d < DIMENSIONS; ++d) {
      result.offset[d]  = 2.0;
      result.width[d]   = 0.1;
      result.targetH[d] = 0.01;
    }
    return result;
  }
} // namespace

peano4::grid::tests::SpacetreeTest::SpacetreeTest() :
  TestCase("peano4::grid::tests::SpacetreeTest") {}

void peano4::grid::tests::SpacetreeTest::run() {
  testMethod(testRestrictToCoarseGrid);
  testMethod(testUpdateVertexRanksWithinCell);
  testMethod(testIncrementNumberOfAdjacentRefinedLocalCells);
  testMethod(testDoesRankIndexIdentifyHorizontalDataExchange);
  testMethod(testHorizontalExchangeSymmetry);
  testMethod(testNoTopologyChangeParity);
  testMethod(testVertexNeighbourPermutationInvariance);
  testMethod(testDescend);
  testMethod(testRefineEventTouchDoesNotTriggerNeighbourRefinement);
  testMethod(testDeferredGridControlChangesAreCounted);
  testMethod(testEraseRingKeepsUnnamedRefinedParent);
  testMethod(testEraseOfRefinementSourceNeighbourhoodIsVetoed);
  testMethod(testRefineCascadeIsMirrorSymmetric);
  testMethod(testEvaluateGridControlEventsEraseRules);
  testMethod(testGetNeighbourTreesVertexSendReceive);
  testMethod(testGetNeighbourTreesFaceSendReceive);
  testMethod(testIsFaceAlongPeriodicBoundaryCondition);
  testMethod(testTraverseWithPeriodicBoundaryConditionsUsesStableVertexBoundaryFlags);
  testMethod(testMergeGridVertexRefinementStateAtHorizontalDomainBoundaryIsSymmetric);
  testMethod(testMergeAccumulatesRemoteRefinedCellCounts);
  testMethod(testPostRefinesVertexSurroundedAcrossTreeBoundaries);
  testMethod(testPostRefinesVertexSurroundedAtDomainBoundary);
  testMethod(testUpdateVertexBeforeStoreTreatsErasingVerticesAsAntecessors);
  testMethod(testUpdateVertexAfterLoadPreviousHangingClassification);
  testMethod(testSplittingTreeCursorSkipsCompletedRequests);
  testMethod(testSplittingTreeCursorResetsAcrossTraversal);
  testMethod(testSplitModeNormalisationUsesLowestTreeIdMode);
}

void peano4::grid::tests::SpacetreeTest::testSplittingTreeCursorSkipsCompletedRequests() {
  auto tree = createUnitTree();

  constexpr int NumberOfCompletedRequests = 4096;
  for (int requestIndex = 0; requestIndex < NumberOfCompletedRequests; requestIndex++) {
    tree._splitTriggered.emplace(
      requestIndex + 1,
      peano4::grid::SplitInstruction{0, peano4::grid::SplitInstruction::Mode::BottomUp}
    );
  }
  constexpr int PendingTreeId = NumberOfCompletedRequests + 1;
  tree._splitTriggered.emplace(
    PendingTreeId,
    peano4::grid::SplitInstruction{2, peano4::grid::SplitInstruction::Mode::BottomUp}
  );

  tree.resetSplittingTreeCursor();
  validateEquals(tree.getSplittingTree(), PendingTreeId);
  validate(tree._nextSplitInstruction != tree._splitTriggered.end());
  validateEquals(tree._nextSplitInstruction->first, PendingTreeId);

  tree.updateSplittingCounter(PendingTreeId);
  validateEquals(tree._splitTriggered.at(PendingTreeId).numberOfFineGridCells, 1);
  validateEquals(tree.getSplittingTree(), PendingTreeId);

  tree.updateSplittingCounter(PendingTreeId);
  validateEquals(tree.getSplittingTree(), -1);
  validate(tree._nextSplitInstruction == tree._splitTriggered.end());
}

void peano4::grid::tests::SpacetreeTest::testSplittingTreeCursorResetsAcrossTraversal() {
  auto tree = createUnitTree();

  CountingTraversalObserver initialTraversalObserver;
  tree.traverse(initialTraversalObserver);

  constexpr int WorkerTreeId = 7;
  tree.split(WorkerTreeId, peano4::grid::SplitInstruction{1, peano4::grid::SplitInstruction::Mode::BottomUp});
  tree._nextSplitInstruction = tree._splitTriggered.end();

  CountingTraversalObserver splitTraversalObserver;
  tree.traverse(splitTraversalObserver);

  validate(tree._splitTriggered.empty());
  validate(tree._nextSplitInstruction == tree._splitTriggered.end());
  validate(tree._splitting.count(WorkerTreeId) == 1);
}

void peano4::grid::tests::SpacetreeTest::testSplitModeNormalisationUsesLowestTreeIdMode() {
  auto higherIncomingIdTree                                    = createUnitTree();
  higherIncomingIdTree._spacetreeState                         = peano4::grid::SpacetreeState::Running;
  higherIncomingIdTree._statistics.numberOfLocalUnrefinedCells = 1;

  constexpr int                        FirstExistingTreeId  = 20;
  constexpr int                        SecondExistingTreeId = 30;
  constexpr int                        HigherIncomingTreeId = 40;
  constexpr int                        SameModeTreeId       = 50;
  const peano4::grid::SplitInstruction bottomUpInstruction{1, peano4::grid::SplitInstruction::Mode::BottomUp};
  const peano4::grid::SplitInstruction aggressiveInstruction{
    1,
    peano4::grid::SplitInstruction::Mode::AggressiveTopDown
  };

  higherIncomingIdTree.split(FirstExistingTreeId, bottomUpInstruction);
  higherIncomingIdTree.split(SecondExistingTreeId, bottomUpInstruction);
  higherIncomingIdTree.split(HigherIncomingTreeId, aggressiveInstruction);
  higherIncomingIdTree.split(SameModeTreeId, bottomUpInstruction);
  for (const auto& splitEntry : higherIncomingIdTree._splitTriggered) {
    validate(splitEntry.second.mode == peano4::grid::SplitInstruction::Mode::BottomUp);
  }

  auto lowerIncomingIdTree                                    = createUnitTree();
  lowerIncomingIdTree._spacetreeState                         = peano4::grid::SpacetreeState::Running;
  lowerIncomingIdTree._statistics.numberOfLocalUnrefinedCells = 1;

  constexpr int LowerIncomingTreeId = 10;
  lowerIncomingIdTree.split(FirstExistingTreeId, bottomUpInstruction);
  lowerIncomingIdTree.split(SecondExistingTreeId, bottomUpInstruction);
  lowerIncomingIdTree.split(LowerIncomingTreeId, aggressiveInstruction);
  for (const auto& splitEntry : lowerIncomingIdTree._splitTriggered) {
    validate(splitEntry.second.mode == peano4::grid::SplitInstruction::Mode::AggressiveTopDown);
  }
}

// Checks the coarse-grid restriction mapping from 3^D patch coordinates back to 2^D parent-cell coordinates.
void peano4::grid::tests::SpacetreeTest::testRestrictToCoarseGrid() {
#if DIMENSIONS == 2
  tarch::la::Vector<DIMENSIONS, int> positionInPatch;
  tarch::la::Vector<DIMENSIONS, int> coarsePosition;

  positionInPatch = {0, 0};
  coarsePosition  = {0, 0};
  validateWithParams2(
    Spacetree::restrictToCoarseGrid(coarsePosition, positionInPatch),
    coarsePosition,
    positionInPatch
  );

  positionInPatch = {0, 3};
  coarsePosition  = {0, 1};
  validateWithParams2(
    Spacetree::restrictToCoarseGrid(coarsePosition, positionInPatch),
    coarsePosition,
    positionInPatch
  );

  positionInPatch = {3, 3};
  coarsePosition  = {1, 1};
  validateWithParams2(
    Spacetree::restrictToCoarseGrid(coarsePosition, positionInPatch),
    coarsePosition,
    positionInPatch
  );

  positionInPatch = {2, 2};
  coarsePosition  = {1, 1};
  validateWithParams2(
    Spacetree::restrictToCoarseGrid(coarsePosition, positionInPatch),
    coarsePosition,
    positionInPatch
  );

  positionInPatch = {2, 3};
  coarsePosition  = {1, 1};
  validateWithParams2(
    Spacetree::restrictToCoarseGrid(coarsePosition, positionInPatch),
    coarsePosition,
    positionInPatch
  );

  positionInPatch = {0, 2};
  coarsePosition  = {1, 1};
  validateWithParams2(
    not Spacetree::restrictToCoarseGrid(coarsePosition, positionInPatch),
    coarsePosition,
    positionInPatch
  );

  positionInPatch = {0, 2};
  coarsePosition  = {0, 0};
  validateWithParams2(
    Spacetree::restrictToCoarseGrid(coarsePosition, positionInPatch),
    coarsePosition,
    positionInPatch
  );
#endif
}

// Verifies that updating ranks within one cell only overwrites the owning adjacency slot of each vertex.
void peano4::grid::tests::SpacetreeTest::testUpdateVertexRanksWithinCell() {
#if DIMENSIONS == 2
  GridVertex fineGridVertices[TWO_POWER_D];

  int before[TWO_POWER_D][TWO_POWER_D];
  for (int k = 0; k < TWO_POWER_D; ++k) {
    fineGridVertices[k].setState(GridVertex::State::Unrefined);
    for (int j = 0; j < TWO_POWER_D; ++j) {
      fineGridVertices[k].adjacentRanks[j]         = 100 + k * 10 + j;
      fineGridVertices[k].backupOfAdjacentRanks[j] = 200 + k * 10 + j;
      before[k][j]                                 = fineGridVertices[k].adjacentRanks[j];
    }
  }

  const int newId = 42;
  Spacetree::updateVertexRanksWithinCell(fineGridVertices, newId);

  for (int k = 0; k < TWO_POWER_D; ++k) {
    const int owningSlot = TWO_POWER_D - k - 1;
    for (int j = 0; j < TWO_POWER_D; ++j) {
      if (j == owningSlot) {
        validateEqualsWithParams2(fineGridVertices[k].adjacentRanks[j], newId, k, j);
      } else {
        validateEqualsWithParams3(fineGridVertices[k].adjacentRanks[j], before[k][j], k, j, owningSlot);
      }
    }
  }
#endif
}

// Verifies vertex neighbour discovery for send/receive, including filtering of splitting ranks and hanging vertices.
void peano4::grid::tests::SpacetreeTest::testGetNeighbourTreesVertexSendReceive() {
#if DIMENSIONS == 2
  Spacetree tree(1, {0.0, 0.0}, {1.0, 1.0}, std::bitset<DIMENSIONS>{});

  GridVertex v;
  v.setState(GridVertex::State::Unrefined);
  v.adjacentRanks         = {1, 3, 2, SpacetreeHelper::RankOfPeriodicBoundaryCondition};
  v.backupOfAdjacentRanks = {1, 5, 4, -1};

  tree._splitting.clear();
  tree._splitting.insert(4); // Should be filtered on receiving side

  const auto sendNeighbours = SpacetreeHelper::getNeighbourTreesAlongVertex(
    v,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    false,
    tree._spacetreeState,
    false
  );
  validateEqualsWithParams1(sendNeighbours.size(), 2, vectorToString(sendNeighbours));
  validateEqualsWithParams1(sendNeighbours[0], 2, vectorToString(sendNeighbours));
  validateEqualsWithParams1(sendNeighbours[1], 3, vectorToString(sendNeighbours));

  const auto recvNeighbours = SpacetreeHelper::getNeighbourTreesAlongVertex(
    v,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState,
    false
  );
  validateEqualsWithParams1(recvNeighbours.size(), 1, vectorToString(recvNeighbours));
  validateEqualsWithParams1(recvNeighbours[0], 5, vectorToString(recvNeighbours));

  // Hanging vertices are never considered for neighbour discovery.
  v.setState(GridVertex::State::HangingVertex);
  const auto sendHanging = SpacetreeHelper::getNeighbourTreesAlongVertex(
    v,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    false,
    tree._spacetreeState,
    false
  );
  const auto recvHanging = SpacetreeHelper::getNeighbourTreesAlongVertex(
    v,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState,
    false
  );
  validateEquals(sendHanging.size(), 0);
  validateEquals(recvHanging.size(), 0);
#endif
#if DIMENSIONS == 3
  Spacetree tree(1, {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, std::bitset<DIMENSIONS>{});

  GridVertex v;
  v.setState(GridVertex::State::Unrefined);

  // adjacentRanks is used to decide whether the vertex is adjacent to the local spacetree.
  // backupOfAdjacentRanks is used on the receiving side to determine neighbours.
  v.adjacentRanks         = {1, 3, 2, SpacetreeHelper::RankOfPeriodicBoundaryCondition, 2, -1, 4, 4};
  v.backupOfAdjacentRanks = {1, 5, 4, -1, 5, SpacetreeHelper::RankOfPeriodicBoundaryCondition, 6, 4};

  tree._splitting.clear();
  tree._splitting.insert(4); // Filtered only on receiving side by doesRankIdentifyHorizontalDataExchange()

  const auto sendNeighbours = SpacetreeHelper::getNeighbourTreesAlongVertex(
    v,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    false,
    tree._spacetreeState,
    false
  );
  // The sending side does not filter _splitting; it filters _splitTriggered and _joining (and the Joining state).
  validateEqualsWithParams1(static_cast<int>(sendNeighbours.size()), 3, vectorToString(sendNeighbours));
  validateEqualsWithParams1(sendNeighbours[0], 2, vectorToString(sendNeighbours));
  validateEqualsWithParams1(sendNeighbours[1], 3, vectorToString(sendNeighbours));
  validateEqualsWithParams1(sendNeighbours[2], 4, vectorToString(sendNeighbours));

  const auto recvNeighbours = SpacetreeHelper::getNeighbourTreesAlongVertex(
    v,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState,
    false
  );
  // The receiving side filters both _splitting and _joining.
  validateEqualsWithParams1(static_cast<int>(recvNeighbours.size()), 2, vectorToString(recvNeighbours));
  validateEqualsWithParams1(recvNeighbours[0], 5, vectorToString(recvNeighbours));
  validateEqualsWithParams1(recvNeighbours[1], 6, vectorToString(recvNeighbours));

  // Hanging vertices are never considered for neighbour discovery.
  v.setState(GridVertex::State::HangingVertex);
  const auto sendHanging = SpacetreeHelper::getNeighbourTreesAlongVertex(
    v,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    false,
    tree._spacetreeState,
    false
  );
  const auto recvHanging = SpacetreeHelper::getNeighbourTreesAlongVertex(
    v,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState,
    false
  );
  validateEquals(sendHanging.size(), 0);
  validateEquals(recvHanging.size(), 0);
#endif
}

// Verifies face neighbour discovery for send/receive, including suppression of invalid vertices and non-local faces.
void peano4::grid::tests::SpacetreeTest::testGetNeighbourTreesFaceSendReceive() {
#if DIMENSIONS == 2
  Spacetree tree(1, {0.0, 0.0}, {1.0, 1.0}, std::bitset<DIMENSIONS>{});

  GridVertex fineGridVertices[TWO_POWER_D];
  for (int v = 0; v < TWO_POWER_D; ++v) {
    fineGridVertices[v].setState(GridVertex::State::Unrefined);
    for (int j = 0; j < TWO_POWER_D; ++j) {
      fineGridVertices[v].adjacentRanks[j]         = tree._id;
      fineGridVertices[v].backupOfAdjacentRanks[j] = tree._id;
    }
  }

  const auto& faceTable = SpacetreeHelper::getFaceAdjTables();

  const int faceNumber        = 0;
  const int normal            = faceNumber % DIMENSIONS;
  const int v0                = faceTable.faceVerts[faceNumber][0];
  const int v1                = faceTable.faceVerts[faceNumber][1];
  const int adjacencyIndexV0  = faceTable.adjIdx0[normal][v0];
  const int adjacencyIndexV1  = faceTable.adjIdx0[normal][v1];

  // Put a unique remote neighbour on the face.
  fineGridVertices[v0].adjacentRanks[adjacencyIndexV0] = 2;
  fineGridVertices[v1].adjacentRanks[adjacencyIndexV1] = tree._id;

  const int sendNeighbour = SpacetreeHelper::getNeighbourTreeAlongFace(
    fineGridVertices,
    faceNumber,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    false,
    tree._spacetreeState,
    tree._noParallelTopologyChanges
  );
  validateEqualsWithParams1(sendNeighbour, 2, sendNeighbour);

  // Receiving side ignores New vertices when forming the face adjacency list.
  fineGridVertices[v0].setState(GridVertex::State::New);
  fineGridVertices[v0].backupOfAdjacentRanks[adjacencyIndexV0] = 2;

  const int recvNeighbourWithNewVertex = SpacetreeHelper::getNeighbourTreeAlongFace(
    fineGridVertices,
    faceNumber,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState,
    tree._noParallelTopologyChanges
  );
  validateEquals(recvNeighbourWithNewVertex, GridTraversalObserver::NoData);

  // Receiving side suppresses neighbours in _splitting.
  fineGridVertices[v0].setState(GridVertex::State::Unrefined);
  fineGridVertices[v0].backupOfAdjacentRanks[adjacencyIndexV0] = 3;
  tree._splitting.clear();
  tree._splitting.insert(3);

  const int recvNeighbourWithSplittingNeighbour = SpacetreeHelper::getNeighbourTreeAlongFace(
    fineGridVertices,
    faceNumber,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState,
    false
  );
  validateEquals(recvNeighbourWithSplittingNeighbour, GridTraversalObserver::NoData);

  // If the face isn't adjacent to the local tree at all, there is no neighbour.
  for (int k = 0; k < TWO_POWER_D / 2; ++k) {
    const int vIdx = faceTable.faceVerts[faceNumber][k];
    for (int j = 0; j < TWO_POWER_D; ++j) {
      fineGridVertices[vIdx].adjacentRanks[j]         = 7;
      fineGridVertices[vIdx].backupOfAdjacentRanks[j] = 7;
    }
  }
  const int noLocalNeighbour = SpacetreeHelper::getNeighbourTreeAlongFace(
    fineGridVertices,
    faceNumber,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    false,
    tree._spacetreeState,
    false
  );
  validateEquals(noLocalNeighbour, GridTraversalObserver::NoData);
#endif
}

// Verifies that periodic boundary markers are recognised on faces only when the face is still adjacent to the local
// tree.
void peano4::grid::tests::SpacetreeTest::testIsFaceAlongPeriodicBoundaryCondition() {
#if DIMENSIONS == 2
  Spacetree tree(1, {0.0, 0.0}, {1.0, 1.0}, std::bitset<DIMENSIONS>{});

  GridVertex fineGridVertices[TWO_POWER_D];
  for (int v = 0; v < TWO_POWER_D; ++v) {
    fineGridVertices[v].setState(GridVertex::State::Unrefined);
    for (int j = 0; j < TWO_POWER_D; ++j) {
      fineGridVertices[v].adjacentRanks[j]         = tree._id;
      fineGridVertices[v].backupOfAdjacentRanks[j] = tree._id;
    }
  }

  const auto& faceTable        = SpacetreeHelper::getFaceAdjTables();
  const int   faceNumber       = 1;
  const int   normal           = faceNumber % DIMENSIONS;
  const int   v0               = faceTable.faceVerts[faceNumber][0];
  const int   adjacencyIndexV0 = faceTable.adjIdx0[normal][v0];

  // Inject periodic BC rank on a face that is still adjacent to the local tree.
  fineGridVertices[v0].adjacentRanks[adjacencyIndexV0] = SpacetreeHelper::RankOfPeriodicBoundaryCondition;

  validate(tree.isFaceAlongPeriodicBoundaryCondition(fineGridVertices, faceNumber, /*calledByReceivingProcess*/ false));

  // Periodic alone (without adjacency to local) should not trigger the flag.
  for (int k = 0; k < TWO_POWER_D / 2; ++k) {
    const int vIdx = faceTable.faceVerts[faceNumber][k];
    for (int j = 0; j < TWO_POWER_D; ++j) {
      fineGridVertices[vIdx].adjacentRanks[j] = 7;
    }
  }
  fineGridVertices[v0].adjacentRanks[adjacencyIndexV0] = SpacetreeHelper::RankOfPeriodicBoundaryCondition;
  validate(not tree.isFaceAlongPeriodicBoundaryCondition(fineGridVertices, faceNumber, false));
#endif
}

// Verifies that traversal reports vertex boundary flags consistently with vertex locality when periodic boundaries are
// active.
void peano4::grid::tests::SpacetreeTest::testTraverseWithPeriodicBoundaryConditionsUsesStableVertexBoundaryFlags() {
  std::bitset<DIMENSIONS> periodicBC;
  periodicBC.set(0); // x-direction
#if DIMENSIONS == 3
  periodicBC.set(2); // z-direction
#endif

  Spacetree tree = createUnitTree(0, periodicBC);

  CountingTraversalObserver observer;
  tree.traverse(observer);

  validate(tree._spacetreeState == SpacetreeState::Running);
  validate(observer.enterNodeCalls > 0);
  validateEquals(static_cast<int>(observer.enteredVertexBoundaryMasks.size()), observer.enterNodeCalls);
  validateEquals(static_cast<int>(observer.enteredVertexLocalMasks.size()), observer.enterNodeCalls);

  for (int i = 0; i < observer.enterNodeCalls; ++i) {
    const int boundaryMask = observer.enteredVertexBoundaryMasks[i];
    const int localMask    = observer.enteredVertexLocalMasks[i];
    validateEqualsWithParams2((boundaryMask & ~localMask), 0, boundaryMask, localMask);
  }
}

// Checks that counting adjacent refined local cells increments the per-vertex counter once per contributing local
// refined cell.
void peano4::grid::tests::SpacetreeTest::testIncrementNumberOfAdjacentRefinedLocalCells() {
#if DIMENSIONS == 2
  GridVertex vertices[TWO_POWER_D];
  for (int v = 0; v < TWO_POWER_D; ++v) {
    vertices[v].setState(GridVertex::State::Unrefined);
    vertices[v].numberOfAdjacentRefinedLocalCells = 0;
  }

  Spacetree::incrementNumberOfAdjacentRefinedLocalCells(vertices);
  for (int v = 0; v < TWO_POWER_D; ++v) {
    validateEqualsWithParams2(vertices[v].numberOfAdjacentRefinedLocalCells, 1, v, vertices[v].toString());
  }

  Spacetree::incrementNumberOfAdjacentRefinedLocalCells(vertices);
  for (int v = 0; v < TWO_POWER_D; ++v) {
    validateEqualsWithParams2(vertices[v].numberOfAdjacentRefinedLocalCells, 2, v, vertices[v].toString());
  }
#endif
}

// Verifies the primitive horizontal-exchange predicate across mandatory exclusions, transition states, and tree states.
void peano4::grid::tests::SpacetreeTest::testDoesRankIndexIdentifyHorizontalDataExchange() {
#if DIMENSIONS == 2
  Spacetree tree(1, {0.0, 0.0}, {1.0, 1.0}, std::bitset<DIMENSIONS>{});
  tree._spacetreeState = SpacetreeState::Running;
  tree._splitting.clear();
  tree._splitTriggered.clear();
  tree._joining.clear();

  // Mandatory exclusions.
  validate(not SpacetreeHelper::doesRankIdentifyHorizontalDataExchange(
    tree._id,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState
  ));
  validate(not SpacetreeHelper::doesRankIdentifyHorizontalDataExchange(
    -1,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState
  ));
  validate(not SpacetreeHelper::doesRankIdentifyHorizontalDataExchange(
    SpacetreeHelper::RankOfPeriodicBoundaryCondition,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState
  ));

  // Default: a remote rank is eligible for both sender and receiver.
  validate(
    SpacetreeHelper::doesRankIdentifyHorizontalDataExchange(
      2,
      tree._id,
      tree._splitTriggered,
      tree._splitting,
      tree._joining,
      true,
      tree._spacetreeState
    )
  );
  validate(
    SpacetreeHelper::doesRankIdentifyHorizontalDataExchange(
      2,
      tree._id,
      tree._splitTriggered,
      tree._splitting,
      tree._joining,
      false,
      tree._spacetreeState
    )
  );

  // Receiver filters ranks that are currently splitting.
  tree._splitting.insert(2);
  validate(not SpacetreeHelper::doesRankIdentifyHorizontalDataExchange(
    2,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState
  ));
  validate(
    SpacetreeHelper::doesRankIdentifyHorizontalDataExchange(
      2,
      tree._id,
      tree._splitTriggered,
      tree._splitting,
      tree._joining,
      false,
      tree._spacetreeState
    )
  );
  tree._splitting.clear();

  // Sender filters ranks for which a split has been triggered.
  tree._splitTriggered[3] = SplitInstruction{0, SplitInstruction::Mode::BottomUp};
  validate(
    SpacetreeHelper::doesRankIdentifyHorizontalDataExchange(
      3,
      tree._id,
      tree._splitTriggered,
      tree._splitting,
      tree._joining,
      true,
      tree._spacetreeState
    )
  );
  validate(not SpacetreeHelper::doesRankIdentifyHorizontalDataExchange(
    3,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    false,
    tree._spacetreeState
  ));
  tree._splitTriggered.clear();

  // Both sender and receiver suppress ranks that are joining.
  tree._joining.insert(4);
  validate(not SpacetreeHelper::doesRankIdentifyHorizontalDataExchange(
    4,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState
  ));
  validate(not SpacetreeHelper::doesRankIdentifyHorizontalDataExchange(
    4,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    false,
    tree._spacetreeState
  ));
  tree._joining.clear();

  // Sender criteria also depends on the spacetree state.
  tree._spacetreeState = SpacetreeState::Joining;
  validate(not SpacetreeHelper::doesRankIdentifyHorizontalDataExchange(
    2,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    false,
    tree._spacetreeState
  ));
  validate(
    SpacetreeHelper::doesRankIdentifyHorizontalDataExchange(
      2,
      tree._id,
      tree._splitTriggered,
      tree._splitting,
      tree._joining,
      true,
      tree._spacetreeState
    )
  );
#endif
}

// Verifies that matching synthetic topologies produce protocol-symmetric sender/receiver decisions for both vertices
// and faces.
void peano4::grid::tests::SpacetreeTest::testHorizontalExchangeSymmetry() {
  Spacetree sender         = createUnitTree(1);
  Spacetree receiver       = createUnitTree(2);
  sender._spacetreeState   = SpacetreeState::Running;
  receiver._spacetreeState = SpacetreeState::Running;

  GridVertex vertexOnSender;
  vertexOnSender.setState(GridVertex::State::Unrefined);
  GridVertex vertexOnReceiver;
  vertexOnReceiver.setState(GridVertex::State::Unrefined);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    vertexOnSender.adjacentRanks[i]           = -1;
    vertexOnSender.backupOfAdjacentRanks[i]   = -1;
    vertexOnReceiver.adjacentRanks[i]         = -1;
    vertexOnReceiver.backupOfAdjacentRanks[i] = -1;
  }
  vertexOnSender.adjacentRanks[0]         = sender._id;
  vertexOnSender.backupOfAdjacentRanks[0] = sender._id;
  vertexOnSender.adjacentRanks[1]         = receiver._id;
  vertexOnSender.backupOfAdjacentRanks[1] = receiver._id;

  vertexOnReceiver.adjacentRanks[0]         = receiver._id;
  vertexOnReceiver.backupOfAdjacentRanks[0] = receiver._id;
  vertexOnReceiver.adjacentRanks[1]         = sender._id;
  vertexOnReceiver.backupOfAdjacentRanks[1] = sender._id;

  const auto senderNeighbours = SpacetreeHelper::getNeighbourTreesAlongVertex(
    vertexOnSender,
    sender._id,
    sender._splitTriggered,
    sender._splitting,
    sender._joining,
    false,
    sender._spacetreeState,
    false
  );
  const auto receiverNeighbours = SpacetreeHelper::getNeighbourTreesAlongVertex(
    vertexOnReceiver,
    receiver._id,
    receiver._splitTriggered,
    receiver._splitting,
    receiver._joining,
    true,
    receiver._spacetreeState,
    false
  );

  validateEqualsWithParams1(senderNeighbours.size(), 1, vectorToString(senderNeighbours));
  validateEqualsWithParams1(receiverNeighbours.size(), 1, vectorToString(receiverNeighbours));
  validateEquals(senderNeighbours[0], receiver._id);
  validateEquals(receiverNeighbours[0], sender._id);

  GridVertex senderFaceVertices[TWO_POWER_D];
  GridVertex receiverFaceVertices[TWO_POWER_D];
  for (int v = 0; v < TWO_POWER_D; ++v) {
    senderFaceVertices[v].setState(GridVertex::State::Unrefined);
    receiverFaceVertices[v].setState(GridVertex::State::Unrefined);
    for (int j = 0; j < TWO_POWER_D; ++j) {
      senderFaceVertices[v].adjacentRanks[j]           = sender._id;
      senderFaceVertices[v].backupOfAdjacentRanks[j]   = sender._id;
      receiverFaceVertices[v].adjacentRanks[j]         = receiver._id;
      receiverFaceVertices[v].backupOfAdjacentRanks[j] = receiver._id;
    }
  }

  const auto& faceTable        = SpacetreeHelper::getFaceAdjTables();
  const int   face             = 0;
  const int   normal           = face % DIMENSIONS;
  const int   v0               = faceTable.faceVerts[face][0];
  const int   adjacencyIndexV0 = faceTable.adjIdx0[normal][v0];

  senderFaceVertices[v0].adjacentRanks[adjacencyIndexV0]           = receiver._id;
  senderFaceVertices[v0].backupOfAdjacentRanks[adjacencyIndexV0]   = receiver._id;
  receiverFaceVertices[v0].adjacentRanks[adjacencyIndexV0]         = sender._id;
  receiverFaceVertices[v0].backupOfAdjacentRanks[adjacencyIndexV0] = sender._id;

  const int senderFaceNeighbour = SpacetreeHelper::getNeighbourTreeAlongFace(
    senderFaceVertices,
    face,
    sender._id,
    sender._splitTriggered,
    sender._splitting,
    sender._joining,
    false,
    sender._spacetreeState,
    false
  );
  const int receiverFaceNeighbour = SpacetreeHelper::getNeighbourTreeAlongFace(
    receiverFaceVertices,
    face,
    receiver._id,
    receiver._splitTriggered,
    receiver._splitting,
    receiver._joining,
    true,
    receiver._spacetreeState,
    false
  );

  validateEquals(senderFaceNeighbour, receiver._id);
  validateEquals(receiverFaceNeighbour, sender._id);
}

// Verifies that the fast no-topology-change path matches the generic helper path when all transition sets are empty.
void peano4::grid::tests::SpacetreeTest::testNoTopologyChangeParity() {
  Spacetree tree       = createUnitTree(1);
  tree._spacetreeState = SpacetreeState::Running;
  tree._splitTriggered.clear();
  tree._splitting.clear();
  tree._joining.clear();

  GridVertex vertex;
  vertex.setState(GridVertex::State::Unrefined);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    vertex.adjacentRanks[i]         = -1;
    vertex.backupOfAdjacentRanks[i] = -1;
  }
  vertex.adjacentRanks[0]         = tree._id;
  vertex.backupOfAdjacentRanks[0] = tree._id;
  vertex.adjacentRanks[1]         = 2;
  vertex.backupOfAdjacentRanks[1] = 2;

  const auto sendNoChange = SpacetreeHelper::getNeighbourTreesAlongVertex(
    vertex,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    false,
    tree._spacetreeState,
    true
  );
  const auto sendGeneric = SpacetreeHelper::getNeighbourTreesAlongVertex(
    vertex,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    false,
    tree._spacetreeState,
    false
  );
  const auto recvNoChange = SpacetreeHelper::getNeighbourTreesAlongVertex(
    vertex,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState,
    true
  );
  const auto recvGeneric = SpacetreeHelper::getNeighbourTreesAlongVertex(
    vertex,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState,
    false
  );

  validateEqualsWithParams2(
    vectorToString(sendNoChange),
    vectorToString(sendGeneric),
    vectorToString(sendNoChange),
    vectorToString(sendGeneric)
  );
  validateEqualsWithParams2(
    vectorToString(recvNoChange),
    vectorToString(recvGeneric),
    vectorToString(recvNoChange),
    vectorToString(recvGeneric)
  );

  GridVertex faceVertices[TWO_POWER_D];
  for (int v = 0; v < TWO_POWER_D; ++v) {
    faceVertices[v].setState(GridVertex::State::Unrefined);
    for (int j = 0; j < TWO_POWER_D; ++j) {
      faceVertices[v].adjacentRanks[j]         = tree._id;
      faceVertices[v].backupOfAdjacentRanks[j] = tree._id;
    }
  }

  const auto& faceTable                                    = SpacetreeHelper::getFaceAdjTables();
  const int   face                                         = 0;
  const int   normal                                       = face % DIMENSIONS;
  const int   v0                                           = faceTable.faceVerts[face][0];
  const int   adjacencyIndexV0                             = faceTable.adjIdx0[normal][v0];
  faceVertices[v0].adjacentRanks[adjacencyIndexV0]         = 2;
  faceVertices[v0].backupOfAdjacentRanks[adjacencyIndexV0] = 2;

  const int sendFaceNoChange = SpacetreeHelper::getNeighbourTreeAlongFace(
    faceVertices,
    face,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    false,
    tree._spacetreeState,
    true
  );
  const int sendFaceGeneric = SpacetreeHelper::getNeighbourTreeAlongFace(
    faceVertices,
    face,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    false,
    tree._spacetreeState,
    false
  );
  const int recvFaceNoChange = SpacetreeHelper::getNeighbourTreeAlongFace(
    faceVertices,
    face,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState,
    true
  );
  const int recvFaceGeneric = SpacetreeHelper::getNeighbourTreeAlongFace(
    faceVertices,
    face,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState,
    false
  );

  validateEquals(sendFaceNoChange, sendFaceGeneric);
  validateEquals(recvFaceNoChange, recvFaceGeneric);

  const auto vertexBoundaryNoChange = SpacetreeHelper::areVerticesAdjacentToParallelDomainBoundary(
    faceVertices,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joinTriggered,
    tree._joining,
    true
  );
  const auto vertexBoundaryGeneric = SpacetreeHelper::areVerticesAdjacentToParallelDomainBoundary(
    faceVertices,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joinTriggered,
    tree._joining,
    false
  );
  const auto faceBoundaryNoChange = SpacetreeHelper::areFacesAdjacentToParallelDomainBoundary(
    faceVertices,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joinTriggered,
    tree._joining,
    false,
    true
  );
  const auto faceBoundaryGeneric = SpacetreeHelper::areFacesAdjacentToParallelDomainBoundary(
    faceVertices,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joinTriggered,
    tree._joining,
    false,
    false
  );

  validateEquals(vertexBoundaryNoChange.to_string(), vertexBoundaryGeneric.to_string());
  validateEquals(faceBoundaryNoChange.to_string(), faceBoundaryGeneric.to_string());
}

// Verifies that permuting adjacency entries without changing the represented topology does not change vertex neighbour
// results.
void peano4::grid::tests::SpacetreeTest::testVertexNeighbourPermutationInvariance() {
  Spacetree tree       = createUnitTree(1);
  tree._spacetreeState = SpacetreeState::Running;

  GridVertex a;
  GridVertex b;
  a.setState(GridVertex::State::Unrefined);
  b.setState(GridVertex::State::Unrefined);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    a.adjacentRanks[i]         = -1;
    a.backupOfAdjacentRanks[i] = -1;
    b.adjacentRanks[i]         = -1;
    b.backupOfAdjacentRanks[i] = -1;
  }
  a.adjacentRanks[0]         = tree._id;
  a.adjacentRanks[1]         = 3;
  a.adjacentRanks[2]         = 2;
  a.backupOfAdjacentRanks[0] = tree._id;
  a.backupOfAdjacentRanks[1] = 5;
  a.backupOfAdjacentRanks[2] = 4;

  b.adjacentRanks[0]                       = 2;
  b.adjacentRanks[TWO_POWER_D - 2]         = tree._id;
  b.adjacentRanks[TWO_POWER_D - 1]         = 3;
  b.backupOfAdjacentRanks[0]               = 4;
  b.backupOfAdjacentRanks[TWO_POWER_D - 2] = tree._id;
  b.backupOfAdjacentRanks[TWO_POWER_D - 1] = 5;

  const auto sendA = SpacetreeHelper::getNeighbourTreesAlongVertex(
    a,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    false,
    tree._spacetreeState,
    false
  );
  const auto sendB = SpacetreeHelper::getNeighbourTreesAlongVertex(
    b,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    false,
    tree._spacetreeState,
    false
  );
  const auto recvA = SpacetreeHelper::getNeighbourTreesAlongVertex(
    a,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState,
    false
  );
  const auto recvB = SpacetreeHelper::getNeighbourTreesAlongVertex(
    b,
    tree._id,
    tree._splitTriggered,
    tree._splitting,
    tree._joining,
    true,
    tree._spacetreeState,
    false
  );

  validateEquals(vectorToString(sendA), vectorToString(sendB));
  validateEquals(vectorToString(recvA), vectorToString(recvB));
}

// Verifies that descending a simple local tree still triggers the expected observer callbacks and statistics updates.
void peano4::grid::tests::SpacetreeTest::testDescend() {
  Spacetree tree = createUnitTree();

  CountingTraversalObserver primingObserver;
  tree.traverse(primingObserver);

  validate(tree._spacetreeState == SpacetreeState::Running);
  validateEquals(tree._vertexLookupTables.size(), 0);

  GridVertex vertices[TWO_POWER_D];
  createRootVertices(tree._id, GridVertex::State::Refined, vertices);

  tree._statistics.clear(tree._id == 0);
  tree._gridControlEvents         = {createFarAwayRefineEvent()};
  tree._noParallelTopologyChanges = true;

  CountingTraversalObserver observer;
  tree.descend(tree._root, tarch::la::Vector<DIMENSIONS, int>(0), vertices, observer);

  validateEquals(observer.loadNodeDataCalls, THREE_POWER_D);
  validateEquals(observer.enterNodeCalls, THREE_POWER_D);
  validateEquals(observer.leaveNodeCalls, THREE_POWER_D);
  validateEquals(observer.storeNodeDataCalls, THREE_POWER_D);
  validateEquals(static_cast<int>(observer.enteredLevels.size()), THREE_POWER_D);
  validateEquals(static_cast<int>(observer.enteredCellIsLocal.size()), THREE_POWER_D);
  for (int level : observer.enteredLevels) {
    validateEquals(level, 1);
  }
  for (bool isLocal : observer.enteredCellIsLocal) {
    validate(isLocal);
  }

  const int expectedLocalRefinedCells = tree._id == 0 ? 1 : 0;
  validateEquals(tree._statistics.numberOfLocalRefinedCells, expectedLocalRefinedCells);
  validateEquals(tree._statistics.numberOfRemoteRefinedCells, 0);
  validateEquals(tree._statistics.numberOfLocalUnrefinedCells, THREE_POWER_D);
  validateEquals(tree._statistics.numberOfRemoteUnrefinedCells, 0);
  validateEquals(tree._vertexLookupTables.size(), 0);
}

// A cell that only touches a refine event's boundary stays untouched; a cell overlapping the
// event's interior triggers refinement on its vertices.
void peano4::grid::tests::SpacetreeTest::testRefineEventTouchDoesNotTriggerNeighbourRefinement() {
  Spacetree tree       = createUnitTree();
  tree._spacetreeState = SpacetreeState::Running;

  GridControlEvent event;
  event.setRefinementControl(GridControlEvent::RefinementControl::Refine);
  event.offset            = tarch::la::Vector<DIMENSIONS, double>(0.0);
  event.width             = tarch::la::Vector<DIMENSIONS, double>(1.0 / 3.0);
  event.targetH           = tarch::la::Vector<DIMENSIONS, double>(1.0 / 9.0);
  tree._gridControlEvents = {event};

  GridVertex coarseGridVertices[TWO_POWER_D];
  createRootVertices(tree._id, GridVertex::State::Refined, coarseGridVertices);

  GridVertex touchingFineGridVertices[TWO_POWER_D];
  createRootVertices(tree._id, GridVertex::State::Unrefined, touchingFineGridVertices);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    touchingFineGridVertices[i].clearFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);
  }

  AutomatonState touchingState;
  touchingState.level = 1;
  touchingState.x     = tarch::la::Vector<DIMENSIONS, double>(0.0);
  touchingState.h     = tarch::la::Vector<DIMENSIONS, double>(1.0 / 3.0);
  touchingState.x[0]  = 1.0 / 3.0;

  tree.evaluateGridControlEvents(touchingState, coarseGridVertices, touchingFineGridVertices);

  for (int i = 0; i < TWO_POWER_D; ++i) {
    validateEquals(
      static_cast<int>(touchingFineGridVertices[i].getState()),
      static_cast<int>(GridVertex::State::Unrefined)
    );
    validateFalse(touchingFineGridVertices[i].getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep));
  }

  GridVertex overlappingFineGridVertices[TWO_POWER_D];
  createRootVertices(tree._id, GridVertex::State::Unrefined, overlappingFineGridVertices);
  for (int i = 0; i < TWO_POWER_D; ++i) {
    overlappingFineGridVertices[i].clearFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);
  }

  AutomatonState overlappingState;
  overlappingState.level = 1;
  overlappingState.x     = tarch::la::Vector<DIMENSIONS, double>(0.0);
  overlappingState.h     = tarch::la::Vector<DIMENSIONS, double>(1.0 / 3.0);

  tree.evaluateGridControlEvents(overlappingState, coarseGridVertices, overlappingFineGridVertices);

  for (int i = 0; i < TWO_POWER_D; ++i) {
    validateEquals(
      static_cast<int>(overlappingFineGridVertices[i].getState()),
      static_cast<int>(GridVertex::State::RefinementTriggered)
    );
    validateTrue(overlappingFineGridVertices[i].getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep));
  }
}

// A transitional coarse vertex forces the traversal to defer requested vertex changes. The
// deferral count and the reset stationarity are what make the event source deliver the
// requests again, so a deferred change is retried instead of silently lost.
void peano4::grid::tests::SpacetreeTest::testDeferredGridControlChangesAreCounted() {
  Spacetree tree       = createUnitTree();
  tree._spacetreeState = SpacetreeState::Running;

  GridControlEvent refineEvent;
  refineEvent.setRefinementControl(GridControlEvent::RefinementControl::Refine);
  refineEvent.offset  = tarch::la::Vector<DIMENSIONS, double>(0.0);
  refineEvent.width   = tarch::la::Vector<DIMENSIONS, double>(1.0 / 3.0);
  refineEvent.targetH = tarch::la::Vector<DIMENSIONS, double>(1.0 / 9.0);

  AutomatonState state;
  state.level = 1;
  state.x     = tarch::la::Vector<DIMENSIONS, double>(0.0);
  state.h     = tarch::la::Vector<DIMENSIONS, double>(1.0 / 3.0);

  GridVertex coarseGridVertices[TWO_POWER_D];
  createRootVertices(tree._id, GridVertex::State::Refined, coarseGridVertices);
  coarseGridVertices[0].setState(GridVertex::State::Erasing);

  {
    tree._gridControlEvents = {refineEvent};
    GridVertex fineGridVertices[TWO_POWER_D];
    createRootVertices(tree._id, GridVertex::State::Unrefined, fineGridVertices);

    tree._statistics.numberOfDeferredGridControlChanges = 0;
    tree._statistics.numberOfStationarySweeps           = 5;
    tree.evaluateGridControlEvents(state, coarseGridVertices, fineGridVertices);

    for (int i = 0; i < TWO_POWER_D; ++i) {
      validateEquals(static_cast<int>(fineGridVertices[i].getState()), static_cast<int>(GridVertex::State::Unrefined));
    }
    validateEquals(tree._statistics.numberOfDeferredGridControlChanges, TWO_POWER_D);
    validateEquals(tree._statistics.numberOfStationarySweeps, 0);
  }

  {
    // Erase events act on the direct parent of the leaves they were computed on, so the
    // evaluated cell is the parent-sized root cell the event contains.
    GridControlEvent eraseEvent;
    eraseEvent.setRefinementControl(GridControlEvent::RefinementControl::Erase);
    eraseEvent.offset  = tarch::la::Vector<DIMENSIONS, double>(0.0);
    eraseEvent.width   = tarch::la::Vector<DIMENSIONS, double>(1.0);
    eraseEvent.targetH = tarch::la::Vector<DIMENSIONS, double>(1.0);

    AutomatonState parentState;
    parentState.level = 0;
    parentState.x     = tarch::la::Vector<DIMENSIONS, double>(0.0);
    parentState.h     = tarch::la::Vector<DIMENSIONS, double>(1.0);

    tree._gridControlEvents = {eraseEvent};
    GridVertex fineGridVertices[TWO_POWER_D];
    createRootVertices(tree._id, GridVertex::State::Refined, fineGridVertices);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      fineGridVertices[i].clearFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep);
    }

    tree._statistics.numberOfDeferredGridControlChanges = 0;
    tree._statistics.numberOfStationarySweeps           = 5;
    tree.evaluateGridControlEvents(parentState, coarseGridVertices, fineGridVertices);

    for (int i = 0; i < TWO_POWER_D; ++i) {
      validateEquals(static_cast<int>(fineGridVertices[i].getState()), static_cast<int>(GridVertex::State::Refined));
    }
    validateEquals(tree._statistics.numberOfDeferredGridControlChanges, TWO_POWER_D);
    validateEquals(tree._statistics.numberOfStationarySweeps, 0);
  }

  // With every coarse vertex stable the same requests go through and nothing is deferred.
  coarseGridVertices[0].setState(GridVertex::State::Refined);
  {
    tree._gridControlEvents = {refineEvent};
    GridVertex fineGridVertices[TWO_POWER_D];
    createRootVertices(tree._id, GridVertex::State::Unrefined, fineGridVertices);

    tree._statistics.numberOfDeferredGridControlChanges = 0;
    tree.evaluateGridControlEvents(state, coarseGridVertices, fineGridVertices);

    for (int i = 0; i < TWO_POWER_D; ++i) {
      validateEquals(
        static_cast<int>(fineGridVertices[i].getState()),
        static_cast<int>(GridVertex::State::RefinementTriggered)
      );
    }
    validateEquals(tree._statistics.numberOfDeferredGridControlChanges, 0);
  }
}

// The boundary merge has to give both trees sharing a vertex the same state and helper flags,
// whichever side contributed the triggered transition or the flag.
void peano4::grid::tests::SpacetreeTest::testMergeGridVertexRefinementStateAtHorizontalDomainBoundaryIsSymmetric() {
  Spacetree tree = createUnitTree(7);

  std::array<int, TWO_POWER_D> sharedAdjacency;
  for (int i = 0; i < TWO_POWER_D; i++) {
    sharedAdjacency[i] = (i % 2 == 0) ? 7 : 8;
  }

  {
    GridVertex lhs = makeVertex(GridVertex::State::RefinementTriggered, sharedAdjacency);
    GridVertex rhs = makeVertex(GridVertex::State::Unrefined, sharedAdjacency);
    tree.mergeGridVertexRefinementStateAtHorizontalDomainBoundary(lhs, rhs, 8);
    validateEquals(static_cast<int>(lhs.getState()), static_cast<int>(GridVertex::State::RefinementTriggered));

    lhs = makeVertex(GridVertex::State::Unrefined, sharedAdjacency);
    rhs = makeVertex(GridVertex::State::RefinementTriggered, sharedAdjacency);
    tree.mergeGridVertexRefinementStateAtHorizontalDomainBoundary(lhs, rhs, 8);
    validateEquals(static_cast<int>(lhs.getState()), static_cast<int>(GridVertex::State::RefinementTriggered));
  }

  {
    GridVertex lhs = makeVertex(GridVertex::State::EraseTriggered, sharedAdjacency);
    GridVertex rhs = makeVertex(GridVertex::State::Refined, sharedAdjacency);
    tree.mergeGridVertexRefinementStateAtHorizontalDomainBoundary(lhs, rhs, 8);
    validateEquals(static_cast<int>(lhs.getState()), static_cast<int>(GridVertex::State::EraseTriggered));

    lhs = makeVertex(GridVertex::State::Refined, sharedAdjacency);
    rhs = makeVertex(GridVertex::State::EraseTriggered, sharedAdjacency);
    tree.mergeGridVertexRefinementStateAtHorizontalDomainBoundary(lhs, rhs, 8);
    validateEquals(static_cast<int>(lhs.getState()), static_cast<int>(GridVertex::State::EraseTriggered));
  }

  {
    GridVertex lhs = makeVertex(
      GridVertex::State::Unrefined,
      sharedAdjacency,
      GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep
    );
    GridVertex rhs =
      makeVertex(GridVertex::State::Unrefined, sharedAdjacency, GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep);
    tree.mergeGridVertexRefinementStateAtHorizontalDomainBoundary(lhs, rhs, 8);
    validateTrue(lhs.getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep));
    validateTrue(lhs.getFlag(GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep));
  }
}

// The horizontal boundary merge sums the neighbour copies' local refined-cell counts into
// the remote counter, so the surrounded-vertex refinement sees every adjacent cell on tree
// boundaries too. Splits and joins in flight pause the accumulation.
void peano4::grid::tests::SpacetreeTest::testMergeAccumulatesRemoteRefinedCellCounts() {
  Spacetree tree = createUnitTree(7);

  std::array<int, TWO_POWER_D> sharedAdjacency;
  for (int i = 0; i < TWO_POWER_D; i++) {
    sharedAdjacency[i] = (i % 2 == 0) ? 7 : 8;
  }

  GridVertex vertex                        = makeVertex(GridVertex::State::Unrefined, sharedAdjacency);
  vertex.numberOfAdjacentRefinedLocalCells = 2;

  GridVertex inVertex                        = makeVertex(GridVertex::State::Unrefined, sharedAdjacency);
  inVertex.numberOfAdjacentRefinedLocalCells = 2;

  tree.mergeGridVertexRefinementStateAtHorizontalDomainBoundary(vertex, inVertex, 8);
  validateEquals(vertex.numberOfAdjacentRefinedRemoteCells, 2);

  inVertex.numberOfAdjacentRefinedLocalCells = 1;
  tree.mergeGridVertexRefinementStateAtHorizontalDomainBoundary(vertex, inVertex, 9);
  validateEquals(vertex.numberOfAdjacentRefinedRemoteCells, 3);
  validateEquals(vertex.numberOfAdjacentRefinedLocalCells, 2);

  tree._splitTriggered[12];
  tree.mergeGridVertexRefinementStateAtHorizontalDomainBoundary(vertex, inVertex, 9);
  validateEquals(vertex.numberOfAdjacentRefinedRemoteCells, 3);
}

// A vertex whose adjacent cells are all refined refines even when some of them live on
// neighbouring trees. A purely local count triggers at once; a count that needs the merged
// remote share is one sweep old, so it triggers only once the surrounded state has been
// seen in two consecutive sweeps. Loading a vertex clears the remote count before the
// merges of the new sweep run.
void peano4::grid::tests::SpacetreeTest::testPostRefinesVertexSurroundedAcrossTreeBoundaries() {
  Spacetree tree       = createUnitTree(7);
  tree._spacetreeState = SpacetreeState::Running;

  std::array<int, TWO_POWER_D> localAdjacency;
  localAdjacency.fill(tree._id);

  GridVertex coarseGridVertices[TWO_POWER_D];
  for (int i = 0; i < TWO_POWER_D; ++i) {
    coarseGridVertices[i] = makeVertex(GridVertex::State::Unrefined, localAdjacency);
    coarseGridVertices[i].clearFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);
  }
  const tarch::la::Vector<DIMENSIONS, int> fineVertexPositionWithinPatch(0);
  const tarch::la::Vector<DIMENSIONS, int> vertexIndex(0);

  GridVertex locallySurroundedVertex                        = makeVertex(GridVertex::State::Unrefined, localAdjacency);
  locallySurroundedVertex.numberOfAdjacentRefinedLocalCells = TWO_POWER_D;
  tree.updateVertexBeforeStore(locallySurroundedVertex, coarseGridVertices, fineVertexPositionWithinPatch, vertexIndex);
  validateEquals(
    static_cast<int>(locallySurroundedVertex.getState()),
    static_cast<int>(GridVertex::State::RefinementTriggered)
  );

  GridVertex surroundedVertex                         = makeVertex(GridVertex::State::Unrefined, localAdjacency);
  surroundedVertex.numberOfAdjacentRefinedLocalCells  = TWO_POWER_D - 1;
  surroundedVertex.numberOfAdjacentRefinedRemoteCells = 1;
  surroundedVertex.clearFlag(GridVertex::HasBeenSurroundedByRefinedCellsInPreviousTreeSweep);
  // A pending two-sweep confirmation keeps the tree non-stationary, so the
  // mesh-refinement drain cannot end between the arming and the confirming sweep.
  tree._statistics.numberOfStationarySweeps = 5;
  tree.updateVertexBeforeStore(surroundedVertex, coarseGridVertices, fineVertexPositionWithinPatch, vertexIndex);
  validateEquals(static_cast<int>(surroundedVertex.getState()), static_cast<int>(GridVertex::State::Unrefined));
  validateTrue(surroundedVertex.getFlag(GridVertex::HasBeenSurroundedByRefinedCellsInPreviousTreeSweep));
  validateEquals(tree._statistics.numberOfStationarySweeps, 0);

  tree._statistics.numberOfStationarySweeps = 5;
  tree.updateVertexBeforeStore(surroundedVertex, coarseGridVertices, fineVertexPositionWithinPatch, vertexIndex);
  validateEquals(
    static_cast<int>(surroundedVertex.getState()),
    static_cast<int>(GridVertex::State::RefinementTriggered)
  );
  validateEquals(tree._statistics.numberOfStationarySweeps, 0);

  GridVertex partiallySurroundedVertex = makeVertex(GridVertex::State::Unrefined, localAdjacency);
  partiallySurroundedVertex.numberOfAdjacentRefinedLocalCells = TWO_POWER_D - 1;
  tree._statistics.numberOfStationarySweeps                   = 5;
  tree
    .updateVertexBeforeStore(partiallySurroundedVertex, coarseGridVertices, fineVertexPositionWithinPatch, vertexIndex);
  validateEquals(
    static_cast<int>(partiallySurroundedVertex.getState()),
    static_cast<int>(GridVertex::State::Unrefined)
  );
  validateFalse(partiallySurroundedVertex.getFlag(GridVertex::HasBeenSurroundedByRefinedCellsInPreviousTreeSweep));
  validateEquals(tree._statistics.numberOfStationarySweeps, 5);

  GridVertex loadedVertex                         = makeVertex(GridVertex::State::Unrefined, localAdjacency);
  loadedVertex.numberOfAdjacentRefinedRemoteCells = 99;
  tree.updateVertexAfterLoad(loadedVertex, vertexIndex);
  validateEquals(loadedVertex.numberOfAdjacentRefinedRemoteCells, 0);
}

// A vertex on the domain wall has fewer than 2^d adjacent cells, because the slots outside
// the domain hold none. It refines once the cells that do exist are refined, so the wall
// faces below it stop hanging and start receiving boundary data.
void peano4::grid::tests::SpacetreeTest::testPostRefinesVertexSurroundedAtDomainBoundary() {
  Spacetree tree       = createUnitTree(7);
  tree._spacetreeState = SpacetreeState::Running;

  std::array<int, TWO_POWER_D> localAdjacency;
  localAdjacency.fill(tree._id);

  GridVertex coarseGridVertices[TWO_POWER_D];
  for (int i = 0; i < TWO_POWER_D; ++i) {
    coarseGridVertices[i] = makeVertex(GridVertex::State::Unrefined, localAdjacency);
    coarseGridVertices[i].clearFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);
  }
  const tarch::la::Vector<DIMENSIONS, int> fineVertexPositionWithinPatch(0);
  const tarch::la::Vector<DIMENSIONS, int> vertexIndex(0);

  // Half of the adjacency slots lie outside the domain, which is what a vertex on a wall
  // looks like.
  std::array<int, TWO_POWER_D> wallAdjacency = localAdjacency;
  constexpr int                ExistingCells = TWO_POWER_D / 2;
  for (int i = ExistingCells; i < TWO_POWER_D; ++i) {
    wallAdjacency[i] = -1;
  }

  GridVertex wallVertex                        = makeVertex(GridVertex::State::Unrefined, wallAdjacency);
  wallVertex.numberOfAdjacentRefinedLocalCells = ExistingCells;
  tree.updateVertexBeforeStore(wallVertex, coarseGridVertices, fineVertexPositionWithinPatch, vertexIndex);
  validateEquals(static_cast<int>(wallVertex.getState()), static_cast<int>(GridVertex::State::RefinementTriggered));

  // One unrefined neighbour still leaves the vertex unrefined, so the relaxed threshold does
  // not refine a wall vertex early.
  GridVertex partialWallVertex                        = makeVertex(GridVertex::State::Unrefined, wallAdjacency);
  partialWallVertex.numberOfAdjacentRefinedLocalCells = ExistingCells - 1;
  tree._statistics.numberOfStationarySweeps           = 5;
  tree.updateVertexBeforeStore(partialWallVertex, coarseGridVertices, fineVertexPositionWithinPatch, vertexIndex);
  validateEquals(static_cast<int>(partialWallVertex.getState()), static_cast<int>(GridVertex::State::Unrefined));
  validateEquals(tree._statistics.numberOfStationarySweeps, 5);

  // A vertex with no adjacent cell at all must not satisfy the threshold by counting zero.
  std::array<int, TWO_POWER_D> detachedAdjacency;
  detachedAdjacency.fill(-1);
  GridVertex detachedVertex                 = makeVertex(GridVertex::State::Unrefined, detachedAdjacency);
  tree._statistics.numberOfStationarySweeps = 5;
  tree.updateVertexBeforeStore(detachedVertex, coarseGridVertices, fineVertexPositionWithinPatch, vertexIndex);
  validateEquals(static_cast<int>(detachedVertex.getState()), static_cast<int>(GridVertex::State::Unrefined));
  validateEquals(tree._statistics.numberOfStationarySweeps, 5);
}

// A vertex finishing its erase this sweep restricts the antecessor flag to the coarse grid,
// exactly like a refined vertex, so the coarsening veto survives the multi-sweep transition.
void peano4::grid::tests::SpacetreeTest::testUpdateVertexBeforeStoreTreatsErasingVerticesAsAntecessors() {
  Spacetree tree       = createUnitTree(7);
  tree._spacetreeState = SpacetreeState::Running;

  std::array<int, TWO_POWER_D> localAdjacency;
  localAdjacency.fill(tree._id);

  GridVertex coarseGridVertices[TWO_POWER_D];
  for (int i = 0; i < TWO_POWER_D; ++i) {
    coarseGridVertices[i] = makeVertex(GridVertex::State::Unrefined, localAdjacency);
    coarseGridVertices[i].clearFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);
  }

  const tarch::la::Vector<DIMENSIONS, int> fineVertexPositionWithinPatch(0);
  const tarch::la::Vector<DIMENSIONS, int> vertexIndex(0);

  GridVertex erasingVertex = makeVertex(GridVertex::State::Erasing, localAdjacency);
  tree.updateVertexBeforeStore(erasingVertex, coarseGridVertices, fineVertexPositionWithinPatch, vertexIndex);

  validateEquals(static_cast<int>(erasingVertex.getState()), static_cast<int>(GridVertex::State::Unrefined));
  validate(coarseGridVertices[0].getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep));

  for (int i = 1; i < TWO_POWER_D; ++i) {
    validate(not coarseGridVertices[i].getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep));
  }

  GridVertex untouchedCoarseGridVertices[TWO_POWER_D];
  for (int i = 0; i < TWO_POWER_D; ++i) {
    untouchedCoarseGridVertices[i] = makeVertex(GridVertex::State::Unrefined, localAdjacency);
    untouchedCoarseGridVertices[i].clearFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);
  }

  GridVertex unrefinedVertex = makeVertex(GridVertex::State::Unrefined, localAdjacency);
  tree
    .updateVertexBeforeStore(unrefinedVertex, untouchedCoarseGridVertices, fineVertexPositionWithinPatch, vertexIndex);

  for (int i = 0; i < TWO_POWER_D; ++i) {
    validate(not untouchedCoarseGridVertices[i].getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep));
  }
}

// Pins which states count as hanging-like when the previous sweep's vertex types are
// reconstructed: refinement-side and unrefined states do, the erase states do not.
void peano4::grid::tests::SpacetreeTest::testUpdateVertexAfterLoadPreviousHangingClassification() {
  Spacetree tree       = createUnitTree(7);
  tree._spacetreeState = SpacetreeState::Running;

  std::array<int, TWO_POWER_D> localAdjacency;
  localAdjacency.fill(tree._id);
  const tarch::la::Vector<DIMENSIONS, int> vertexIndex(0);

  GridVertex refinementTriggeredVertex = makeVertex(GridVertex::State::RefinementTriggered, localAdjacency);
  refinementTriggeredVertex.clearFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep);
  tree.updateVertexAfterLoad(refinementTriggeredVertex, vertexIndex);
  validate(refinementTriggeredVertex.getFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep));
  validateEquals(static_cast<int>(refinementTriggeredVertex.getState()), static_cast<int>(GridVertex::State::Refining));

  GridVertex unrefinedVertex = makeVertex(GridVertex::State::Unrefined, localAdjacency);
  unrefinedVertex.clearFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep);
  tree.updateVertexAfterLoad(unrefinedVertex, vertexIndex);
  validate(unrefinedVertex.getFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep));
  validateEquals(static_cast<int>(unrefinedVertex.getState()), static_cast<int>(GridVertex::State::Unrefined));

  GridVertex eraseTriggeredVertex = makeVertex(GridVertex::State::EraseTriggered, localAdjacency);
  eraseTriggeredVertex.clearFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep);
  eraseTriggeredVertex.clearFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep);
  tree.updateVertexAfterLoad(eraseTriggeredVertex, vertexIndex);
  validate(not eraseTriggeredVertex.getFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep));
  validateEquals(static_cast<int>(eraseTriggeredVertex.getState()), static_cast<int>(GridVertex::State::Erasing));

  GridVertex erasingVertex = makeVertex(GridVertex::State::Erasing, localAdjacency);
  erasingVertex.setFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep);
  tree.updateVertexAfterLoad(erasingVertex, vertexIndex);
  validate(not erasingVertex.getFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep));
  validateEquals(static_cast<int>(erasingVertex.getState()), static_cast<int>(GridVertex::State::Erasing));

  GridVertex refinedVertex = makeVertex(GridVertex::State::Refined, localAdjacency);
  refinedVertex.setFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep);
  tree.updateVertexAfterLoad(refinedVertex, vertexIndex);
  validate(not refinedVertex.getFlag(GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep));
  validateEquals(static_cast<int>(refinedVertex.getState()), static_cast<int>(GridVertex::State::Refined));
}

// A grid-control erase names cells; cells it does not name must survive. We build a block of
// refined parents, erase every parent but the centre one, and let the grid settle. Vertex-based
// refinement makes a cell refined once one of its vertices is refined, so refining the 5^d event
// block yields a 7^d refined block (every vertex neighbour joins), and the centre together with
// the 3^d neighbourhood that shares its corners must survive the erase. Everything further out
// has to go, so the erase itself is exercised too.
void peano4::grid::tests::SpacetreeTest::testEraseRingKeepsUnnamedRefinedParent() {
  Spacetree tree = createUnitTree();

  struct EventDrivenTraversalObserver final : public GridTraversalObserver {
    std::vector<GridControlEvent> events;

    void                          loadNodeData(const GridTraversalEvent&) override {}
    void                          enterNode(const GridTraversalEvent&) override {}
    void                          leaveNode(const GridTraversalEvent&) override {}
    void                          storeNodeData(const GridTraversalEvent&) override {}
    GridTraversalObserver*        clone(int) override { return new EventDrivenTraversalObserver(*this); }
    std::vector<GridControlEvent> getGridControlEvents() const override { return events; }
    void                          beginTraversal(
                               const tarch::la::Vector<DIMENSIONS, double>&,
                               const tarch::la::Vector<DIMENSIONS, double>&
                             ) override {}
    void
    endTraversal(const tarch::la::Vector<DIMENSIONS, double>&, const tarch::la::Vector<DIMENSIONS, double>&) override {}
  };

  EventDrivenTraversalObserver observer;
  tree.traverse(observer);

  constexpr double BaseH      = 1.0 / 9.0;
  constexpr int    Cells      = 9;
  constexpr int    EventBlock = 5;
  constexpr int    Block      = EventBlock + 2;
  constexpr int    First      = 1;

  int baseCells   = 1;
  int blockCells  = 1;
  int childCells  = 1;
  int keptParents = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    baseCells *= Cells;
    blockCells *= Block;
    childCells *= 3;
    keptParents *= 3;
  }

  GridControlEvent refineToBaseGrid;
  refineToBaseGrid.setRefinementControl(GridControlEvent::RefinementControl::Refine);
  refineToBaseGrid.offset  = tarch::la::Vector<DIMENSIONS, double>(0.0);
  refineToBaseGrid.width   = tarch::la::Vector<DIMENSIONS, double>(1.0);
  refineToBaseGrid.targetH = tarch::la::Vector<DIMENSIONS, double>(BaseH);

  GridControlEvent refineBlock;
  refineBlock.setRefinementControl(GridControlEvent::RefinementControl::Refine);
  refineBlock.offset  = tarch::la::Vector<DIMENSIONS, double>((First + 1) * BaseH);
  refineBlock.width   = tarch::la::Vector<DIMENSIONS, double>(EventBlock * BaseH);
  refineBlock.targetH = tarch::la::Vector<DIMENSIONS, double>(BaseH / 3.0);

  observer.events = {refineToBaseGrid, refineBlock};
  for (int sweep = 0; sweep < 12; sweep++) {
    tree.traverse(observer);
  }

  validateEquals(tree._statistics.numberOfLocalUnrefinedCells, baseCells - blockCells + blockCells * childCells);
  validateEquals(tree._statistics.numberOfLocalRefinedCells, 1 + childCells + blockCells);

  observer.events.clear();
  const int centre = Block / 2;
  for (int parent = 0; parent < blockCells; parent++) {
    bool                                  isCentre  = true;
    int                                   remainder = parent;
    tarch::la::Vector<DIMENSIONS, double> corner;
    for (int d = 0; d < DIMENSIONS; d++) {
      const int index = remainder % Block;
      remainder /= Block;
      isCentre  = isCentre and (index == centre);
      corner(d) = static_cast<double>(First + index) * BaseH;
    }
    if (isCentre) {
      continue;
    }
    GridControlEvent erase;
    erase.setRefinementControl(GridControlEvent::RefinementControl::Erase);
    erase.offset  = corner;
    erase.width   = tarch::la::Vector<DIMENSIONS, double>(BaseH);
    erase.targetH = tarch::la::Vector<DIMENSIONS, double>(BaseH);
    observer.events.push_back(erase);
  }

  for (int sweep = 0; sweep < 2; sweep++) {
    tree.traverse(observer);
  }
  observer.events.clear();
  for (int sweep = 0; sweep < 6; sweep++) {
    tree.traverse(observer);
  }

  validateEqualsWithParams1(
    tree._statistics.numberOfLocalRefinedCells,
    1 + childCells + keptParents,
    tree._statistics.numberOfLocalUnrefinedCells
  );
  validateEquals(tree._statistics.numberOfLocalUnrefinedCells, baseCells - keptParents + keptParents * childCells);
}

// The dual case: an erase set that spares the cells whose vertices carry the refinement must
// change nothing. Requesting refinement for a block marks the block's vertex lattice, and the
// surrounding hull cells pick up children through the shared vertices. Erasing the hull while
// the block stays unnamed may not take a single vertex - every candidate is a corner of an
// unnamed refined cell - and re-delivering the set must not make the grid oscillate.
void peano4::grid::tests::SpacetreeTest::testEraseOfRefinementSourceNeighbourhoodIsVetoed() {
  Spacetree tree = createUnitTree();

  struct EventDrivenTraversalObserver final : public GridTraversalObserver {
    std::vector<GridControlEvent> events;

    void                          loadNodeData(const GridTraversalEvent&) override {}
    void                          enterNode(const GridTraversalEvent&) override {}
    void                          leaveNode(const GridTraversalEvent&) override {}
    void                          storeNodeData(const GridTraversalEvent&) override {}
    GridTraversalObserver*        clone(int) override { return new EventDrivenTraversalObserver(*this); }
    std::vector<GridControlEvent> getGridControlEvents() const override { return events; }
    void                          beginTraversal(
                               const tarch::la::Vector<DIMENSIONS, double>&,
                               const tarch::la::Vector<DIMENSIONS, double>&
                             ) override {}
    void
    endTraversal(const tarch::la::Vector<DIMENSIONS, double>&, const tarch::la::Vector<DIMENSIONS, double>&) override {}
  };

  EventDrivenTraversalObserver observer;
  tree.traverse(observer);

  constexpr double BaseH      = 1.0 / 9.0;
  constexpr int    Cells      = 9;
  constexpr int    EventBlock = 3;
  constexpr int    Block      = EventBlock + 2;
  constexpr int    First      = 2;

  int baseCells  = 1;
  int blockCells = 1;
  int childCells = 1;
  for (int d = 0; d < DIMENSIONS; d++) {
    baseCells *= Cells;
    blockCells *= Block;
    childCells *= 3;
  }

  GridControlEvent refineToBaseGrid;
  refineToBaseGrid.setRefinementControl(GridControlEvent::RefinementControl::Refine);
  refineToBaseGrid.offset  = tarch::la::Vector<DIMENSIONS, double>(0.0);
  refineToBaseGrid.width   = tarch::la::Vector<DIMENSIONS, double>(1.0);
  refineToBaseGrid.targetH = tarch::la::Vector<DIMENSIONS, double>(BaseH);

  GridControlEvent refineBlock;
  refineBlock.setRefinementControl(GridControlEvent::RefinementControl::Refine);
  refineBlock.offset  = tarch::la::Vector<DIMENSIONS, double>((First + 1) * BaseH);
  refineBlock.width   = tarch::la::Vector<DIMENSIONS, double>(EventBlock * BaseH);
  refineBlock.targetH = tarch::la::Vector<DIMENSIONS, double>(BaseH / 3.0);

  observer.events = {refineToBaseGrid, refineBlock};
  for (int sweep = 0; sweep < 12; sweep++) {
    tree.traverse(observer);
  }

  const int refinedCells   = 1 + childCells + blockCells;
  const int unrefinedCells = baseCells - blockCells + blockCells * childCells;
  validateEquals(tree._statistics.numberOfLocalRefinedCells, refinedCells);
  validateEquals(tree._statistics.numberOfLocalUnrefinedCells, unrefinedCells);

  // Name every hull parent: the block cells the refine event covered stay unnamed.
  observer.events.clear();
  for (int parent = 0; parent < blockCells; parent++) {
    bool                                  isEventBlockCell = true;
    int                                   remainder        = parent;
    tarch::la::Vector<DIMENSIONS, double> corner;
    for (int d = 0; d < DIMENSIONS; d++) {
      const int index = remainder % Block;
      remainder /= Block;
      isEventBlockCell = isEventBlockCell and index > 0 and index < Block - 1;
      corner(d)        = static_cast<double>(First + index) * BaseH;
    }
    if (isEventBlockCell) {
      continue;
    }
    GridControlEvent erase;
    erase.setRefinementControl(GridControlEvent::RefinementControl::Erase);
    erase.offset  = corner;
    erase.width   = tarch::la::Vector<DIMENSIONS, double>(BaseH);
    erase.targetH = tarch::la::Vector<DIMENSIONS, double>(BaseH);
    observer.events.push_back(erase);
  }

  for (int sweep = 0; sweep < 8; sweep++) {
    tree.traverse(observer);
    validateEqualsWithParams1(tree._statistics.numberOfLocalRefinedCells, refinedCells, sweep);
    validateEqualsWithParams1(tree._statistics.numberOfLocalUnrefinedCells, unrefinedCells, sweep);
  }

  // Naming the block cells as well withdraws every veto, and the whole cluster coarsens.
  for (int parent = 0; parent < blockCells; parent++) {
    bool                                  isEventBlockCell = true;
    int                                   remainder        = parent;
    tarch::la::Vector<DIMENSIONS, double> corner;
    for (int d = 0; d < DIMENSIONS; d++) {
      const int index = remainder % Block;
      remainder /= Block;
      isEventBlockCell = isEventBlockCell and index > 0 and index < Block - 1;
      corner(d)        = static_cast<double>(First + index) * BaseH;
    }
    if (not isEventBlockCell) {
      continue;
    }
    GridControlEvent erase;
    erase.setRefinementControl(GridControlEvent::RefinementControl::Erase);
    erase.offset  = corner;
    erase.width   = tarch::la::Vector<DIMENSIONS, double>(BaseH);
    erase.targetH = tarch::la::Vector<DIMENSIONS, double>(BaseH);
    observer.events.push_back(erase);
  }

  for (int sweep = 0; sweep < 2; sweep++) {
    tree.traverse(observer);
  }
  observer.events.clear();
  for (int sweep = 0; sweep < 6; sweep++) {
    tree.traverse(observer);
  }

  validateEquals(tree._statistics.numberOfLocalRefinedCells, 1 + childCells);
  validateEquals(tree._statistics.numberOfLocalUnrefinedCells, baseCells);
}

// A mirror-symmetric refinement criterion must converge to a mirror-symmetric grid. The
// observer emulates a solver's criterion loop: every sweep it asks each leaf whose cell
// reaches into a centred disc to refine by one level, and the collected cell-exact events
// are delivered with the next traversal. After convergence the leaf boxes must map onto
// each other under every axis mirror and under the axis swap.
void peano4::grid::tests::SpacetreeTest::testRefineCascadeIsMirrorSymmetric() {
  Spacetree tree = createUnitTree();

  constexpr double BaseH   = 1.0 / 9.0;
  constexpr double FinestH = 1.0 / 81.0;
  constexpr double Radius  = 0.075;

  struct CriterionDrivenTraversalObserver final : public GridTraversalObserver {
    std::vector<GridControlEvent>                      events;
    std::vector<GridControlEvent>                      collectedEvents;
    std::vector<std::array<long long, DIMENSIONS + 1>> leaves;

    void loadNodeData(const GridTraversalEvent&) override {}
    void enterNode(const GridTraversalEvent& event) override {
      if (event.getHasBeenRefined() != 0 or event.getWillBeRefined() != 0) {
        return;
      }

      std::array<long long, DIMENSIONS + 1> leaf{};
      for (int d = 0; d < DIMENSIONS; d++) {
        leaf[d] = std::llround((event.getX(d) - 0.5 * event.getH(d)) / FinestH);
      }
      leaf[DIMENSIONS] = std::llround(event.getH(0) / FinestH);
      leaves.push_back(leaf);

      const tarch::la::Vector<DIMENSIONS, double> domainCentre(0.5);
      const bool                                  coarserThanBase = event.getH(0) > 1.5 * BaseH;
      const bool                                  reachesDisc =
        tarch::la::norm2(event.getX() - domainCentre) <= Radius + 0.5 * tarch::la::norm2(event.getH());
      const bool aboveFinest = event.getH(0) > 1.5 * FinestH;
      if (coarserThanBase or (reachesDisc and aboveFinest)) {
        GridControlEvent refine;
        refine.setRefinementControl(GridControlEvent::RefinementControl::Refine);
        refine.offset  = event.getX() - 0.5 * event.getH();
        refine.width   = event.getH();
        refine.targetH = event.getH() / 3.0;
        collectedEvents.push_back(refine);
      }
    }
    void                          leaveNode(const GridTraversalEvent&) override {}
    void                          storeNodeData(const GridTraversalEvent&) override {}
    GridTraversalObserver*        clone(int) override { return new CriterionDrivenTraversalObserver(*this); }
    std::vector<GridControlEvent> getGridControlEvents() const override { return events; }
    void                          beginTraversal(
                               const tarch::la::Vector<DIMENSIONS, double>&,
                               const tarch::la::Vector<DIMENSIONS, double>&
                             ) override {
      leaves.clear();
      collectedEvents.clear();
    }
    void
    endTraversal(const tarch::la::Vector<DIMENSIONS, double>&, const tarch::la::Vector<DIMENSIONS, double>&) override {}
  };

  CriterionDrivenTraversalObserver observer;
  for (int sweep = 0; sweep < 30; sweep++) {
    tree.traverse(observer);
    observer.events = observer.collectedEvents;
  }

  constexpr long long                                   FinestCells = 81;
  const std::set<std::array<long long, DIMENSIONS + 1>> leafSet(observer.leaves.begin(), observer.leaves.end());
  validateEquals(leafSet.size(), observer.leaves.size());

  for (int mirroredAxis = 0; mirroredAxis < DIMENSIONS; mirroredAxis++) {
    std::set<std::array<long long, DIMENSIONS + 1>> mirroredSet;
    for (auto leaf : leafSet) {
      leaf[mirroredAxis] = FinestCells - leaf[mirroredAxis] - leaf[DIMENSIONS];
      mirroredSet.insert(leaf);
    }
    validateWithParams1(leafSet == mirroredSet, mirroredAxis);
  }

  std::set<std::array<long long, DIMENSIONS + 1>> swappedSet;
  for (auto leaf : leafSet) {
    std::swap(leaf[0], leaf[1]);
    swappedSet.insert(leaf);
  }
  validate(leafSet == swappedSet);
}

// The erase rules of evaluateGridControlEvents, cell by cell: an erase event acts on the
// cells one level above its target resolution through a containment test, its trigger
// honours the antecessor veto, and a refined cell outside every event marks its vertices
// against the erase.
void peano4::grid::tests::SpacetreeTest::testEvaluateGridControlEventsEraseRules() {
  Spacetree tree       = createUnitTree();
  tree._spacetreeState = SpacetreeState::Running;

  GridControlEvent erase;
  erase.setRefinementControl(GridControlEvent::RefinementControl::Erase);
  erase.offset                        = tarch::la::Vector<DIMENSIONS, double>(0.0);
  erase.width                         = tarch::la::Vector<DIMENSIONS, double>(1.0 / 3.0);
  erase.targetH                       = tarch::la::Vector<DIMENSIONS, double>(1.0 / 3.0);
  tree._gridControlEvents             = {erase};
  tree._gridControlEventsContainErase = true;

  GridVertex coarseGridVertices[TWO_POWER_D];
  createRootVertices(tree._id, GridVertex::State::Refined, coarseGridVertices);

  AutomatonState containedState;
  containedState.level = 1;
  containedState.x     = tarch::la::Vector<DIMENSIONS, double>(0.0);
  containedState.h     = tarch::la::Vector<DIMENSIONS, double>(1.0 / 3.0);

  // The contained cell of the event's own resolution triggers the erase on its vertices.
  {
    GridVertex fineGridVertices[TWO_POWER_D];
    createRootVertices(tree._id, GridVertex::State::Refined, fineGridVertices);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      fineGridVertices[i].clearFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep);
    }
    tree.evaluateGridControlEvents(containedState, coarseGridVertices, fineGridVertices);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      validateEquals(
        static_cast<int>(fineGridVertices[i].getState()),
        static_cast<int>(GridVertex::State::EraseTriggered)
      );
    }
  }

  // The antecessor veto blocks the trigger: such a vertex still carries refined structure.
  {
    GridVertex fineGridVertices[TWO_POWER_D];
    createRootVertices(tree._id, GridVertex::State::Refined, fineGridVertices);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      fineGridVertices[i].setFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep, true);
    }
    tree.evaluateGridControlEvents(containedState, coarseGridVertices, fineGridVertices);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      validateEquals(static_cast<int>(fineGridVertices[i].getState()), static_cast<int>(GridVertex::State::Refined));
    }
  }

  // A cell finer than one level below the event's target resolution stays untouched: the
  // erase acts on the parents of the leaves it wants to remove.
  {
    GridVertex fineGridVertices[TWO_POWER_D];
    createRootVertices(tree._id, GridVertex::State::Refined, fineGridVertices);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      fineGridVertices[i].clearFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep);
    }
    AutomatonState leafState;
    leafState.level = 2;
    leafState.x     = tarch::la::Vector<DIMENSIONS, double>(0.0);
    leafState.h     = tarch::la::Vector<DIMENSIONS, double>(1.0 / 9.0);
    tree.evaluateGridControlEvents(leafState, coarseGridVertices, fineGridVertices);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      validateEquals(static_cast<int>(fineGridVertices[i].getState()), static_cast<int>(GridVertex::State::Refined));
    }
  }

  // A cell that only partially overlaps the event is not contained and stays untouched.
  {
    GridVertex fineGridVertices[TWO_POWER_D];
    createRootVertices(tree._id, GridVertex::State::Refined, fineGridVertices);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      fineGridVertices[i].clearFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep);
    }
    AutomatonState partiallyOverlappingState = containedState;
    partiallyOverlappingState.x[0]           = 1.0 / 6.0;
    tree.evaluateGridControlEvents(partiallyOverlappingState, coarseGridVertices, fineGridVertices);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      validateEquals(static_cast<int>(fineGridVertices[i].getState()), static_cast<int>(GridVertex::State::Refined));
    }
  }

  // A refined cell outside the event marks its vertices, so the erase cannot take them.
  {
    GridVertex fineGridVertices[TWO_POWER_D];
    createRootVertices(tree._id, GridVertex::State::Refined, fineGridVertices);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      fineGridVertices[i].clearFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);
    }
    AutomatonState outsideState = containedState;
    outsideState.x[0]           = 2.0 / 3.0;
    tree.evaluateGridControlEvents(outsideState, coarseGridVertices, fineGridVertices);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      validateEquals(static_cast<int>(fineGridVertices[i].getState()), static_cast<int>(GridVertex::State::Refined));
      validateTrue(fineGridVertices[i].getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep));
    }
  }

  // An unrefined cell outside the event marks nothing: it has no children to keep.
  {
    GridVertex fineGridVertices[TWO_POWER_D];
    createRootVertices(tree._id, GridVertex::State::Unrefined, fineGridVertices);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      fineGridVertices[i].clearFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep);
    }
    AutomatonState outsideState = containedState;
    outsideState.x[0]           = 2.0 / 3.0;
    tree.evaluateGridControlEvents(outsideState, coarseGridVertices, fineGridVertices);
    for (int i = 0; i < TWO_POWER_D; ++i) {
      validateFalse(fineGridVertices[i].getFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep));
    }
  }
}
