// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TreeTest.h"

#include "peano4/Constants.h"
#include "peano4/RefinementRequest.h"
#include "peano4/grid/GridVertex.h"
#include "peano4/sfc/ActiveCurve.h"
#include "peano4/topology/Tree.h"
#include "tarch/la/Vector.h"

namespace {
  constexpr int Depth             = 2;
  constexpr int LocalRank         = 0;
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;

  constexpr int CellsPerAxis = peano4::power(SubdivisionFactor, Depth);

  /**
   * Lattice position of the cell the refinement-plan tests request, away from the boundary.
   */
  constexpr int RequestedCell = CellsPerAxis / 2;
} // namespace

peano4::topology::tests::TreeTest::TreeTest() :
  TestCase("peano4::topology::tests::TreeTest") {}

void
peano4::topology::tests::TreeTest::run() {
  testMethod(testRegularGridSizes);
  testMethod(testCellLinks);
  testMethod(testDenseCellLookupMatchesTheLattice);
  testMethod(testAdjacentCellsReferenceOneVertex);
  testMethod(testVertexContent);
  testMethod(testRefinementPlanDilation);
  testMethod(testRefinementPlanCoarseDilation);
  testMethod(testRefinementPlanWrapsOnAPeriodicAxis);
  testMethod(testRefinementPlanErase);
  testMethod(testRefinementPlanRoundKeepsLevelsWithoutRequests);
  testMethod(testRefinementPlanEraseKeepsFlagsBetweenRefinedCells);
  testMethod(testVertexBetweenRefinedCellsIsPersistent);
  testMethod(testBalancedOwnersFromPlan);
  testMethod(testTheBalancedPartitionKeepsSiblingLeavesTogether);
  testMethod(testRankLoadsOfADeeperOwnershipPlan);
  testMethod(testAPartitionSplittingSiblingLeavesIsDetected);
  testMethod(testARankWithoutABlockMaterialisesNothingBelowTheSpawnLevel);
  testMethod(testAPartitionedBuildLinksEveryEntityOnce);
  testMethod(testBytesGrowWithTheGrid);
}

/**
 * The bytes of a tree cover at least its cells and vertices and grow with the depth.
 */
void
peano4::topology::tests::TreeTest::testBytesGrowWithTheGrid() {
  const tarch::la::Vector<DIMENSIONS, double> offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> width(1.0);

  peano4::topology::Tree shallow(0, offset, width);
  shallow.buildRegularGrid(1);
  validate(
    shallow.getAllocatedBytes() >= shallow.getNumberOfCells() * sizeof(peano4::topology::TreeCell)
      + shallow.getNumberOfVertices() * sizeof(peano4::grid::GridVertex)
  );

  peano4::topology::Tree deep(0, offset, width);
  deep.buildRegularGrid(2);
  validate(deep.getAllocatedBytes() > shallow.getAllocatedBytes());
}

/**
 * In a partitioned build with and without a periodic axis, every cell, vertex and face lookup returns
 * the index the cells point to. No index belongs to two lattice positions.
 */
void
peano4::topology::tests::TreeTest::testAPartitionedBuildLinksEveryEntityOnce() {
  constexpr int PartitionedDepth = 4;
  constexpr int SpawnLevel       = 1;
  constexpr int NumberOfRanks    = 2;

  peano4::topology::Tree::RefinementPlan refinementPlan;
  refinementPlan.emplace_back().emplace_back(5);

  const auto getLatticeKeyOf = [](const tarch::la::Vector<DIMENSIONS, int>& lattice) {
    std::int64_t key = 0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      key = key * 4096 + lattice(axis);
    }
    return key;
  };

  for (const bool periodic : {false, true}) {
    std::bitset<DIMENSIONS> periodicBoundaries;
    periodicBoundaries[0] = periodic;
    for (int rank = 0; rank < NumberOfRanks; rank++) {
      peano4::topology::Tree tree(
        rank,
        tarch::la::Vector<DIMENSIONS, double>(0.0),
        tarch::la::Vector<DIMENSIONS, double>(1.0),
        periodicBoundaries
      );
      const auto ownershipPlan = tree.deriveBalancedBlockOwners(PartitionedDepth, NumberOfRanks, refinementPlan);
      tree.buildRegularGrid(PartitionedDepth, SpawnLevel, NumberOfRanks, refinementPlan, &ownershipPlan);
      validate(tree.getNumberOfCells() > 0);

      std::unordered_map<int, std::int64_t> latticeOfVertexIndex;
      std::unordered_map<int, std::int64_t> latticeOfFaceIndex;
      int                                   numberOfBlockLevelCells = 0;
      for (int cellIndex = 0; cellIndex < static_cast<int>(tree.getNumberOfCells()); cellIndex++) {
        const auto& cell = tree.getCell(cellIndex);
        if (cell.level <= SpawnLevel or cell.level > PartitionedDepth) {
          continue;
        }
        numberOfBlockLevelCells++;
        const int cellsPerAxisOnLevel = tree.getCellsPerAxisOfLevel(cell.level);
        validateEqualsWithParams3(tree.getCellAt(cell.level, cell.latticeIndex), cellIndex, periodic, rank, cell.level);
        validateEqualsWithParams2(tree.getCell(cell.parent).level, cell.level - 1, periodic, rank);
        validateWithParams3(tree.isRefined(cellIndex) or cell.level == PartitionedDepth, periodic, rank, cellIndex);
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          validateEqualsWithParams3(
            tree.getCell(cell.parent).latticeIndex(axis),
            cell.latticeIndex(axis) / SubdivisionFactor,
            periodic,
            rank,
            axis
          );
        }
        if (tree.isRefined(cellIndex)) {
          for (int childPosition = 0; childPosition < ChildrenPerCell; childPosition++) {
            validateEqualsWithParams3(
              tree.getCell(tree.getChild(cellIndex, childPosition)).parent,
              cellIndex,
              periodic,
              rank,
              childPosition
            );
          }
        }
        for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
          const auto vertexIndex   = cell.vertexIndices[vertexNumber];
          auto       cornerLattice = cell.latticeIndex;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            cornerLattice(axis) += (vertexNumber >> axis) & 1;
            if (periodicBoundaries[axis] and cornerLattice(axis) == cellsPerAxisOnLevel) {
              cornerLattice(axis) = 0;
            }
          }
          validate(vertexIndex >= 0);
          validateEqualsWithParams3(
            tree.getVertexAt(cell.level, cornerLattice),
            vertexIndex,
            periodic,
            rank,
            vertexNumber
          );
          validateEqualsWithParams2(tree.getVertex(vertexIndex).level, cell.level, periodic, rank);
#if PEANO_DEBUG
          // A periodic boundary vertex has the position of the lower boundary in every block.
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            validateNumericalEqualsWithParams3(
              tree.getVertex(vertexIndex).getX(axis),
              static_cast<double>(cornerLattice(axis)) / cellsPerAxisOnLevel,
              periodic,
              rank,
              vertexIndex
            );
          }
#endif
          const auto [entry, isNew] = latticeOfVertexIndex.insert({vertexIndex, getLatticeKeyOf(cornerLattice)});
          validateEqualsWithParams3(entry->second, getLatticeKeyOf(cornerLattice), periodic, rank, vertexIndex);
          static_cast<void>(isNew);
        }
        for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
          const auto faceIndex   = cell.faceIndices[faceNumber];
          const int  normal      = faceNumber % DIMENSIONS;
          auto       faceLattice = cell.latticeIndex;
          if (faceNumber >= DIMENSIONS) {
            faceLattice(normal) += 1;
          }
          validate(faceIndex >= 0);
          validateEqualsWithParams3(
            tree.getFaceAt(cell.level, normal, faceLattice),
            faceIndex,
            periodic,
            rank,
            faceNumber
          );
          if (periodicBoundaries[normal] and faceLattice(normal) == cellsPerAxisOnLevel) {
            faceLattice(normal) = 0;
          }
          faceLattice(normal) += normal * 4096 * 4096;
          const auto [entry, isNew] = latticeOfFaceIndex.insert({faceIndex, getLatticeKeyOf(faceLattice)});
          validateEqualsWithParams3(entry->second, getLatticeKeyOf(faceLattice), periodic, rank, faceIndex);
          static_cast<void>(isNew);
        }
      }
      validate(numberOfBlockLevelCells > 0);
    }
  }
}

void
peano4::topology::tests::TreeTest::testARankWithoutABlockMaterialisesNothingBelowTheSpawnLevel() {
  constexpr int SpawnLevel              = 1;
  constexpr int OwningRankId            = 0;
  constexpr int NumberOfRanks           = 4096;
  constexpr int OwnershipPlanHeaderSize = 2;
  constexpr int TopologyNodeStride      = 3;

  peano4::topology::Tree::RefinementPlan refinementPlan;
  refinementPlan.emplace_back().emplace_back(RequestedCell);

  const peano4::topology::Tree planningTree(
    OwningRankId,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  const auto ownershipPlan = planningTree.deriveBalancedBlockOwners(Depth, NumberOfRanks, refinementPlan);

  // The plan has far fewer leaves than ranks. The ranks above its highest owner own nothing.
  int highestOwner = -1;
  for (int nodeIndex = 0; nodeIndex < ownershipPlan[1]; nodeIndex++) {
    const auto planOffset = OwnershipPlanHeaderSize + nodeIndex * TopologyNodeStride;
    highestOwner          = std::max(highestOwner, ownershipPlan[planOffset + 1]);
  }
  validate(highestOwner >= 0 and highestOwner < NumberOfRanks - 1);
  const int rankWithoutABlock = highestOwner + 1;

  const auto getNumberOfCellsOfRank = [&](const int rank) {
    peano4::topology::Tree tree(
      rank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0)
    );
    tree.buildRegularGrid(Depth, SpawnLevel, NumberOfRanks, refinementPlan, &ownershipPlan);
    return tree.getNumberOfCells();
  };

  // Every rank stores the levels up to the spawn level densely. A rank without a block stores
  // nothing below.
  std::size_t cellsToTheSpawnLevel = 0;
  for (int level = 0; level <= SpawnLevel; level++) {
    std::size_t cellsOnLevel = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellsOnLevel *= peano4::topology::getCellsPerAxis(level);
    }
    cellsToTheSpawnLevel += cellsOnLevel;
  }

  validateEquals(getNumberOfCellsOfRank(rankWithoutABlock), cellsToTheSpawnLevel);
  validate(getNumberOfCellsOfRank(OwningRankId) > cellsToTheSpawnLevel);
}

/**
 * The ownership plan is the same on every tree, every node covers the rank ranges of its children,
 * and the built tree reports the same owners for its cells.
 */
void
peano4::topology::tests::TreeTest::testBalancedOwnersFromPlan() {
  constexpr int SpawnLevel              = 1;
  constexpr int NumberOfRanks           = 4;
  constexpr int OwnershipPlanHeaderSize = 2;
  constexpr int TopologyNodeStride      = 3;

  peano4::topology::Tree::RefinementPlan refinementPlan;
  refinementPlan.emplace_back().emplace_back(RequestedCell);

  peano4::topology::Tree tree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  const auto ownershipPlan = tree.deriveBalancedBlockOwners(Depth, NumberOfRanks, refinementPlan);

  const peano4::topology::Tree replicaTree(
    LocalRank + 1,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  validate(ownershipPlan == replicaTree.deriveBalancedBlockOwners(Depth, NumberOfRanks, refinementPlan));

  // The leaves of the plan are the regular grid with the children of the cells around the flagged
  // vertex in place of these cells.
  std::int64_t expectedLeaves = 1;
  for (int level = 0; level < Depth; level++) {
    expectedLeaves *= ChildrenPerCell;
  }
  expectedLeaves += static_cast<std::int64_t>(peano4::CellsPerVertex) * (ChildrenPerCell - 1);
  const auto loads = tree.deriveRankLoads(Depth, NumberOfRanks, refinementPlan, ownershipPlan);
  validateEquals(static_cast<int>(loads.size()), NumberOfRanks);
  std::int64_t totalLoad = 0;
  for (const auto load : loads) {
    validate(load > 0);
    totalLoad += load;
  }
  validateEquals(totalLoad, expectedLeaves);
  validate(loads == replicaTree.deriveRankLoads(Depth, NumberOfRanks, refinementPlan, ownershipPlan));

  validate(static_cast<int>(ownershipPlan.size()) > OwnershipPlanHeaderSize);
  validateEquals(ownershipPlan[0], peano4::topology::Tree::OwnershipPlanMarker);
  const auto numberOfTopologyNodes = ownershipPlan[1];
  validateEquals(
    static_cast<int>(ownershipPlan.size()),
    OwnershipPlanHeaderSize + numberOfTopologyNodes * TopologyNodeStride
  );

  for (int nodeIndex = 0; nodeIndex < numberOfTopologyNodes; nodeIndex++) {
    const auto planOffset    = OwnershipPlanHeaderSize + nodeIndex * TopologyNodeStride;
    const auto firstOwner    = ownershipPlan[planOffset];
    const auto lastOwner     = ownershipPlan[planOffset + 1];
    const auto childrenBlock = ownershipPlan[planOffset + 2];
    validate(firstOwner >= 0 and firstOwner <= lastOwner and lastOwner < NumberOfRanks);
    if (childrenBlock < 0) {
      validateEquals(firstOwner, lastOwner);
      continue;
    }
    // A node has children only where a rank boundary crosses it. Its range spans several ranks and
    // covers the ranges of its children.
    validate(firstOwner < lastOwner);
    validate(childrenBlock > nodeIndex and childrenBlock + ChildrenPerCell <= numberOfTopologyNodes);
    for (int childPosition = 0; childPosition < ChildrenPerCell; childPosition++) {
      const auto childOffset = OwnershipPlanHeaderSize + (childrenBlock + childPosition) * TopologyNodeStride;
      validate(ownershipPlan[childOffset] >= firstOwner and ownershipPlan[childOffset + 1] <= lastOwner);
    }
  }

  tree.buildRegularGrid(Depth, SpawnLevel, NumberOfRanks, refinementPlan, &ownershipPlan);

  // The tree materialises a subset of the leaves of the domain. A parent precedes its children in the
  // cell array and one backward pass collects for every cell the owners of its materialised leaves.
  std::vector<std::bitset<NumberOfRanks>> ranksBelowCell(tree.getNumberOfCells());
  for (int cellIndex = static_cast<int>(tree.getNumberOfCells()) - 1; cellIndex >= 0; cellIndex--) {
    if (tree.isRefined(cellIndex)) {
      for (int childPosition = 0; childPosition < ChildrenPerCell; childPosition++) {
        ranksBelowCell[cellIndex] |= ranksBelowCell[tree.getChild(cellIndex, childPosition)];
      }
      continue;
    }
    const auto owner = tree.getCell(cellIndex).ownerRank;
    validate(owner >= 0 and owner < NumberOfRanks);
    ranksBelowCell[cellIndex].set(owner);
  }
  // The tree's own leaves always materialise.
  validate(ranksBelowCell[peano4::topology::Tree::RootCell].test(LocalRank));

  for (int cellIndex = 0; cellIndex < static_cast<int>(tree.getNumberOfCells()); cellIndex++) {
    const auto& cell  = tree.getCell(cellIndex);
    const auto& ranks = ranksBelowCell[cellIndex];
    // The owner of every materialised leaf lies in the owner interval of each ancestor. The owner of
    // a cell is the first rank of its interval.
    for (int rank = 0; rank < NumberOfRanks; rank++) {
      if (ranks.test(rank)) {
        validate(tree.subtreeTouchesRank(cellIndex, rank));
      }
      validateEquals(
        tree.subtreeTouchesRankAt(cell.level, cell.latticeIndex, rank),
        tree.subtreeTouchesRank(cellIndex, rank)
      );
    }
    validateEquals(tree.getOwnerOfCellAt(cell.level, cell.latticeIndex), cell.ownerRank);
  }
}

namespace {
  constexpr int ManyRanks = 4096;

  /**
   * Flags every vertex of the children of the cell with coordinate parentCell on every axis. All
   * its children then refine and the partition descends through the cell.
   */
  void flagVerticesOfCellChildren(std::vector<tarch::la::Vector<DIMENSIONS, int>>& flags, const int parentCell) {
    constexpr int VerticesPerAxis  = SubdivisionFactor + 1;
    const int     firstCoordinate  = SubdivisionFactor * parentCell;
    int           numberOfVertices = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      numberOfVertices *= VerticesPerAxis;
    }
    for (int vertexNumber = 0; vertexNumber < numberOfVertices; vertexNumber++) {
      tarch::la::Vector<DIMENSIONS, int> vertexLattice(0);
      int                                remainingNumber = vertexNumber;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        vertexLattice(axis) = firstCoordinate + remainingNumber % VerticesPerAxis;
        remainingNumber /= VerticesPerAxis;
      }
      flags.push_back(vertexLattice);
    }
  }

  /**
   * Returns a plan that refines all children of one cell on each of three levels below the base depth.
   */
  peano4::topology::Tree::RefinementPlan getThreeLevelPlan() {
    // The flags of each level cover the children of a cell that the flags of the coarser level refine.
    constexpr int MiddleCell  = SubdivisionFactor - 1;
    constexpr int DeepestCell = SubdivisionFactor * MiddleCell + 1;

    peano4::topology::Tree::RefinementPlan deepPlan(3);
    flagVerticesOfCellChildren(deepPlan[0], 0);
    flagVerticesOfCellChildren(deepPlan[1], MiddleCell);
    flagVerticesOfCellChildren(deepPlan[2], DeepestCell);
    return deepPlan;
  }
} // namespace

/**
 * An ownership plan splits no cell with a leaf child of its own refinement plan. The ownership
 * plan of a deeper refinement plan splits the leaves of a shallower one.
 */
void
peano4::topology::tests::TreeTest::testAPartitionSplittingSiblingLeavesIsDetected() {
  const auto                                   deepPlan = getThreeLevelPlan();
  const peano4::topology::Tree::RefinementPlan shallowPlan(deepPlan.begin(), deepPlan.begin() + 1);
  const peano4::topology::Tree                 tree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  const auto deepOwnershipPlan    = tree.deriveBalancedBlockOwners(Depth, ManyRanks, deepPlan);
  const auto shallowOwnershipPlan = tree.deriveBalancedBlockOwners(Depth, ManyRanks, shallowPlan);
  const int  spawnLevel           = peano4::topology::Tree::deriveSpawnLevel(Depth, ManyRanks);
  validateFalse(tree.ownershipPlanSplitsCellWithLeafChild(Depth, spawnLevel, deepPlan, deepOwnershipPlan));
  validateFalse(tree.ownershipPlanSplitsCellWithLeafChild(Depth, spawnLevel, shallowPlan, shallowOwnershipPlan));
  validate(tree.ownershipPlanSplitsCellWithLeafChild(Depth, spawnLevel, shallowPlan, deepOwnershipPlan));
}

/**
 * The balanced partition of a regular grid gives every rank whole groups of sibling leaves on the
 * base depth.
 */
void
peano4::topology::tests::TreeTest::testTheBalancedPartitionKeepsSiblingLeavesTogether() {
  // The spawn level grows with the rank count and shrinks with the children per cell. This depth
  // keeps one level of sibling leaves below the spawn level.
  constexpr int DeepDepth = SubdivisionFactor >= 3 ? 3 : 4;

  const peano4::topology::Tree::RefinementPlan emptyPlan;
  const peano4::topology::Tree                 tree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  for (const int numberOfRanks : {2, 4}) {
    const int spawnLevel = peano4::topology::Tree::deriveSpawnLevel(DeepDepth, numberOfRanks);
    validateWithParams2(spawnLevel <= DeepDepth - 2, numberOfRanks, spawnLevel);
    const auto ownershipPlan = tree.deriveBalancedBlockOwners(DeepDepth, numberOfRanks, emptyPlan);
    validateFalse(tree.ownershipPlanSplitsCellWithLeafChild(DeepDepth, spawnLevel, emptyPlan, ownershipPlan));
    for (const auto load : tree.deriveRankLoads(DeepDepth, numberOfRanks, emptyPlan, ownershipPlan)) {
      validateEqualsWithParams1(load % ChildrenPerCell, 0, numberOfRanks);
    }
  }
}

/**
 * The ownership plan of a deeper refinement plan counts a leaf of a shallower plan that it splits for
 * the first rank of the split.
 */
void
peano4::topology::tests::TreeTest::testRankLoadsOfADeeperOwnershipPlan() {
  const auto                                   deepPlan = getThreeLevelPlan();
  const peano4::topology::Tree::RefinementPlan shallowPlan(deepPlan.begin(), deepPlan.begin() + 1);

  const peano4::topology::Tree tree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  const auto deepOwnershipPlan = tree.deriveBalancedBlockOwners(Depth, ManyRanks, deepPlan);
  // Every cell up to the base depth splits. Below it the two cells split whose children are all
  // refined.
  int expectedTopologyNodes = 2 * ChildrenPerCell;
  int nodesOnLevel          = 1;
  for (int level = 0; level <= Depth; level++) {
    expectedTopologyNodes += nodesOnLevel;
    nodesOnLevel *= ChildrenPerCell;
  }
  validateEquals(deepOwnershipPlan[1], expectedTopologyNodes);

  // The flags of the shallow plan cover the corners of the children of one cell. They refine the
  // subdivision factor plus one cells per axis.
  std::int64_t shallowLeaves       = 1;
  std::int64_t refinedShallowCells = 1;
  for (int level = 0; level < Depth; level++) {
    shallowLeaves *= ChildrenPerCell;
  }
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    refinedShallowCells *= SubdivisionFactor + 1;
  }
  shallowLeaves += refinedShallowCells * (ChildrenPerCell - 1);
  const auto   loads     = tree.deriveRankLoads(Depth, ManyRanks, shallowPlan, deepOwnershipPlan);
  std::int64_t totalLoad = 0;
  for (const auto load : loads) {
    validate(load >= 0);
    totalLoad += load;
  }
  validateEquals(totalLoad, shallowLeaves);
}

void
peano4::topology::tests::TreeTest::testRegularGridSizes() {
  peano4::topology::Tree tree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  tree.buildRegularGrid(Depth);

  std::size_t expectedCells    = 0;
  std::size_t expectedVertices = 0;
  for (int level = 0; level <= Depth; level++) {
    std::size_t cellsOnLevel    = 1;
    std::size_t verticesOnLevel = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellsOnLevel *= peano4::topology::getCellsPerAxis(level);
      verticesOnLevel *= peano4::topology::getCellsPerAxis(level) + 1;
    }
    expectedCells += cellsOnLevel;
    expectedVertices += verticesOnLevel;
  }

  validateEquals(tree.getNumberOfCells(), expectedCells);
  validateEquals(tree.getNumberOfVertices(), expectedVertices);
  validateEquals(tree.getDepth(), Depth);
}

/**
 * getCellAt() returns the index of every dense cell on every level, also with a periodic axis.
 */
void
peano4::topology::tests::TreeTest::testDenseCellLookupMatchesTheLattice() {
  constexpr int LookupDepth = 4;
  for (const bool periodic : {false, true}) {
    std::bitset<DIMENSIONS> periodicBoundaries;
    periodicBoundaries[0] = periodic;
    peano4::topology::Tree tree(
      LocalRank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0),
      periodicBoundaries
    );
    tree.buildRegularGrid(LookupDepth);
    for (int cellIndex = 0; cellIndex < static_cast<int>(tree.getNumberOfCells()); cellIndex++) {
      const auto& cell = tree.getCell(cellIndex);
      validateEqualsWithParams2(
        tree.getCellAt(cell.level, cell.latticeIndex),
        cellIndex,
        cell.level,
        cell.latticeIndex
      );
    }
  }
}

void
peano4::topology::tests::TreeTest::testCellLinks() {
  peano4::topology::Tree tree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  tree.buildRegularGrid(Depth);

  const auto& root = tree.getCell(peano4::topology::Tree::RootCell);
  validateEquals(root.level, 0);
  validateEquals(root.parent, -1);
  validate(tree.isRefined(peano4::topology::Tree::RootCell));

  for (int cellIndex = 0; cellIndex < static_cast<int>(tree.getNumberOfCells()); cellIndex++) {
    const auto& cell = tree.getCell(cellIndex);
    validateEquals(cell.ownerRank, LocalRank);
    validateEquals(tree.isRefined(cellIndex), (cell.level < Depth));
    if (not tree.isRefined(cellIndex)) {
      continue;
    }
    for (int childPosition = 0; childPosition < ChildrenPerCell; childPosition++) {
      const auto  childIndex = tree.getChild(cellIndex, childPosition);
      const auto& child      = tree.getCell(childIndex);
      validateEquals(child.parent, cellIndex);
      validateEquals(child.level, cell.level + 1);
      int positionRemainder = childPosition;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        validateEquals(
          child.latticeIndex(axis),
          SubdivisionFactor * cell.latticeIndex(axis) + positionRemainder % SubdivisionFactor
        );
        positionRemainder /= SubdivisionFactor;
      }
    }
  }

  constexpr int                          DeepestSparseLevel = 12;
  peano4::topology::Tree::RefinementPlan deepRefinementPlan(DeepestSparseLevel);
  for (int level = 0; level < DeepestSparseLevel; level++) {
    deepRefinementPlan[level].emplace_back(peano4::topology::getCellsPerAxis(level));
  }
  peano4::topology::Tree deepTree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  deepTree.buildRegularGrid(0, 0, 1, deepRefinementPlan);

  // One sparse map per normal keeps every lattice bit at depths where a face key with the normal would
  // need more than 64 bits in 3D.
  const int                   cellsPerAxisOnDeepestLevel = peano4::topology::getCellsPerAxis(DeepestSparseLevel);
  std::array<int, DIMENSIONS> faceIndices{};
  for (int normal = 0; normal < DIMENSIONS; normal++) {
    tarch::la::Vector<DIMENSIONS, int> faceLattice(cellsPerAxisOnDeepestLevel - 1);
    faceLattice(normal) = cellsPerAxisOnDeepestLevel;
    faceIndices[normal] = deepTree.getFaceAt(DeepestSparseLevel, normal, faceLattice);
    validate(faceIndices[normal] >= 0);
    for (int otherNormal = 0; otherNormal < normal; otherNormal++) {
      validate(faceIndices[normal] != faceIndices[otherNormal]);
    }
  }
}

void
peano4::topology::tests::TreeTest::testAdjacentCellsReferenceOneVertex() {
  peano4::topology::Tree tree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  tree.buildRegularGrid(Depth);

  // A cell and its neighbour along axis 0 point to the same indices for the vertices between them.
  const auto  leftCellIndex  = tree.getChild(peano4::topology::Tree::RootCell, 0);
  const auto  rightCellIndex = tree.getChild(peano4::topology::Tree::RootCell, 1);
  const auto& leftCell       = tree.getCell(leftCellIndex);
  const auto& rightCell      = tree.getCell(rightCellIndex);

  for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
    if ((vertexNumber & 1) == 1) {
      validateEquals(leftCell.vertexIndices[vertexNumber], rightCell.vertexIndices[vertexNumber ^ 1]);
    }
  }
}

void
peano4::topology::tests::TreeTest::testVertexContent() {
  peano4::topology::Tree tree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  tree.buildRegularGrid(Depth);

  for (int cellIndex = 0; cellIndex < static_cast<int>(tree.getNumberOfCells()); cellIndex++) {
    const auto& cell                = tree.getCell(cellIndex);
    const int   cellsPerAxisOnLevel = peano4::topology::getCellsPerAxis(cell.level);
    const bool  isFinestLevel       = cell.level == Depth;

    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      const auto& vertex = tree.getVertex(cell.vertexIndices[vertexNumber]);
      validateEquals(vertex.level, cell.level);
      validate(
        vertex.getState()
        == (isFinestLevel ? peano4::grid::GridVertex::State::Unrefined : peano4::grid::GridVertex::State::Refined)
      );

      // Every adjacency entry inside the domain returns this rank. Every entry outside returns -1.
      tarch::la::Vector<DIMENSIONS, int> vertexLattice(0);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        vertexLattice(axis) = cell.latticeIndex(axis) + ((vertexNumber >> axis) & 1);
      }
      for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
        bool isInsideDomain = true;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          const int adjacentCellCoordinate = vertexLattice(axis) - 1 + ((adjacencyEntry >> axis) & 1);
          isInsideDomain =
            isInsideDomain and adjacentCellCoordinate >= 0 and adjacentCellCoordinate < cellsPerAxisOnLevel;
        }
        validateEquals(
          tree.getAdjacentCellOwnerAt(cell.level, vertexLattice, adjacencyEntry),
          (isInsideDomain ? LocalRank : -1)
        );
      }
    }
  }

  std::bitset<DIMENSIONS> periodicBoundaries;
  periodicBoundaries.set();
  peano4::topology::Tree periodicTree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0),
    periodicBoundaries
  );
  periodicTree.buildRegularGrid(Depth);

  // On the periodic grid, every canonical vertex has local cells at all adjacency entries. A
  // wrapped corner maps to its canonical vertex.
  std::size_t expectedCanonicalVertices = 0;
  for (int level = 0; level <= Depth; level++) {
    std::size_t verticesOnLevel = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      verticesOnLevel *= peano4::topology::getCellsPerAxis(level);
    }
    expectedCanonicalVertices += verticesOnLevel;
  }
  std::unordered_set<std::int64_t> canonicalCornerKeys;
  for (int cellIndex = 0; cellIndex < static_cast<int>(periodicTree.getNumberOfCells()); cellIndex++) {
    const auto& cell                = periodicTree.getCell(cellIndex);
    const int   cellsPerAxisOnLevel = peano4::topology::getCellsPerAxis(cell.level);
    for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
      tarch::la::Vector<DIMENSIONS, int> vertexLattice(0);
      std::int64_t                       cornerKey = cell.level;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        int coordinate = cell.latticeIndex(axis) + ((vertexNumber >> axis) & 1);
        if (coordinate == cellsPerAxisOnLevel) {
          coordinate = 0;
        }
        vertexLattice(axis) = coordinate;
        cornerKey           = (cornerKey << 8) | coordinate;
      }
      if (canonicalCornerKeys.insert(cornerKey).second) {
        validate(
          periodicTree.getNumberOfLocalAdjacentCellsOfVertexAt(cell.level, vertexLattice) == peano4::CellsPerVertex
        );
      }
    }
  }
  validateEquals(canonicalCornerKeys.size(), expectedCanonicalVertices);
}

void
peano4::topology::tests::TreeTest::testRefinementPlanDilation() {
  peano4::topology::Tree tree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  peano4::topology::Tree::RefinementPlan plan(1);
  plan[0].emplace_back(RequestedCell);
  tree.buildRegularGrid(Depth, 0, 1, plan);

  // A refine request flags every unrefined corner of the three cells per axis around the requested
  // cell. These are the four vertices per axis from one below the cell to two above it.
  peano4::RefinementRequest refineRequest;
  refineRequest.setRefinementControl(peano4::RefinementRequest::RefinementControl::Refine);
  refineRequest.level        = Depth;
  refineRequest.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(RequestedCell);

  validate(tree.updateRefinementPlan({refineRequest}, plan));
  validateEquals(plan.size(), 1U);
  std::size_t numberOfVerticesInBand = 1;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    numberOfVerticesInBand *= 4;
  }
  validateEquals(plan[0].size(), numberOfVerticesInBand);
  for (const auto& flaggedVertex : plan[0]) {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      validate(flaggedVertex(axis) >= RequestedCell - 1 and flaggedVertex(axis) <= RequestedCell + 2);
    }
  }
}

/**
 * A request from a fine cell also flags the coarse vertices beside the refinement transition. The
 * refined region grows by one ring.
 */
void
peano4::topology::tests::TreeTest::testRefinementPlanCoarseDilation() {
  peano4::topology::Tree tree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  peano4::topology::Tree::RefinementPlan plan(1);
  plan[0].emplace_back(RequestedCell);
  tree.buildRegularGrid(Depth, 0, 1, plan);

  // The fine cell is the first child of the coarse cell below the requested one. Its band covers this
  // coarse cell and its lower neighbour per axis and flags the ring of coarse vertices outside the
  // fine region.
  constexpr int             CoarseCell = RequestedCell - 1;
  constexpr int             CoarseRing = RequestedCell - 2;
  peano4::RefinementRequest refineRequest;
  refineRequest.setRefinementControl(peano4::RefinementRequest::RefinementControl::Refine);
  refineRequest.level        = Depth + 1;
  refineRequest.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(SubdivisionFactor * CoarseCell);

  validate(tree.updateRefinementPlan({refineRequest}, plan));
  validate(not plan.empty());

  bool hasFlaggedCoarseRing = false;
  for (const auto& flaggedVertex : plan[0]) {
    bool isOnCoarseRing = true;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      isOnCoarseRing = isOnCoarseRing and flaggedVertex(axis) == CoarseRing;
    }
    hasFlaggedCoarseRing = hasFlaggedCoarseRing or isOnCoarseRing;
  }
  validate(hasFlaggedCoarseRing);
}

/**
 * A refine request next to the periodic boundary flags as many vertices as one inside the domain.
 */
void
peano4::topology::tests::TreeTest::testRefinementPlanWrapsOnAPeriodicAxis() {
  std::bitset<DIMENSIONS> periodicBoundaries;
  periodicBoundaries.set();

  const auto getFlaggedVerticesOfRequestAt = [&periodicBoundaries](const int firstCellPosition) {
    peano4::topology::Tree tree(
      LocalRank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0),
      periodicBoundaries
    );
    tree.buildRegularGrid(Depth);

    peano4::RefinementRequest refineRequest;
    refineRequest.setRefinementControl(peano4::RefinementRequest::RefinementControl::Refine);
    refineRequest.level        = Depth;
    refineRequest.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(firstCellPosition);

    peano4::topology::Tree::RefinementPlan plan;
    tree.updateRefinementPlan({refineRequest}, plan);
    return plan.empty() ? std::size_t(0) : plan[0].size();
  };

  // The band of the centre cell does not reach the periodic boundary.
  const auto interiorFlags = getFlaggedVerticesOfRequestAt(peano4::topology::getCellsPerAxis(Depth) / 2);
  validate(interiorFlags > 0);
  validateEquals(getFlaggedVerticesOfRequestAt(0), interiorFlags);
  validateEquals(getFlaggedVerticesOfRequestAt(peano4::topology::getCellsPerAxis(Depth) - 1), interiorFlags);
}

void
peano4::topology::tests::TreeTest::testRefinementPlanErase() {
  peano4::topology::Tree tree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  peano4::topology::Tree::RefinementPlan plan(1);
  plan[0].emplace_back(RequestedCell);
  tree.buildRegularGrid(Depth, 0, 1, plan);

  // Erase requests for every cell around the flagged vertex remove its flag and the empty level
  // drops. Each request names the base-depth parent that a caller promotes an erase request to.
  std::vector<peano4::RefinementRequest> eraseRequests;
  for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
    peano4::RefinementRequest eraseRequest;
    eraseRequest.setRefinementControl(peano4::RefinementRequest::RefinementControl::Erase);
    eraseRequest.level = Depth;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      eraseRequest.latticeIndex(axis) = RequestedCell - 1 + ((adjacencyEntry >> axis) & 1);
    }
    eraseRequests.push_back(eraseRequest);
  }

  validate(tree.updateRefinementPlan(eraseRequests, plan));
  validate(plan.empty());

  std::bitset<DIMENSIONS> periodicBoundaries;
  periodicBoundaries.set(0);
  peano4::topology::Tree periodicTree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0),
    periodicBoundaries
  );
  peano4::topology::Tree::RefinementPlan periodicPlan(2);
  periodicPlan[0].emplace_back(0);
  auto fineVertexAcrossWrap = tarch::la::Vector<DIMENSIONS, int>(0);
  fineVertexAcrossWrap(0)   = SubdivisionFactor * peano4::topology::getCellsPerAxis(Depth) - 1;
  periodicPlan[1].push_back(fineVertexAcrossWrap);
  periodicTree.buildRegularGrid(Depth, 0, 1, periodicPlan);

  // The erase requests cover both cells around the coarse flag, one of them across the periodic boundary.
  std::vector<peano4::RefinementRequest> periodicEraseRequests;
  for (const int firstAxisPosition : {0, peano4::topology::getCellsPerAxis(Depth) - 1}) {
    peano4::RefinementRequest eraseRequest;
    eraseRequest.setRefinementControl(peano4::RefinementRequest::RefinementControl::Erase);
    eraseRequest.level           = Depth;
    eraseRequest.latticeIndex    = tarch::la::Vector<DIMENSIONS, int>(0);
    eraseRequest.latticeIndex(0) = firstAxisPosition;
    periodicEraseRequests.push_back(eraseRequest);
  }

  // The fine flag across the periodic boundary depends on the coarse flag. The coarse flag stays in
  // this round.
  validate(not periodicTree.updateRefinementPlan(periodicEraseRequests, periodicPlan));
  validateEquals(periodicPlan.size(), 2U);
  validateEquals(periodicPlan[0].size(), 1U);
  validateEquals(periodicPlan[1].size(), 1U);
}

/**
 * A round changes only the levels its requests touch and keeps every level sorted by lattice key
 * without duplicates.
 */
void
peano4::topology::tests::TreeTest::testRefinementPlanRoundKeepsLevelsWithoutRequests() {
  // The lattice has room for the coarse flag, the band of the refine request and an unrefined cell
  // between them. The depth depends on the subdivision factor.
  constexpr int RoundDepth          = SubdivisionFactor >= 3 ? Depth : Depth + 1;
  constexpr int RoundCellsPerAxis   = peano4::power(SubdivisionFactor, RoundDepth);
  constexpr int CoarseFlag          = RoundCellsPerAxis / 2;
  constexpr int FineFlag            = SubdivisionFactor * CoarseFlag;
  constexpr int RequestedCellOfBand = RoundCellsPerAxis - 2;
  constexpr int BandVerticesPerAxis = peano4::NeighbourhoodWidth + 1;

  const auto validateSorted = [this](const peano4::topology::Tree::RefinementPlan& sortedPlan) {
    for (const auto& planLevel : sortedPlan) {
      for (std::size_t flagNumber = 1; flagNumber < planLevel.size(); flagNumber++) {
        validate(peano4::topology::Tree::latticeKeyPrecedes(planLevel[flagNumber - 1], planLevel[flagNumber]));
      }
    }
  };

  // The coarse flag refines the two cells per axis around it. The fine flag lies on the corner
  // where the children of these cells meet.
  peano4::topology::Tree tree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  peano4::topology::Tree::RefinementPlan plan(2);
  plan[0].emplace_back(CoarseFlag);
  plan[1].emplace_back(FineFlag);
  tree.buildRegularGrid(RoundDepth, 0, 1, plan);
  const auto standingFineLevel = plan[1];

  // A refine request on the base depth covers the neighbourhood of its cell. All corners of that
  // neighbourhood lie away from the coarse flag.
  peano4::RefinementRequest refineRequest;
  refineRequest.setRefinementControl(peano4::RefinementRequest::RefinementControl::Refine);
  refineRequest.level        = RoundDepth;
  refineRequest.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(RequestedCellOfBand);
  validate(tree.updateRefinementPlan({refineRequest}, plan));
  validateEquals(plan.size(), 2U);
  validate(plan[1] == standingFineLevel);
  std::size_t numberOfBandCorners = 1;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    numberOfBandCorners *= BandVerticesPerAxis;
  }
  validateEquals(plan[0].size(), 1 + numberOfBandCorners);
  validateSorted(plan);

  // Erase requests for every fine cell around the fine flag remove it. The fine level drops and the
  // coarse level stays.
  tree.buildRegularGrid(RoundDepth, 0, 1, plan);
  const auto                             standingCoarseLevel = plan[0];
  std::vector<peano4::RefinementRequest> eraseRequests;
  for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
    peano4::RefinementRequest eraseRequest;
    eraseRequest.setRefinementControl(peano4::RefinementRequest::RefinementControl::Erase);
    eraseRequest.level = RoundDepth + 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      eraseRequest.latticeIndex(axis) = FineFlag - 1 + ((adjacencyEntry >> axis) & 1);
    }
    eraseRequests.push_back(eraseRequest);
  }
  validate(tree.updateRefinementPlan(eraseRequests, plan));
  validateEquals(plan.size(), 1U);
  validate(plan[0] == standingCoarseLevel);
  validateSorted(plan);
}

/**
 * Erase requests that leave every cell with a flagged corner on the rim coarsen no cell. Erase
 * requests for a whole neighbourhood coarsen its centre cell and remove only the corner flags of
 * that cell.
 */
void
peano4::topology::tests::TreeTest::testRefinementPlanEraseKeepsFlagsBetweenRefinedCells() {
  // The flagged block, the cells it refines and one unrefined ring around them fit into the lattice.
  // The depth depends on the subdivision factor.
  constexpr int  EraseDepth              = SubdivisionFactor >= 3 ? Depth : Depth + 1;
  constexpr int  EraseCellsPerAxis       = peano4::power(SubdivisionFactor, EraseDepth);
  constexpr int  FlaggedVerticesPerAxis  = peano4::NeighbourhoodWidth + 1;
  constexpr int  FirstFlaggedCoordinate  = EraseCellsPerAxis / 2 - 1;
  constexpr int  CentreCellOfBlock       = FirstFlaggedCoordinate + 1;
  constexpr auto NumberOfFlaggedVertices = static_cast<std::size_t>(peano4::power(FlaggedVerticesPerAxis, DIMENSIONS));

  peano4::topology::Tree tree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  peano4::topology::Tree::RefinementPlan plan(1);
  for (std::size_t vertexNumber = 0; vertexNumber < NumberOfFlaggedVertices; vertexNumber++) {
    tarch::la::Vector<DIMENSIONS, int> vertexLattice(0);
    auto                               remainingNumber = vertexNumber;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      vertexLattice(axis) = FirstFlaggedCoordinate + static_cast<int>(remainingNumber % FlaggedVerticesPerAxis);
      remainingNumber /= FlaggedVerticesPerAxis;
    }
    plan[0].push_back(vertexLattice);
  }
  tree.buildRegularGrid(EraseDepth, 0, 1, plan);

  const auto getEraseRequestsForBlock = [](const tarch::la::Vector<DIMENSIONS, int>& blockCellsPerAxis) {
    std::vector<peano4::RefinementRequest> eraseRequests;
    int                                    numberOfCells = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      numberOfCells *= blockCellsPerAxis(axis);
    }
    for (int cellNumber = 0; cellNumber < numberOfCells; cellNumber++) {
      peano4::RefinementRequest eraseRequest;
      eraseRequest.setRefinementControl(peano4::RefinementRequest::RefinementControl::Erase);
      eraseRequest.level   = EraseDepth;
      auto remainingNumber = cellNumber;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        eraseRequest.latticeIndex(axis) = FirstFlaggedCoordinate + remainingNumber % blockCellsPerAxis(axis);
        remainingNumber /= blockCellsPerAxis(axis);
      }
      eraseRequests.push_back(eraseRequest);
    }
    return eraseRequests;
  };

  // The block spans one neighbourhood along the first axis and one cell less along the other axes. The
  // vertices between its cells are candidates and every cell around them keeps a flagged corner on
  // the rim.
  tarch::la::Vector<DIMENSIONS, int> partialBlock(peano4::NeighbourhoodWidth - 1);
  partialBlock(0) = peano4::NeighbourhoodWidth;
  validate(not tree.updateRefinementPlan(getEraseRequestsForBlock(partialBlock), plan));
  validateEquals(plan.size(), 1U);
  validateEquals(plan[0].size(), NumberOfFlaggedVertices);

  const tarch::la::Vector<DIMENSIONS, int> wholeBlock(peano4::NeighbourhoodWidth);
  validate(tree.updateRefinementPlan(getEraseRequestsForBlock(wholeBlock), plan));
  validateEquals(plan.size(), 1U);
  validateEquals(plan[0].size(), NumberOfFlaggedVertices - peano4::VerticesPerCell);
  for (const auto& flaggedVertex : plan[0]) {
    bool isCornerOfCentreCell = true;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      const int coordinate = flaggedVertex(axis);
      isCornerOfCentreCell =
        isCornerOfCentreCell and (coordinate == CentreCellOfBlock or coordinate == CentreCellOfBlock + 1);
    }
    validate(not isCornerOfCentreCell);
  }
}

/**
 * A fine vertex between two refined base-depth cells is persistent in the plan and in the built tree.
 * A fine vertex towards an unrefined cell hangs.
 */
void
peano4::topology::tests::TreeTest::testVertexBetweenRefinedCellsIsPersistent() {
  // The two cells, their flags and the unrefined cell beside them lie away from the boundary.
  constexpr int FirstCell = CellsPerAxis / 2 - 1;

  peano4::topology::Tree tree(
    LocalRank,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  // The first cell refines through its lower corner and its neighbour along the first axis through its
  // upper corner. The corners between them stay without a flag.
  peano4::topology::Tree::RefinementPlan plan(1);
  plan[0].emplace_back(FirstCell);
  auto farCorner = tarch::la::Vector<DIMENSIONS, int>(FirstCell);
  farCorner(0)   = FirstCell + 2;
  plan[0].push_back(farCorner);
  tree.buildRegularGrid(Depth, 0, 1, plan);

  const int fineLevel              = Depth + 1;
  auto      sideBetweenCellsVertex = tarch::la::Vector<DIMENSIONS, int>(SubdivisionFactor * FirstCell + 1);
  sideBetweenCellsVertex(0)        = SubdivisionFactor * (FirstCell + 1);
  validate(tree.isVertexPersistentInPlan(fineLevel, sideBetweenCellsVertex));
  const int sideBetweenCellsVertexIndex = tree.getVertexAt(fineLevel, sideBetweenCellsVertex);
  validate(sideBetweenCellsVertexIndex >= 0);
  validate(tree.getVertex(sideBetweenCellsVertexIndex).getState() != peano4::grid::GridVertex::State::HangingVertex);

  // A fine vertex on the side of the first cell towards the unrefined cell along the second axis.
  auto outerSideVertex = tarch::la::Vector<DIMENSIONS, int>(SubdivisionFactor * FirstCell + 1);
  outerSideVertex(1)   = SubdivisionFactor * (FirstCell + 1);
  validate(not tree.isVertexPersistentInPlan(fineLevel, outerSideVertex));
  const int outerSideVertexIndex = tree.getVertexAt(fineLevel, outerSideVertex);
  validate(outerSideVertexIndex >= 0);
  validate(tree.getVertex(outerSideVertexIndex).getState() == peano4::grid::GridVertex::State::HangingVertex);
}
