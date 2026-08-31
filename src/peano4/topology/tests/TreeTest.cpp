// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TreeTest.h"

#include "peano4/Globals.h"
#include "peano4/grid/GridControlEvent.h"
#include "peano4/topology/Tree.h"

namespace {
  constexpr int Depth  = 2;
  constexpr int TreeId = 0;
} // namespace

peano4::topology::tests::TreeTest::TreeTest() :
  TestCase("peano4::topology::tests::TreeTest") {}

void peano4::topology::tests::TreeTest::run() {
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
  testMethod(testAppendedDeltaMatchesADirectBuild);
  testMethod(testPlanViewFromDeltaMatchesADirectBuild);
  testMethod(testBalancedOwnersFromPlan);
  testMethod(testTheBalancedPartitionKeepsBaseFamiliesWhole);
  testMethod(testRankLoadsOfADeeperOwnershipPlan);
  testMethod(testAPartitionSplittingALeafFamilyIsDetected);
  testMethod(testARankWithoutABlockMaterialisesNothingBelowTheSpawnLevel);
  testMethod(testAPartitionedBuildLinksEveryEntityOnce);
}

/**
 * A partitioned build answers every cell, vertex and face lattice lookup with
 * the index its cells link to, and no index stands for two lattice positions,
 * with and without a periodic axis.
 */
void peano4::topology::tests::TreeTest::testAPartitionedBuildLinksEveryEntityOnce() {
  constexpr int PartitionedDepth = 4;
  constexpr int SpawnLevel       = 1;
  constexpr int NumberOfRanks    = 2;

  peano4::topology::Tree::RefinementPlan refinementPlan;
  refinementPlan.emplace_back().emplace_back(5);

  const auto latticeKeyOf = [](const tarch::la::Vector<DIMENSIONS, int>& lattice) {
    std::int64_t key = 0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      key = key * 4096 + lattice(axis);
    }
    return key;
  };

  for (const bool periodic : {false, true}) {
    std::bitset<DIMENSIONS> periodicBC;
    periodicBC[0] = periodic;
    for (int rank = 0; rank < NumberOfRanks; rank++) {
      peano4::topology::Tree
        tree(rank, tarch::la::Vector<DIMENSIONS, double>(0.0), tarch::la::Vector<DIMENSIONS, double>(1.0), periodicBC);
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
            cell.latticeIndex(axis) / 3,
            periodic,
            rank,
            axis
          );
        }
        if (tree.isRefined(cellIndex)) {
          for (int childPosition = 0; childPosition < THREE_POWER_D; childPosition++) {
            validateEqualsWithParams3(
              tree.getCell(tree.getChild(cellIndex, childPosition)).parent,
              cellIndex,
              periodic,
              rank,
              childPosition
            );
          }
        }
        for (int vertexNumber = 0; vertexNumber < TWO_POWER_D; vertexNumber++) {
          const auto vertexIndex   = cell.vertexIndices[vertexNumber];
          auto       cornerLattice = cell.latticeIndex;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            cornerLattice(axis) += (vertexNumber >> axis) & 1;
            if (periodicBC[axis] and cornerLattice(axis) == cellsPerAxisOnLevel) {
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
          // A periodic wall vertex carries the lower wall's position in every block.
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
          const auto [entry, isNew] = latticeOfVertexIndex.insert({vertexIndex, latticeKeyOf(cornerLattice)});
          validateEqualsWithParams3(entry->second, latticeKeyOf(cornerLattice), periodic, rank, vertexIndex);
          static_cast<void>(isNew);
        }
        for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
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
          if (periodicBC[normal] and faceLattice(normal) == cellsPerAxisOnLevel) {
            faceLattice(normal) = 0;
          }
          faceLattice(normal) += normal * 4096 * 4096;
          const auto [entry, isNew] = latticeOfFaceIndex.insert({faceIndex, latticeKeyOf(faceLattice)});
          validateEqualsWithParams3(entry->second, latticeKeyOf(faceLattice), periodic, rank, faceIndex);
          static_cast<void>(isNew);
        }
      }
      validate(numberOfBlockLevelCells > 0);
    }
  }
}

/**
 * A rank the ownership plan hands no block materialises nothing below the
 * spawn level.
 */
void peano4::topology::tests::TreeTest::testARankWithoutABlockMaterialisesNothingBelowTheSpawnLevel() {
  constexpr int SpawnLevel              = 1;
  constexpr int OwningRankId            = 0;
  constexpr int NumberOfRanks           = 4096;
  constexpr int OwnershipPlanHeaderSize = 2;
  constexpr int TopologyNodeStride      = 3;

  peano4::topology::Tree::RefinementPlan refinementPlan;
  refinementPlan.emplace_back().emplace_back(3);

  const peano4::topology::Tree planningTree(
    OwningRankId,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  const auto ownershipPlan = planningTree.deriveBalancedBlockOwners(Depth, NumberOfRanks, refinementPlan);

  // Far more ranks than the plan can hand work to, so the ranks above the
  // highest owner it names hold nothing at all.
  int highestOwner = -1;
  for (int nodeIndex = 0; nodeIndex < ownershipPlan[1]; nodeIndex++) {
    const auto planOffset = OwnershipPlanHeaderSize + nodeIndex * TopologyNodeStride;
    highestOwner          = std::max(highestOwner, ownershipPlan[planOffset + 1]);
  }
  validate(highestOwner >= 0 and highestOwner < NumberOfRanks - 1);
  const int rankWithoutABlock = highestOwner + 1;

  const auto numberOfCellsOfRank = [&](const int treeId) {
    peano4::topology::Tree tree(
      treeId,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0)
    );
    tree.buildRegularGrid(Depth, SpawnLevel, NumberOfRanks, refinementPlan, &ownershipPlan);
    return tree.getNumberOfCells();
  };

  // The spawn level and everything above it is dense on every rank; a rank
  // without a block adds nothing below it.
  std::size_t cellsToTheSpawnLevel = 0;
  for (int level = 0; level <= SpawnLevel; level++) {
    std::size_t cellsOnLevel = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellsOnLevel *= peano4::topology::cellsPerAxis(level);
    }
    cellsToTheSpawnLevel += cellsOnLevel;
  }

  validateEquals(numberOfCellsOfRank(rankWithoutABlock), cellsToTheSpawnLevel);
  validate(numberOfCellsOfRank(OwningRankId) > cellsToTheSpawnLevel);
}

/**
 * The derived ownership plan is deterministic across trees, every topology node
 * names a rank range inside the rank count and covers its children's ranges,
 * and the built tree answers the same ownership from its cells.
 */
void peano4::topology::tests::TreeTest::testBalancedOwnersFromPlan() {
  constexpr int SpawnLevel              = 1;
  constexpr int NumberOfRanks           = 4;
  constexpr int OwnershipPlanHeaderSize = 2;
  constexpr int TopologyNodeStride      = 3;

  peano4::topology::Tree::RefinementPlan refinementPlan;
  refinementPlan.emplace_back().emplace_back(3);

  peano4::topology::Tree tree(
    TreeId,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  const auto ownershipPlan = tree.deriveBalancedBlockOwners(Depth, NumberOfRanks, refinementPlan);

  const peano4::topology::Tree replicaTree(
    TreeId + 1,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  validate(ownershipPlan == replicaTree.deriveBalancedBlockOwners(Depth, NumberOfRanks, refinementPlan));

  // Every leaf of the plan's structure belongs to one rank: The regular grid
  // plus the 2^d cells around the flagged vertex, each replaced by 3^d
  // children.
  std::int64_t expectedLeaves = 1;
  for (int level = 0; level < Depth; level++) {
    expectedLeaves *= THREE_POWER_D;
  }
  expectedLeaves += static_cast<std::int64_t>(TWO_POWER_D) * (THREE_POWER_D - 1);
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
    // A node carries children only where a rank boundary crosses it, so its
    // range spans more than one rank and covers every child's range.
    validate(firstOwner < lastOwner);
    validate(childrenBlock > nodeIndex and childrenBlock + THREE_POWER_D <= numberOfTopologyNodes);
    for (int childPosition = 0; childPosition < THREE_POWER_D; childPosition++) {
      const auto childOffset = OwnershipPlanHeaderSize + (childrenBlock + childPosition) * TopologyNodeStride;
      validate(ownershipPlan[childOffset] >= firstOwner and ownershipPlan[childOffset + 1] <= lastOwner);
    }
  }

  tree.buildRegularGrid(Depth, SpawnLevel, NumberOfRanks, refinementPlan, &ownershipPlan);

  // The tree materialises its owned spawn blocks plus one ring of
  // neighbours, so the built structure holds a subset of the domain's
  // leaves. A parent precedes its children in the cell array, so one backward
  // pass gathers the ranks owning a materialised leaf below every cell.
  std::vector<std::bitset<NumberOfRanks>> ranksBelowCell(tree.getNumberOfCells());
  for (int cellIndex = static_cast<int>(tree.getNumberOfCells()) - 1; cellIndex >= 0; cellIndex--) {
    if (tree.isRefined(cellIndex)) {
      for (int childPosition = 0; childPosition < THREE_POWER_D; childPosition++) {
        ranksBelowCell[cellIndex] |= ranksBelowCell[tree.getChild(cellIndex, childPosition)];
      }
      continue;
    }
    const auto owner = tree.getCell(cellIndex).ownerTreeId;
    validate(owner >= 0 and owner < NumberOfRanks);
    ranksBelowCell[cellIndex].set(owner);
  }
  // The tree's own leaves always materialise.
  validate(ranksBelowCell[peano4::topology::Tree::RootCell].test(TreeId));

  for (int cellIndex = 0; cellIndex < static_cast<int>(tree.getNumberOfCells()); cellIndex++) {
    const auto& cell  = tree.getCell(cellIndex);
    const auto& ranks = ranksBelowCell[cellIndex];
    // Every materialised leaf's owner lies inside its ancestors' ownership
    // intervals, and a cell's owner is its interval's first rank whether or
    // not that rank's leaves materialise here.
    for (int rank = 0; rank < NumberOfRanks; rank++) {
      if (ranks.test(rank)) {
        validate(tree.subtreeTouchesRank(cellIndex, rank));
      }
      validateEquals(
        tree.subtreeTouchesRankAt(cell.level, cell.latticeIndex, rank),
        tree.subtreeTouchesRank(cellIndex, rank)
      );
    }
    validateEquals(tree.getOwnerOfCellAt(cell.level, cell.latticeIndex), cell.ownerTreeId);
  }
}

namespace {
  constexpr int ManyRanks = 4096;

  /**
   * Flags every vertex of one cell's children, so the cell's children are all
   * refined and the partition descends through it.
   */
  void flagVerticesOfCellChildren(std::vector<tarch::la::Vector<DIMENSIONS, int>>& flags, const int firstCoordinate) {
    constexpr int VerticesPerAxis  = 4;
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

  /** Returns a plan that refines one cell of every level below the base depth in full, three levels deep. */
  peano4::topology::Tree::RefinementPlan threeLevelPlan() {
    peano4::topology::Tree::RefinementPlan deepPlan(3);
    flagVerticesOfCellChildren(deepPlan[0], 0);
    flagVerticesOfCellChildren(deepPlan[1], 6);
    flagVerticesOfCellChildren(deepPlan[2], 21);
    return deepPlan;
  }
} // namespace

/**
 * An ownership plan derived from a plan without leaf families splits no cell
 * with a leaf child, and it does so under a shallower plan whose leaves it
 * splits.
 */
void peano4::topology::tests::TreeTest::testAPartitionSplittingALeafFamilyIsDetected() {
  const auto                                   deepPlan = threeLevelPlan();
  const peano4::topology::Tree::RefinementPlan shallowPlan(deepPlan.begin(), deepPlan.begin() + 1);
  const peano4::topology::Tree                 tree(
    TreeId,
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
 * The balanced partition of a regular grid hands every rank whole families of
 * base-depth leaves below the spawn level.
 */
void peano4::topology::tests::TreeTest::testTheBalancedPartitionKeepsBaseFamiliesWhole() {
  constexpr int DeepDepth = 3;

  const peano4::topology::Tree::RefinementPlan emptyPlan;
  const peano4::topology::Tree                 tree(
    TreeId,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  for (const int numberOfRanks : {2, 4}) {
    const int spawnLevel = peano4::topology::Tree::deriveSpawnLevel(DeepDepth, numberOfRanks);
    validateEqualsWithParams1(spawnLevel, 1, numberOfRanks);
    const auto ownershipPlan = tree.deriveBalancedBlockOwners(DeepDepth, numberOfRanks, emptyPlan);
    validateFalse(tree.ownershipPlanSplitsCellWithLeafChild(DeepDepth, spawnLevel, emptyPlan, ownershipPlan));
    for (const auto load : tree.deriveRankLoads(DeepDepth, numberOfRanks, emptyPlan, ownershipPlan)) {
      validateEqualsWithParams1(load % THREE_POWER_D, 0, numberOfRanks);
    }
  }
}

/**
 * An ownership plan derived from a deeper refinement plan counts a cell it
 * splits and the shallower plan keeps as a leaf for the split's first rank.
 */
void peano4::topology::tests::TreeTest::testRankLoadsOfADeeperOwnershipPlan() {
  const auto                                   deepPlan = threeLevelPlan();
  const peano4::topology::Tree::RefinementPlan shallowPlan(deepPlan.begin(), deepPlan.begin() + 1);

  const peano4::topology::Tree tree(
    TreeId,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  const auto deepOwnershipPlan = tree.deriveBalancedBlockOwners(Depth, ManyRanks, deepPlan);
  // Every cell up to the base depth splits, and below it the two cells whose
  // children are all refined.
  int expectedTopologyNodes = 2 * THREE_POWER_D;
  int nodesOnLevel          = 1;
  for (int level = 0; level <= Depth; level++) {
    expectedTopologyNodes += nodesOnLevel;
    nodesOnLevel *= THREE_POWER_D;
  }
  validateEquals(deepOwnershipPlan[1], expectedTopologyNodes);

  // The flags of the shallow plan refine the 4^d cells around them.
  std::int64_t shallowLeaves       = 1;
  std::int64_t refinedShallowCells = 1;
  for (int level = 0; level < Depth; level++) {
    shallowLeaves *= THREE_POWER_D;
  }
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    refinedShallowCells *= 4;
  }
  shallowLeaves += refinedShallowCells * (THREE_POWER_D - 1);
  const auto   loads     = tree.deriveRankLoads(Depth, ManyRanks, shallowPlan, deepOwnershipPlan);
  std::int64_t totalLoad = 0;
  for (const auto load : loads) {
    validate(load >= 0);
    totalLoad += load;
  }
  validateEquals(totalLoad, shallowLeaves);
}

void peano4::topology::tests::TreeTest::testRegularGridSizes() {
  peano4::topology::Tree tree(
    TreeId,
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
      cellsOnLevel *= peano4::topology::cellsPerAxis(level);
      verticesOnLevel *= peano4::topology::cellsPerAxis(level) + 1;
    }
    expectedCells += cellsOnLevel;
    expectedVertices += verticesOnLevel;
  }

  validateEquals(tree.getNumberOfCells(), expectedCells);
  validateEquals(tree.getNumberOfVertices(), expectedVertices);
  validateEquals(tree.getDepth(), Depth);
}

/**
 * Every dense cell answers getCellAt() with its own index, on every level
 * and with a periodic axis.
 */
void peano4::topology::tests::TreeTest::testDenseCellLookupMatchesTheLattice() {
  constexpr int LookupDepth = 4;
  for (const bool periodic : {false, true}) {
    std::bitset<DIMENSIONS> periodicBC;
    periodicBC[0] = periodic;
    peano4::topology::Tree
      tree(TreeId, tarch::la::Vector<DIMENSIONS, double>(0.0), tarch::la::Vector<DIMENSIONS, double>(1.0), periodicBC);
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

void peano4::topology::tests::TreeTest::testCellLinks() {
  peano4::topology::Tree tree(
    TreeId,
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
    validateEquals(cell.ownerTreeId, TreeId);
    validateEquals(tree.isRefined(cellIndex), (cell.level < Depth));
    if (not tree.isRefined(cellIndex)) {
      continue;
    }
    for (int childPosition = 0; childPosition < THREE_POWER_D; childPosition++) {
      const auto  childIndex = tree.getChild(cellIndex, childPosition);
      const auto& child      = tree.getCell(childIndex);
      validateEquals(child.parent, cellIndex);
      validateEquals(child.level, cell.level + 1);
      int positionRemainder = childPosition;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        validateEquals(child.latticeIndex(axis), 3 * cell.latticeIndex(axis) + positionRemainder % 3);
        positionRemainder /= 3;
      }
    }
  }

  constexpr int                          DeepestSparseLevel = 12;
  peano4::topology::Tree::RefinementPlan deepRefinementPlan(DeepestSparseLevel);
  for (int level = 0; level < DeepestSparseLevel; level++) {
    deepRefinementPlan[level].emplace_back(peano4::topology::cellsPerAxis(level));
  }
  peano4::topology::Tree deepTree(
    TreeId,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  deepTree.buildRegularGrid(0, 0, 1, deepRefinementPlan);

  // Keeping one sparse map per normal preserves all lattice bits at depths
  // where a combined face key would need more than 64 bits in three dimensions.
  const int                   cellsPerAxisOnDeepestLevel = peano4::topology::cellsPerAxis(DeepestSparseLevel);
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

void peano4::topology::tests::TreeTest::testAdjacentCellsReferenceOneVertex() {
  peano4::topology::Tree tree(
    TreeId,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  tree.buildRegularGrid(Depth);

  // A cell and its neighbour along axis 0 reference the vertices of the face
  // between them through the same vertex indices.
  const auto  leftCellIndex  = tree.getChild(peano4::topology::Tree::RootCell, 0);
  const auto  rightCellIndex = tree.getChild(peano4::topology::Tree::RootCell, 1);
  const auto& leftCell       = tree.getCell(leftCellIndex);
  const auto& rightCell      = tree.getCell(rightCellIndex);

  for (int vertexNumber = 0; vertexNumber < TWO_POWER_D; vertexNumber++) {
    if ((vertexNumber & 1) == 1) {
      validateEquals(leftCell.vertexIndices[vertexNumber], rightCell.vertexIndices[vertexNumber ^ 1]);
    }
  }
}

void peano4::topology::tests::TreeTest::testVertexContent() {
  peano4::topology::Tree tree(
    TreeId,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  tree.buildRegularGrid(Depth);

  for (int cellIndex = 0; cellIndex < static_cast<int>(tree.getNumberOfCells()); cellIndex++) {
    const auto& cell                = tree.getCell(cellIndex);
    const int   cellsPerAxisOnLevel = peano4::topology::cellsPerAxis(cell.level);
    const bool  isFinestLevel       = cell.level == Depth;

    for (int vertexNumber = 0; vertexNumber < TWO_POWER_D; vertexNumber++) {
      const auto& vertex = tree.getVertex(cell.vertexIndices[vertexNumber]);
      validateEquals(vertex.level, cell.level);
      validate(
        vertex.getState()
        == (isFinestLevel ? peano4::grid::GridVertex::State::Unrefined : peano4::grid::GridVertex::State::Refined)
      );

      // The vertex answers one owner per surrounding cell position; positions
      // outside the domain carry no owner.
      tarch::la::Vector<DIMENSIONS, int> vertexLattice(0);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        vertexLattice(axis) = cell.latticeIndex(axis) + ((vertexNumber >> axis) & 1);
      }
      for (int adjacencyEntry = 0; adjacencyEntry < TWO_POWER_D; adjacencyEntry++) {
        bool isInsideDomain = true;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          const int adjacentCellCoordinate = vertexLattice(axis) - 1 + ((adjacencyEntry >> axis) & 1);
          isInsideDomain =
            isInsideDomain and adjacentCellCoordinate >= 0 and adjacentCellCoordinate < cellsPerAxisOnLevel;
        }
        validateEquals(
          tree.getAdjacentCellOwnerAt(cell.level, vertexLattice, adjacencyEntry),
          (isInsideDomain ? TreeId : -1)
        );
      }
    }
  }

  std::bitset<DIMENSIONS> periodicBC;
  periodicBC.set();
  peano4::topology::Tree periodicTree(
    TreeId,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0),
    periodicBC
  );
  periodicTree.buildRegularGrid(Depth);

  // Every canonical vertex of the wrapped grid sees local cells all around;
  // the wrapped duplicates alias them by lattice position.
  std::size_t expectedCanonicalVertices = 0;
  for (int level = 0; level <= Depth; level++) {
    std::size_t verticesOnLevel = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      verticesOnLevel *= peano4::topology::cellsPerAxis(level);
    }
    expectedCanonicalVertices += verticesOnLevel;
  }
  std::unordered_set<std::int64_t> canonicalCornerKeys;
  for (int cellIndex = 0; cellIndex < static_cast<int>(periodicTree.getNumberOfCells()); cellIndex++) {
    const auto& cell                = periodicTree.getCell(cellIndex);
    const int   cellsPerAxisOnLevel = peano4::topology::cellsPerAxis(cell.level);
    for (int vertexNumber = 0; vertexNumber < TWO_POWER_D; vertexNumber++) {
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
        validate(periodicTree.getNumberOfLocalAdjacentCellsOfVertexAt(cell.level, vertexLattice) == TWO_POWER_D);
      }
    }
  }
  validateEquals(canonicalCornerKeys.size(), expectedCanonicalVertices);
}

void peano4::topology::tests::TreeTest::testRefinementPlanDilation() {
  peano4::topology::Tree tree(
    TreeId,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  peano4::topology::Tree::RefinementPlan plan(1);
  plan[0].emplace_back(4);
  tree.buildRegularGrid(Depth, 0, 1, plan);

  // A refine request from the cell at lattice (4,...,4) covers that cell and one full
  // cell of every neighbour, so every still-unrefined corner of the block [3,5] per
  // axis is flagged. Those are the vertices [3,6] per axis, all but the one the plan
  // already refines.
  peano4::grid::GridControlEvent refineRequest;
  refineRequest.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Refine);
  refineRequest.level        = Depth;
  refineRequest.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(4);

  validate(tree.updateRefinementPlan({refineRequest}, plan));
  validateEquals(plan.size(), 1U);
  std::size_t numberOfVerticesInBand = 1;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    numberOfVerticesInBand *= 4;
  }
  validateEquals(plan[0].size(), numberOfVerticesInBand);
  for (const auto& flaggedVertex : plan[0]) {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      validate(flaggedVertex(axis) >= 3 and flaggedVertex(axis) <= 6);
    }
  }
}

/**
 * A request from a fine cell also reaches the coarse leaves beside the refinement
 * transition, so the refined region dilates outward by a ring.
 */
void peano4::topology::tests::TreeTest::testRefinementPlanCoarseDilation() {
  peano4::topology::Tree tree(
    TreeId,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  peano4::topology::Tree::RefinementPlan plan(1);
  plan[0].emplace_back(4);
  tree.buildRegularGrid(Depth, 0, 1, plan);

  // The fine cell at lattice (9,...,9) is the first child of the coarse cell
  // (3,...,3). Its band covers the coarse cells [2,3] per axis, so the coarse
  // vertices [2,4] per axis are flagged: The ring outside the fine region.
  peano4::grid::GridControlEvent refineRequest;
  refineRequest.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Refine);
  refineRequest.level        = Depth + 1;
  refineRequest.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(9);

  validate(tree.updateRefinementPlan({refineRequest}, plan));
  validate(not plan.empty());

  bool hasFlaggedCoarseRing = false;
  for (const auto& flaggedVertex : plan[0]) {
    bool isOnCoarseRing = true;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      isOnCoarseRing = isOnCoarseRing and flaggedVertex(axis) == 2;
    }
    hasFlaggedCoarseRing = hasFlaggedCoarseRing or isOnCoarseRing;
  }
  validate(hasFlaggedCoarseRing);
}

/**
 * A refine request raised beside a periodic wrap flags as many vertices as the
 * same request raised in the interior.
 */
void peano4::topology::tests::TreeTest::testRefinementPlanWrapsOnAPeriodicAxis() {
  std::bitset<DIMENSIONS> periodicBC;
  periodicBC.set();

  const auto flaggedVerticesOfRequestAt = [&periodicBC](const int firstCellPosition) {
    peano4::topology::Tree
      tree(TreeId, tarch::la::Vector<DIMENSIONS, double>(0.0), tarch::la::Vector<DIMENSIONS, double>(1.0), periodicBC);
    tree.buildRegularGrid(Depth);

    peano4::grid::GridControlEvent refineRequest;
    refineRequest.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Refine);
    refineRequest.level        = Depth;
    refineRequest.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(firstCellPosition);

    peano4::topology::Tree::RefinementPlan plan;
    tree.updateRefinementPlan({refineRequest}, plan);
    return plan.empty() ? std::size_t(0) : plan[0].size();
  };

  // The centre of the lattice is as far from the wrap as a cell gets, so its
  // band never reaches it.
  const auto interiorFlags = flaggedVerticesOfRequestAt(peano4::topology::cellsPerAxis(Depth) / 2);
  validate(interiorFlags > 0);
  validateEquals(flaggedVerticesOfRequestAt(0), interiorFlags);
  validateEquals(flaggedVerticesOfRequestAt(peano4::topology::cellsPerAxis(Depth) - 1), interiorFlags);
}

void peano4::topology::tests::TreeTest::testRefinementPlanErase() {
  peano4::topology::Tree tree(
    TreeId,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  peano4::topology::Tree::RefinementPlan plan(1);
  plan[0].emplace_back(4);
  tree.buildRegularGrid(Depth, 0, 1, plan);

  // Erase requests naming every cell adjacent to the flagged vertex unflag it, and the
  // trailing empty level drops from the plan. The requests are the parent cells the
  // consensus has already promoted to, so each names one whole cell of the base depth.
  std::vector<peano4::grid::GridControlEvent> eraseRequests;
  for (int adjacencyEntry = 0; adjacencyEntry < TWO_POWER_D; adjacencyEntry++) {
    peano4::grid::GridControlEvent eraseRequest;
    eraseRequest.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Erase);
    eraseRequest.level = Depth;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      eraseRequest.latticeIndex(axis) = 3 + ((adjacencyEntry >> axis) & 1);
    }
    eraseRequests.push_back(eraseRequest);
  }

  validate(tree.updateRefinementPlan(eraseRequests, plan));
  validate(plan.empty());

  std::bitset<DIMENSIONS> periodicBC;
  periodicBC.set(0);
  peano4::topology::Tree periodicTree(
    TreeId,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0),
    periodicBC
  );
  peano4::topology::Tree::RefinementPlan periodicPlan(2);
  periodicPlan[0].emplace_back(0);
  auto fineVertexAcrossWrap = tarch::la::Vector<DIMENSIONS, int>(0);
  fineVertexAcrossWrap(0)   = 3 * peano4::topology::cellsPerAxis(Depth) - 1;
  periodicPlan[1].push_back(fineVertexAcrossWrap);
  periodicTree.buildRegularGrid(Depth, 0, 1, periodicPlan);

  // Both cells the coarse flag sits between want to be erased, one of them reached
  // across the periodic wrap.
  std::vector<peano4::grid::GridControlEvent> periodicEraseRequests;
  for (const int firstAxisPosition : {0, peano4::topology::cellsPerAxis(Depth) - 1}) {
    peano4::grid::GridControlEvent eraseRequest;
    eraseRequest.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Erase);
    eraseRequest.level           = Depth;
    eraseRequest.latticeIndex    = tarch::la::Vector<DIMENSIONS, int>(0);
    eraseRequest.latticeIndex(0) = firstAxisPosition;
    periodicEraseRequests.push_back(eraseRequest);
  }

  // The fine flag across the periodic wrap still depends on the coarse flag,
  // so the coarse level cannot disappear in this erase round.
  validate(not periodicTree.updateRefinementPlan(periodicEraseRequests, periodicPlan));
  validateEquals(periodicPlan.size(), 2U);
  validateEquals(periodicPlan[0].size(), 1U);
  validateEquals(periodicPlan[1].size(), 1U);
}

/**
 * A round rewrites only the levels its requests reach: A refine request on
 * the base depth flags the corners of its band on that level and leaves the
 * finer level as it stands, and an erase request on the finer level removes
 * that level's last flag and drops the level. Every level stays sorted by
 * lattice key without a duplicate.
 */
void peano4::topology::tests::TreeTest::testRefinementPlanRoundKeepsLevelsWithoutRequests() {
  const auto validateSorted = [this](const peano4::topology::Tree::RefinementPlan& sortedPlan) {
    for (const auto& planLevel : sortedPlan) {
      for (std::size_t flagNumber = 1; flagNumber < planLevel.size(); flagNumber++) {
        validate(peano4::topology::Tree::latticeKeyPrecedes(planLevel[flagNumber - 1], planLevel[flagNumber]));
      }
    }
  };

  // The coarse flag at (4, ...) refines the cells with corners in [3, 4] per
  // axis, and the fine flag at (12, ...) sits inside that region.
  peano4::topology::Tree tree(
    TreeId,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  peano4::topology::Tree::RefinementPlan plan(2);
  plan[0].emplace_back(4);
  plan[1].emplace_back(12);
  tree.buildRegularGrid(Depth, 0, 1, plan);
  const auto standingFineLevel = plan[1];

  // A refine request at the cell (7, ...) of the base depth covers the band
  // [6, 8] per axis, whose 4^d corners all lie away from the coarse flag.
  peano4::grid::GridControlEvent refineRequest;
  refineRequest.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Refine);
  refineRequest.level        = Depth;
  refineRequest.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(7);
  validate(tree.updateRefinementPlan({refineRequest}, plan));
  validateEquals(plan.size(), 2U);
  validate(plan[1] == standingFineLevel);
  std::size_t numberOfBandCorners = 1;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    numberOfBandCorners *= 4;
  }
  validateEquals(plan[0].size(), 1 + numberOfBandCorners);
  validateSorted(plan);

  // Erase requests naming every fine cell around the fine flag remove it, so
  // the fine level drops and the coarse level stays as it is.
  tree.buildRegularGrid(Depth, 0, 1, plan);
  const auto                                  standingCoarseLevel = plan[0];
  std::vector<peano4::grid::GridControlEvent> eraseRequests;
  for (int adjacencyEntry = 0; adjacencyEntry < TWO_POWER_D; adjacencyEntry++) {
    peano4::grid::GridControlEvent eraseRequest;
    eraseRequest.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Erase);
    eraseRequest.level = Depth + 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      eraseRequest.latticeIndex(axis) = 11 + ((adjacencyEntry >> axis) & 1);
    }
    eraseRequests.push_back(eraseRequest);
  }
  validate(tree.updateRefinementPlan(eraseRequests, plan));
  validateEquals(plan.size(), 1U);
  validate(plan[0] == standingCoarseLevel);
  validateSorted(plan);
}

/**
 * Flags on every vertex with coordinates in [3, 6] refine every cell with
 * coordinates in [2, 6]. An erase block whose every cell keeps a flagged
 * corner on the block's rim coarsens no cell, so the block's interior flags
 * stay. A 3^d block coarsens its centre cell, and exactly that cell's corner
 * flags go.
 */
void peano4::topology::tests::TreeTest::testRefinementPlanEraseKeepsFlagsBetweenRefinedCells() {
  constexpr int  FirstFlaggedCoordinate  = 3;
  constexpr int  FlaggedVerticesPerAxis  = 4;
  constexpr auto NumberOfFlaggedVertices = static_cast<std::size_t>(1 << (2 * DIMENSIONS));

  peano4::topology::Tree tree(
    TreeId,
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
  tree.buildRegularGrid(Depth, 0, 1, plan);

  const auto eraseRequestsForBlock = [](const tarch::la::Vector<DIMENSIONS, int>& cellsPerAxis) {
    std::vector<peano4::grid::GridControlEvent> eraseRequests;
    int                                         numberOfCells = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      numberOfCells *= cellsPerAxis(axis);
    }
    for (int cellNumber = 0; cellNumber < numberOfCells; cellNumber++) {
      peano4::grid::GridControlEvent eraseRequest;
      eraseRequest.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Erase);
      eraseRequest.level   = Depth;
      auto remainingNumber = cellNumber;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        eraseRequest.latticeIndex(axis) = FirstFlaggedCoordinate + remainingNumber % cellsPerAxis(axis);
        remainingNumber /= cellsPerAxis(axis);
      }
      eraseRequests.push_back(eraseRequest);
    }
    return eraseRequests;
  };

  // Three cells along the first axis and two along every other one: The
  // vertices between the block's cells are candidates, and every cell around
  // them keeps a flagged corner on the rim.
  tarch::la::Vector<DIMENSIONS, int> partialBlock(2);
  partialBlock(0) = 3;
  validate(not tree.updateRefinementPlan(eraseRequestsForBlock(partialBlock), plan));
  validateEquals(plan.size(), 1U);
  validateEquals(plan[0].size(), NumberOfFlaggedVertices);

  validate(tree.updateRefinementPlan(eraseRequestsForBlock(tarch::la::Vector<DIMENSIONS, int>(3)), plan));
  validateEquals(plan.size(), 1U);
  validateEquals(plan[0].size(), NumberOfFlaggedVertices - TWO_POWER_D);
  for (const auto& flaggedVertex : plan[0]) {
    bool isCornerOfCentreCell = true;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      isCornerOfCentreCell = isCornerOfCentreCell and (flaggedVertex(axis) == 4 or flaggedVertex(axis) == 5);
    }
    validate(not isCornerOfCentreCell);
  }
}

/**
 * Two neighbouring cells of the base depth refine through one far corner
 * each, so the corners on the side between them carry no flag. The fine
 * vertices on that side have a cell at every neighbour position and are
 * persistent, in the plan query and in the built structure; a fine vertex on a
 * side towards an unrefined cell hangs.
 */
void peano4::topology::tests::TreeTest::testVertexBetweenRefinedCellsIsPersistent() {
  peano4::topology::Tree tree(
    TreeId,
    tarch::la::Vector<DIMENSIONS, double>(0.0),
    tarch::la::Vector<DIMENSIONS, double>(1.0)
  );
  // The cell with corners in [3, 4] per axis refines through corner (3, ...)
  // and its first-axis neighbour with corners in [4, 5] through corner (5, ...).
  peano4::topology::Tree::RefinementPlan plan(1);
  plan[0].emplace_back(3);
  auto farCorner = tarch::la::Vector<DIMENSIONS, int>(3);
  farCorner(0)   = 5;
  plan[0].push_back(farCorner);
  tree.buildRegularGrid(Depth, 0, 1, plan);

  const int fineLevel = Depth + 1;
  // A fine vertex in the middle of the side between the two cells at
  // first-axis position 4.
  auto sideBetweenCellsVertex = tarch::la::Vector<DIMENSIONS, int>(10);
  sideBetweenCellsVertex(0)   = 12;
  validate(tree.isVertexPersistentInPlan(fineLevel, sideBetweenCellsVertex));
  const int sideBetweenCellsVertexIndex = tree.getVertexAt(fineLevel, sideBetweenCellsVertex);
  validate(sideBetweenCellsVertexIndex >= 0);
  validate(tree.getVertex(sideBetweenCellsVertexIndex).getState() != peano4::grid::GridVertex::State::HangingVertex);

  // A fine vertex in the middle of the first cell's side towards the unrefined
  // cell at second-axis position 4.
  auto outerSideVertex = tarch::la::Vector<DIMENSIONS, int>(10);
  outerSideVertex(1)   = 12;
  validate(not tree.isVertexPersistentInPlan(fineLevel, outerSideVertex));
  const int outerSideVertexIndex = tree.getVertexAt(fineLevel, outerSideVertex);
  validate(outerSideVertexIndex >= 0);
  validate(tree.getVertex(outerSideVertexIndex).getState() == peano4::grid::GridVertex::State::HangingVertex);
}

/**
 * A tree built for a plan and then appended to the plan's dilation through a
 * plan-query view holds every cell of a tree built for the dilated plan
 * directly, with the same vertex state at every corner.
 */
void peano4::topology::tests::TreeTest::testAppendedDeltaMatchesADirectBuild() {
  const tarch::la::Vector<DIMENSIONS, double> offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> width(1.0);
  peano4::topology::Tree                      tree(TreeId, offset, width);
  peano4::topology::Tree::RefinementPlan      plan(1);
  plan[0].emplace_back(4);
  tree.buildRegularGrid(Depth, 0, 1, plan);
  const auto numberOfCellsBefore = tree.getNumberOfCells();

  peano4::grid::GridControlEvent refineRequest;
  refineRequest.setRefinementControl(peano4::grid::GridControlEvent::RefinementControl::Refine);
  refineRequest.level        = Depth;
  refineRequest.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(4);
  auto dilatedPlan           = plan;
  validate(tree.updateRefinementPlan({refineRequest}, dilatedPlan));
  validate(dilatedPlan[0].size() > plan[0].size());
  peano4::topology::Tree::RefinementPlan addedFlags(1);
  for (const auto& flaggedVertex : dilatedPlan[0]) {
    if (peano4::topology::Tree::latticeKeyPrecedes(plan[0][0], flaggedVertex)
        or peano4::topology::Tree::latticeKeyPrecedes(flaggedVertex, plan[0][0])) {
      addedFlags[0].push_back(flaggedVertex);
    }
  }
  validateEquals(addedFlags[0].size(), dilatedPlan[0].size() - 1);

  peano4::topology::Tree view(TreeId, offset, width);
  view.buildPlanQueryTables(Depth, dilatedPlan);
  const auto extent = tree.appendRefinementDelta(view, dilatedPlan, addedFlags);
  validateEquals(static_cast<std::size_t>(extent.firstAppendedCell), numberOfCellsBefore);

  peano4::topology::Tree direct(TreeId, offset, width);
  direct.buildRegularGrid(Depth, 0, 1, dilatedPlan);
  validateEquals(tree.getNumberOfCells(), direct.getNumberOfCells());
  validateEquals(tree.getNumberOfVertices(), direct.getNumberOfVertices());
  validateEquals(tree.getNumberOfFaces(), direct.getNumberOfFaces());
  for (std::size_t directCellIndex = 0; directCellIndex < direct.getNumberOfCells(); directCellIndex++) {
    const auto& directCell = direct.getCell(static_cast<int>(directCellIndex));
    const int   cellIndex  = tree.getCellAt(directCell.level, directCell.latticeIndex);
    validateWithParams2(cellIndex >= 0, directCell.level, directCell.latticeIndex);
    const bool treeCellIsLeaf   = tree.getCell(cellIndex).childrenBlock < 0;
    const bool directCellIsLeaf = directCell.childrenBlock < 0;
    validateEqualsWithParams2(treeCellIsLeaf, directCellIsLeaf, directCell.level, directCell.latticeIndex);
    for (int vertexNumber = 0; vertexNumber < TWO_POWER_D; vertexNumber++) {
      auto cornerLattice = directCell.latticeIndex;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        cornerLattice(axis) += (vertexNumber >> axis) & 1;
      }
      const int vertexIndex = tree.getVertexAt(directCell.level, cornerLattice);
      validateWithParams2(vertexIndex >= 0, directCell.level, cornerLattice);
      const auto expectedState = direct.getVertex(directCell.vertexIndices[vertexNumber]).getState();
      validateEqualsWithParams2(
        static_cast<int>(tree.getVertex(vertexIndex).getState()),
        static_cast<int>(expectedState),
        directCell.level,
        cornerLattice
      );
    }
  }
}

/**
 * A plan-query view derived from a standing structure and a delta answers every
 * refinement, existence and persistence query like the view built from the new
 * plan alone.
 */
void peano4::topology::tests::TreeTest::testPlanViewFromDeltaMatchesADirectBuild() {
  const tarch::la::Vector<DIMENSIONS, double> offset(0.0);
  const tarch::la::Vector<DIMENSIONS, double> width(1.0);

  peano4::topology::Tree::RefinementPlan standingPlan(2);
  standingPlan[0].emplace_back(3);
  standingPlan[0].emplace_back(4);
  standingPlan[1].emplace_back(7);
  standingPlan[1].emplace_back(10);
  peano4::topology::Tree::RefinementPlan newPlan(2);
  newPlan[0].emplace_back(4);
  newPlan[0].emplace_back(5);
  newPlan[1].emplace_back(7);
  newPlan[1].emplace_back(13);
  peano4::topology::Tree::RefinementPlan addedFlags(2);
  addedFlags[0].emplace_back(5);
  addedFlags[1].emplace_back(13);
  peano4::topology::Tree::RefinementPlan removedFlags(2);
  removedFlags[0].emplace_back(3);
  removedFlags[1].emplace_back(10);

  peano4::topology::Tree standing(TreeId, offset, width);
  standing.buildRegularGrid(Depth, 0, 1, standingPlan);
  validate(standing.isCellRefinedInPlan(Depth + 1, tarch::la::Vector<DIMENSIONS, int>(6)));

  peano4::topology::Tree view(TreeId, offset, width);
  view.buildPlanQueryTablesFrom(Depth, standing, newPlan, addedFlags, removedFlags);
  peano4::topology::Tree direct(TreeId, offset, width);
  direct.buildPlanQueryTables(Depth, newPlan);

  // The parent of the kept fine flag lost its refinement, so its children
  // are unrefined; the added fine flag refines below a kept parent.
  validate(not direct.isCellRefinedInPlan(Depth + 1, tarch::la::Vector<DIMENSIONS, int>(6)));
  validate(direct.isCellRefinedInPlan(Depth + 1, tarch::la::Vector<DIMENSIONS, int>(12)));

  for (int level = Depth; level <= Depth + 2; level++) {
    const int cellsPerAxisOnLevel      = peano4::topology::cellsPerAxis(level);
    int       numberOfLatticePositions = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      numberOfLatticePositions *= cellsPerAxisOnLevel + 1;
    }
    for (int position = 0; position < numberOfLatticePositions; position++) {
      tarch::la::Vector<DIMENSIONS, int> lattice(0);
      int                                remaining      = position;
      bool                               isCellPosition = true;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        lattice(axis) = remaining % (cellsPerAxisOnLevel + 1);
        remaining /= cellsPerAxisOnLevel + 1;
        isCellPosition = isCellPosition and lattice(axis) < cellsPerAxisOnLevel;
      }
      validateEqualsWithParams2(
        view.isVertexRefinedInPlan(level, lattice),
        direct.isVertexRefinedInPlan(level, lattice),
        level,
        lattice
      );
      validateEqualsWithParams2(
        view.isVertexPersistentInPlan(level, lattice),
        direct.isVertexPersistentInPlan(level, lattice),
        level,
        lattice
      );
      if (isCellPosition) {
        validateEqualsWithParams2(
          view.cellExistsInPlan(level, lattice),
          direct.cellExistsInPlan(level, lattice),
          level,
          lattice
        );
        validateEqualsWithParams2(
          view.isCellRefinedInPlan(level, lattice),
          direct.isCellRefinedInPlan(level, lattice),
          level,
          lattice
        );
      }
    }
  }
}
