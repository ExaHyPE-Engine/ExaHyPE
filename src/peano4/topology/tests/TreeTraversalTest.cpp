// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TreeTraversalTest.h"

#include "peano4/Constants.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/sfc/ActiveCurve.h"
#include "peano4/topology/Tree.h"
#include "peano4/topology/TreeTraversal.h"
#include "tarch/tasking/Runtime.h"

namespace {
  constexpr int Depth             = 2;
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;

  /** Lattice position of the vertex at the domain centre on the base depth. */
  constexpr int CentreVertex = peano4::Power<SubdivisionFactor, Depth> / 2;

  /** Lattice position of a vertex inside the first child of the root on the base depth. */
  constexpr int VertexInsideFirstChild = peano4::Power<SubdivisionFactor, Depth - 1> / 2;

  /**
   * Records, per vertex pool slot, how often a sweep first-touches and
   * last-touches it, and whether an enter event of a remote cell moved a held
   * vertex's data.
   */
  class TouchRecordingObserver final : public peano4::grid::GridTraversalObserver {
  public:
    std::map<int, int> firstTouchesOfSlot;
    std::map<int, int> lastTouchesOfSlot;
    int                heldVerticesMovedAtRemoteCells{0};
    int                heldVerticesWithoutDataAtRemoteCells{0};

    peano4::grid::GridTraversalObserver* clone(const int treeId) override {
      static_cast<void>(treeId);
      return new TouchRecordingObserver();
    }
    [[nodiscard]] std::vector<peano4::grid::GridControlEvent> getGridControlEvents() const override { return {}; }
    void                                                      beginTraversal(
                                                           const tarch::la::Vector<DIMENSIONS, double>& x,
                                                           const tarch::la::Vector<DIMENSIONS, double>& h
                                                         ) override {
      static_cast<void>(x);
      static_cast<void>(h);
    }
    void endTraversal(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h
    ) override {
      static_cast<void>(x);
      static_cast<void>(h);
    }
    void enterNode(const peano4::grid::GridTraversalEvent& event) override {
      for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
        const auto dataFrom  = event.getVertexDataFrom(vertexNumber);
        const auto poolIndex = event.getVertexPoolIndex(vertexNumber);
        if (dataFrom == NoData) {
          if (not event.getIsCellLocal() and poolIndex >= 0 and not event.getIsVertexHanging(vertexNumber)) {
            heldVerticesWithoutDataAtRemoteCells++;
          }
          continue;
        }
        if (not event.getIsCellLocal()) {
          heldVerticesMovedAtRemoteCells++;
        }
        if (peano4::grid::StackTopology::isInOutStack(dataFrom)) {
          firstTouchesOfSlot[poolIndex]++;
        }
      }
    }
    void leaveNode(const peano4::grid::GridTraversalEvent& event) override {
      for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
        const auto dataTo = event.getVertexDataTo(vertexNumber);
        if (dataTo != NoData and peano4::grid::StackTopology::isInOutStack(dataTo)) {
          lastTouchesOfSlot[event.getVertexPoolIndex(vertexNumber)]++;
        }
      }
    }
  };

  /**
   * Compares the pool slot of every entity an enter event carries with the
   * entity's tree index, which a whole-domain rank's fresh numbering has to
   * reproduce. The cell's lattice position follows from the event's centre.
   */
  class SlotOrderObserver final : public peano4::grid::GridTraversalObserver {
  public:
    explicit SlotOrderObserver(const peano4::topology::Tree& tree) :
      _tree(tree) {}

    int numberOfCells{0};
    int numberOfMismatches{0};

    peano4::grid::GridTraversalObserver* clone(const int treeId) override {
      static_cast<void>(treeId);
      return new SlotOrderObserver(_tree);
    }
    [[nodiscard]] std::vector<peano4::grid::GridControlEvent> getGridControlEvents() const override { return {}; }
    void                                                      beginTraversal(
                                                           const tarch::la::Vector<DIMENSIONS, double>& x,
                                                           const tarch::la::Vector<DIMENSIONS, double>& h
                                                         ) override {
      static_cast<void>(x);
      static_cast<void>(h);
    }
    void endTraversal(
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h
    ) override {
      static_cast<void>(x);
      static_cast<void>(h);
    }
    void enterNode(const peano4::grid::GridTraversalEvent& event) override {
      tarch::la::Vector<DIMENSIONS, int> cellLattice(0);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        cellLattice(axis) = static_cast<int>(std::floor(event.getX()(axis) / event.getH()(axis)));
      }
      numberOfCells++;
      if (event.getCellPoolIndex() != _tree.getCellAt(event.level, cellLattice)) {
        numberOfMismatches++;
      }
      for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
        auto cornerLattice = cellLattice;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          cornerLattice(axis) += (vertexNumber >> axis) & 1;
        }
        if (event.getVertexPoolIndex(vertexNumber) != _tree.getVertexAt(event.level, cornerLattice)) {
          numberOfMismatches++;
        }
      }
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        const int normal      = faceNumber % DIMENSIONS;
        auto      faceLattice = cellLattice;
        if (faceNumber >= DIMENSIONS) {
          faceLattice(normal) += 1;
        }
        if (event.getFacePoolIndex(faceNumber) != _tree.getFaceAt(event.level, normal, faceLattice)) {
          numberOfMismatches++;
        }
      }
    }
    void leaveNode(const peano4::grid::GridTraversalEvent& event) override { static_cast<void>(event); }

  private:
    const peano4::topology::Tree& _tree;
  };
} // namespace

peano4::topology::tests::TreeTraversalTest::TreeTraversalTest() :
  TestCase("peano4::topology::tests::TreeTraversalTest") {}

void peano4::topology::tests::TreeTraversalTest::run() {
  testMethod(testRootChildrenBecomeTasksAtSpawnLevelZero);
  testMethod(testFreshSlotsNumberTheEntitiesInIndexOrder);
  testMethod(testRegridTransitionClasses);
  testMethod(testTransitionTasksRootAtChangingCells);
  testMethod(testSlotInheritanceAcrossRegrid);
  testMethod(testShrunkInterfaceTopologyKeepsTheSurvivingSlots);
  testMethod(testRankPairsAgreeOnTheirBoundaryWithoutCommunicating);
  testMethod(testTransferListsPairAcrossRanksWhenThePartitionMoves);
  testMethod(testInPlaceRegridRederivesTheRankBoundary);
  testMethod(testRemoteBlockRootsMoveTheVerticesTheRankHolds);
  testMethod(testVerticesCountOnceAcrossRanks);
}

/**
 * A rank traverses a block root another rank owns, and a vertex the rank holds
 * moves its data there and is first- and last-touched exactly once per sweep.
 * A vertex the rank does not hold moves no data at that root.
 */
void peano4::topology::tests::TreeTraversalTest::testRemoteBlockRootsMoveTheVerticesTheRankHolds() {
  constexpr int SpawnLevel = 1;
  // One rank more than the subdivision factor never divides the spawn-block
  // count, so no rank boundary falls on a block boundary and every rank holds
  // part of a block another rank holds too.
  constexpr int NumberOfRanks = peano4::sfc::ActiveCurve::SubdivisionFactor + 1;

  const tarch::la::Vector<DIMENSIONS, double>  offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double>  width(1.0);
  const peano4::topology::Tree::RefinementPlan emptyPlan;

  const peano4::topology::Tree ownershipSource(0, offset, width);
  const auto ownershipPlan = ownershipSource.deriveBalancedBlockOwners(Depth, NumberOfRanks, emptyPlan);

  int heldVerticesMovedAtRemoteCells = 0;
  for (int rank = 0; rank < NumberOfRanks; rank++) {
    peano4::topology::Tree tree(rank, offset, width);
    tree.buildRegularGrid(Depth, SpawnLevel, NumberOfRanks, emptyPlan, &ownershipPlan);
    peano4::topology::TreeTraversal traversal(rank, offset, width);
    traversal.prepareRankTopology(tree);

    // The partition of two ranks over the level-two cells splits one
    // spawn-level block between them: The rank that does not own it
    // traverses its root without owning it.
    bool hasSplitBlock = false;
    for (std::size_t cellIndex = 0; cellIndex < tree.getNumberOfCells(); cellIndex++) {
      const auto& cell = tree.getCell(static_cast<int>(cellIndex));
      hasSplitBlock    = hasSplitBlock or (cell.level == SpawnLevel and tree.isSplitBlockAt(cell.latticeIndex));
    }
    validate(hasSplitBlock);

    TouchRecordingObserver observer;
    traversal.traverseWithPools(tree, observer, peano4::topology::TreeTraversal::PoolSweepKind::Standing, nullptr);

    heldVerticesMovedAtRemoteCells += observer.heldVerticesMovedAtRemoteCells;
    validateEquals(observer.heldVerticesWithoutDataAtRemoteCells, 0);
    validate(not observer.firstTouchesOfSlot.empty());
    for (const auto& [slot, firstTouches] : observer.firstTouchesOfSlot) {
      validateEquals(firstTouches, 1);
      validateEquals(observer.lastTouchesOfSlot[slot], 1);
    }
  }
  validate(heldVerticesMovedAtRemoteCells > 0);
}

/**
 * The sweep statistics count every persistent and every hanging vertex once
 * across the ranks, with and without a refined cell.
 */
void peano4::topology::tests::TreeTraversalTest::testVerticesCountOnceAcrossRanks() {
  constexpr int SpawnLevel = 1;

  const tarch::la::Vector<DIMENSIONS, double> offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> width(1.0);

  // The flag at the centre vertex refines the two cells per axis around it,
  // whose children carry (2k+1)^d fine vertices for the subdivision factor k;
  // the (2k-1)^d interior ones are persistent and the ones on the rim of the
  // region hang.
  const peano4::topology::Tree::RefinementPlan emptyPlan;
  const peano4::topology::Tree::RefinementPlan refinedPlan(1, {tarch::la::Vector<DIMENSIONS, int>(CentreVertex)});

  int regularVertices      = 0;
  int fineInteriorVertices = 1;
  int fineVertices         = 1;
  for (int level = 0; level <= Depth; level++) {
    int verticesOnLevel = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      verticesOnLevel *= peano4::topology::cellsPerAxis(level) + 1;
    }
    regularVertices += verticesOnLevel;
  }
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    fineInteriorVertices *= 2 * SubdivisionFactor - 1;
    fineVertices *= 2 * SubdivisionFactor + 1;
  }

  for (const int numberOfRanks : {1, 2}) {
    for (const auto* const plan : {&emptyPlan, &refinedPlan}) {
      const peano4::topology::Tree ownershipSource(0, offset, width);
      const auto        ownershipPlan      = ownershipSource.deriveBalancedBlockOwners(Depth, numberOfRanks, *plan);
      const auto* const owners             = numberOfRanks > 1 ? &ownershipPlan : nullptr;
      int               persistentVertices = 0;
      int               hangingVertices    = 0;
      for (int rank = 0; rank < numberOfRanks; rank++) {
        peano4::topology::Tree tree(rank, offset, width);
        tree.buildRegularGrid(Depth, SpawnLevel, numberOfRanks, *plan, owners);
        peano4::topology::TreeTraversal traversal(rank, offset, width);
        traversal.prepareRankTopology(tree);
        TouchRecordingObserver observer;
        traversal.traverseWithPools(tree, observer, peano4::topology::TreeTraversal::PoolSweepKind::Standing, nullptr);
        persistentVertices += traversal.getGridStatistics().numberOfVertices;
        hangingVertices += traversal.getGridStatistics().numberOfHangingVertices;
      }
      const bool isRefined                  = plan == &refinedPlan;
      const int  expectedPersistentVertices = regularVertices + (isRefined ? fineInteriorVertices : 0);
      const int  expectedHangingVertices    = isRefined ? fineVertices - fineInteriorVertices : 0;
      validateEqualsWithParams2(persistentVertices, expectedPersistentVertices, numberOfRanks, isRefined);
      validateEqualsWithParams2(hangingVertices, expectedHangingVertices, numberOfRanks, isRefined);
    }
  }
}

/**
 * Two ranks with a boundary between them derive matching pack lists from the
 * partition alone. Each rank names the other, both pack the same number of
 * entities towards it, every packed slot addresses a materialised local entity
 * once, and the frozen slots follow the local slot range.
 */
void peano4::topology::tests::TreeTraversalTest::testRankPairsAgreeOnTheirBoundaryWithoutCommunicating() {
  constexpr int SpawnLevel    = 1;
  constexpr int NumberOfRanks = 2;

  const tarch::la::Vector<DIMENSIONS, double>  offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double>  width(1.0);
  const peano4::topology::Tree::RefinementPlan emptyPlan;

  const peano4::topology::Tree ownershipSource(0, offset, width);
  const auto ownershipPlan = ownershipSource.deriveBalancedBlockOwners(Depth, NumberOfRanks, emptyPlan);

  // Both ranks build from the same plan and read nothing else from each other.
  std::vector<std::unique_ptr<peano4::topology::Tree>>          trees;
  std::vector<std::unique_ptr<peano4::topology::TreeTraversal>> traversals;
  for (int rank = 0; rank < NumberOfRanks; rank++) {
    auto tree = std::make_unique<peano4::topology::Tree>(rank, offset, width);
    tree->buildRegularGrid(Depth, SpawnLevel, NumberOfRanks, emptyPlan, &ownershipPlan);
    auto traversal = std::make_unique<peano4::topology::TreeTraversal>(rank, offset, width);
    traversal->prepareRankTopology(*tree);
    trees.push_back(std::move(tree));
    traversals.push_back(std::move(traversal));
  }

  for (int rank = 0; rank < NumberOfRanks; rank++) {
    const auto& neighbourRanks = traversals[rank]->getNeighbourRanks();
    validateEquals(static_cast<int>(neighbourRanks.size()), 1);
    validateEquals(neighbourRanks[0], 1 - rank);
  }

  const auto& firstVertexPackList  = traversals[0]->getVertexPackLists()[0];
  const auto& secondVertexPackList = traversals[1]->getVertexPackLists()[0];
  const auto& firstFacePackList    = traversals[0]->getFacePackLists()[0];
  const auto& secondFacePackList   = traversals[1]->getFacePackLists()[0];

  // A boundary with no entity would make the checks below vacuous.
  validate(not firstVertexPackList.empty());
  validate(not firstFacePackList.empty());
  validateEquals(secondVertexPackList.size(), firstVertexPackList.size());
  validateEquals(secondFacePackList.size(), firstFacePackList.size());

  for (int rank = 0; rank < NumberOfRanks; rank++) {
    const auto validatePackList = [this](const std::vector<int>& packList, const int slotSpaceSize) {
      std::vector<int> packedSlots(packList);
      std::sort(packedSlots.begin(), packedSlots.end());
      validateEquals(
        static_cast<int>(std::unique(packedSlots.begin(), packedSlots.end()) - packedSlots.begin()),
        static_cast<int>(packList.size())
      );
      for (const auto packedSlot : packList) {
        validate(packedSlot >= 0);
        validate(packedSlot < slotSpaceSize);
      }
    };
    validatePackList(traversals[rank]->getVertexPackLists()[0], traversals[rank]->getVertexSlotSpaceSize());
    validatePackList(traversals[rank]->getFacePackLists()[0], traversals[rank]->getFaceSlotSpaceSize());

    // One frozen slot per packed entity, starting behind the local slot range,
    // so a receive can write the neighbour's message in pack-list order.
    validateEquals(traversals[rank]->getVertexFrozenSlotBases()[0], traversals[rank]->getVertexSlotSpaceSize());
    validateEquals(traversals[rank]->getFaceFrozenSlotBases()[0], traversals[rank]->getFaceSlotSpaceSize());
    validateEquals(
      traversals[rank]->getNumberOfVertexRankFrozenSlots(),
      static_cast<int>(traversals[rank]->getVertexPackLists()[0].size())
    );
    validateEquals(
      traversals[rank]->getNumberOfFaceRankFrozenSlots(),
      static_cast<int>(traversals[rank]->getFacePackLists()[0].size())
    );
  }
}

/** A root-level decomposition assigns every first-level cell to a task. */
void peano4::topology::tests::TreeTraversalTest::testRootChildrenBecomeTasksAtSpawnLevelZero() {
  const tarch::la::Vector<DIMENSIONS, double> offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> width(1.0);

  peano4::topology::Tree tree(0, offset, width);
  tree.buildRegularGrid(1, 0, 1);

  peano4::topology::TreeTraversal traversal(0, offset, width);
  traversal.prepareRankTopology(tree);
  traversal.prepareInterfaceTopology(tree, 0, false);
  traversal.prepareParallelSweep(tree, 0);

  validateEquals(traversal.getNumberOfSubtreeTasks(), peano4::sfc::ActiveCurve::ChildrenPerCell);
}

/**
 * On a whole-domain rank the fresh slot numbering hands every entity its own
 * tree index, over an entity range spanning several chunks.
 */
void peano4::topology::tests::TreeTraversalTest::testFreshSlotsNumberTheEntitiesInIndexOrder() {
  constexpr int ChunkedDepth = DIMENSIONS == 2 ? 5 : 3;

  const tarch::la::Vector<DIMENSIONS, double> offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> width(1.0);

  peano4::topology::Tree tree(0, offset, width);
  tree.buildRegularGrid(ChunkedDepth);
  peano4::topology::TreeTraversal traversal(0, offset, width);
  traversal.prepareRankTopology(tree);

  validateEquals(static_cast<std::size_t>(traversal.getNumberOfLocalCellSlots()), tree.getNumberOfCells());
  validateEquals(static_cast<std::size_t>(traversal.getCellSlotSpaceSize()), tree.getNumberOfCells());
  validateEquals(static_cast<std::size_t>(traversal.getVertexSlotSpaceSize()), tree.getNumberOfVertices());
  validateEquals(static_cast<std::size_t>(traversal.getFaceSlotSpaceSize()), tree.getNumberOfFaces());

  SlotOrderObserver observer(tree);
  traversal.traverseWithPools(tree, observer, peano4::topology::TreeTraversal::PoolSweepKind::Standing, nullptr);
  validateEquals(static_cast<std::size_t>(observer.numberOfCells), tree.getNumberOfCells() - 1);
  validateEquals(observer.numberOfMismatches, 0);
}

/**
 * A topology derived with a predecessor keeps every surviving entity's pool
 * slot, so both hops omit entity migration pairs and the erase direction
 * leaves gaps behind without shrinking the slot range.
 */
void peano4::topology::tests::TreeTraversalTest::testSlotInheritanceAcrossRegrid() {
  const tarch::la::Vector<DIMENSIONS, double> offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> width(1.0);

  peano4::topology::Tree::RefinementPlan refinedPlan;
  refinedPlan.emplace_back().emplace_back(CentreVertex);

  peano4::topology::Tree oldTree(0, offset, width);
  oldTree.buildRegularGrid(Depth);
  peano4::topology::Tree unionTree(0, offset, width);
  unionTree.buildRegularGrid(Depth, 0, 1, refinedPlan);

  peano4::topology::TreeTraversal oldTraversal(0, offset, width);
  oldTraversal.prepareRankTopology(oldTree);
  peano4::topology::TreeTraversal unionTraversal(0, offset, width);
  unionTraversal.prepareRankTopology(unionTree, &oldTree, &oldTraversal);
  peano4::topology::TreeTraversal newTraversal(0, offset, width);
  newTraversal.prepareRankTopology(oldTree, &unionTree, &unionTraversal);

  validate(unionTraversal.getCellSlotSpaceSize() >= oldTraversal.getCellSlotSpaceSize());
  validate(unionTraversal.getVertexSlotSpaceSize() >= oldTraversal.getVertexSlotSpaceSize());
  validate(unionTraversal.getFaceSlotSpaceSize() >= oldTraversal.getFaceSlotSpaceSize());

  const auto refineTransition =
    oldTraversal
      .deriveRegridTransition(oldTree, unionTree, unionTraversal, unionTree, unionTraversal, refinedPlan, false);
  validate(refineTransition.oldToUnionCellPairs.empty());
  validate(refineTransition.oldToUnionFacePairs.empty());
  validate(refineTransition.oldToUnionVertexPairs.empty());

  const auto eraseTransition =
    unionTraversal
      .deriveRegridTransition(unionTree, unionTree, unionTraversal, oldTree, newTraversal, refinedPlan, false);
  validate(eraseTransition.unionToNewCellPairs.empty());
  validate(eraseTransition.unionToNewFacePairs.empty());
  validate(eraseTransition.unionToNewVertexPairs.empty());
  validateEquals(newTraversal.getCellSlotSpaceSize(), unionTraversal.getCellSlotSpaceSize());
  validateEquals(newTraversal.getNumberOfLocalCellSlots(), static_cast<int>(oldTree.getNumberOfCells()));
}

/**
 * A single refinement step creates exactly the new structure's surplus cells,
 * destroys none, flips the flagged vertex's refined-after mask, and pairs every
 * persisting cell in both hops. The erase direction mirrors the classes.
 */
void peano4::topology::tests::TreeTraversalTest::testRegridTransitionClasses() {
  const tarch::la::Vector<DIMENSIONS, double> offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> width(1.0);

  peano4::topology::Tree::RefinementPlan refinedPlan;
  refinedPlan.emplace_back().emplace_back(CentreVertex);

  peano4::topology::Tree oldTree(0, offset, width);
  oldTree.buildRegularGrid(Depth);
  peano4::topology::Tree newTree(0, offset, width);
  newTree.buildRegularGrid(Depth, 0, 1, refinedPlan);
  // A pure refinement's union is the new structure.
  peano4::topology::Tree unionTree(0, offset, width);
  unionTree.buildRegularGrid(Depth, 0, 1, refinedPlan);

  peano4::topology::TreeTraversal oldTraversal(0, offset, width);
  oldTraversal.prepareRankTopology(oldTree);
  peano4::topology::TreeTraversal unionTraversal(0, offset, width);
  unionTraversal.prepareRankTopology(unionTree);
  peano4::topology::TreeTraversal newTraversal(0, offset, width);
  newTraversal.prepareRankTopology(newTree);

  const auto countFlags = [](const std::vector<char>& flags) {
    int count = 0;
    for (const auto flag : flags) {
      count += flag != 0 ? 1 : 0;
    }
    return count;
  };

  const auto refineTransition =
    oldTraversal.deriveRegridTransition(oldTree, unionTree, unionTraversal, newTree, newTraversal, refinedPlan, false);
  const auto numberOfCreatedCells = static_cast<int>(unionTree.getNumberOfCells() - oldTree.getNumberOfCells());
  validateEquals(countFlags(refineTransition.overrides.cellCreate), numberOfCreatedCells);
  validateEquals(countFlags(refineTransition.overrides.cellDestroy), 0);
  const auto flaggedVertexIndex = unionTree.getVertexAt(Depth, tarch::la::Vector<DIMENSIONS, int>(CentreVertex));
  validate(flaggedVertexIndex >= 0);
  validateEquals(refineTransition.overrides.vertexRefinedBefore[flaggedVertexIndex], 0);
  validateEquals(refineTransition.overrides.vertexRefinedAfter[flaggedVertexIndex], 1);
  validateEquals(
    static_cast<int>(refineTransition.oldToUnionCellPairs.size()),
    static_cast<int>(oldTree.getNumberOfCells())
  );
  validateEquals(
    static_cast<int>(refineTransition.unionToNewCellPairs.size()),
    static_cast<int>(newTree.getNumberOfCells())
  );

  const auto eraseTransition =
    unionTraversal
      .deriveRegridTransition(newTree, unionTree, unionTraversal, oldTree, oldTraversal, refinedPlan, false);
  validateEquals(countFlags(eraseTransition.overrides.cellCreate), 0);
  validateEquals(countFlags(eraseTransition.overrides.cellDestroy), numberOfCreatedCells);
  validateEquals(eraseTransition.overrides.vertexRefinedBefore[flaggedVertexIndex], 1);
  validateEquals(eraseTransition.overrides.vertexRefinedAfter[flaggedVertexIndex], 0);
  validateEquals(
    static_cast<int>(eraseTransition.unionToNewCellPairs.size()),
    static_cast<int>(oldTree.getNumberOfCells())
  );
}

/**
 * A transition roots a task at every cell whose children it creates or
 * destroys.
 */
void peano4::topology::tests::TreeTraversalTest::testTransitionTasksRootAtChangingCells() {
  const tarch::la::Vector<DIMENSIONS, double> offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> width(1.0);

  peano4::topology::Tree::RefinementPlan refinedPlan;
  refinedPlan.emplace_back().emplace_back(VertexInsideFirstChild);

  peano4::topology::Tree oldTree(0, offset, width);
  oldTree.buildRegularGrid(Depth);
  peano4::topology::Tree newTree(0, offset, width);
  newTree.buildRegularGrid(Depth, 0, 1, refinedPlan);
  peano4::topology::Tree unionTree(0, offset, width);
  unionTree.buildRegularGrid(Depth, 0, 1, refinedPlan);

  peano4::topology::TreeTraversal oldTraversal(0, offset, width);
  oldTraversal.prepareRankTopology(oldTree);
  peano4::topology::TreeTraversal unionTraversal(0, offset, width);
  unionTraversal.prepareRankTopology(unionTree);
  peano4::topology::TreeTraversal newTraversal(0, offset, width);
  newTraversal.prepareRankTopology(newTree);

  // The cells around the flagged vertex lie in one first-level task, so every thread count above
  // one splits that task and each of the cells roots a task. A single thread splits no task.
  const int expectedTasks = tarch::tasking::Runtime::getInstance().getNumberOfThreads() > 1
    ? ChildrenPerCell + peano4::CellsPerVertex
    : ChildrenPerCell;

  const auto refineTransition =
    oldTraversal.deriveRegridTransition(oldTree, unionTree, unionTraversal, newTree, newTraversal, refinedPlan, false);
  unionTraversal.prepareTransitionTopology(unionTree, 0, refineTransition.overrides);
  unionTraversal.prepareParallelSweep(unionTree, 0, false);
  validateEquals(unionTraversal.getNumberOfSubtreeTasks(), expectedTasks);

  const auto eraseTransition =
    unionTraversal
      .deriveRegridTransition(newTree, unionTree, unionTraversal, oldTree, oldTraversal, refinedPlan, false);
  unionTraversal.prepareTransitionTopology(unionTree, 0, eraseTransition.overrides);
  unionTraversal.prepareParallelSweep(unionTree, 0, false);
  validateEquals(unionTraversal.getNumberOfSubtreeTasks(), expectedTasks);
}

/**
 * An in-place shrink after a prune drops the touches of the pruned cells, keeps
 * every surviving toucher's live slot and the shadow slot range, and names the
 * blocks around the pruned region as the stale units.
 */
void peano4::topology::tests::TreeTraversalTest::testShrunkInterfaceTopologyKeepsTheSurvivingSlots() {
  const tarch::la::Vector<DIMENSIONS, double> offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> width(1.0);
  // The spawn level holds more blocks than the pruned region touches, so the
  // base depth follows the subdivision factor.
  constexpr int PrunedDepth = SubdivisionFactor >= 3 ? Depth : Depth + 1;
  constexpr int SpawnLevel  = PrunedDepth - 1;

  peano4::topology::Tree::RefinementPlan refinedPlan;
  // One block of the spawn level spans SubdivisionFactor cells per axis of the
  // base depth.
  refinedPlan.emplace_back().emplace_back(SubdivisionFactor);
  const peano4::topology::Tree::RefinementPlan emptyPlan;

  peano4::topology::Tree tree(0, offset, width);
  tree.buildRegularGrid(PrunedDepth, SpawnLevel, 1, refinedPlan);
  peano4::topology::TreeTraversal traversal(0, offset, width);
  traversal.prepareRankTopology(tree);
  traversal.prepareInterfaceTopology(tree, SpawnLevel, false);

  const auto seedVertexPairsBefore = traversal.getInterfaceSeedVertexPairs();
  const auto seedFacePairsBefore   = traversal.getInterfaceSeedFacePairs();
  const auto vertexShadowSlots     = traversal.getNumberOfVertexShadowSlots();
  const auto faceShadowSlots       = traversal.getNumberOfFaceShadowSlots();
  validate(not seedVertexPairsBefore.empty());
  validate(not seedFacePairsBefore.empty());

  tree.adoptRefinementPlanTables(emptyPlan);
  const auto    pruneExtent         = tree.pruneRefinementDelta(refinedPlan);
  constexpr int NumberOfPrunedCells = peano4::CellsPerVertex * ChildrenPerCell;
  validateEquals(static_cast<int>(pruneExtent.deadCells.size()), NumberOfPrunedCells);
  validate(traversal.standingCutsMatchFreshSelection(tree, SpawnLevel));

  const auto staleUnits = traversal.shrinkInterfaceTopologyForPrunedEntities(tree, pruneExtent);

  // The flagged vertex lies on the corner the first 2^d spawn-level blocks
  // meet at, so their roots are the stale units.
  std::vector<int> expectedStaleUnits;
  for (int blockNumber = 0; blockNumber < peano4::CellsPerVertex; blockNumber++) {
    tarch::la::Vector<DIMENSIONS, int> blockLattice(0);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      blockLattice(axis) = (blockNumber >> axis) & 1;
    }
    expectedStaleUnits.push_back(tree.getCellAt(SpawnLevel, blockLattice));
  }
  std::sort(expectedStaleUnits.begin(), expectedStaleUnits.end());
  validate(staleUnits == expectedStaleUnits);

  const auto pairsSurvive =
    [](const std::vector<std::pair<int, int>>& before, const std::vector<std::pair<int, int>>& after) {
      for (const auto& pair : after) {
        if (std::find(before.begin(), before.end(), pair) == before.end()) {
          return false;
        }
      }
      return after.size() < before.size();
    };
  validate(pairsSurvive(seedVertexPairsBefore, traversal.getInterfaceSeedVertexPairs()));
  validate(pairsSurvive(seedFacePairsBefore, traversal.getInterfaceSeedFacePairs()));
  validateEquals(traversal.getNumberOfVertexShadowSlots(), vertexShadowSlots);
  validateEquals(traversal.getNumberOfFaceShadowSlots(), faceShadowSlots);
}

/**
 * A regrid that moves the partition pairs its transfer lists positionally
 * across the two ranks, and the lists carry every cell whose owner changes.
 */
void peano4::topology::tests::TreeTraversalTest::testTransferListsPairAcrossRanksWhenThePartitionMoves() {
  constexpr int SpawnLevel    = 1;
  constexpr int NumberOfRanks = 2;

  const tarch::la::Vector<DIMENSIONS, double> offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> width(1.0);
  // The flag at (1, ...) refines the cells of the first level-one cell on the
  // curve, so the leaves before the half-way split grow and the split
  // moves to an earlier level-one cell.
  const peano4::topology::Tree::RefinementPlan emptyPlan;
  const peano4::topology::Tree::RefinementPlan refinedPlan(1, {tarch::la::Vector<DIMENSIONS, int>(1)});

  const peano4::topology::Tree ownershipSource(0, offset, width);
  const auto                   unionOwners = ownershipSource.deriveBalancedBlockOwners(Depth, NumberOfRanks, emptyPlan);
  const auto                   newOwners = ownershipSource.deriveBalancedBlockOwners(Depth, NumberOfRanks, refinedPlan);
  validate(unionOwners != newOwners);

  std::vector<peano4::topology::TreeTraversal::RegridTransition> transitions;
  std::vector<int>                                               movedCellsPerRank;
  for (int rank = 0; rank < NumberOfRanks; rank++) {
    peano4::topology::Tree oldTree(rank, offset, width);
    oldTree.buildRegularGrid(Depth, SpawnLevel, NumberOfRanks, emptyPlan, &unionOwners);
    peano4::topology::Tree unionTree(rank, offset, width);
    unionTree.buildRegularGrid(Depth, SpawnLevel, NumberOfRanks, refinedPlan, &unionOwners);
    peano4::topology::Tree newTree(rank, offset, width);
    newTree.buildRegularGrid(Depth, SpawnLevel, NumberOfRanks, refinedPlan, &newOwners);

    peano4::topology::TreeTraversal oldTraversal(rank, offset, width);
    oldTraversal.prepareRankTopology(oldTree);
    peano4::topology::TreeTraversal unionTraversal(rank, offset, width);
    unionTraversal.prepareRankTopology(unionTree, &oldTree, &oldTraversal);
    peano4::topology::TreeTraversal newTraversal(rank, offset, width);
    newTraversal.prepareRankTopology(newTree, &unionTree, &unionTraversal);
    transitions.push_back(
      oldTraversal.deriveRegridTransition(oldTree, unionTree, unionTraversal, newTree, newTraversal, refinedPlan, true)
    );

    // The rank holds every cell it owns, so the count covers every cell it
    // hands to the other rank.
    int movedCells = 0;
    for (std::size_t cellIndex = 0; cellIndex < unionTree.getNumberOfCells(); cellIndex++) {
      const auto& cell = unionTree.getCell(static_cast<int>(cellIndex));
      if (cell.ownerTreeId == rank and newTree.getOwnerOfCellAt(cell.level, cell.latticeIndex) != rank) {
        movedCells++;
      }
    }
    movedCellsPerRank.push_back(movedCells);
  }

  const auto sizeOf = [](const std::map<int, std::vector<int>>& slots, const int remoteRank) {
    const auto entry = slots.find(remoteRank);
    return entry == slots.end() ? std::size_t{0} : entry->second.size();
  };
  int movedCells = 0;
  for (int rank = 0; rank < NumberOfRanks; rank++) {
    const int   otherRank = NumberOfRanks - 1 - rank;
    const auto& sender    = transitions[rank];
    const auto& receiver  = transitions[otherRank];
    validateEqualsWithParams2(
      sizeOf(sender.sendCellSlots, otherRank),
      static_cast<std::size_t>(movedCellsPerRank[rank]),
      rank,
      otherRank
    );
    validateEqualsWithParams2(
      sizeOf(sender.sendCellSlots, otherRank),
      sizeOf(receiver.receiveCellSlots, rank),
      rank,
      otherRank
    );
    validateEqualsWithParams2(
      sizeOf(sender.sendFaceSlots, otherRank),
      sizeOf(receiver.receiveFaceSlots, rank),
      rank,
      otherRank
    );
    validateEqualsWithParams2(
      sizeOf(sender.sendVertexSlots, otherRank),
      sizeOf(receiver.receiveVertexSlots, rank),
      rank,
      otherRank
    );
    movedCells += movedCellsPerRank[rank];
  }
  validate(movedCells > 0);
}

/**
 * The rank boundary of an in-place append and prune on a partitioned tree
 * equals a fresh derivation, and every surviving rank-boundary entity keeps a
 * frozen slot through the relocation.
 */
void peano4::topology::tests::TreeTraversalTest::testInPlaceRegridRederivesTheRankBoundary() {
  constexpr int SpawnLevel    = 1;
  constexpr int NumberOfRanks = 2;

  const tarch::la::Vector<DIMENSIONS, double>  offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double>  width(1.0);
  const peano4::topology::Tree::RefinementPlan emptyPlan;
  // The flag at the centre vertex refines the cells around it, which
  // both ranks of the two-rank partition touch.
  const peano4::topology::Tree::RefinementPlan refinedPlan(1, {tarch::la::Vector<DIMENSIONS, int>(CentreVertex)});

  const peano4::topology::Tree ownershipSource(0, offset, width);
  const auto ownershipPlan = ownershipSource.deriveBalancedBlockOwners(Depth, NumberOfRanks, emptyPlan);

  const auto boundariesMatch =
    [](const peano4::topology::TreeTraversal& left, const peano4::topology::TreeTraversal& right) {
      return left.getNeighbourRanks() == right.getNeighbourRanks()
        and left.getVertexPackLists().size() == right.getVertexPackLists().size()
        and left.getFacePackLists().size() == right.getFacePackLists().size()
        and left.getNumberOfVertexRankFrozenSlots() == right.getNumberOfVertexRankFrozenSlots()
        and left.getNumberOfFaceRankFrozenSlots() == right.getNumberOfFaceRankFrozenSlots();
    };
  const auto packListSizesMatch =
    [](const std::vector<std::vector<int>>& left, const std::vector<std::vector<int>>& right) {
      if (left.size() != right.size()) {
        return false;
      }
      for (std::size_t rankNumber = 0; rankNumber < left.size(); rankNumber++) {
        if (left[rankNumber].size() != right[rankNumber].size()) {
          return false;
        }
      }
      return true;
    };

  for (int rank = 0; rank < NumberOfRanks; rank++) {
    peano4::topology::Tree tree(rank, offset, width);
    tree.buildRegularGrid(Depth, SpawnLevel, NumberOfRanks, emptyPlan, &ownershipPlan);
    peano4::topology::TreeTraversal traversal(rank, offset, width);
    traversal.prepareRankTopology(tree);
    validate(not traversal.getNeighbourRanks().empty());
    const auto vertexSlotSpaceBefore   = traversal.getVertexSlotSpaceSize();
    const auto vertexFrozenSlotsBefore = traversal.getNumberOfVertexRankFrozenSlots();
    const auto faceFrozenSlotsBefore   = traversal.getNumberOfFaceRankFrozenSlots();

    peano4::topology::Tree planView(rank, offset, width);
    planView.buildPlanQueryTablesFrom(Depth, tree, refinedPlan, refinedPlan, emptyPlan);
    const auto appendedDelta = tree.appendRefinementDelta(planView, refinedPlan, refinedPlan);
    const auto relocation    = traversal.extendRankTopologyForAppendedEntities(tree, appendedDelta);
    tree.adoptRefinementPlanTablesFrom(planView);

    peano4::topology::Tree refinedTree(rank, offset, width);
    refinedTree.buildRegularGrid(Depth, SpawnLevel, NumberOfRanks, refinedPlan, &ownershipPlan);
    peano4::topology::TreeTraversal refinedTraversal(rank, offset, width);
    refinedTraversal.prepareRankTopology(refinedTree);
    validate(boundariesMatch(traversal, refinedTraversal));
    validate(packListSizesMatch(traversal.getVertexPackLists(), refinedTraversal.getVertexPackLists()));
    validate(packListSizesMatch(traversal.getFacePackLists(), refinedTraversal.getFacePackLists()));
    validate(traversal.getNumberOfVertexRankFrozenSlots() > vertexFrozenSlotsBefore);
    // Every frozen slot of the regular grid moves along, so the pairs count
    // the entries before the append, every source lies in the frozen range
    // before it and every target in the new frozen range behind the grown
    // slot range.
    validateEquals(static_cast<int>(relocation.vertexPairs.size()), vertexFrozenSlotsBefore);
    validateEquals(static_cast<int>(relocation.facePairs.size()), faceFrozenSlotsBefore);
    for (const auto& [oldSlot, newSlot] : relocation.vertexPairs) {
      validate(oldSlot >= vertexSlotSpaceBefore and oldSlot < vertexSlotSpaceBefore + vertexFrozenSlotsBefore);
      validate(newSlot >= traversal.getVertexSlotSpaceSize());
      validate(newSlot < traversal.getVertexSlotSpaceSize() + traversal.getNumberOfVertexRankFrozenSlots());
    }

    // The prune reads the plan the structure returns to, so the tables adopt
    // it first.
    tree.adoptRefinementPlanTables(emptyPlan);
    const auto pruneExtent = tree.pruneRefinementDelta(refinedPlan);
    validate(not pruneExtent.deadCells.empty());
    const auto                      pruneRelocation = traversal.rederiveRankBoundary(tree);
    peano4::topology::TreeTraversal regularTraversal(rank, offset, width);
    peano4::topology::Tree          regularTree(rank, offset, width);
    regularTree.buildRegularGrid(Depth, SpawnLevel, NumberOfRanks, emptyPlan, &ownershipPlan);
    regularTraversal.prepareRankTopology(regularTree);
    validate(boundariesMatch(traversal, regularTraversal));
    validate(packListSizesMatch(traversal.getVertexPackLists(), regularTraversal.getVertexPackLists()));
    validate(packListSizesMatch(traversal.getFacePackLists(), regularTraversal.getFacePackLists()));
    validateEquals(static_cast<int>(pruneRelocation.vertexPairs.size()), vertexFrozenSlotsBefore);
    validateEquals(static_cast<int>(pruneRelocation.facePairs.size()), faceFrozenSlotsBefore);
  }
}
