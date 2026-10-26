// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/topology/Tree.h"

#include "peano4/Constants.h"
#include "peano4/RefinementRequest.h"
#include "peano4/grid/GridVertex.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"

namespace {
  constexpr int Depth             = 2;
  constexpr int LocalRank         = 0;
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;
  constexpr int ManyRanks         = 4096;

  constexpr int CellsPerAxis = peano4::power(SubdivisionFactor, Depth);

  /**
   * The refinement-plan tests request the cell at this lattice position away from the boundary.
   */
  constexpr int RequestedCell = CellsPerAxis / 2;

  std::int64_t getLatticeKeyOf(const tarch::la::Vector<DIMENSIONS, int>& lattice) {
    std::int64_t key = 0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      key = key * 4096 + lattice(axis);
    }
    return key;
  }

  /**
   * Flags every vertex of the children of the cell with coordinate parentCell on every axis. All
   * these children refine. The partition descends through the cell.
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

  /**
   * Returns the number of vertices that a refine request from the cell at the position flags on the
   * base depth of a regular grid with the periodic boundaries.
   */
  std::size_t getNumberOfFlaggedVerticesOfRequestAt(
    const std::bitset<DIMENSIONS>& periodicBoundaries,
    const int                      firstCellPosition
  ) {
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
  }

  void validateTheLevelsAreSorted(const peano4::topology::Tree::RefinementPlan& sortedPlan) {
    for (const auto& planLevel : sortedPlan) {
      for (std::size_t flagNumber = 1; flagNumber < planLevel.size(); flagNumber++) {
        const auto previousKey = peano4::topology::Tree::serialiseLatticeKey(planLevel[flagNumber - 1]);
        const auto key         = peano4::topology::Tree::serialiseLatticeKey(planLevel[flagNumber]);
        VALIDATE(previousKey < key, previousKey, key);
      }
    }
  }

  /**
   * Checks that every cell, vertex and face lookup below the spawn level of a partitioned tree returns
   * the index the cells point to and that distinct lattice positions return distinct indices.
   */
  void validateTheLookupsOfAPartitionedTree(
    const peano4::topology::Tree&  tree,
    const std::bitset<DIMENSIONS>& periodicBoundaries,
    const int                      rank,
    const int                      partitionedDepth,
    const int                      spawnLevel
  ) {
    const bool periodic = periodicBoundaries[0];
    VALIDATE(tree.getNumberOfCells() > 0);

    std::unordered_map<int, std::int64_t> latticeOfVertexIndex;
    std::unordered_map<int, std::int64_t> latticeOfFaceIndex;
    int                                   numberOfBlockLevelCells = 0;
    for (int cellIndex = 0; cellIndex < static_cast<int>(tree.getNumberOfCells()); cellIndex++) {
      const auto& cell = tree.getCell(cellIndex);
      if (cell.level <= spawnLevel or cell.level > partitionedDepth) {
        continue;
      }
      numberOfBlockLevelCells++;
      const int cellsPerAxisOnLevel = tree.getCellsPerAxisOfLevel(cell.level);
      VALIDATE_EQUALS(tree.getCellAt(cell.level, cell.latticeIndex), cellIndex, periodic, rank, cell.level);
      VALIDATE_EQUALS(tree.getCell(cell.parent).level, cell.level - 1, periodic, rank);
      VALIDATE(tree.isRefined(cellIndex) or cell.level == partitionedDepth, periodic, rank, cellIndex);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        VALIDATE_EQUALS(
          tree.getCell(cell.parent).latticeIndex(axis),
          cell.latticeIndex(axis) / SubdivisionFactor,
          periodic,
          rank,
          axis
        );
      }
      if (tree.isRefined(cellIndex)) {
        for (int childPosition = 0; childPosition < ChildrenPerCell; childPosition++) {
          VALIDATE_EQUALS(
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
        VALIDATE(vertexIndex >= 0);
        VALIDATE_EQUALS(tree.getVertexAt(cell.level, cornerLattice), vertexIndex, periodic, rank, vertexNumber);
        VALIDATE_EQUALS(tree.getVertex(vertexIndex).level, cell.level, periodic, rank);
#if PEANO_DEBUG
        // A periodic boundary vertex has the position of the lower boundary in every block.
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          VALIDATE_NUMERICAL_EQUALS(
            tree.getVertex(vertexIndex).getX(axis),
            static_cast<double>(cornerLattice(axis)) / cellsPerAxisOnLevel,
            periodic,
            rank,
            vertexIndex
          );
        }
#endif
        const auto entry = latticeOfVertexIndex.insert({vertexIndex, getLatticeKeyOf(cornerLattice)}).first;
        VALIDATE_EQUALS(entry->second, getLatticeKeyOf(cornerLattice), periodic, rank, vertexIndex);
      }
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        const auto faceIndex   = cell.faceIndices[faceNumber];
        const int  normal      = faceNumber % DIMENSIONS;
        auto       faceLattice = cell.latticeIndex;
        if (faceNumber >= DIMENSIONS) {
          faceLattice(normal) += 1;
        }
        VALIDATE(faceIndex >= 0);
        VALIDATE_EQUALS(tree.getFaceAt(cell.level, normal, faceLattice), faceIndex, periodic, rank, faceNumber);
        if (periodicBoundaries[normal] and faceLattice(normal) == cellsPerAxisOnLevel) {
          faceLattice(normal) = 0;
        }
        faceLattice(normal) += normal * 4096 * 4096;
        const auto entry = latticeOfFaceIndex.insert({faceIndex, getLatticeKeyOf(faceLattice)}).first;
        VALIDATE_EQUALS(entry->second, getLatticeKeyOf(faceLattice), periodic, rank, faceIndex);
      }
    }
    VALIDATE(numberOfBlockLevelCells > 0);
  }
} // namespace

UNIT_TEST("A regular grid build creates the cells and the vertices of every level down to its depth") {
  GIVEN("A tree and the numbers of cells and vertices of all levels down to the depth") {
    peano4::topology::Tree tree(
      LocalRank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0)
    );

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

    WHEN("The tree builds the regular grid of the depth") {
      tree.buildRegularGrid(Depth);

      THEN("The tree counts these cells and vertices and reports the depth") {
        VALIDATE_EQUALS(tree.getNumberOfCells(), expectedCells);
        VALIDATE_EQUALS(tree.getNumberOfVertices(), expectedVertices);
        VALIDATE_EQUALS(tree.getDepth(), Depth);
      }
    }
  }
}

UNIT_TEST("Every cell links its parent and its children and deep sparse faces keep indices of their own") {
  GIVEN("A tree") {
    peano4::topology::Tree tree(
      LocalRank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0)
    );

    WHEN("The tree builds the regular grid of the depth") {
      tree.buildRegularGrid(Depth);

      THEN("The root is refined on level zero without a parent and every refined cell links its children below it") {
        const auto& root = tree.getCell(peano4::topology::Tree::RootCell);
        VALIDATE_EQUALS(root.level, 0);
        VALIDATE_EQUALS(root.parent, -1);
        VALIDATE(tree.isRefined(peano4::topology::Tree::RootCell));

        for (int cellIndex = 0; cellIndex < static_cast<int>(tree.getNumberOfCells()); cellIndex++) {
          const auto& cell = tree.getCell(cellIndex);
          VALIDATE_EQUALS(cell.ownerRank, LocalRank);
          VALIDATE_EQUALS(tree.isRefined(cellIndex), (cell.level < Depth));
          if (not tree.isRefined(cellIndex)) {
            continue;
          }
          for (int childPosition = 0; childPosition < ChildrenPerCell; childPosition++) {
            const auto  childIndex = tree.getChild(cellIndex, childPosition);
            const auto& child      = tree.getCell(childIndex);
            VALIDATE_EQUALS(child.parent, cellIndex);
            VALIDATE_EQUALS(child.level, cell.level + 1);
            int positionRemainder = childPosition;
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              VALIDATE_EQUALS(
                child.latticeIndex(axis),
                SubdivisionFactor * cell.latticeIndex(axis) + positionRemainder % SubdivisionFactor
              );
              positionRemainder /= SubdivisionFactor;
            }
          }
        }
      }
    }
  }

  GIVEN("A tree and a plan that refines one cell on each of the first 12 levels") {
    // One sparse map per normal keeps every lattice bit at depths where a face key with the normal would
    // need more than 64 bits in 3D.
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

    WHEN("The tree builds the grid of the plan and looks up the upper face of the last cell of level 12 per normal") {
      deepTree.buildRegularGrid(0, 0, 1, deepRefinementPlan);
      const int                   cellsPerAxisOnDeepestLevel = peano4::topology::getCellsPerAxis(DeepestSparseLevel);
      std::array<int, DIMENSIONS> faceIndices{};
      for (int normal = 0; normal < DIMENSIONS; normal++) {
        tarch::la::Vector<DIMENSIONS, int> faceLattice(cellsPerAxisOnDeepestLevel - 1);
        faceLattice(normal) = cellsPerAxisOnDeepestLevel;
        faceIndices[normal] = deepTree.getFaceAt(DeepestSparseLevel, normal, faceLattice);
      }

      THEN("Every face exists with an index of its own") {
        for (int normal = 0; normal < DIMENSIONS; normal++) {
          VALIDATE(faceIndices[normal] >= 0);
          for (int otherNormal = 0; otherNormal < normal; otherNormal++) {
            VALIDATE(faceIndices[normal] != faceIndices[otherNormal]);
          }
        }
      }
    }
  }
}

UNIT_TEST("getCellAt() returns the index of every dense cell on every level, also with a periodic axis") {
  GIVEN("A tree, a tree with a periodic first axis and the depth four") {
    constexpr int LookupDepth = 4;

    std::bitset<DIMENSIONS> periodicBoundaries;
    periodicBoundaries[0] = true;
    peano4::topology::Tree tree(
      LocalRank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0)
    );
    peano4::topology::Tree periodicTree(
      LocalRank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0),
      periodicBoundaries
    );

    WHEN("Both trees build the regular grid of this depth") {
      tree.buildRegularGrid(LookupDepth);
      periodicTree.buildRegularGrid(LookupDepth);

      THEN("Every cell of either tree answers the lookup of its level and lattice position with its own index") {
        for (const peano4::topology::Tree* const builtTree : {&tree, &periodicTree}) {
          for (int cellIndex = 0; cellIndex < static_cast<int>(builtTree->getNumberOfCells()); cellIndex++) {
            const auto& cell = builtTree->getCell(cellIndex);
            VALIDATE_EQUALS(
              builtTree->getCellAt(cell.level, cell.latticeIndex),
              cellIndex,
              cell.level,
              cell.latticeIndex
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("A cell and its neighbour along the first axis point to the same indices for the vertices between them") {
  GIVEN("A tree") {
    peano4::topology::Tree tree(
      LocalRank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0)
    );

    WHEN("The tree builds the regular grid of the depth") {
      tree.buildRegularGrid(Depth);
      const auto  leftCellIndex  = tree.getChild(peano4::topology::Tree::RootCell, 0);
      const auto  rightCellIndex = tree.getChild(peano4::topology::Tree::RootCell, 1);
      const auto& leftCell       = tree.getCell(leftCellIndex);
      const auto& rightCell      = tree.getCell(rightCellIndex);

      THEN("Every upper vertex of the first child of the root is the matching lower vertex of the second child") {
        for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
          if ((vertexNumber & 1) == 1) {
            VALIDATE_EQUALS(leftCell.vertexIndices[vertexNumber], rightCell.vertexIndices[vertexNumber ^ 1]);
          }
        }
      }
    }
  }
}

UNIT_TEST("Every vertex stores its level and its refinement state") {
  GIVEN("A tree") {
    peano4::topology::Tree tree(
      LocalRank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0)
    );

    WHEN("The tree builds the regular grid of the depth") {
      tree.buildRegularGrid(Depth);

      THEN("Every vertex lies on its cell level and is refined above the depth") {
        for (int cellIndex = 0; cellIndex < static_cast<int>(tree.getNumberOfCells()); cellIndex++) {
          const auto& cell          = tree.getCell(cellIndex);
          const bool  isFinestLevel = cell.level == Depth;

          for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
            const auto& vertex = tree.getVertex(cell.vertexIndices[vertexNumber]);
            VALIDATE_EQUALS(vertex.level, cell.level);
            VALIDATE(
              vertex.getState()
              == (isFinestLevel ? peano4::grid::GridVertex::State::Unrefined : peano4::grid::GridVertex::State::Refined)
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("A refine request flags every unrefined corner of the block of three cells around the requested cell") {
  GIVEN("A tree whose plan refines through a vertex of the requested cell and a refine request from that cell") {
    peano4::topology::Tree tree(
      LocalRank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0)
    );
    peano4::topology::Tree::RefinementPlan plan(1);
    plan[0].emplace_back(RequestedCell);
    tree.buildRegularGrid(Depth, 0, 1, plan);

    peano4::RefinementRequest refineRequest;
    refineRequest.setRefinementControl(peano4::RefinementRequest::RefinementControl::Refine);
    refineRequest.level        = Depth;
    refineRequest.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(RequestedCell);

    WHEN("The tree updates the plan with the request and the plan covers one full cell of every neighbour") {
      const bool changed = tree.updateRefinementPlan({refineRequest}, plan);

      THEN("The base level flags the four vertices per axis from one below the requested cell to two above it") {
        VALIDATE(changed);
        VALIDATE_EQUALS(plan.size(), 1U);
        std::size_t numberOfVerticesInBand = 1;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          numberOfVerticesInBand *= 4;
        }
        VALIDATE_EQUALS(plan[0].size(), numberOfVerticesInBand);
        for (const auto& flaggedVertex : plan[0]) {
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            VALIDATE(flaggedVertex(axis) >= RequestedCell - 1 and flaggedVertex(axis) <= RequestedCell + 2);
          }
        }
      }
    }
  }
}

UNIT_TEST("A request from a fine cell also flags the coarse vertices beside the refinement transition") {
  GIVEN("A tree whose plan refines through the requested cell and a request from the first child of the cell below") {
    peano4::topology::Tree tree(
      LocalRank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0)
    );
    peano4::topology::Tree::RefinementPlan plan(1);
    plan[0].emplace_back(RequestedCell);
    tree.buildRegularGrid(Depth, 0, 1, plan);

    // The band of the fine cell covers the coarse cell below the requested one
    // and its lower neighbour per axis.
    constexpr int             CoarseCell = RequestedCell - 1;
    constexpr int             CoarseRing = RequestedCell - 2;
    peano4::RefinementRequest refineRequest;
    refineRequest.setRefinementControl(peano4::RefinementRequest::RefinementControl::Refine);
    refineRequest.level        = Depth + 1;
    refineRequest.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(SubdivisionFactor * CoarseCell);

    WHEN("The tree updates the plan with the request") {
      const bool changed = tree.updateRefinementPlan({refineRequest}, plan);

      THEN("The plan flags the coarse vertex two cells below the requested cell, on the ring outside the fine region") {
        VALIDATE(changed);
        VALIDATE(not plan.empty());

        bool hasFlaggedCoarseRing = false;
        for (const auto& flaggedVertex : plan[0]) {
          bool isOnCoarseRing = true;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            isOnCoarseRing = isOnCoarseRing and flaggedVertex(axis) == CoarseRing;
          }
          hasFlaggedCoarseRing = hasFlaggedCoarseRing or isOnCoarseRing;
        }
        VALIDATE(hasFlaggedCoarseRing);
      }
    }
  }
}

UNIT_TEST("A refine request next to the periodic boundary flags as many vertices as one inside the domain") {
  GIVEN("Periodic boundaries along every axis") {
    std::bitset<DIMENSIONS> periodicBoundaries;
    periodicBoundaries.set();

    WHEN("Requests from the lattice centre and from the first and the last cell update a plan each") {
      const auto interiorFlags =
        getNumberOfFlaggedVerticesOfRequestAt(periodicBoundaries, peano4::topology::getCellsPerAxis(Depth) / 2);
      const auto firstCellFlags = getNumberOfFlaggedVerticesOfRequestAt(periodicBoundaries, 0);
      const auto lastCellFlags =
        getNumberOfFlaggedVerticesOfRequestAt(periodicBoundaries, peano4::topology::getCellsPerAxis(Depth) - 1);

      THEN("The request from the centre flags vertices and the requests next to the boundary flag as many") {
        VALIDATE(interiorFlags > 0);
        VALIDATE_EQUALS(firstCellFlags, interiorFlags);
        VALIDATE_EQUALS(lastCellFlags, interiorFlags);
      }
    }
  }
}

UNIT_TEST("Erase requests around a flagged vertex remove its flag unless a fine flag across the boundary needs it") {
  GIVEN("A tree whose plan flags a vertex of the requested cell and erases of the base cells adjacent to the vertex") {
    peano4::topology::Tree tree(
      LocalRank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0)
    );
    peano4::topology::Tree::RefinementPlan plan(1);
    plan[0].emplace_back(RequestedCell);
    tree.buildRegularGrid(Depth, 0, 1, plan);

    // Each request names a base-depth cell. A caller promotes an erase request to the parent cell.
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

    WHEN("The tree updates the plan with the erase requests") {
      const bool changed = tree.updateRefinementPlan(eraseRequests, plan);

      THEN("The vertex loses its flag and the empty level drops from the plan") {
        VALIDATE(changed);
        VALIDATE(plan.empty());
      }
    }
  }

  GIVEN("A tree with a periodic first axis, a coarse flag on the boundary, a fine flag across it and erases beside") {
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

    // The erase requests cover both cells around the coarse flag. One of them lies across the periodic
    // boundary.
    std::vector<peano4::RefinementRequest> periodicEraseRequests;
    for (const int firstAxisPosition : {0, peano4::topology::getCellsPerAxis(Depth) - 1}) {
      peano4::RefinementRequest eraseRequest;
      eraseRequest.setRefinementControl(peano4::RefinementRequest::RefinementControl::Erase);
      eraseRequest.level           = Depth;
      eraseRequest.latticeIndex    = tarch::la::Vector<DIMENSIONS, int>(0);
      eraseRequest.latticeIndex(0) = firstAxisPosition;
      periodicEraseRequests.push_back(eraseRequest);
    }

    WHEN("The tree updates the plan with the erase requests") {
      const bool changed = periodicTree.updateRefinementPlan(periodicEraseRequests, periodicPlan);

      THEN("The fine flag depends on the coarse flag and the plan keeps both levels with one flag each") {
        VALIDATE(not changed);
        VALIDATE_EQUALS(periodicPlan.size(), 2U);
        VALIDATE_EQUALS(periodicPlan[0].size(), 1U);
        VALIDATE_EQUALS(periodicPlan[1].size(), 1U);
      }
    }
  }
}

UNIT_TEST("A round changes only the levels its requests touch and keeps every level sorted without duplicates") {
  GIVEN("A tree whose plan flags a coarse vertex and a fine vertex where the children of the refined cells meet") {
    // The lattice has room for the coarse flag, the band of the refine request and an unrefined cell
    // between them. The depth therefore depends on the subdivision factor.
    constexpr int RoundDepth          = SubdivisionFactor >= 3 ? Depth : Depth + 1;
    constexpr int RoundCellsPerAxis   = peano4::power(SubdivisionFactor, RoundDepth);
    constexpr int CoarseFlag          = RoundCellsPerAxis / 2;
    constexpr int FineFlag            = SubdivisionFactor * CoarseFlag;
    constexpr int RequestedCellOfBand = RoundCellsPerAxis - 2;
    constexpr int BandVerticesPerAxis = peano4::NeighbourhoodWidth + 1;

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

    WHEN("The tree updates the plan with a base-depth refine request whose band lies away from the coarse flag") {
      peano4::RefinementRequest refineRequest;
      refineRequest.setRefinementControl(peano4::RefinementRequest::RefinementControl::Refine);
      refineRequest.level        = RoundDepth;
      refineRequest.latticeIndex = tarch::la::Vector<DIMENSIONS, int>(RequestedCellOfBand);
      const bool changed         = tree.updateRefinementPlan({refineRequest}, plan);

      THEN("The base level gains the band corners, the fine level stays as it stands and every level stays sorted") {
        VALIDATE(changed);
        VALIDATE_EQUALS(plan.size(), 2U);
        VALIDATE(plan[1] == standingFineLevel);
        std::size_t numberOfBandCorners = 1;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          numberOfBandCorners *= BandVerticesPerAxis;
        }
        VALIDATE_EQUALS(plan[0].size(), 1 + numberOfBandCorners);
        validateTheLevelsAreSorted(plan);
      }

      AND_WHEN("The tree builds the new plan and updates it with erases of every fine cell around the fine flag") {
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
        const bool erased = tree.updateRefinementPlan(eraseRequests, plan);

        THEN("The fine level drops, the coarse level stays as it stands and every level stays sorted") {
          VALIDATE(erased);
          VALIDATE_EQUALS(plan.size(), 1U);
          VALIDATE(plan[0] == standingCoarseLevel);
          validateTheLevelsAreSorted(plan);
        }
      }
    }
  }
}

UNIT_TEST("An erase block whose cells keep flagged rim corners keeps the plan and a whole block coarsens its centre") {
  GIVEN("A tree whose plan flags a vertex block one neighbourhood wide and erases of a partial and a whole block") {
    // The flagged block, the cells it refines and one unrefined ring around them fit into the lattice.
    // The depth therefore depends on the subdivision factor.
    constexpr int  EraseDepth             = SubdivisionFactor >= 3 ? Depth : Depth + 1;
    constexpr int  EraseCellsPerAxis      = peano4::power(SubdivisionFactor, EraseDepth);
    constexpr int  FlaggedVerticesPerAxis = peano4::NeighbourhoodWidth + 1;
    constexpr int  FirstFlaggedCoordinate = EraseCellsPerAxis / 2 - 1;
    constexpr int  CentreCellOfBlock      = FirstFlaggedCoordinate + 1;
    constexpr auto NumberOfFlaggedVertices =
      static_cast<std::size_t>(peano4::power(FlaggedVerticesPerAxis, DIMENSIONS));

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

    // The partial block spans one neighbourhood along the first axis and one cell less along the other
    // axes. The vertices between its cells are candidates. Every cell around them keeps a flagged
    // corner on the rim.
    tarch::la::Vector<DIMENSIONS, int> partialBlock(peano4::NeighbourhoodWidth - 1);
    partialBlock(0) = peano4::NeighbourhoodWidth;
    const tarch::la::Vector<DIMENSIONS, int> wholeBlock(peano4::NeighbourhoodWidth);

    WHEN("The tree updates the plan with the erases of the partial block") {
      const bool changedByThePartialBlock = tree.updateRefinementPlan(getEraseRequestsForBlock(partialBlock), plan);

      THEN("The plan keeps every flag") {
        VALIDATE(not changedByThePartialBlock);
        VALIDATE_EQUALS(plan.size(), 1U);
        VALIDATE_EQUALS(plan[0].size(), NumberOfFlaggedVertices);
      }

      AND_WHEN("The tree updates the plan with the erases of the whole block") {
        const bool changedByTheWholeBlock = tree.updateRefinementPlan(getEraseRequestsForBlock(wholeBlock), plan);

        THEN("The plan drops the corner flags of the centre cell of the block and keeps every other flag") {
          VALIDATE(changedByTheWholeBlock);
          VALIDATE_EQUALS(plan.size(), 1U);
          VALIDATE_EQUALS(plan[0].size(), NumberOfFlaggedVertices - peano4::VerticesPerCell);
          for (const auto& flaggedVertex : plan[0]) {
            bool isCornerOfCentreCell = true;
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              const int coordinate = flaggedVertex(axis);
              isCornerOfCentreCell =
                isCornerOfCentreCell and (coordinate == CentreCellOfBlock or coordinate == CentreCellOfBlock + 1);
            }
            VALIDATE(not isCornerOfCentreCell);
          }
        }
      }
    }
  }
}

UNIT_TEST("A fine vertex between two refined cells persists and a fine vertex towards an unrefined cell hangs") {
  GIVEN("A tree whose plan refines two neighbouring cells, a fine vertex between them, one towards an unrefined cell") {
    // The two cells, their flags and the unrefined cell beside them lie away from the boundary.
    constexpr int FirstCell = CellsPerAxis / 2 - 1;
    constexpr int FineLevel = Depth + 1;

    peano4::topology::Tree tree(
      LocalRank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0)
    );
    // The first cell refines through its lower corner and its neighbour along the first axis through
    // its upper corner. The corners between them stay unflagged.
    peano4::topology::Tree::RefinementPlan plan(1);
    plan[0].emplace_back(FirstCell);
    auto farCorner = tarch::la::Vector<DIMENSIONS, int>(FirstCell);
    farCorner(0)   = FirstCell + 2;
    plan[0].push_back(farCorner);

    auto sideBetweenCellsVertex = tarch::la::Vector<DIMENSIONS, int>(SubdivisionFactor * FirstCell + 1);
    sideBetweenCellsVertex(0)   = SubdivisionFactor * (FirstCell + 1);
    // outerSideVertex lies on the side of the first cell towards the unrefined cell along the second axis.
    auto outerSideVertex = tarch::la::Vector<DIMENSIONS, int>(SubdivisionFactor * FirstCell + 1);
    outerSideVertex(1)   = SubdivisionFactor * (FirstCell + 1);

    WHEN("The tree builds the grid of the plan") {
      tree.buildRegularGrid(Depth, 0, 1, plan);

      THEN("The vertex between the cells persists in the tree and the other vertex hangs") {
        const int sideBetweenCellsVertexIndex = tree.getVertexAt(FineLevel, sideBetweenCellsVertex);
        VALIDATE(sideBetweenCellsVertexIndex >= 0);
        VALIDATE(
          tree.getVertex(sideBetweenCellsVertexIndex).getState() != peano4::grid::GridVertex::State::HangingVertex
        );

        const int outerSideVertexIndex = tree.getVertexAt(FineLevel, outerSideVertex);
        VALIDATE(outerSideVertexIndex >= 0);
        VALIDATE(tree.getVertex(outerSideVertexIndex).getState() == peano4::grid::GridVertex::State::HangingVertex);
      }
    }
  }
}

UNIT_TEST("The ownership plan is the same on every tree, nests the rank ranges of its nodes and matches the tree") {
  GIVEN("A tree, a replica tree, a plan that refines through the requested cell, four ranks and the spawn level one") {
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
    const peano4::topology::Tree replicaTree(
      LocalRank + 1,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0)
    );

    WHEN("Both trees derive the ownership plan and the rank loads of the plan for four ranks") {
      const auto ownershipPlan        = tree.deriveBalancedBlockOwners(Depth, NumberOfRanks, refinementPlan);
      const auto replicaOwnershipPlan = replicaTree.deriveBalancedBlockOwners(Depth, NumberOfRanks, refinementPlan);
      const auto loads                = tree.deriveRankLoads(Depth, NumberOfRanks, refinementPlan, ownershipPlan);
      const auto replicaLoads = replicaTree.deriveRankLoads(Depth, NumberOfRanks, refinementPlan, ownershipPlan);

      THEN("Both trees derive the same plan and loads and the loads of all ranks together cover the leaves") {
        VALIDATE(ownershipPlan == replicaOwnershipPlan);

        // The leaves of the plan are the regular grid plus the children of the cells around the flagged
        // vertex. These children replace their parent cells.
        std::int64_t expectedLeaves = 1;
        for (int level = 0; level < Depth; level++) {
          expectedLeaves *= ChildrenPerCell;
        }
        expectedLeaves += static_cast<std::int64_t>(peano4::CellsPerVertex) * (ChildrenPerCell - 1);
        VALIDATE_EQUALS(static_cast<int>(loads.size()), NumberOfRanks);
        std::int64_t totalLoad = 0;
        for (const auto load : loads) {
          VALIDATE(load > 0);
          totalLoad += load;
        }
        VALIDATE_EQUALS(totalLoad, expectedLeaves);
        VALIDATE(loads == replicaLoads);
      }

      THEN("Every topology node names a rank range inside the rank count and a node with children covers theirs") {
        VALIDATE(static_cast<int>(ownershipPlan.size()) > OwnershipPlanHeaderSize);
        VALIDATE_EQUALS(ownershipPlan[0], peano4::topology::Tree::OwnershipPlanMarker);
        const auto numberOfTopologyNodes = ownershipPlan[1];
        VALIDATE_EQUALS(
          static_cast<int>(ownershipPlan.size()),
          OwnershipPlanHeaderSize + numberOfTopologyNodes * TopologyNodeStride
        );

        for (int nodeIndex = 0; nodeIndex < numberOfTopologyNodes; nodeIndex++) {
          const auto planOffset    = OwnershipPlanHeaderSize + nodeIndex * TopologyNodeStride;
          const auto firstOwner    = ownershipPlan[planOffset];
          const auto lastOwner     = ownershipPlan[planOffset + 1];
          const auto childrenBlock = ownershipPlan[planOffset + 2];
          VALIDATE(firstOwner >= 0 and firstOwner <= lastOwner and lastOwner < NumberOfRanks);
          if (childrenBlock < 0) {
            VALIDATE_EQUALS(firstOwner, lastOwner);
            continue;
          }
          // A node has children only where a rank boundary crosses it. Its range then spans several ranks
          // and covers the ranges of its children.
          VALIDATE(firstOwner < lastOwner);
          VALIDATE(childrenBlock > nodeIndex and childrenBlock + ChildrenPerCell <= numberOfTopologyNodes);
          for (int childPosition = 0; childPosition < ChildrenPerCell; childPosition++) {
            const auto childOffset = OwnershipPlanHeaderSize + (childrenBlock + childPosition) * TopologyNodeStride;
            VALIDATE(ownershipPlan[childOffset] >= firstOwner and ownershipPlan[childOffset + 1] <= lastOwner);
          }
        }
      }

      AND_WHEN("The tree builds the grid of the plan with the ownership plan") {
        tree.buildRegularGrid(Depth, SpawnLevel, NumberOfRanks, refinementPlan, &ownershipPlan);

        THEN("The subtree of every cell touches the owners of its leaves and its lattice position returns its owner") {
          // The tree materialises a subset of the leaves of the domain. A parent precedes its children in
          // the cell array. One backward pass therefore collects for every cell the owners of its
          // materialised leaves.
          std::vector<std::bitset<NumberOfRanks>> ranksBelowCell(tree.getNumberOfCells());
          for (int cellIndex = static_cast<int>(tree.getNumberOfCells()) - 1; cellIndex >= 0; cellIndex--) {
            if (tree.isRefined(cellIndex)) {
              for (int childPosition = 0; childPosition < ChildrenPerCell; childPosition++) {
                ranksBelowCell[cellIndex] |= ranksBelowCell[tree.getChild(cellIndex, childPosition)];
              }
              continue;
            }
            const auto owner = tree.getCell(cellIndex).ownerRank;
            VALIDATE(owner >= 0 and owner < NumberOfRanks);
            ranksBelowCell[cellIndex].set(owner);
          }
          // The tree's own leaves always materialise.
          VALIDATE(ranksBelowCell[peano4::topology::Tree::RootCell].test(LocalRank));

          for (int cellIndex = 0; cellIndex < static_cast<int>(tree.getNumberOfCells()); cellIndex++) {
            const auto& cell  = tree.getCell(cellIndex);
            const auto& ranks = ranksBelowCell[cellIndex];
            // The owner of every materialised leaf lies in the owner interval of each ancestor. The owner
            // of a cell is the first rank of its interval.
            for (int rank = 0; rank < NumberOfRanks; rank++) {
              if (ranks.test(rank)) {
                VALIDATE(tree.subtreeTouchesRank(cellIndex, rank));
              }
            }
            VALIDATE_EQUALS(tree.getOwnerOfCellAt(cell.level, cell.latticeIndex), cell.ownerRank);
          }
        }
      }
    }
  }
}

UNIT_TEST("The balanced partition of a regular grid never splits the children of one parent across ranks") {
  GIVEN("A tree, an empty plan, two and four ranks and a base depth with the leaf parents below the spawn level") {
    // The spawn level grows with the rank count and shrinks with the children per cell. This depth puts
    // the parents of the leaves below the spawn level.
    constexpr int DeepDepth = SubdivisionFactor >= 3 ? 3 : 4;

    const std::vector<int>                       rankCounts{2, 4};
    const peano4::topology::Tree::RefinementPlan emptyPlan;
    const peano4::topology::Tree                 tree(
      LocalRank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0)
    );

    WHEN("The tree derives the spawn level, the ownership plan and the rank loads for every rank count") {
      std::vector<int>                       spawnLevelOfRankCount;
      std::vector<bool>                      splitsParentOfLeafOfRankCount;
      std::vector<std::vector<std::int64_t>> loadsOfRankCount;
      for (const int numberOfRanks : rankCounts) {
        const int  spawnLevel    = peano4::topology::Tree::deriveSpawnLevel(DeepDepth, numberOfRanks);
        const auto ownershipPlan = tree.deriveBalancedBlockOwners(DeepDepth, numberOfRanks, emptyPlan);
        spawnLevelOfRankCount.push_back(spawnLevel);
        splitsParentOfLeafOfRankCount.push_back(
          tree.ownershipPlanSplitsParentOfLeaf(DeepDepth, spawnLevel, emptyPlan, ownershipPlan)
        );
        loadsOfRankCount.push_back(tree.deriveRankLoads(DeepDepth, numberOfRanks, emptyPlan, ownershipPlan));
      }

      THEN("The spawn level lies at least two levels above the base depth and every parent of a leaf stays whole") {
        for (std::size_t rankCount = 0; rankCount < rankCounts.size(); rankCount++) {
          const int numberOfRanks = rankCounts[rankCount];
          const int spawnLevel    = spawnLevelOfRankCount[rankCount];
          VALIDATE(spawnLevel <= DeepDepth - 2, numberOfRanks, spawnLevel);
          VALIDATE(not splitsParentOfLeafOfRankCount[rankCount]);
        }
      }

      THEN("Every rank load is a multiple of the children per cell") {
        for (std::size_t rankCount = 0; rankCount < rankCounts.size(); rankCount++) {
          for (const auto load : loadsOfRankCount[rankCount]) {
            VALIDATE_EQUALS(load % ChildrenPerCell, 0, rankCounts[rankCount]);
          }
        }
      }
    }
  }
}

UNIT_TEST("An ownership plan of a deeper plan counts a cell the shallower plan keeps as a leaf for its first rank") {
  GIVEN("A tree, a plan that refines one cell on three levels below the base depth and its first level alone") {
    const auto                                   deepPlan = getThreeLevelPlan();
    const peano4::topology::Tree::RefinementPlan shallowPlan(deepPlan.begin(), deepPlan.begin() + 1);
    const peano4::topology::Tree                 tree(
      LocalRank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0)
    );

    WHEN("The tree derives the ownership plan of the deep plan and the rank loads of the shallow plan under it") {
      const auto deepOwnershipPlan = tree.deriveBalancedBlockOwners(Depth, ManyRanks, deepPlan);
      const auto loads             = tree.deriveRankLoads(Depth, ManyRanks, shallowPlan, deepOwnershipPlan);

      THEN("The ownership plan splits the cells down to the base depth and two below and the loads cover the leaves") {
        // Every cell up to the base depth splits. Below it the two cells split whose children are all
        // refined.
        int expectedTopologyNodes = 2 * ChildrenPerCell;
        int nodesOnLevel          = 1;
        for (int level = 0; level <= Depth; level++) {
          expectedTopologyNodes += nodesOnLevel;
          nodesOnLevel *= ChildrenPerCell;
        }
        VALIDATE_EQUALS(deepOwnershipPlan[1], expectedTopologyNodes);

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
        std::int64_t totalLoad = 0;
        for (const auto load : loads) {
          VALIDATE(load >= 0);
          totalLoad += load;
        }
        VALIDATE_EQUALS(totalLoad, shallowLeaves);
      }
    }
  }
}

UNIT_TEST("An ownership plan keeps every parent of a leaf whole under its plan and splits one under a shallower plan") {
  GIVEN("A tree, a plan that refines one cell on three levels below the base depth and its first level alone") {
    const auto                                   deepPlan = getThreeLevelPlan();
    const peano4::topology::Tree::RefinementPlan shallowPlan(deepPlan.begin(), deepPlan.begin() + 1);
    const peano4::topology::Tree                 tree(
      LocalRank,
      tarch::la::Vector<DIMENSIONS, double>(0.0),
      tarch::la::Vector<DIMENSIONS, double>(1.0)
    );

    WHEN("The tree derives the ownership plans of both plans for many ranks and the spawn level of the base depth") {
      const auto deepOwnershipPlan    = tree.deriveBalancedBlockOwners(Depth, ManyRanks, deepPlan);
      const auto shallowOwnershipPlan = tree.deriveBalancedBlockOwners(Depth, ManyRanks, shallowPlan);
      const int  spawnLevel           = peano4::topology::Tree::deriveSpawnLevel(Depth, ManyRanks);

      THEN("Both ownership plans keep every parent of a leaf whole under their own plan") {
        VALIDATE(not tree.ownershipPlanSplitsParentOfLeaf(Depth, spawnLevel, deepPlan, deepOwnershipPlan));
        VALIDATE(not tree.ownershipPlanSplitsParentOfLeaf(Depth, spawnLevel, shallowPlan, shallowOwnershipPlan));
      }

      THEN("The deep ownership plan splits a parent of a leaf under the shallow plan") {
        VALIDATE(tree.ownershipPlanSplitsParentOfLeaf(Depth, spawnLevel, shallowPlan, deepOwnershipPlan));
      }
    }
  }
}

UNIT_TEST("A rank without a block in the ownership plan materialises only the levels down to the spawn level") {
  GIVEN("A planning tree, a plan through the requested cell, 4096 ranks and the dense cells down to the spawn level") {
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

    // Every rank stores the levels up to the spawn level densely. A rank without a block stores only
    // these levels.
    std::size_t cellsToTheSpawnLevel = 0;
    for (int level = 0; level <= SpawnLevel; level++) {
      std::size_t cellsOnLevel = 1;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        cellsOnLevel *= peano4::topology::getCellsPerAxis(level);
      }
      cellsToTheSpawnLevel += cellsOnLevel;
    }

    WHEN("The ranks receive their blocks and the owning rank and the first rank above the highest owner build trees") {
      const auto ownershipPlan = planningTree.deriveBalancedBlockOwners(Depth, NumberOfRanks, refinementPlan);

      // The plan has far fewer leaves than ranks.
      int highestOwner = -1;
      for (int nodeIndex = 0; nodeIndex < ownershipPlan[1]; nodeIndex++) {
        const auto planOffset = OwnershipPlanHeaderSize + nodeIndex * TopologyNodeStride;
        highestOwner          = std::max(highestOwner, ownershipPlan[planOffset + 1]);
      }
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
      const std::size_t cellsOfRankWithoutABlock = getNumberOfCellsOfRank(rankWithoutABlock);
      const std::size_t cellsOfOwningRank        = getNumberOfCellsOfRank(OwningRankId);

      THEN("A rank above the highest owner builds the dense levels only and the owning rank builds more cells") {
        VALIDATE(highestOwner >= 0 and highestOwner < NumberOfRanks - 1);
        VALIDATE_EQUALS(cellsOfRankWithoutABlock, cellsToTheSpawnLevel);
        VALIDATE(cellsOfOwningRank > cellsToTheSpawnLevel);
      }
    }
  }
}

UNIT_TEST("Every cell, vertex and face lookup of a partitioned build returns the index the cells point to") {
  GIVEN("The depth four, the spawn level one, two ranks and a plan that flags one vertex") {
    constexpr int PartitionedDepth = 4;
    constexpr int SpawnLevel       = 1;
    constexpr int NumberOfRanks    = 2;

    peano4::topology::Tree::RefinementPlan refinementPlan;
    refinementPlan.emplace_back().emplace_back(5);

    WHEN("Every rank builds its part of the partitioned grid, without and with a periodic first axis") {
      std::vector<std::unique_ptr<peano4::topology::Tree>> trees;
      std::vector<std::bitset<DIMENSIONS>>                 periodicBoundariesOfTree;
      std::vector<int>                                     rankOfTree;
      for (const bool periodic : {false, true}) {
        std::bitset<DIMENSIONS> periodicBoundaries;
        periodicBoundaries[0] = periodic;
        for (int rank = 0; rank < NumberOfRanks; rank++) {
          auto tree = std::make_unique<peano4::topology::Tree>(
            rank,
            tarch::la::Vector<DIMENSIONS, double>(0.0),
            tarch::la::Vector<DIMENSIONS, double>(1.0),
            periodicBoundaries
          );
          const auto ownershipPlan = tree->deriveBalancedBlockOwners(PartitionedDepth, NumberOfRanks, refinementPlan);
          tree->buildRegularGrid(PartitionedDepth, SpawnLevel, NumberOfRanks, refinementPlan, &ownershipPlan);
          trees.push_back(std::move(tree));
          periodicBoundariesOfTree.push_back(periodicBoundaries);
          rankOfTree.push_back(rank);
        }
      }

      THEN("Every lookup returns the index the cells point to and distinct lattice positions return distinct indices") {
        for (std::size_t treeNumber = 0; treeNumber < trees.size(); treeNumber++) {
          validateTheLookupsOfAPartitionedTree(
            *trees[treeNumber],
            periodicBoundariesOfTree[treeNumber],
            rankOfTree[treeNumber],
            PartitionedDepth,
            SpawnLevel
          );
        }
      }
    }
  }
}

UNIT_TEST("The bytes of a tree cover at least its cells and vertices and grow with the depth") {
  GIVEN("A shallow tree and a deep tree") {
    const tarch::la::Vector<DIMENSIONS, double> offset(0.0);
    const tarch::la::Vector<DIMENSIONS, double> width(1.0);

    peano4::topology::Tree shallow(0, offset, width);
    peano4::topology::Tree deep(0, offset, width);

    WHEN("The shallow tree builds the grid of depth one and the deep tree the grid of depth two") {
      shallow.buildRegularGrid(1);
      deep.buildRegularGrid(2);

      THEN("The bytes of the shallow tree cover its cells and vertices and the deep tree counts more bytes") {
        VALIDATE(
          shallow.getAllocatedBytes() >= shallow.getNumberOfCells() * sizeof(peano4::topology::TreeCell)
            + shallow.getNumberOfVertices() * sizeof(peano4::grid::GridVertex)
        );
        VALIDATE(deep.getAllocatedBytes() > shallow.getAllocatedBytes());
      }
    }
  }
}
