// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeTest.h"

#include "peano4/Globals.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/grid/SpacetreeHelper.h"
#include "peano4/utils/Loop.h"
#include "tarch/la/Vector.h"

#include <bitset>
#include <sstream>

namespace {
  struct CountingTraversalObserver final: public peano4::grid::GridTraversalObserver {
    int               loadCellDataCalls    = 0;
    int               loadFaceDataCalls    = 0;
    int               loadVertexDataCalls  = 0;
    int               enterNodeCalls       = 0;
    int               leaveNodeCalls       = 0;
    int               storeVertexDataCalls = 0;
    int               storeFaceDataCalls   = 0;
    int               storeCellDataCalls   = 0;
    std::vector<int>  enteredLevels;
    std::vector<bool> enteredCellIsLocal;
    std::vector<int>  enteredVertexBoundaryMasks;
    std::vector<int>  enteredVertexLocalMasks;

    void loadCellData(const peano4::grid::GridTraversalEvent&) override { ++loadCellDataCalls; }
    void loadFaceData(const peano4::grid::GridTraversalEvent&) override { ++loadFaceDataCalls; }
    void loadVertexData(const peano4::grid::GridTraversalEvent&) override { ++loadVertexDataCalls; }

    void enterNode(const peano4::grid::GridTraversalEvent& event) override {
      ++enterNodeCalls;
      enteredLevels.push_back(event.level);
      enteredCellIsLocal.push_back(event.getIsCellLocal());
      enteredVertexBoundaryMasks.push_back(event.getIsVertexAdjacentToParallelDomainBoundary());
      enteredVertexLocalMasks.push_back(event.getIsVertexLocal());
    }

    void leaveNode(const peano4::grid::GridTraversalEvent&) override { ++leaveNodeCalls; }

    void storeVertexData(const peano4::grid::GridTraversalEvent&) override { ++storeVertexDataCalls; }
    void storeFaceData(const peano4::grid::GridTraversalEvent&) override { ++storeFaceDataCalls; }
    void storeCellData(const peano4::grid::GridTraversalEvent&) override { ++storeCellDataCalls; }

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
  testMethod(testDescend);
  testMethod(testDescendWhileInRunningState);
  testMethod(testGetNeighbourTreesVertexSendReceive);
  testMethod(testGetNeighbourTreesFaceSendReceive);
  testMethod(testIsFaceAlongPeriodicBoundaryCondition);
  testMethod(testTraverseWithPeriodicBoundaryConditionsUsesStableVertexBoundaryFlags);
}

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

void peano4::grid::tests::SpacetreeTest::testTraverseWithPeriodicBoundaryConditionsUsesStableVertexBoundaryFlags() {
  std::bitset<DIMENSIONS> periodicBC;
  periodicBC.set(0); // x-direction
#if DIMENSIONS == 3
  periodicBC.set(2); // z-direction
#endif

  Spacetree tree = createUnitTree(periodicBC);
  Spacetree::_vertexStack.clear(tree._id);

  CountingTraversalObserver observer;
  tree.traverse(observer, true);

  validate(tree._spacetreeState == SpacetreeState::Running);
  validate(observer.enterNodeCalls > 0);
  validateEquals(static_cast<int>(observer.enteredVertexBoundaryMasks.size()), observer.enterNodeCalls);
  validateEquals(static_cast<int>(observer.enteredVertexLocalMasks.size()), observer.enterNodeCalls);

  for (int i = 0; i < observer.enterNodeCalls; ++i) {
    const int boundaryMask = observer.enteredVertexBoundaryMasks[i];
    const int localMask    = observer.enteredVertexLocalMasks[i];
    validateEqualsWithParams2((boundaryMask & ~localMask), 0, boundaryMask, localMask);
  }

  Spacetree::_vertexStack.clear(tree._id);
}

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

void peano4::grid::tests::SpacetreeTest::testDescend() {
  Spacetree tree = createUnitTree();
  Spacetree::_vertexStack.clear(tree._id);

  CountingTraversalObserver primingObserver;
  tree.traverse(primingObserver, true);

  validate(tree._spacetreeState == SpacetreeState::Running);
  validateEquals(tree._vertexLookupTables.size(), 0);

  GridVertex vertices[TWO_POWER_D];
  createRootVertices(tree._id, GridVertex::State::Refined, vertices);

  tree._statistics.clear(tree._id == 0);
  tree._gridControlEvents         = {createFarAwayRefineEvent()};
  tree._noParallelTopologyChanges = true;

  CountingTraversalObserver observer;
  tree.descend(tree._root, vertices, observer);

  validateEquals(observer.loadVertexDataCalls, THREE_POWER_D);
  validateEquals(observer.loadFaceDataCalls, THREE_POWER_D);
  validateEquals(observer.loadCellDataCalls, THREE_POWER_D);
  validateEquals(observer.enterNodeCalls, THREE_POWER_D);
  validateEquals(observer.leaveNodeCalls, THREE_POWER_D);
  validateEquals(observer.storeVertexDataCalls, THREE_POWER_D);
  validateEquals(observer.storeFaceDataCalls, THREE_POWER_D);
  validateEquals(observer.storeCellDataCalls, THREE_POWER_D);
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

  Spacetree::_vertexStack.clear(tree._id);
}

void peano4::grid::tests::SpacetreeTest::testDescendWhileInRunningState() {
  Spacetree tree = createUnitTree();
  Spacetree::_vertexStack.clear(tree._id);

  CountingTraversalObserver primingObserver;
  tree.traverse(primingObserver, true);

  validate(tree._spacetreeState == SpacetreeState::Running);
  validateEquals(tree._vertexLookupTables.size(), 0);

  GridVertex vertices[TWO_POWER_D];
  createRootVertices(tree._id, GridVertex::State::Refined, vertices);

  tree._statistics.clear(tree._id == 0);
  tree._gridControlEvents.clear();
  tree._noParallelTopologyChanges = true;

  CountingTraversalObserver observer;
  tree.descendWhileInRunningState(tree._root, vertices, observer);

  validateEquals(observer.loadVertexDataCalls, THREE_POWER_D);
  validateEquals(observer.loadFaceDataCalls, THREE_POWER_D);
  validateEquals(observer.loadCellDataCalls, THREE_POWER_D);
  validateEquals(observer.enterNodeCalls, THREE_POWER_D);
  validateEquals(observer.leaveNodeCalls, THREE_POWER_D);
  validateEquals(observer.storeVertexDataCalls, THREE_POWER_D);
  validateEquals(observer.storeFaceDataCalls, THREE_POWER_D);
  validateEquals(observer.storeCellDataCalls, THREE_POWER_D);
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

  Spacetree::_vertexStack.clear(tree._id);
}
