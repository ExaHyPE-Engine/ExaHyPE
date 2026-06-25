// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeTest.h"

#include "peano4/Globals.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/grid/SpacetreeHelper.h"
#include "peano4/Loop.h"
#include "tarch/la/Vector.h"

#include <bitset>
#include <sstream>

namespace {
  struct CountingTraversalObserver final: public peano4::grid::GridTraversalObserver {
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

    void endTraversal(
      const tarch::la::Vector<DIMENSIONS, double>&,
      const tarch::la::Vector<DIMENSIONS, double>&
    ) override {}
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

  peano4::grid::Spacetree createUnitTree(const std::bitset<DIMENSIONS>& periodicBC = std::bitset<DIMENSIONS>{}) {
#if DIMENSIONS == 2
    return peano4::grid::Spacetree({0.0, 0.0}, {1.0, 1.0}, periodicBC);
#else
    return peano4::grid::Spacetree({0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, periodicBC);
#endif
  }

  void createRootVertices(
    int                             treeId,
    peano4::grid::GridVertex::State state,
    peano4::grid::GridVertex        vertices[TWO_POWER_D]
  ) {
    dfor2(k) std::array<int, TWO_POWER_D> adjacentRanks;
    tarch::la::Vector<DIMENSIONS, double> x(0.0);
    for (int d = 0; d < DIMENSIONS; ++d) {
      x(d) = static_cast<double>(k(d));
    }
    adjacentRanks.fill(-1);
    adjacentRanks[TWO_POWER_D - 1 - kScalar] = treeId;
    vertices[kScalar]                        = peano4::grid::GridVertex::create(0, state, x, adjacentRanks, false);
    vertices[kScalar].setFlag(peano4::grid::GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep, true);
    enddforx
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

peano4::grid::tests::SpacetreeTest::SpacetreeTest():
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
  testMethod(testGetNeighbourTreesVertexSendReceive);
  testMethod(testGetNeighbourTreesFaceSendReceive);
  testMethod(testIsFaceAlongPeriodicBoundaryCondition);
  testMethod(testTraverseWithPeriodicBoundaryConditionsUsesStableVertexBoundaryFlags);
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
  Spacetree tree({0.0, 0.0}, {1.0, 1.0});
  tree._id = 1;

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
  Spacetree tree({0.0, 0.0, 0.0}, {1.0, 1.0, 1.0});
  tree._id = 1;

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
  // senderCriteria does not filter _splitting, only _splitTriggered and _joining (and Joining state)
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
  // receiverCriteria filters _splitting and _joining
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
  Spacetree tree({0.0, 0.0}, {1.0, 1.0});
  tree._id = 1;

  GridVertex fineGridVertices[TWO_POWER_D];
  for (int v = 0; v < TWO_POWER_D; ++v) {
    fineGridVertices[v].setState(GridVertex::State::Unrefined);
    for (int j = 0; j < TWO_POWER_D; ++j) {
      fineGridVertices[v].adjacentRanks[j]         = tree._id;
      fineGridVertices[v].backupOfAdjacentRanks[j] = tree._id;
    }
  }

  const auto& T = SpacetreeHelper::getFaceAdjTables();

  const int faceNumber = 0;
  const int normal     = faceNumber % DIMENSIONS;
  const int v0         = T.faceVerts[faceNumber][0];
  const int v1         = T.faceVerts[faceNumber][1];
  const int i0_v0      = T.adjIdx0[normal][v0];
  const int i0_v1      = T.adjIdx0[normal][v1];

  // Put a unique remote neighbour on the face.
  fineGridVertices[v0].adjacentRanks[i0_v0] = 2;
  fineGridVertices[v1].adjacentRanks[i0_v1] = tree._id;

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
  fineGridVertices[v0].backupOfAdjacentRanks[i0_v0] = 2;

  const int recvNeighbour_newIgnored = SpacetreeHelper::getNeighbourTreeAlongFace(
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
  validateEquals(recvNeighbour_newIgnored, GridTraversalObserver::NoData);

  // Receiving side suppresses neighbours in _splitting.
  fineGridVertices[v0].setState(GridVertex::State::Unrefined);
  fineGridVertices[v0].backupOfAdjacentRanks[i0_v0] = 3;
  tree._splitting.clear();
  tree._splitting.insert(3);

  const int recvNeighbour_splittingSuppressed = SpacetreeHelper::getNeighbourTreeAlongFace(
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
  validateEquals(recvNeighbour_splittingSuppressed, GridTraversalObserver::NoData);

  // If the face isn't adjacent to the local tree at all, there is no neighbour.
  for (int k = 0; k < TWO_POWER_D / 2; ++k) {
    const int vIdx = T.faceVerts[faceNumber][k];
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
  Spacetree tree({0.0, 0.0}, {1.0, 1.0});
  tree._id = 1;

  GridVertex fineGridVertices[TWO_POWER_D];
  for (int v = 0; v < TWO_POWER_D; ++v) {
    fineGridVertices[v].setState(GridVertex::State::Unrefined);
    for (int j = 0; j < TWO_POWER_D; ++j) {
      fineGridVertices[v].adjacentRanks[j]         = tree._id;
      fineGridVertices[v].backupOfAdjacentRanks[j] = tree._id;
    }
  }

  const auto& T          = SpacetreeHelper::getFaceAdjTables();
  const int   faceNumber = 1;
  const int   normal     = faceNumber % DIMENSIONS;
  const int   v0         = T.faceVerts[faceNumber][0];
  const int   i0_v0      = T.adjIdx0[normal][v0];

  // Inject periodic BC rank on a face that is still adjacent to the local tree.
  fineGridVertices[v0].adjacentRanks[i0_v0] = SpacetreeHelper::RankOfPeriodicBoundaryCondition;

  validate(tree.isFaceAlongPeriodicBoundaryCondition(fineGridVertices, faceNumber, /*calledByReceivingProcess*/ false));

  // Periodic alone (without adjacency to local) should not trigger the flag.
  for (int k = 0; k < TWO_POWER_D / 2; ++k) {
    const int vIdx = T.faceVerts[faceNumber][k];
    for (int j = 0; j < TWO_POWER_D; ++j) {
      fineGridVertices[vIdx].adjacentRanks[j] = 7;
    }
  }
  fineGridVertices[v0].adjacentRanks[i0_v0] = SpacetreeHelper::RankOfPeriodicBoundaryCondition;
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

  Spacetree tree = createUnitTree(periodicBC);

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
  Spacetree tree({0.0, 0.0}, {1.0, 1.0});
  tree._id             = 1;
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
  Spacetree sender         = createUnitTree();
  Spacetree receiver       = createUnitTree();
  sender._id               = 1;
  receiver._id             = 2;
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

  const auto& T      = SpacetreeHelper::getFaceAdjTables();
  const int   face   = 0;
  const int   normal = face % DIMENSIONS;
  const int   v0     = T.faceVerts[face][0];
  const int   i0     = T.adjIdx0[normal][v0];

  senderFaceVertices[v0].adjacentRanks[i0]           = receiver._id;
  senderFaceVertices[v0].backupOfAdjacentRanks[i0]   = receiver._id;
  receiverFaceVertices[v0].adjacentRanks[i0]         = sender._id;
  receiverFaceVertices[v0].backupOfAdjacentRanks[i0] = sender._id;

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
  Spacetree tree       = createUnitTree();
  tree._id             = 1;
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

  const auto& T                              = SpacetreeHelper::getFaceAdjTables();
  const int   face                           = 0;
  const int   normal                         = face % DIMENSIONS;
  const int   v0                             = T.faceVerts[face][0];
  const int   i0                             = T.adjIdx0[normal][v0];
  faceVertices[v0].adjacentRanks[i0]         = 2;
  faceVertices[v0].backupOfAdjacentRanks[i0] = 2;

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
  Spacetree tree       = createUnitTree();
  tree._id             = 1;
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
  tree.descend(tree._root, vertices, observer);

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
