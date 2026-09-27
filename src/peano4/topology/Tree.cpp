// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Tree.h"

#include "peano4/RefinementRequest.h"
#include "peano4/grid/AutomatonState.h"
#include "peano4/sfc/ActiveCurve.h"
#include "peano4/topology/ChunkedTasks.h"
#include "tarch/MemoryUsage.h"
#include "tarch/Termination.h"
#include "tarch/tasking/FunctorTask.h"
#include "tarch/tasking/TaskGraph.h"

namespace {
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;

  /** Returns the containing coarse-lattice position by rounding towards minus infinity. */
  int floorDivide(const int value, const int divisor) noexcept {
    return value >= 0 ? value / divisor : -((-value + divisor - 1) / divisor);
  }

  /**
   * Returns the lattice position of one cell corner.
   *
   * A periodic axis holds one vertex for both walls, so a corner on the upper
   * wall carries the position of the lower one.
   */
  tarch::la::Vector<DIMENSIONS, int> getCornerLatticeOfCell(
    const tarch::la::Vector<DIMENSIONS, int>& cellLattice,
    const int                                 vertexNumber,
    const int                                 cellsPerAxisOnLevel,
    const std::bitset<DIMENSIONS>&            periodicBoundaries
  ) {
    tarch::la::Vector<DIMENSIONS, int> cornerLattice(0);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      const int coordinate = cellLattice(axis) + ((vertexNumber >> axis) & 1);
      cornerLattice(axis)  = (periodicBoundaries[axis] and coordinate == cellsPerAxisOnLevel) ? 0 : coordinate;
    }
    return cornerLattice;
  }

  struct AdjacentCell {
    tarch::la::Vector<DIMENSIONS, int> lattice{0};
    bool                               isInsideDomain{true};
  };

  /**
   * Returns the cell lattice position at one vertex adjacency entry.
   *
   * A periodic axis wraps its position back into the domain, so only a wall of
   * a non-periodic axis reports a position outside.
   */
  AdjacentCell getAdjacentCellOfVertex(
    const tarch::la::Vector<DIMENSIONS, int>& vertexLattice,
    const int                                 adjacencyEntry,
    const int                                 cellsPerAxisOnLevel,
    const std::bitset<DIMENSIONS>&            periodicBoundaries
  ) {
    AdjacentCell adjacentCell;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      int coordinate = vertexLattice(axis) - 1 + ((adjacencyEntry >> axis) & 1);
      if (periodicBoundaries[axis]) {
        coordinate = (coordinate + cellsPerAxisOnLevel) % cellsPerAxisOnLevel;
      }
      adjacentCell.isInsideDomain =
        adjacentCell.isInsideDomain and coordinate >= 0 and coordinate < cellsPerAxisOnLevel;
      adjacentCell.lattice(axis) = coordinate;
    }
    return adjacentCell;
  }

  /**
   * Returns whether any coarse vertex the given vertex lies between satisfies
   * the predicate.
   *
   * A fine coordinate divisible by the subdivision factor sits on one coarse
   * vertex; every other coordinate lies between two, so a vertex has up to 2^d
   * governing vertices.
   */
  template <typename Predicate>
  bool anyGoverningVertexSatisfies(
    const tarch::la::Vector<DIMENSIONS, int>& vertexLattice,
    const int                                 coarseCellsPerAxis,
    const std::bitset<DIMENSIONS>&            periodicBoundaries,
    const Predicate&                          satisfies
  ) {
    int numberOfCandidates[DIMENSIONS];
    int candidateCoordinates[DIMENSIONS][2];
    int numberOfCombinations = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      const int quotient            = vertexLattice(axis) / SubdivisionFactor;
      const int remainder           = vertexLattice(axis) % SubdivisionFactor;
      candidateCoordinates[axis][0] = quotient;
      candidateCoordinates[axis][1] = quotient + 1;
      numberOfCandidates[axis]      = remainder == 0 ? 1 : 2;
      numberOfCombinations *= numberOfCandidates[axis];
    }
    for (int combination = 0; combination < numberOfCombinations; combination++) {
      tarch::la::Vector<DIMENSIONS, int> coarseVertexLattice(0);
      int                                remainingCombination = combination;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        coarseVertexLattice(axis) = candidateCoordinates[axis][remainingCombination % numberOfCandidates[axis]];
        remainingCombination /= numberOfCandidates[axis];
        if (periodicBoundaries[axis] and coarseVertexLattice(axis) == coarseCellsPerAxis) {
          coarseVertexLattice(axis) = 0;
        }
      }
      if (satisfies(coarseVertexLattice)) {
        return true;
      }
    }
    return false;
  }

  /**
   * Returns whether a cell exists at every adjacency position of the vertex
   * inside the domain.
   *
   * Such a vertex is a real grid vertex: Every coarser cell beside it is
   * refined, so no coarser neighbour exists for it to hang below.
   */
  template <typename CellExists>
  bool everyAdjacentCellExists(
    const tarch::la::Vector<DIMENSIONS, int>& vertexLattice,
    const int                                 cellsPerAxisOnLevel,
    const std::bitset<DIMENSIONS>&            periodicBoundaries,
    const CellExists&                         cellExists
  ) {
    for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
      const auto adjacentCell =
        getAdjacentCellOfVertex(vertexLattice, adjacencyEntry, cellsPerAxisOnLevel, periodicBoundaries);
      if (adjacentCell.isInsideDomain and not cellExists(adjacentCell.lattice)) {
        return false;
      }
    }
    return true;
  }
} // namespace

peano4::topology::Tree::Tree(
  const int                                    treeId,
  const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
  const tarch::la::Vector<DIMENSIONS, double>& domainWidth,
  const std::bitset<DIMENSIONS>&               periodicBoundaries
) :
  _id(treeId),
  _domainOffset(domainOffset),
  _domainWidth(domainWidth),
  _periodicBoundaries(periodicBoundaries) {}

/**
 * Validates that every plan level fits the tree's lattice keys.
 */
int
peano4::topology::Tree::validateAndGetMaximumLevel(const int depth, const RefinementPlan& refinementPlan) {
  if (depth < 0) {
    tarch::terminateWithError("The grid depth has to be non-negative.");
  }
  if (refinementPlan.size() > static_cast<std::size_t>(std::numeric_limits<int>::max() - depth)) {
    tarch::terminateWithError("The refinement plan exceeds the supported level range.");
  }

  const int              maximumLevel             = depth + static_cast<int>(refinementPlan.size());
  constexpr std::int64_t MaximumLatticeCoordinate = (std::int64_t{1} << LatticeKeyBitsPerAxis) - 1;
  std::int64_t           cellsPerAxisOnMaximumLevel{1};
  for (int level = 0; level < maximumLevel; level++) {
    if (cellsPerAxisOnMaximumLevel > MaximumLatticeCoordinate / SubdivisionFactor) {
      tarch::terminateWithError("The grid depth exceeds the lattice key range.");
    }
    cellsPerAxisOnMaximumLevel *= SubdivisionFactor;
  }
  return maximumLevel;
}

/**
 * Builds the regular grid of the given depth, refined further where the plan
 * flags vertices. A dense level reads a level-local cell index as a number in
 * base ChildrenPerCell whose digits are the linearised child positions from the
 * root down, so every size and link follows in closed form and the levels fill
 * in parallel. The levels beyond the base depth hold one contiguous block of
 * ChildrenPerCell cells per refined cell of the level above and answer through their lattice
 * maps; with several ranks the topology nodes give every cell its owner, and a
 * partitioned rank materialises its owned spawn blocks plus one ring of
 * neighbours below the spawn level.
 */
void
peano4::topology::Tree::buildRegularGrid(
  const int               depth,
  const int               spawnLevel,
  const int               numberOfRanks,
  const RefinementPlan&   refinementPlan,
  const std::vector<int>* ownersOfBlocks
) {
  assertion(spawnLevel >= 0 and spawnLevel <= depth, spawnLevel, depth);
  assertion(numberOfRanks >= 1, numberOfRanks);
  const int maxLevel = validateAndGetMaximumLevel(depth, refinementPlan);

  _depth                       = depth;
  _spawnLevel                  = spawnLevel;
  _denseBoundaryLevel          = depth;
  const int denseBoundaryLevel = _denseBoundaryLevel;
  // A partitioned rank stores the levels down to its spawn level as whole
  // levels and the levels below it block by block.
  const int wholeLevelsUpTo = numberOfRanks > 1 ? spawnLevel : depth;
  _cellsPerAxisOfLevel.resize(maxLevel + 1);
  for (int level = 0; level <= maxLevel; level++) {
    _cellsPerAxisOfLevel[level] = peano4::topology::getCellsPerAxis(level);
  }
  _surroundedVertices.clear();
  _surroundedVerticesDerived = false;
  assignBuildFlagKeys(refinementPlan);
  derivePlanRefinedCellKeys(refinementPlan);
  _topologyNodes.clear();
  _localBlockKeys.clear();
  _ownerOfSpawnCell.clear();
  _blockSlotOfSpawnCell.clear();
  _blockLatticeOfSlot.clear();
  _materialisesWholeDomain = numberOfRanks <= 1;
  if (numberOfRanks > 1) {
    constexpr int    OwnershipPlanHeaderSize = 2;
    constexpr int    TopologyNodeStride      = 3;
    std::vector<int> derivedOwnershipPlan;
    if (ownersOfBlocks == nullptr) {
      derivedOwnershipPlan = deriveBalancedBlockOwners(depth, numberOfRanks, refinementPlan);
    }
    const auto& ownershipPlan = ownersOfBlocks == nullptr ? derivedOwnershipPlan : *ownersOfBlocks;
    if (
      ownershipPlan.size() < OwnershipPlanHeaderSize or ownershipPlan[0] != OwnershipPlanMarker or ownershipPlan[1] < 1
      or ownershipPlan.size()
        != OwnershipPlanHeaderSize + static_cast<std::size_t>(ownershipPlan[1]) * TopologyNodeStride
    ) {
      tarch::terminateWithError("The rank ownership plan is invalid.");
    }
    const auto numberOfTopologyNodes = ownershipPlan[1];
    _topologyNodes.resize(numberOfTopologyNodes);
    for (int nodeIndex = 0; nodeIndex < numberOfTopologyNodes; nodeIndex++) {
      const auto planOffset    = OwnershipPlanHeaderSize + nodeIndex * TopologyNodeStride;
      const auto firstOwner    = ownershipPlan[planOffset];
      const auto lastOwner     = ownershipPlan[planOffset + 1];
      const auto childrenBlock = ownershipPlan[planOffset + 2];
      const bool childrenAreValid =
        childrenBlock == -1 or (childrenBlock > nodeIndex and childrenBlock <= numberOfTopologyNodes - ChildrenPerCell);
      if (firstOwner < 0 or firstOwner > lastOwner or lastOwner >= numberOfRanks or not childrenAreValid) {
        tarch::terminateWithError("The rank ownership plan is invalid.");
      }
      _topologyNodes[nodeIndex] = {firstOwner, lastOwner, childrenBlock};
    }

    // The rank materialises its owned spawn blocks plus one ring of
    // neighbours below the spawn level; everything else answers from the
    // plan and the topology nodes.
    const int    blocksPerAxis       = getCellsPerAxisOfLevel(spawnLevel);
    std::int64_t numberOfSpawnBlocks = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      numberOfSpawnBlocks *= blocksPerAxis;
    }
    for (std::int64_t blockNumber = 0; blockNumber < numberOfSpawnBlocks; blockNumber++) {
      tarch::la::Vector<DIMENSIONS, int> blockLattice(0);
      auto                               remainingIndex = blockNumber;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        blockLattice(axis) = static_cast<int>(remainingIndex % blocksPerAxis);
        remainingIndex /= blocksPerAxis;
      }
      const auto topologyNode = getTopologyNodeAt(spawnLevel, blockLattice);
      const bool blockIsOwned = topologyNode >= 0 and _topologyNodes[topologyNode].firstOwner <= _id
        and _id <= _topologyNodes[topologyNode].lastOwner;
      if (not blockIsOwned) {
        continue;
      }
      for (int neighbourNumber = 0; neighbourNumber < peano4::CellsPerNeighbourhood; neighbourNumber++) {
        tarch::la::Vector<DIMENSIONS, int> neighbourLattice(0);
        int                                remainingNeighbour = neighbourNumber;
        bool                               isInsideDomain     = true;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          int coordinate = blockLattice(axis) + remainingNeighbour % peano4::NeighbourhoodWidth - 1;
          remainingNeighbour /= peano4::NeighbourhoodWidth;
          if (_periodicBoundaries[axis]) {
            coordinate = (coordinate + blocksPerAxis) % blocksPerAxis;
          }
          isInsideDomain         = isInsideDomain and coordinate >= 0 and coordinate < blocksPerAxis;
          neighbourLattice(axis) = coordinate;
        }
        if (isInsideDomain) {
          _localBlockKeys.insert(serialiseLatticeKey(neighbourLattice));
        }
      }
    }
  }
  std::vector<peano4::grid::LatticeIndexMap> flaggedVertexKeys(refinementPlan.size());
  for (std::size_t planLevel = 0; planLevel < refinementPlan.size(); planLevel++) {
    assertion(not refinementPlan[planLevel].empty(), planLevel);
    flaggedVertexKeys[planLevel].reserve(refinementPlan[planLevel].size());
#if PEANO_ASSERTIONS
    const int cellsPerAxisOnPlanLevel = getCellsPerAxisOfLevel(depth + static_cast<int>(planLevel));
#endif
    for (const auto& flaggedVertex : refinementPlan[planLevel]) {
#if PEANO_ASSERTIONS
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        assertion(
          flaggedVertex(axis) >= 0 and flaggedVertex(axis) <= cellsPerAxisOnPlanLevel
            and (not _periodicBoundaries[axis] or flaggedVertex(axis) < cellsPerAxisOnPlanLevel),
          planLevel,
          axis,
          flaggedVertex(axis),
          cellsPerAxisOnPlanLevel
        );
      }
#endif
      flaggedVertexKeys[planLevel].insert(serialiseLatticeKey(flaggedVertex), 0);
    }
  }
  const auto isFlaggedVertex = [&flaggedVertexKeys,
                                depth](const int level, const tarch::la::Vector<DIMENSIONS, int>& vertexLattice) {
    const int planLevel = level - depth;
    return planLevel >= 0 and planLevel < static_cast<int>(flaggedVertexKeys.size())
      and flaggedVertexKeys[planLevel].find(serialiseLatticeKey(vertexLattice))
      != peano4::grid::LatticeIndexMap::NoEntry;
  };

  _cellLevelOffsets.assign(maxLevel + 2, 0);
  _vertexLevelOffsets.assign(maxLevel + 2, 0);
  _faceLevelOffsets.assign(maxLevel + 2, 0);
  _sparseCellIndexOfLattice.assign(maxLevel - denseBoundaryLevel, {});
  _sparseVertexIndexOfLattice.assign(maxLevel - denseBoundaryLevel, {});
  _sparseFaceIndexOfLattice.assign(maxLevel - denseBoundaryLevel, {});

  std::int64_t numberOfCells    = 0;
  std::int64_t numberOfVertices = 0;
  std::int64_t numberOfFaces    = 0;
  for (int level = 0; level <= wholeLevelsUpTo; level++) {
    _cellLevelOffsets[level]   = static_cast<int>(numberOfCells);
    _vertexLevelOffsets[level] = static_cast<int>(numberOfVertices);
    _faceLevelOffsets[level]   = static_cast<int>(numberOfFaces);

    const std::int64_t cellsPerAxisOnLevel = getCellsPerAxisOfLevel(level);
    std::int64_t       cellsOnLevel        = 1;
    std::int64_t       verticesOnLevel     = 1;
    std::int64_t       facesPerNormal      = cellsPerAxisOnLevel + 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellsOnLevel *= cellsPerAxisOnLevel;
      verticesOnLevel *= cellsPerAxisOnLevel + 1;
      if (axis > 0) {
        facesPerNormal *= cellsPerAxisOnLevel;
      }
    }
    numberOfCells += cellsOnLevel;
    numberOfVertices += verticesOnLevel;
    numberOfFaces += DIMENSIONS * facesPerNormal;

    if (
      numberOfCells > std::numeric_limits<int>::max() or numberOfVertices > std::numeric_limits<int>::max()
      or numberOfFaces > std::numeric_limits<int>::max()
    ) {
      tarch::terminateWithError(
        "A regular grid of depth " + std::to_string(depth) + " exceeds the tree's 32-bit index range."
      );
    }
  }
  _cellLevelOffsets[wholeLevelsUpTo + 1]   = static_cast<int>(numberOfCells);
  _vertexLevelOffsets[wholeLevelsUpTo + 1] = static_cast<int>(numberOfVertices);
  _faceLevelOffsets[wholeLevelsUpTo + 1]   = static_cast<int>(numberOfFaces);

  _cells.resize(numberOfCells);
  _vertices.resize(numberOfVertices);

  // The closed-form levels answer getCellAt() through one table read per
  // axis: A whole level interleaves its own coordinates, a block level the
  // coordinates within one block.
  _levelLocalIndexOfCoordinate.assign(depth + 1, {});
  const auto buildInterleaveTable = [this](const int level, const int digits) {
    int entriesPerAxis = 1;
    for (int digit = 0; digit < digits; digit++) {
      entriesPerAxis *= SubdivisionFactor;
    }
    auto& indexOfCoordinate = _levelLocalIndexOfCoordinate[level];
    indexOfCoordinate.assign(static_cast<std::size_t>(DIMENSIONS) * entriesPerAxis, 0);
    int axisStride = 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      for (int coordinate = 0; coordinate < entriesPerAxis; coordinate++) {
        int levelLocalIndex     = 0;
        int digitWeight         = axisStride;
        int remainingCoordinate = coordinate;
        for (int digit = 0; digit < digits; digit++) {
          levelLocalIndex += (remainingCoordinate % SubdivisionFactor) * digitWeight;
          remainingCoordinate /= SubdivisionFactor;
          digitWeight *= ChildrenPerCell;
        }
        indexOfCoordinate[static_cast<std::size_t>(axis) * entriesPerAxis + coordinate] = levelLocalIndex;
      }
      axisStride *= SubdivisionFactor;
    }
  };
  for (int level = 0; level <= wholeLevelsUpTo; level++) {
    buildInterleaveTable(level, level);
  }

  for (int level = 0; level <= wholeLevelsUpTo; level++) {
    const int  cellLevelOffset       = _cellLevelOffsets[level];
    const int  vertexLevelOffset     = _vertexLevelOffsets[level];
    const int  faceLevelOffset       = _faceLevelOffsets[level];
    const int  cellsPerAxisOnLevel   = getCellsPerAxisOfLevel(level);
    const int  verticesPerAxis       = cellsPerAxisOnLevel + 1;
    const int  cellsOnLevel          = _cellLevelOffsets[level + 1] - cellLevelOffset;
    const int  verticesOnLevel       = _vertexLevelOffsets[level + 1] - vertexLevelOffset;
    const int  facesPerNormalOnLevel = (_faceLevelOffsets[level + 1] - faceLevelOffset) / DIMENSIONS;
    const int  parentLevelOffset     = level > 0 ? _cellLevelOffsets[level - 1] : -1;
    const int  childLevelOffset      = level < wholeLevelsUpTo ? _cellLevelOffsets[level + 1] : -1;
    const bool isFinestStoredLevel   = level == wholeLevelsUpTo;
    const bool isFinestLevel         = level == depth;

    peano4::topology::runChunkedTasks(cellsOnLevel, [=, this](const int chunkNumber, const int begin, const int end) {
      static_cast<void>(chunkNumber);
      for (int levelLocalIndex = begin; levelLocalIndex < end; levelLocalIndex++) {
        auto& cell         = _cells[cellLevelOffset + levelLocalIndex];
        cell.level         = level;
        cell.parent        = level > 0 ? parentLevelOffset + levelLocalIndex / ChildrenPerCell : -1;
        cell.childrenBlock = isFinestStoredLevel ? -1 : childLevelOffset + levelLocalIndex * ChildrenPerCell;

        tarch::la::Vector<DIMENSIONS, int> latticeIndex(0);
        int                                remainingDigits = levelLocalIndex;
        int                                axisStride      = 1;
        for (int digit = 0; digit < level; digit++) {
          int childPositionLinearised = remainingDigits % ChildrenPerCell;
          remainingDigits /= ChildrenPerCell;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            latticeIndex(axis) += (childPositionLinearised % SubdivisionFactor) * axisStride;
            childPositionLinearised /= SubdivisionFactor;
          }
          axisStride *= SubdivisionFactor;
        }
        cell.latticeIndex = latticeIndex;
        cell.ownerTreeId  = getOwnerOfCellAt(level, latticeIndex);

        for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
          const auto cornerLattice =
            getCornerLatticeOfCell(latticeIndex, vertexNumber, cellsPerAxisOnLevel, _periodicBoundaries);
          int rowMajorIndex = 0;
          int rowStride     = 1;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            rowMajorIndex += cornerLattice(axis) * rowStride;
            rowStride *= verticesPerAxis;
          }
          cell.vertexIndices[vertexNumber] = vertexLevelOffset + rowMajorIndex;
        }

        for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
          const int normal          = faceNumber % DIMENSIONS;
          int       planeCoordinate = latticeIndex(normal) + (faceNumber >= DIMENSIONS ? 1 : 0);
          if (_periodicBoundaries[normal] and planeCoordinate == cellsPerAxisOnLevel) {
            planeCoordinate = 0;
          }
          int rowMajorIndex = 0;
          int rowStride     = 1;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            rowMajorIndex += (axis == normal ? planeCoordinate : latticeIndex(axis)) * rowStride;
            rowStride *= axis == normal ? verticesPerAxis : cellsPerAxisOnLevel;
          }
          cell.faceIndices[faceNumber] = faceLevelOffset + normal * facesPerNormalOnLevel + rowMajorIndex;
        }
      }
    });

    peano4::topology::runChunkedTasks(
      verticesOnLevel,
      [=, this](const int chunkNumber, const int begin, const int end) {
        static_cast<void>(chunkNumber);
        for (int levelLocalIndex = begin; levelLocalIndex < end; levelLocalIndex++) {
          auto& vertex = _vertices[vertexLevelOffset + levelLocalIndex];

          tarch::la::Vector<DIMENSIONS, int> latticeIndex(0);
          int                                remainingIndex = levelLocalIndex;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            latticeIndex(axis) = remainingIndex % verticesPerAxis;
            remainingIndex /= verticesPerAxis;
          }

          vertex.level = level;
          if (isFinestLevel) {
            vertex.setState(peano4::grid::GridVertex::State::Unrefined);
          } else {
            vertex.setState(peano4::grid::GridVertex::State::Refined);
          }

#if PEANO_DEBUG
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            vertex.setX(axis, _domainOffset(axis) + latticeIndex(axis) * _domainWidth(axis) / cellsPerAxisOnLevel);
          }
#endif
        }
      }
    );
  }

  // The block levels of a partitioned rank: Every materialised block is a
  // regular grid of its own in closed-form layout, and the slots follow the
  // spawn-level cell order.
  if (wholeLevelsUpTo < depth) {
    const int firstSpawnCell = _cellLevelOffsets[wholeLevelsUpTo];
    const int spawnCellEnd   = _cellLevelOffsets[wholeLevelsUpTo + 1];
    _blockSlotOfSpawnCell.assign(spawnCellEnd - firstSpawnCell, -1);
    _ownerOfSpawnCell.assign(spawnCellEnd - firstSpawnCell, -1);
    for (int cellIndex = firstSpawnCell; cellIndex < spawnCellEnd; cellIndex++) {
      if (not isSplitBlockAt(_cells[cellIndex].latticeIndex)) {
        _ownerOfSpawnCell[cellIndex - firstSpawnCell] = _cells[cellIndex].ownerTreeId;
      }
      if (_localBlockKeys.contains(serialiseLatticeKey(_cells[cellIndex].latticeIndex))) {
        _blockSlotOfSpawnCell[cellIndex - firstSpawnCell] = static_cast<int>(_blockLatticeOfSlot.size());
        _blockLatticeOfSlot.push_back(_cells[cellIndex].latticeIndex);
      }
    }
    const auto numberOfBlocks = static_cast<std::int64_t>(_blockLatticeOfSlot.size());
    for (int level = wholeLevelsUpTo + 1; level <= depth; level++) {
      _cellLevelOffsets[level]                  = static_cast<int>(numberOfCells);
      _vertexLevelOffsets[level]                = static_cast<int>(numberOfVertices);
      _faceLevelOffsets[level]                  = static_cast<int>(numberOfFaces);
      const std::int64_t cellsPerAxisOfBlock    = getCellsPerAxisOfBlock(level);
      std::int64_t       cellsPerBlock          = 1;
      std::int64_t       verticesPerBlock       = 1;
      std::int64_t       facesPerNormalPerBlock = cellsPerAxisOfBlock + 1;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        cellsPerBlock *= cellsPerAxisOfBlock;
        verticesPerBlock *= cellsPerAxisOfBlock + 1;
        if (axis > 0) {
          facesPerNormalPerBlock *= cellsPerAxisOfBlock;
        }
      }
      numberOfCells += numberOfBlocks * cellsPerBlock;
      numberOfVertices += numberOfBlocks * verticesPerBlock;
      numberOfFaces += DIMENSIONS * numberOfBlocks * facesPerNormalPerBlock;
      if (
        numberOfCells > std::numeric_limits<int>::max() or numberOfVertices > std::numeric_limits<int>::max()
        or numberOfFaces > std::numeric_limits<int>::max()
      ) {
        tarch::terminateWithError(
          "A regular grid of depth " + std::to_string(depth) + " exceeds the tree's 32-bit index range."
        );
      }
      buildInterleaveTable(level, level - wholeLevelsUpTo);
    }
    _cellLevelOffsets[depth + 1]   = static_cast<int>(numberOfCells);
    _vertexLevelOffsets[depth + 1] = static_cast<int>(numberOfVertices);
    _faceLevelOffsets[depth + 1]   = static_cast<int>(numberOfFaces);
    _cells.resize(numberOfCells);
    _vertices.resize(numberOfVertices);

    for (int cellIndex = firstSpawnCell; cellIndex < spawnCellEnd; cellIndex++) {
      const int blockSlot = _blockSlotOfSpawnCell[cellIndex - firstSpawnCell];
      if (blockSlot >= 0) {
        _cells[cellIndex].childrenBlock = _cellLevelOffsets[wholeLevelsUpTo + 1] + blockSlot * ChildrenPerCell;
      }
    }

    for (int level = wholeLevelsUpTo + 1; level <= depth; level++) {
      const int cellsPerAxisOfBlock    = getCellsPerAxisOfBlock(level);
      const int cellsPerAxisOnLevel    = getCellsPerAxisOfLevel(level);
      int       cellsPerBlock          = 1;
      int       verticesPerBlock       = 1;
      int       facesPerNormalPerBlock = cellsPerAxisOfBlock + 1;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        cellsPerBlock *= cellsPerAxisOfBlock;
        verticesPerBlock *= cellsPerAxisOfBlock + 1;
        if (axis > 0) {
          facesPerNormalPerBlock *= cellsPerAxisOfBlock;
        }
      }
      const auto numberOfBlocksOnLevel = static_cast<int>(numberOfBlocks);
      const int  faceLevelOffset       = _faceLevelOffsets[level];
      const int  cellLevelOffset       = _cellLevelOffsets[level];
      const int  vertexLevelOffset     = _vertexLevelOffsets[level];
      const int  parentLevelOffset     = _cellLevelOffsets[level - 1];
      const int  childLevelOffset      = level < depth ? _cellLevelOffsets[level + 1] : -1;
      const int  digits                = level - wholeLevelsUpTo;
      const bool isFinestLevel         = level == depth;
      const auto numberOfCellsOnLevel  = static_cast<int>(numberOfBlocks * cellsPerBlock);

      // Every cell's content is a closed-form function of its block and its
      // position within the block, so the structs fill in parallel.
      peano4::topology::runChunkedTasks(
        numberOfCellsOnLevel,
        [=, this](const int chunkNumber, const int begin, const int end) {
          static_cast<void>(chunkNumber);
          for (int levelLocalIndex = begin; levelLocalIndex < end; levelLocalIndex++) {
            const int   blockSlot       = levelLocalIndex / cellsPerBlock;
            const int   blockLocalIndex = levelLocalIndex % cellsPerBlock;
            const auto& blockLattice    = _blockLatticeOfSlot[blockSlot];
            auto&       cell            = _cells[cellLevelOffset + levelLocalIndex];
            cell.level                  = level;

            tarch::la::Vector<DIMENSIONS, int> blockLocalLattice(0);
            int                                remainingDigits = blockLocalIndex;
            int                                axisStride      = 1;
            for (int digit = 0; digit < digits; digit++) {
              int childPositionLinearised = remainingDigits % ChildrenPerCell;
              remainingDigits /= ChildrenPerCell;
              for (int axis = 0; axis < DIMENSIONS; axis++) {
                blockLocalLattice(axis) += (childPositionLinearised % SubdivisionFactor) * axisStride;
                childPositionLinearised /= SubdivisionFactor;
              }
              axisStride *= SubdivisionFactor;
            }
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              cell.latticeIndex(axis) = blockLattice(axis) * cellsPerAxisOfBlock + blockLocalLattice(axis);
            }
            cell.parent = digits == 1
              ? firstSpawnCell + getInterleavedIndex(wholeLevelsUpTo, blockLattice)
              : parentLevelOffset + blockSlot * (cellsPerBlock / ChildrenPerCell) + blockLocalIndex / ChildrenPerCell;
            cell.childrenBlock =
              isFinestLevel ? -1 : childLevelOffset + (blockSlot * cellsPerBlock + blockLocalIndex) * ChildrenPerCell;
            cell.ownerTreeId = getOwnerOfCellAt(level, cell.latticeIndex);

            // A cell away from the block's boundary owns every corner and side
            // it touches, so their indices follow from the block-local position
            // alone; a cell on the boundary asks the lookups, which resolve the
            // owner among the neighbouring blocks.
            bool touchesBlockBoundary = false;
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              touchesBlockBoundary = touchesBlockBoundary or blockLocalLattice(axis) == 0
                or blockLocalLattice(axis) == cellsPerAxisOfBlock - 1;
            }
            if (touchesBlockBoundary) {
              for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
                cell.vertexIndices[vertexNumber] = getVertexAt(
                  level,
                  getCornerLatticeOfCell(cell.latticeIndex, vertexNumber, cellsPerAxisOnLevel, _periodicBoundaries)
                );
                assertion(cell.vertexIndices[vertexNumber] >= 0, level, levelLocalIndex, vertexNumber);
              }
              for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
                const int normal      = faceNumber % DIMENSIONS;
                auto      faceLattice = cell.latticeIndex;
                if (faceNumber >= DIMENSIONS) {
                  faceLattice(normal) += 1;
                }
                cell.faceIndices[faceNumber] = getFaceAt(level, normal, faceLattice);
                assertion(cell.faceIndices[faceNumber] >= 0, level, levelLocalIndex, faceNumber);
              }
            } else {
              for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
                int rowMajorIndex = 0;
                int rowStride     = 1;
                for (int axis = 0; axis < DIMENSIONS; axis++) {
                  rowMajorIndex += (blockLocalLattice(axis) + ((vertexNumber >> axis) & 1)) * rowStride;
                  rowStride *= cellsPerAxisOfBlock + 1;
                }
                cell.vertexIndices[vertexNumber] = vertexLevelOffset + blockSlot * verticesPerBlock + rowMajorIndex;
              }
              for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
                const int normal        = faceNumber % DIMENSIONS;
                int       rowMajorIndex = 0;
                int       rowStride     = 1;
                for (int axis = 0; axis < DIMENSIONS; axis++) {
                  const int sideOffset = (axis == normal and faceNumber >= DIMENSIONS) ? 1 : 0;
                  rowMajorIndex += (blockLocalLattice(axis) + sideOffset) * rowStride;
                  rowStride *= axis == normal ? cellsPerAxisOfBlock + 1 : cellsPerAxisOfBlock;
                }
                cell.faceIndices[faceNumber] = faceLevelOffset
                  + (normal * numberOfBlocksOnLevel + blockSlot) * facesPerNormalPerBlock + rowMajorIndex;
              }
#if PEANO_ASSERTIONS
              const auto firstCornerLattice =
                getCornerLatticeOfCell(cell.latticeIndex, 0, cellsPerAxisOnLevel, _periodicBoundaries);
              assertion(cell.vertexIndices[0] == getVertexAt(level, firstCornerLattice), level, levelLocalIndex);
#endif
              assertion(cell.faceIndices[0] == getFaceAt(level, 0, cell.latticeIndex), level, levelLocalIndex);
            }
          }
        }
      );

      // A slot on a block boundary that a lower block owns stays without a
      // cell that references it.
      peano4::topology::runChunkedTasks(
        static_cast<int>(numberOfBlocks * verticesPerBlock),
        [=, this](const int chunkNumber, const int begin, const int end) {
          static_cast<void>(chunkNumber);
          for (int slotNumber = begin; slotNumber < end; slotNumber++) {
            const int                          blockSlot      = slotNumber / verticesPerBlock;
            int                                remainingIndex = slotNumber % verticesPerBlock;
            tarch::la::Vector<DIMENSIONS, int> vertexLattice(0);
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              vertexLattice(axis) =
                _blockLatticeOfSlot[blockSlot](axis) * cellsPerAxisOfBlock + remainingIndex % (cellsPerAxisOfBlock + 1);
              remainingIndex /= cellsPerAxisOfBlock + 1;
            }
            auto& vertex = _vertices[vertexLevelOffset + slotNumber];
            vertex.level = level;
            if (getVertexAt(level, vertexLattice) != vertexLevelOffset + slotNumber) {
              continue;
            }
            if (isFinestLevel) {
              vertex.setState(peano4::grid::GridVertex::State::Unrefined);
            } else {
              vertex.setState(peano4::grid::GridVertex::State::Refined);
            }
#if PEANO_DEBUG
            // The slot of a periodic wall vertex sits in the last block, so
            // its lattice coordinate wraps onto the lower wall.
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              const int wrappedCoordinate =
                (_periodicBoundaries[axis] and vertexLattice(axis) == cellsPerAxisOnLevel) ? 0 : vertexLattice(axis);
              vertex.setX(axis, _domainOffset(axis) + wrappedCoordinate * _domainWidth(axis) / cellsPerAxisOnLevel);
            }
#endif
          }
        }
      );
    }
  }

  if (not refinementPlan.empty()) {
    // A partitioned rank holds no vertex for a flag outside its materialised blocks.
    for (const auto& flaggedVertex : refinementPlan[0]) {
      const int vertexIndex = getVertexAt(depth, flaggedVertex);
      if (vertexIndex >= 0) {
        _vertices[vertexIndex].setState(peano4::grid::GridVertex::State::Refined);
      }
    }
  }

  if (maxLevel == denseBoundaryLevel) {
    _numberOfFaces = static_cast<int>(numberOfFaces);
    return;
  }

  // Cells that refine on the level above the one being built, in ascending
  // cell index order: On the base depth they are the flagged vertices'
  // adjacent cells.
  std::vector<int> refinedCells;
  {
    const int cellsPerAxisOnBaseDepth = getCellsPerAxisOfLevel(depth);
    for (const auto& flaggedVertex : refinementPlan[0]) {
      for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
        const auto adjacentCell =
          getAdjacentCellOfVertex(flaggedVertex, adjacencyEntry, cellsPerAxisOnBaseDepth, _periodicBoundaries);
        if (adjacentCell.isInsideDomain and isLocalBlockOfCell(depth, adjacentCell.lattice)) {
          refinedCells.push_back(getCellAt(depth, adjacentCell.lattice));
        }
      }
    }
    std::sort(refinedCells.begin(), refinedCells.end());
    refinedCells.erase(std::unique(refinedCells.begin(), refinedCells.end()), refinedCells.end());
  }

  for (int level = denseBoundaryLevel + 1; level <= maxLevel; level++) {
    const int sparseLevelIndex    = level - denseBoundaryLevel - 1;
    const int cellsPerAxisOnLevel = getCellsPerAxisOfLevel(level);
    const int firstCellOnLevel    = static_cast<int>(_cells.size());
    _cellLevelOffsets[level]      = firstCellOnLevel;
    _vertexLevelOffsets[level]    = static_cast<int>(_vertices.size());
    _faceLevelOffsets[level]      = static_cast<int>(numberOfFaces);

    auto& cellIndexOfLattice   = _sparseCellIndexOfLattice[sparseLevelIndex];
    auto& vertexIndexOfLattice = _sparseVertexIndexOfLattice[sparseLevelIndex];
    auto& faceIndexOfLattice   = _sparseFaceIndexOfLattice[sparseLevelIndex];
    // A refined cell contributes one block of children, whose corners span a
    // (k+1)^d vertex lattice and whose sides span (k+1) * k^(d-1) faces per
    // normal, k being the subdivision factor. Two adjacent parents hold the
    // vertices and faces between them once, so these counts bound the table
    // sizes from above.
    int verticesPerRefinedCell       = 1;
    int facesPerNormalPerRefinedCell = SubdivisionFactor + 1;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      verticesPerRefinedCell *= SubdivisionFactor + 1;
      if (axis > 0) {
        facesPerNormalPerRefinedCell *= SubdivisionFactor;
      }
    }
    cellIndexOfLattice.reserve(refinedCells.size() * ChildrenPerCell);
    vertexIndexOfLattice.reserve(refinedCells.size() * verticesPerRefinedCell);
    for (auto& faceMap : faceIndexOfLattice) {
      faceMap.reserve(refinedCells.size() * facesPerNormalPerRefinedCell);
    }

    _cells.resize(_cells.size() + refinedCells.size() * ChildrenPerCell);
    // Every child cell's content is a closed-form function of its parent, so
    // the structs fill in parallel.
    peano4::topology::runChunkedTasks(
      static_cast<int>(refinedCells.size()),
      [=, this, &refinedCells](const int chunkNumber, const int begin, const int end) {
        static_cast<void>(chunkNumber);
        for (int parentNumber = begin; parentNumber < end; parentNumber++) {
          const int parentCellIndex             = refinedCells[parentNumber];
          const int childrenBlock               = firstCellOnLevel + parentNumber * ChildrenPerCell;
          _cells[parentCellIndex].childrenBlock = childrenBlock;
          for (int childPositionLinearised = 0; childPositionLinearised < ChildrenPerCell; childPositionLinearised++) {
            auto& cell          = _cells[childrenBlock + childPositionLinearised];
            cell.level          = level;
            cell.parent         = parentCellIndex;
            cell.childrenBlock  = -1;
            int remainingDigits = childPositionLinearised;
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              cell.latticeIndex(axis) =
                _cells[parentCellIndex].latticeIndex(axis) * SubdivisionFactor + remainingDigits % SubdivisionFactor;
              remainingDigits /= SubdivisionFactor;
            }
            cell.ownerTreeId = getOwnerOfCellAt(level, cell.latticeIndex);
          }
        }
      }
    );
    // The cell, vertex, and face lattice maps write disjoint structures and
    // disjoint cell links, so the three fills run as concurrent tasks. Each
    // fill stays serial in visit order within its task, so every rank derives
    // the same numbering.
    const int                                       firstVertexOnLevel = static_cast<int>(_vertices.size());
    std::vector<tarch::la::Vector<DIMENSIONS, int>> latticeOfNewVertex;
    char                                            cellMapFilled{};
    char                                            vertexMapFilled{};
    char                                            faceMapFilled{};
    tarch::tasking::spawnTask(
      new tarch::tasking::FunctorTask([this, firstCellOnLevel, &cellIndexOfLattice]() {
        for (int cellIndex = firstCellOnLevel; cellIndex < static_cast<int>(_cells.size()); cellIndex++) {
          cellIndexOfLattice.insert(serialiseLatticeKey(_cells[cellIndex].latticeIndex), cellIndex);
        }
      }),
      {},
      {&cellMapFilled}
    );
    tarch::tasking::spawnTask(
      new tarch::tasking::FunctorTask([this,
                                       firstCellOnLevel,
                                       firstVertexOnLevel,
                                       cellsPerAxisOnLevel,
                                       &vertexIndexOfLattice,
                                       &latticeOfNewVertex]() {
        for (int cellIndex = firstCellOnLevel; cellIndex < static_cast<int>(_cells.size()); cellIndex++) {
          auto& cell = _cells[cellIndex];
          for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
            const auto vertexLattice =
              getCornerLatticeOfCell(cell.latticeIndex, vertexNumber, cellsPerAxisOnLevel, _periodicBoundaries);
            const auto [vertexIndex, isNewVertex] = vertexIndexOfLattice.insert(
              serialiseLatticeKey(vertexLattice),
              firstVertexOnLevel + static_cast<int>(latticeOfNewVertex.size())
            );
            if (isNewVertex) {
              latticeOfNewVertex.push_back(vertexLattice);
            }
            cell.vertexIndices[vertexNumber] = vertexIndex;
          }
        }
      }),
      {},
      {&vertexMapFilled}
    );
    tarch::tasking::spawnTask(
      new tarch::tasking::FunctorTask(
        [this, firstCellOnLevel, cellsPerAxisOnLevel, &faceIndexOfLattice, &numberOfFaces]() {
          for (int cellIndex = firstCellOnLevel; cellIndex < static_cast<int>(_cells.size()); cellIndex++) {
            auto& cell = _cells[cellIndex];
            for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
              const int normal      = faceNumber % DIMENSIONS;
              auto      faceLattice = cell.latticeIndex;
              if (faceNumber >= DIMENSIONS) {
                faceLattice(normal) += 1;
              }
              if (_periodicBoundaries[normal] and faceLattice(normal) == cellsPerAxisOnLevel) {
                faceLattice(normal) = 0;
              }
              const auto [faceIndex, isNewFace] =
                faceIndexOfLattice[normal].insert(serialiseLatticeKey(faceLattice), static_cast<int>(numberOfFaces));
              if (isNewFace) {
                numberOfFaces++;
              }
              cell.faceIndices[faceNumber] = faceIndex;
            }
          }
        }
      ),
      {},
      {&faceMapFilled}
    );

    // A vertex is persistent when every cell around it exists, so a refinement
    // transition leaves a hanging vertex beside every unrefined coarser cell.
    const auto isPersistentVertex = [&](const tarch::la::Vector<DIMENSIONS, int>& vertexLattice) {
      return everyAdjacentCellExists(
        vertexLattice,
        cellsPerAxisOnLevel,
        _periodicBoundaries,
        [&](const tarch::la::Vector<DIMENSIONS, int>& cellLattice) { return cellExistsInPlan(level, cellLattice); }
      );
    };

    {
      std::array<void*, 2> classifyDependencies{&cellMapFilled, &vertexMapFilled};
      tarch::tasking::waitForDependencies(classifyDependencies);
    }
    // The per-vertex classification runs in parallel over the unique
    // vertices, whose slots are disjoint.
    _vertices.resize(_vertices.size() + latticeOfNewVertex.size());
    peano4::topology::runChunkedTasks(
      static_cast<int>(latticeOfNewVertex.size()),
      [&](const int chunkNumber, const int begin, const int end) {
        static_cast<void>(chunkNumber);
        for (int newVertexNumber = begin; newVertexNumber < end; newVertexNumber++) {
          const auto&              vertexLattice = latticeOfNewVertex[newVertexNumber];
          peano4::grid::GridVertex vertex{};
          vertex.level         = level;
          const bool isFlagged = isFlaggedVertex(level, vertexLattice);
          // Up to the base depth the grid is dense, so every vertex is a real
          // grid vertex and only the plan's finest dense flags decide refinement.
          if (level > depth and not isPersistentVertex(vertexLattice)) {
            assertion(not isFlagged, level);
            vertex.setState(peano4::grid::GridVertex::State::HangingVertex);
          } else if (level < depth or isFlagged) {
            vertex.setState(peano4::grid::GridVertex::State::Refined);
          } else {
            vertex.setState(peano4::grid::GridVertex::State::Unrefined);
          }
#if PEANO_DEBUG
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            vertex.setX(axis, _domainOffset(axis) + vertexLattice(axis) * _domainWidth(axis) / cellsPerAxisOnLevel);
          }
#endif
          _vertices[firstVertexOnLevel + newVertexNumber] = vertex;
        }
      }
    );

    tarch::tasking::waitForDependency(&faceMapFilled);

    if (level < maxLevel) {
#if PEANO_ASSERTIONS
      if (level >= depth) {
        for (const auto& flaggedVertex : refinementPlan[level - depth]) {
          const auto flaggedVertexIndex = vertexIndexOfLattice.find(serialiseLatticeKey(flaggedVertex));
          // A partitioned rank materialises its own blocks and one ring around
          // them, so a flag outside that region has no vertex here. Where the
          // vertex does exist the plan has to have refined it.
          assertion(flaggedVertexIndex >= 0 or not _materialisesWholeDomain, level);
          if (flaggedVertexIndex >= 0) {
            assertion(_vertices[flaggedVertexIndex].getState() == peano4::grid::GridVertex::State::Refined, level);
          }
        }
      }
#endif
      std::vector<int> refinedCellsOnLevel;
      for (int cellIndex = firstCellOnLevel; cellIndex < static_cast<int>(_cells.size()); cellIndex++) {
        const auto& cell = _cells[cellIndex];
        for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
          if (_vertices[cell.vertexIndices[vertexNumber]].getState() == peano4::grid::GridVertex::State::Refined) {
            refinedCellsOnLevel.push_back(cellIndex);
            break;
          }
        }
      }
      refinedCells = std::move(refinedCellsOnLevel);
    }

    if (
      static_cast<std::int64_t>(_cells.size()) > std::numeric_limits<int>::max()
      or static_cast<std::int64_t>(_vertices.size()) > std::numeric_limits<int>::max()
      or numberOfFaces > std::numeric_limits<int>::max()
    ) {
      tarch::terminateWithError(
        "An adaptive grid over depth " + std::to_string(depth) + " exceeds the tree's 32-bit index range."
      );
    }
  }

  _cellLevelOffsets[maxLevel + 1]   = static_cast<int>(_cells.size());
  _vertexLevelOffsets[maxLevel + 1] = static_cast<int>(_vertices.size());
  _faceLevelOffsets[maxLevel + 1]   = static_cast<int>(numberOfFaces);
  _numberOfFaces                    = static_cast<int>(numberOfFaces);
}

/**
 * Derives a plan's refined-cell key tables over the whole domain. A base-depth
 * cell is refined when one of its corner vertices is flagged; a finer cell is
 * refined when its parent is refined and it carries a flagged corner itself.
 * The refinement query then answers with one key lookup.
 */
std::vector<peano4::grid::LatticeIndexMap>
peano4::topology::Tree::deriveRefinedCellKeyTables(
  const int                      depth,
  const std::bitset<DIMENSIONS>& periodicBoundaries,
  const RefinementPlan&          refinementPlan
) {
  std::vector<peano4::grid::LatticeIndexMap> refinedCellKeyTables(refinementPlan.size());
  if (refinementPlan.empty()) {
    return refinedCellKeyTables;
  }

  std::vector<peano4::grid::LatticeIndexMap> flaggedVertexKeys(refinementPlan.size());
  for (std::size_t planLevel = 0; planLevel < refinementPlan.size(); planLevel++) {
    flaggedVertexKeys[planLevel].reserve(refinementPlan[planLevel].size());
    for (const auto& flaggedVertex : refinementPlan[planLevel]) {
      flaggedVertexKeys[planLevel].insert(serialiseLatticeKey(flaggedVertex), 0);
    }
  }
  const auto hasFlaggedCorner =
    [&](const int level, const int cellsPerAxisOnLevel, const tarch::la::Vector<DIMENSIONS, int>& cellLattice) {
      for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
        const auto cornerLattice =
          getCornerLatticeOfCell(cellLattice, vertexNumber, cellsPerAxisOnLevel, periodicBoundaries);
        if (
          flaggedVertexKeys[level - depth].find(serialiseLatticeKey(cornerLattice))
          != peano4::grid::LatticeIndexMap::NoEntry
        ) {
          return true;
        }
      }
      return false;
    };

  const int cellsPerAxisOnBaseDepth = peano4::topology::getCellsPerAxis(depth);
  auto&     refinedOnBaseDepth      = refinedCellKeyTables[0];
  refinedOnBaseDepth.reserve(refinementPlan[0].size() * peano4::CellsPerVertex);
  std::vector<tarch::la::Vector<DIMENSIONS, int>> refinedOnCoarserLevel;
  for (const auto& flaggedVertex : refinementPlan[0]) {
    for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
      const auto adjacentCell =
        getAdjacentCellOfVertex(flaggedVertex, adjacencyEntry, cellsPerAxisOnBaseDepth, periodicBoundaries);
      if (
        adjacentCell.isInsideDomain and refinedOnBaseDepth.insert(serialiseLatticeKey(adjacentCell.lattice), 0).second
      ) {
        refinedOnCoarserLevel.push_back(adjacentCell.lattice);
      }
    }
  }

  for (std::size_t planLevel = 1; planLevel < refinementPlan.size(); planLevel++) {
    const int level               = depth + static_cast<int>(planLevel);
    const int cellsPerAxisOnLevel = peano4::topology::getCellsPerAxis(level);
    refinedCellKeyTables[planLevel].reserve(refinedOnCoarserLevel.size() * ChildrenPerCell);
    std::vector<tarch::la::Vector<DIMENSIONS, int>> refinedOnLevel;
    for (const auto& parentLattice : refinedOnCoarserLevel) {
      for (int childPositionLinearised = 0; childPositionLinearised < ChildrenPerCell; childPositionLinearised++) {
        tarch::la::Vector<DIMENSIONS, int> childLattice(0);
        int                                remainingDigits = childPositionLinearised;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          childLattice(axis) = parentLattice(axis) * SubdivisionFactor + remainingDigits % SubdivisionFactor;
          remainingDigits /= SubdivisionFactor;
        }
        if (hasFlaggedCorner(level, cellsPerAxisOnLevel, childLattice)) {
          refinedCellKeyTables[planLevel].insert(serialiseLatticeKey(childLattice), 0);
          refinedOnLevel.push_back(childLattice);
        }
      }
    }
    refinedOnCoarserLevel = std::move(refinedOnLevel);
  }
  return refinedCellKeyTables;
}

void
peano4::topology::Tree::derivePlanRefinedCellKeys(const RefinementPlan& refinementPlan) {
  _planRefinedCellKeys = deriveRefinedCellKeyTables(_depth, _periodicBoundaries, refinementPlan);
}

/** Fills the flag tables from the plan, one table per plan level. */
void
peano4::topology::Tree::assignBuildFlagKeys(const RefinementPlan& refinementPlan) {
  _buildFlagKeys.assign(refinementPlan.size(), {});
  for (std::size_t planLevel = 0; planLevel < refinementPlan.size(); planLevel++) {
    _buildFlagKeys[planLevel].reserve(refinementPlan[planLevel].size());
    for (const auto& flaggedVertex : refinementPlan[planLevel]) {
      _buildFlagKeys[planLevel].insert(serialiseLatticeKey(flaggedVertex), 0);
    }
  }
}

/**
 * Returns whether the cell at the given lattice position exists in the
 * structure the build plan describes. Dense levels exist throughout the
 * domain; a finer cell exists when its parent cell is refined, so one lookup
 * in the refined cell keys answers.
 */
bool
peano4::topology::Tree::cellExistsInPlan(const int level, const tarch::la::Vector<DIMENSIONS, int>& cellLattice) const {
  if (level <= _depth) {
    return true;
  }
  tarch::la::Vector<DIMENSIONS, int> parentLattice(0);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    parentLattice(axis) = cellLattice(axis) / SubdivisionFactor;
  }
  return isCellRefinedInPlan(level - 1, parentLattice);
}

/**
 * Returns whether the cell at the given lattice position exists and carries a
 * flagged corner. buildRegularGrid() refines exactly those cells.
 */
bool
peano4::topology::Tree::isCellRefinedInPlan(
  const int                                 level,
  const tarch::la::Vector<DIMENSIONS, int>& cellLattice
) const {
  if (level < _depth) {
    return false;
  }
  const auto planLevel = static_cast<std::size_t>(level - _depth);
  return planLevel < _planRefinedCellKeys.size()
    and _planRefinedCellKeys[planLevel].find(serialiseLatticeKey(cellLattice))
    != peano4::grid::LatticeIndexMap::NoEntry;
}

/**
 * Returns whether the vertex at the given position is a real grid vertex of
 * the structure the build plan describes. A vertex is persistent when a cell
 * exists at every neighbour position inside the domain; a vertex beside an
 * unrefined coarser cell hangs.
 */
bool
peano4::topology::Tree::isVertexPersistentInPlan(
  const int                                 level,
  const tarch::la::Vector<DIMENSIONS, int>& vertexLattice
) const {
  if (level <= _depth) {
    return true;
  }
  return everyAdjacentCellExists(
    vertexLattice,
    peano4::topology::getCellsPerAxis(level),
    _periodicBoundaries,
    [&](const tarch::la::Vector<DIMENSIONS, int>& cellLattice) { return cellExistsInPlan(level, cellLattice); }
  );
}

/**
 * Returns whether a coarse vertex the given vertex lies between is refined,
 * which marks the region the refinement plan covers. A vertex between refined
 * cells is persistent without a flagged governing vertex, so its state alone
 * does not separate the plan's rim from its interior.
 */
bool
peano4::topology::Tree::hasRefinedGoverningVertex(
  const int                                 level,
  const tarch::la::Vector<DIMENSIONS, int>& vertexLattice
) const {
  if (level <= _depth) {
    return true;
  }
  return anyGoverningVertexSatisfies(
    vertexLattice,
    peano4::topology::getCellsPerAxis(level - 1),
    _periodicBoundaries,
    [&](const tarch::la::Vector<DIMENSIONS, int>& coarseVertexLattice) {
      return isVertexRefinedInPlan(level - 1, coarseVertexLattice);
    }
  );
}

/**
 * Derives the vertices the surroundedness rule closes.
 *
 * A vertex is surrounded when it is unrefined and every neighbour cell that
 * exists is refined. A refined cell carries a flagged corner, so every
 * candidate is a corner of a cell around one of the build plan's flags, and
 * the derivation runs over the flag sets.
 */
void
peano4::topology::Tree::deriveSurroundedVertices() const {
  if (_surroundedVerticesDerived) {
    return;
  }

  _surroundedVertices.assign(_buildFlagKeys.size(), {});

  for (std::size_t planLevel = 0; planLevel < _buildFlagKeys.size(); planLevel++) {
    const int level               = _depth + static_cast<int>(planLevel);
    const int cellsPerAxisOnLevel = getCellsPerAxisOfLevel(level);

    const auto forEachAdjacentCell = [&](const tarch::la::Vector<DIMENSIONS, int>& vertexLattice, const auto& visit) {
      for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
        const auto adjacentCell =
          getAdjacentCellOfVertex(vertexLattice, adjacencyEntry, cellsPerAxisOnLevel, _periodicBoundaries);
        if (adjacentCell.isInsideDomain) {
          visit(adjacentCell.lattice);
        }
      }
    };

    // The refined cells of the level are the plan's refined-cell keys, so
    // their corners are the candidate vertices; a vertex further from every
    // flag has an unrefined neighbour.
    const auto&               refinedCellKeys = _planRefinedCellKeys[planLevel];
    std::vector<std::int64_t> candidateVertexKeys;
    candidateVertexKeys.reserve(refinedCellKeys.getSize() * peano4::VerticesPerCell);
    refinedCellKeys.forEachEntry([&](const std::int64_t refinedCellKey, const int unusedValue) {
      static_cast<void>(unusedValue);
      const auto cellLattice = deserialiseLatticeKey(refinedCellKey);
      for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
        candidateVertexKeys.push_back(serialiseLatticeKey(
          getCornerLatticeOfCell(cellLattice, vertexNumber, cellsPerAxisOnLevel, _periodicBoundaries)
        ));
      }
    });
    std::sort(candidateVertexKeys.begin(), candidateVertexKeys.end());
    candidateVertexKeys.erase(
      std::unique(candidateVertexKeys.begin(), candidateVertexKeys.end()),
      candidateVertexKeys.end()
    );

    // Every candidate decides independently from read-only tables, so the
    // checks run as chunked tasks.
    std::vector<char> candidateIsSurrounded(candidateVertexKeys.size(), 0);
    peano4::topology::runChunkedTasks(
      static_cast<int>(candidateVertexKeys.size()),
      [&](const int chunkNumber, const int begin, const int end) {
        static_cast<void>(chunkNumber);
        for (int candidateNumber = begin; candidateNumber < end; candidateNumber++) {
          const auto vertexLattice = deserialiseLatticeKey(candidateVertexKeys[candidateNumber]);
          if (isVertexRefinedInPlan(level, vertexLattice) or not hasRefinedGoverningVertex(level, vertexLattice)) {
            continue;
          }
          bool surrounded            = true;
          int  numberOfExistingCells = 0;
          forEachAdjacentCell(vertexLattice, [&](const tarch::la::Vector<DIMENSIONS, int>& cellLattice) {
            if (not cellExistsInPlan(level, cellLattice)) {
              return;
            }
            numberOfExistingCells++;
            surrounded = surrounded
              and refinedCellKeys.find(serialiseLatticeKey(cellLattice)) != peano4::grid::LatticeIndexMap::NoEntry;
          });
          if (surrounded and numberOfExistingCells > 0) {
            candidateIsSurrounded[candidateNumber] = 1;
          }
        }
      }
    );
    for (std::size_t candidateNumber = 0; candidateNumber < candidateVertexKeys.size(); candidateNumber++) {
      if (candidateIsSurrounded[candidateNumber] != 0) {
        _surroundedVertices[planLevel].push_back(candidateVertexKeys[candidateNumber]);
      }
    }
  }

  _surroundedVerticesDerived = true;
}

/**
 * Updates the refinement plan by one evaluation round of the given refinement
 * requests against the current structure, and returns whether the plan changed.
 *
 * A refine request covers the cell that raised it together with one full cell
 * of every neighbour, on the requester's own level and on every coarser one,
 * and each covered cell that exists flags its still unrefined corner vertices.
 * Every unrefined vertex whose existing neighbour cells are all refined is
 * flagged too, and a flagged vertex unflags when every one of its neighbour
 * cells is named by an erase request and by no refine request, no flagged
 * vertex one level finer sits within its restriction neighbourhood, and at
 * least one of those neighbour cells loses its last flag.
 */
bool
peano4::topology::Tree::updateRefinementPlan(
  const std::vector<peano4::RefinementRequest>& requests,
  RefinementPlan&                               refinementPlan
) const {
  bool hasRefineRequests = false;
  bool hasEraseRequests  = false;
  for (const auto& request : requests) {
    if (request.getRefinementControl() == peano4::RefinementRequest::RefinementControl::Refine) {
      hasRefineRequests = true;
    } else {
      hasEraseRequests = true;
    }
  }
  if (not hasRefineRequests and not hasEraseRequests) {
    return false;
  }

  const int maxLevel = _depth + static_cast<int>(refinementPlan.size());

  // The build flag tables hold the given plan's flags, so the round records
  // its delta only: The added flags per level, with an extra entry for a
  // level the plan has not reached yet, and the removed flags per level.
  assertion(_buildFlagKeys.size() == refinementPlan.size(), _buildFlagKeys.size(), refinementPlan.size());
  for (std::size_t planLevel = 0; planLevel < refinementPlan.size(); planLevel++) {
    assertion(
      _buildFlagKeys[planLevel].getSize() == refinementPlan[planLevel].size(),
      planLevel,
      _buildFlagKeys[planLevel].getSize(),
      refinementPlan[planLevel].size()
    );
  }
  const auto          numberOfFlagLevels = refinementPlan.size() + 1;
  FlaggedKeysPerLevel addedKeys(numberOfFlagLevels);
  FlaggedKeysPerLevel removedKeysPerLevel(numberOfFlagLevels);
  const auto          isFlagged = [&](const int level, const std::int64_t vertexKey) {
    const auto planLevel = static_cast<std::size_t>(level - _depth);
    assertion(planLevel < numberOfFlagLevels, level, _depth, numberOfFlagLevels);
    return (planLevel < _buildFlagKeys.size()
            and _buildFlagKeys[planLevel].find(vertexKey) != peano4::grid::LatticeIndexMap::NoEntry)
      or addedKeys[planLevel].contains(vertexKey);
  };
  const auto flagVertex = [&](const int level, const tarch::la::Vector<DIMENSIONS, int>& vertexLattice) {
    assertion(not isVertexRefinedInPlan(level, vertexLattice), level);
    return addedKeys[level - _depth].insert(serialiseLatticeKey(vertexLattice)).second;
  };

  const auto vertexLatticeOfCorner =
    [this](const int level, const tarch::la::Vector<DIMENSIONS, int>& cellLattice, const int vertexNumber) {
      return getCornerLatticeOfCell(
        cellLattice,
        vertexNumber,
        peano4::topology::getCellsPerAxis(level),
        _periodicBoundaries
      );
    };

  bool changed = false;

  // The cells a refine request covers, indexed by level - depth. The erase
  // direction consults them, because a cell a request refines never coarsens.
  FlaggedKeysPerLevel refiningCells(numberOfFlagLevels);

  for (const auto& request : requests) {
    if (request.getRefinementControl() != peano4::RefinementRequest::RefinementControl::Refine) {
      continue;
    }
    // A level finer than the requester carries cells smaller than the width the
    // request names, and no cell of the band is that small.
    const int finestLevelInBand = std::min(request.level, maxLevel);
    for (int level = _depth; level <= finestLevelInBand; level++) {
      const int cellsPerAxisOnLevel = peano4::topology::getCellsPerAxis(level);
      const int cellsPerBandCell    = peano4::topology::getCellsPerAxis(request.level - level);

      tarch::la::Vector<DIMENSIONS, int> firstCell(0);
      tarch::la::Vector<DIMENSIONS, int> numberOfCellsPerAxis(0);
      int                                numberOfCandidates = 1;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        // The band is the three cells i-1 to i+1 of the requester's own level, so
        // on a coarser level it is the cells that contain its two end members. A
        // periodic axis keeps the full width and wraps the candidates below, so a
        // request beside the wrap covers the same ring as one in the interior.
        int lowestCandidate  = floorDivide(request.latticeIndex(axis) - 1, cellsPerBandCell);
        int highestCandidate = floorDivide(request.latticeIndex(axis) + 1, cellsPerBandCell);
        if (not _periodicBoundaries[axis]) {
          lowestCandidate  = std::max(lowestCandidate, 0);
          highestCandidate = std::min(highestCandidate, cellsPerAxisOnLevel - 1);
        }
        firstCell(axis)            = lowestCandidate;
        numberOfCellsPerAxis(axis) = std::max(highestCandidate - lowestCandidate + 1, 0);
        numberOfCandidates *= numberOfCellsPerAxis(axis);
      }

      for (int candidate = 0; candidate < numberOfCandidates; candidate++) {
        tarch::la::Vector<DIMENSIONS, int> cellLattice(0);
        int                                remainingIndex = candidate;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          int coordinate = firstCell(axis) + remainingIndex % numberOfCellsPerAxis(axis);
          if (_periodicBoundaries[axis]) {
            coordinate = (coordinate % cellsPerAxisOnLevel + cellsPerAxisOnLevel) % cellsPerAxisOnLevel;
          }
          cellLattice(axis) = coordinate;
          remainingIndex /= numberOfCellsPerAxis(axis);
        }
        if (not cellExistsInPlan(level, cellLattice)) {
          continue;
        }
        if (hasEraseRequests) {
          refiningCells[level - _depth].insert(serialiseLatticeKey(cellLattice));
        }
        for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
          const auto cornerLattice = vertexLatticeOfCorner(level, cellLattice, vertexNumber);
          if (not isVertexRefinedInPlan(level, cornerLattice) and hasRefinedGoverningVertex(level, cornerLattice)) {
            changed = flagVertex(level, cornerLattice) or changed;
          }
        }
      }
    }
  }

  // Surroundedness counts against the built structure, so a vertex flagged
  // this round does not refine its neighbourhood before its own refinement
  // exists.
  deriveSurroundedVertices();
  assertion(_surroundedVertices.size() < numberOfFlagLevels, _surroundedVertices.size(), numberOfFlagLevels);
  for (std::size_t planLevel = 0; planLevel < _surroundedVertices.size(); planLevel++) {
    for (const auto surroundedKey : _surroundedVertices[planLevel]) {
      assertion(
        _buildFlagKeys[planLevel].find(surroundedKey) == peano4::grid::LatticeIndexMap::NoEntry,
        planLevel,
        surroundedKey
      );
      changed = addedKeys[planLevel].insert(surroundedKey).second or changed;
    }
  }

  if (hasEraseRequests) {
    // An erase request has been promoted to the parent cell it coarsens away,
    // so it names exactly the cell whose corner flags have to go.
    FlaggedKeysPerLevel erasingCells(numberOfFlagLevels);
    for (const auto& request : requests) {
      if (request.getRefinementControl() != peano4::RefinementRequest::RefinementControl::Erase) {
        continue;
      }
      const int planLevel = request.level - _depth;
      if (planLevel >= 0 and planLevel < static_cast<int>(erasingCells.size())) {
        erasingCells[planLevel].insert(serialiseLatticeKey(request.latticeIndex));
      }
    }

    const auto wantsErase = [&](const int level, const tarch::la::Vector<DIMENSIONS, int>& cellLattice) {
      const auto cellKey   = serialiseLatticeKey(cellLattice);
      const int  planLevel = level - _depth;
      return erasingCells[planLevel].contains(cellKey) and not refiningCells[planLevel].contains(cellKey);
    };

    // Only a corner vertex of an erasing cell can lose its flag, because the
    // removal needs every neighbour cell to erase.
    struct EraseCandidate {
      int          level{0};
      std::int64_t vertexKey{0};
    };
    std::vector<EraseCandidate> candidates;
    for (std::size_t planLevel = 0; planLevel < erasingCells.size(); planLevel++) {
      const int                        level               = _depth + static_cast<int>(planLevel);
      const int                        cellsPerAxisOnLevel = peano4::topology::getCellsPerAxis(level);
      std::unordered_set<std::int64_t> candidateKeysOnLevel;
      for (const auto erasingCellKey : erasingCells[planLevel]) {
        const auto cellLattice = deserialiseLatticeKey(erasingCellKey);
        for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
          const auto vertexKey = serialiseLatticeKey(
            getCornerLatticeOfCell(cellLattice, vertexNumber, cellsPerAxisOnLevel, _periodicBoundaries)
          );
          if (isFlagged(level, vertexKey) and candidateKeysOnLevel.insert(vertexKey).second) {
            candidates.push_back({level, vertexKey});
          }
        }
      }
    }

    // The removals evaluate against the snapshot of this round's flags and
    // apply together, so the outcome does not depend on the visit order, and
    // every candidate decides independently from read-only tables, so the
    // candidates of every level run as one loop of chunked tasks.
    std::vector<char> candidateIsRemoved(candidates.size(), 0);
    peano4::topology::runChunkedTasks(
      static_cast<int>(candidates.size()),
      [&](const int chunkNumber, const int begin, const int end) {
        static_cast<void>(chunkNumber);
        for (int candidateNumber = begin; candidateNumber < end; candidateNumber++) {
          const int  level               = candidates[candidateNumber].level;
          const int  cellsPerAxisOnLevel = peano4::topology::getCellsPerAxis(level);
          const auto vertexLattice       = deserialiseLatticeKey(candidates[candidateNumber].vertexKey);

          // The veto: Refined structure one level finer still needs this
          // vertex while a flagged vertex sits in the fine neighbourhood it
          // governs.
          bool       vetoed          = false;
          const auto finerPlanLevel  = static_cast<std::size_t>(level - _depth) + 1;
          const bool finerLevelFlags = finerPlanLevel < numberOfFlagLevels
            and ((finerPlanLevel < _buildFlagKeys.size() and _buildFlagKeys[finerPlanLevel].getSize() > 0)
                 or not addedKeys[finerPlanLevel].empty());
          if (finerLevelFlags) {
            // The flags strictly inside the adjacent cells, k-1 fine cells on
            // either side of the vertex along every axis for the subdivision
            // factor k.
            constexpr int                      GovernedWidth = 2 * SubdivisionFactor - 1;
            tarch::la::Vector<DIMENSIONS, int> fineNeighbourhoodLattice(0);
            int                                numberOfPositions = 1;
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              numberOfPositions *= GovernedWidth;
            }
            for (int positionNumber = 0; positionNumber < numberOfPositions and not vetoed; positionNumber++) {
              int  remainingIndex = positionNumber;
              bool isValid        = true;
              for (int axis = 0; axis < DIMENSIONS; axis++) {
                int fineCoordinate =
                  SubdivisionFactor * vertexLattice(axis) + remainingIndex % GovernedWidth - (SubdivisionFactor - 1);
                remainingIndex /= GovernedWidth;
                if (_periodicBoundaries[axis]) {
                  const int fineCellsPerAxis = SubdivisionFactor * cellsPerAxisOnLevel;
                  fineCoordinate             = (fineCoordinate + fineCellsPerAxis) % fineCellsPerAxis;
                } else {
                  isValid =
                    isValid and fineCoordinate >= 0 and fineCoordinate <= SubdivisionFactor * cellsPerAxisOnLevel;
                }
                fineNeighbourhoodLattice(axis) = fineCoordinate;
              }
              vetoed = isValid and isFlagged(level + 1, serialiseLatticeKey(fineNeighbourhoodLattice));
            }
          }
          if (vetoed) {
            continue;
          }

          bool everyNeighbourCellErases = true;
          for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex and everyNeighbourCellErases;
               adjacencyEntry++) {
            const auto adjacentCell =
              getAdjacentCellOfVertex(vertexLattice, adjacencyEntry, cellsPerAxisOnLevel, _periodicBoundaries);
            if (not adjacentCell.isInsideDomain or not cellExistsInPlan(level, adjacentCell.lattice)) {
              continue;
            }
            everyNeighbourCellErases = wantsErase(level, adjacentCell.lattice);
          }
          if (everyNeighbourCellErases) {
            candidateIsRemoved[candidateNumber] = 1;
          }
        }
      }
    );
    for (std::size_t candidateNumber = 0; candidateNumber < candidates.size(); candidateNumber++) {
      if (candidateIsRemoved[candidateNumber] != 0) {
        removedKeysPerLevel[candidates[candidateNumber].level - _depth].insert(candidates[candidateNumber].vertexKey);
      }
    }

    // A removal that coarsens no cell stays out: With every neighbour cell
    // refined through another flag, the finer vertices along this one would
    // hang between two persistent cells. A kept flag keeps its neighbour
    // cells refined, so the pass repeats until no further flag is kept.
    for (int level = _depth; level <= maxLevel; level++) {
      const int  cellsPerAxisOnLevel = peano4::topology::getCellsPerAxis(level);
      auto&      removedKeys         = removedKeysPerLevel[level - _depth];
      const auto cellKeepsAFlag      = [&](const tarch::la::Vector<DIMENSIONS, int>& cellLattice) {
        for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
          const auto cornerKey = serialiseLatticeKey(
            getCornerLatticeOfCell(cellLattice, vertexNumber, cellsPerAxisOnLevel, _periodicBoundaries)
          );
          if (isFlagged(level, cornerKey) and not removedKeys.contains(cornerKey)) {
            return true;
          }
        }
        return false;
      };
      bool keptAFlag = true;
      while (keptAFlag) {
        keptAFlag = false;
        for (auto removedKey = removedKeys.begin(); removedKey != removedKeys.end();) {
          const auto vertexLattice                = deserialiseLatticeKey(*removedKey);
          bool       everyNeighbourCellKeepsAFlag = true;
          for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex and everyNeighbourCellKeepsAFlag;
               adjacencyEntry++) {
            const auto adjacentCell =
              getAdjacentCellOfVertex(vertexLattice, adjacencyEntry, cellsPerAxisOnLevel, _periodicBoundaries);
            if (adjacentCell.isInsideDomain and cellExistsInPlan(level, adjacentCell.lattice)) {
              everyNeighbourCellKeepsAFlag = cellKeepsAFlag(adjacentCell.lattice);
            }
          }
          if (everyNeighbourCellKeepsAFlag) {
            removedKey = removedKeys.erase(removedKey);
            keptAFlag  = true;
          } else {
            ++removedKey;
          }
        }
      }
      changed = changed or not removedKeys.empty();
    }
  }

  if (not changed) {
    return false;
  }

  // Every plan level is sorted by lattice key, so a level with a delta
  // rebuilds by one sorted merge of the standing flags and the added flags
  // that skips the removed flags, and every rank derives the same plan. A
  // level without a delta stays. Trailing empty levels drop, so the maximum
  // level tracks the erase.
  RefinementPlan updatedPlan;
  updatedPlan.reserve(numberOfFlagLevels);
  for (std::size_t planLevel = 0; planLevel < numberOfFlagLevels; planLevel++) {
    auto&       updatedLevel = updatedPlan.emplace_back();
    const auto& removedKeys  = removedKeysPerLevel[planLevel];
    const bool  hasStanding  = planLevel < refinementPlan.size();
    if (addedKeys[planLevel].empty() and removedKeys.empty()) {
      if (hasStanding) {
        updatedLevel = std::move(refinementPlan[planLevel]);
      }
      continue;
    }
    std::vector<std::int64_t> sortedAddedKeys(addedKeys[planLevel].begin(), addedKeys[planLevel].end());
    std::sort(sortedAddedKeys.begin(), sortedAddedKeys.end());
    const std::vector<tarch::la::Vector<DIMENSIONS, int>> emptyLevel;
    const auto& standingLevel = hasStanding ? refinementPlan[planLevel] : emptyLevel;
    const bool  hasRemovals   = not removedKeys.empty();
    updatedLevel.reserve(standingLevel.size() + sortedAddedKeys.size());
    std::size_t addedNumber = 0;
    const auto  appendAdded = [&](const std::int64_t addedKey) {
      if (not hasRemovals or not removedKeys.contains(addedKey)) {
        updatedLevel.push_back(deserialiseLatticeKey(addedKey));
      }
    };
#if PEANO_ASSERTIONS
    std::int64_t previousStandingKey = -1;
#endif
    for (const auto& standingVertex : standingLevel) {
      const auto standingKey = serialiseLatticeKey(standingVertex);
#if PEANO_ASSERTIONS
      assertion(previousStandingKey < standingKey, planLevel, previousStandingKey, standingKey);
      previousStandingKey = standingKey;
#endif
      for (; addedNumber < sortedAddedKeys.size() and sortedAddedKeys[addedNumber] < standingKey; addedNumber++) {
        appendAdded(sortedAddedKeys[addedNumber]);
      }
      assertion(addedNumber == sortedAddedKeys.size() or sortedAddedKeys[addedNumber] != standingKey, standingKey);
      if (not hasRemovals or not removedKeys.contains(standingKey)) {
        updatedLevel.push_back(standingVertex);
      }
    }
    for (; addedNumber < sortedAddedKeys.size(); addedNumber++) {
      appendAdded(sortedAddedKeys[addedNumber]);
    }
  }
  while (not updatedPlan.empty() and updatedPlan.back().empty()) {
    updatedPlan.pop_back();
  }
  refinementPlan = std::move(updatedPlan);
  return true;
}

std::vector<int>
peano4::topology::Tree::getOwnersOfBlocks() const {
  if (_topologyNodes.empty()) {
    return {};
  }
  constexpr int    OwnershipPlanHeaderSize = 2;
  constexpr int    TopologyNodeStride      = 3;
  std::vector<int> ownershipPlan(OwnershipPlanHeaderSize + _topologyNodes.size() * TopologyNodeStride, 0);
  ownershipPlan[0] = OwnershipPlanMarker;
  ownershipPlan[1] = static_cast<int>(_topologyNodes.size());
  for (int nodeIndex = 0; nodeIndex < static_cast<int>(_topologyNodes.size()); nodeIndex++) {
    const auto planOffset         = OwnershipPlanHeaderSize + nodeIndex * TopologyNodeStride;
    ownershipPlan[planOffset]     = _topologyNodes[nodeIndex].firstOwner;
    ownershipPlan[planOffset + 1] = _topologyNodes[nodeIndex].lastOwner;
    ownershipPlan[planOffset + 2] = _topologyNodes[nodeIndex].childrenBlock;
  }
  return ownershipPlan;
}

/**
 * Derives the refined cells of every level from the plan and counts the
 * leaves every refinement adds below its cell, accumulated up to the root.
 */
peano4::topology::Tree::PlanLeafWeights
peano4::topology::Tree::derivePlanLeafWeights(const int depth, const RefinementPlan& refinementPlan) const {
  const int       maximumLevel = validateAndGetMaximumLevel(depth, refinementPlan);
  PlanLeafWeights weights;
  weights.depth = depth;

  std::vector<peano4::grid::LatticeIndexMap> flaggedVertexKeys(refinementPlan.size());
  for (std::size_t planLevel = 0; planLevel < refinementPlan.size(); planLevel++) {
    flaggedVertexKeys[planLevel].reserve(refinementPlan[planLevel].size());
    for (const auto& flaggedVertex : refinementPlan[planLevel]) {
      flaggedVertexKeys[planLevel].insert(serialiseLatticeKey(flaggedVertex), 0);
    }
  }

  auto& refinedCellKeys = weights.refinedCellKeys;
  refinedCellKeys.resize(maximumLevel + 1);
  std::vector<std::vector<tarch::la::Vector<DIMENSIONS, int>>> refinedCells(maximumLevel + 1);
  const int cellsPerAxisOnBaseDepth = peano4::topology::getCellsPerAxis(depth);
  if (not refinementPlan.empty()) {
    for (const auto& flaggedVertex : refinementPlan[0]) {
      for (int adjacencyEntry = 0; adjacencyEntry < peano4::CellsPerVertex; adjacencyEntry++) {
        const auto adjacentCell =
          getAdjacentCellOfVertex(flaggedVertex, adjacencyEntry, cellsPerAxisOnBaseDepth, _periodicBoundaries);
        if (
          adjacentCell.isInsideDomain
          and refinedCellKeys[depth].insert(serialiseLatticeKey(adjacentCell.lattice), 0).second
        ) {
          refinedCells[depth].push_back(adjacentCell.lattice);
        }
      }
    }
  }

  for (int level = depth + 1; level < maximumLevel and not refinedCells[level - 1].empty(); level++) {
    const int   cellsPerAxisOnLevel = peano4::topology::getCellsPerAxis(level);
    const auto& flagsOnLevel        = flaggedVertexKeys[level - depth];
    for (const auto& parentLattice : refinedCells[level - 1]) {
      for (int childPositionLinearised = 0; childPositionLinearised < ChildrenPerCell; childPositionLinearised++) {
        tarch::la::Vector<DIMENSIONS, int> childLattice(0);
        int                                remainingDigits = childPositionLinearised;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          childLattice(axis) = parentLattice(axis) * SubdivisionFactor + remainingDigits % SubdivisionFactor;
          remainingDigits /= SubdivisionFactor;
        }
        bool childIsRefined = false;
        for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell and not childIsRefined; vertexNumber++) {
          const auto vertexLattice =
            getCornerLatticeOfCell(childLattice, vertexNumber, cellsPerAxisOnLevel, _periodicBoundaries);
          childIsRefined =
            flagsOnLevel.find(serialiseLatticeKey(vertexLattice)) != peano4::grid::LatticeIndexMap::NoEntry;
        }
        if (childIsRefined) {
          refinedCellKeys[level].insert(serialiseLatticeKey(childLattice), 0);
          refinedCells[level].push_back(childLattice);
        }
      }
    }
  }

  auto& adaptiveLeafDeltas = weights.adaptiveLeafDeltas;
  adaptiveLeafDeltas.resize(maximumLevel + 1);
  for (int level = depth; level < maximumLevel; level++) {
    for (const auto& refinedCell : refinedCells[level]) {
      adaptiveLeafDeltas[level][serialiseLatticeKey(refinedCell)] += ChildrenPerCell - 1;
    }
  }
  for (int level = maximumLevel; level > 0; level--) {
    for (const auto& [cellKey, leafDelta] : adaptiveLeafDeltas[level]) {
      auto parentLattice = deserialiseLatticeKey(cellKey);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        parentLattice(axis) /= SubdivisionFactor;
      }
      adaptiveLeafDeltas[level - 1][serialiseLatticeKey(parentLattice)] += leafDelta;
    }
  }

  auto& regularLeafWeights = weights.regularLeafWeights;
  regularLeafWeights.assign(depth + 1, 1);
  for (int level = depth - 1; level >= 0; level--) {
    if (regularLeafWeights[level + 1] > std::numeric_limits<int>::max() / ChildrenPerCell) {
      tarch::terminateWithError("The refinement plan exceeds the tree's 32-bit index range.");
    }
    regularLeafWeights[level] = regularLeafWeights[level + 1] * ChildrenPerCell;
  }
  return weights;
}

/** Returns the number of leaves below the cell at the given level and lattice position. */
std::int64_t
peano4::topology::Tree::PlanLeafWeights::getLeafWeight(
  const int                                 level,
  const tarch::la::Vector<DIMENSIONS, int>& latticeIndex
) const {
  assertion(level >= 0 and level < static_cast<int>(adaptiveLeafDeltas.size()), level, adaptiveLeafDeltas.size());
  auto       leafWeight = level < depth ? regularLeafWeights[level] : std::int64_t{1};
  const auto leafDelta  = adaptiveLeafDeltas[level].find(serialiseLatticeKey(latticeIndex));
  if (leafDelta != adaptiveLeafDeltas[level].end()) {
    leafWeight += leafDelta->second;
  }
  return leafWeight;
}

/**
 * Returns the number of leaves every rank owns under the given ownership
 * plan and refinement plan. A topology node one rank owns adds the leaves
 * below its cell to that rank, and a node several ranks own descends into
 * its children.
 */
std::vector<std::int64_t>
peano4::topology::Tree::deriveRankLoads(
  const int               depth,
  const int               numberOfRanks,
  const RefinementPlan&   refinementPlan,
  const std::vector<int>& ownershipPlan
) const {
  assertion(numberOfRanks >= 1, numberOfRanks);
  constexpr int OwnershipPlanHeaderSize = 2;
  constexpr int TopologyNodeStride      = 3;
  if (
    ownershipPlan.size() < OwnershipPlanHeaderSize or ownershipPlan[0] != OwnershipPlanMarker or ownershipPlan[1] < 1
    or ownershipPlan.size() != OwnershipPlanHeaderSize + static_cast<std::size_t>(ownershipPlan[1]) * TopologyNodeStride
  ) {
    tarch::terminateWithError("The rank ownership plan is invalid.");
  }
  const int  numberOfTopologyNodes = ownershipPlan[1];
  const auto planLeafWeights       = derivePlanLeafWeights(depth, refinementPlan);

  std::vector<std::int64_t> loads(numberOfRanks, 0);
  const auto                accumulate = [&](
                                           const auto&                               recurse,
                                           const int                                 nodeIndex,
                                           const int                                 level,
                                           const tarch::la::Vector<DIMENSIONS, int>& latticeIndex
                                         ) -> void {
    const auto planOffset    = OwnershipPlanHeaderSize + nodeIndex * TopologyNodeStride;
    const int  firstOwner    = ownershipPlan[planOffset];
    const int  lastOwner     = ownershipPlan[planOffset + 1];
    const int  childrenBlock = ownershipPlan[planOffset + 2];
    if (firstOwner < 0 or firstOwner > lastOwner or lastOwner >= numberOfRanks) {
      tarch::terminateWithError("The rank ownership plan is invalid.");
    }
    // An ownership plan derived from a deeper refinement plan splits a cell
    // that is a leaf under this plan, and the first rank of the split owns
    // that leaf.
    const auto leafWeight = planLeafWeights.getLeafWeight(level, latticeIndex);
    if (firstOwner == lastOwner or leafWeight == 1) {
      loads[firstOwner] += leafWeight;
      return;
    }
    if (childrenBlock <= nodeIndex or childrenBlock > numberOfTopologyNodes - ChildrenPerCell) {
      tarch::terminateWithError("The rank ownership plan is invalid.");
    }
    for (int childPositionLinearised = 0; childPositionLinearised < ChildrenPerCell; childPositionLinearised++) {
      tarch::la::Vector<DIMENSIONS, int> childLattice(0);
      int                                remainingDigits = childPositionLinearised;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        childLattice(axis) = latticeIndex(axis) * SubdivisionFactor + remainingDigits % SubdivisionFactor;
        remainingDigits /= SubdivisionFactor;
      }
      recurse(recurse, childrenBlock + childPositionLinearised, level + 1, childLattice);
    }
  };
  accumulate(accumulate, 0, 0, tarch::la::Vector<DIMENSIONS, int>(0));
  return loads;
}

/**
 * Returns whether the ownership plan splits a cell below the spawn level
 * whose children include a leaf under the refinement plan. Such children
 * coarsen together and restrict onto one coarse face, so they stay on one
 * rank.
 */
bool
peano4::topology::Tree::ownershipPlanSplitsCellWithLeafChild(
  const int               depth,
  const int               spawnLevel,
  const RefinementPlan&   refinementPlan,
  const std::vector<int>& ownershipPlan
) const {
  constexpr int OwnershipPlanHeaderSize = 2;
  constexpr int TopologyNodeStride      = 3;
  if (
    ownershipPlan.size() < OwnershipPlanHeaderSize or ownershipPlan[0] != OwnershipPlanMarker or ownershipPlan[1] < 1
    or ownershipPlan.size() != OwnershipPlanHeaderSize + static_cast<std::size_t>(ownershipPlan[1]) * TopologyNodeStride
  ) {
    tarch::terminateWithError("The rank ownership plan is invalid.");
  }
  const int  numberOfTopologyNodes = ownershipPlan[1];
  const auto planLeafWeights       = derivePlanLeafWeights(depth, refinementPlan);

  const auto splitsSiblingLeaves = [&](
                                     const auto&                               recurse,
                                     const int                                 nodeIndex,
                                     const int                                 level,
                                     const tarch::la::Vector<DIMENSIONS, int>& latticeIndex
                                   ) -> bool {
    const auto planOffset    = OwnershipPlanHeaderSize + nodeIndex * TopologyNodeStride;
    const int  firstOwner    = ownershipPlan[planOffset];
    const int  lastOwner     = ownershipPlan[planOffset + 1];
    const int  childrenBlock = ownershipPlan[planOffset + 2];
    if (firstOwner == lastOwner) {
      return false;
    }
    if (childrenBlock <= nodeIndex or childrenBlock > numberOfTopologyNodes - ChildrenPerCell) {
      tarch::terminateWithError("The rank ownership plan is invalid.");
    }
    for (int childPositionLinearised = 0; childPositionLinearised < ChildrenPerCell; childPositionLinearised++) {
      tarch::la::Vector<DIMENSIONS, int> childLattice(0);
      int                                remainingDigits = childPositionLinearised;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        childLattice(axis) = latticeIndex(axis) * SubdivisionFactor + remainingDigits % SubdivisionFactor;
        remainingDigits /= SubdivisionFactor;
      }
      if (
        (level + 1 >= depth and level > spawnLevel and planLeafWeights.getLeafWeight(level + 1, childLattice) == 1)
        or recurse(recurse, childrenBlock + childPositionLinearised, level + 1, childLattice)
      ) {
        return true;
      }
    }
    return false;
  };
  return splitsSiblingLeaves(splitsSiblingLeaves, 0, 0, tarch::la::Vector<DIMENSIONS, int>(0));
}

/**
 * Returns the level whose cells' children give every rank enough ownership
 * units, bounded by one level above the target level.
 */
int
peano4::topology::Tree::deriveSpawnLevel(const int targetLevel, const int numberOfRanks) {
  assertion(targetLevel >= 0, targetLevel);
  assertion(numberOfRanks >= 1, numberOfRanks);
  constexpr auto MaximumOwnershipUnits        = std::numeric_limits<std::int64_t>::max();
  constexpr int  MinimumOwnershipUnitsPerRank = 16;
  const auto     ownershipUnitsOnLevel        = [](const int level) {
    const auto cellsPerAxisOnLevel = static_cast<std::int64_t>(peano4::topology::getCellsPerAxis(level));
    auto       ownershipUnits      = static_cast<std::int64_t>(ChildrenPerCell);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      if (ownershipUnits > MaximumOwnershipUnits / cellsPerAxisOnLevel) {
        return MaximumOwnershipUnits;
      }
      ownershipUnits *= cellsPerAxisOnLevel;
    }
    return ownershipUnits;
  };
  const auto ownershipUnitsPerRank  = numberOfRanks == 1 ? 1 : MinimumOwnershipUnitsPerRank;
  const auto requiredOwnershipUnits = static_cast<std::int64_t>(numberOfRanks) * ownershipUnitsPerRank;
  int        spawnLevel             = 0;
  while (spawnLevel < targetLevel - 1 and ownershipUnitsOnLevel(spawnLevel) < requiredOwnershipUnits) {
    spawnLevel++;
  }
  return spawnLevel;
}

/**
 * Derives contiguous space-filling-curve intervals over weighted adaptive leaves. The
 * partition descends through cells whose children are all refined, while a
 * cell with any leaf child below the spawn level remains indivisible to
 * preserve transition faces and the sibling leaves that coarsen together.
 * Topology nodes store only paths crossed by a rank boundary.
 */
std::vector<int>
peano4::topology::Tree::deriveBalancedBlockOwners(
  const int             depth,
  const int             numberOfRanks,
  const RefinementPlan& refinementPlan
) const {
  if (numberOfRanks < 1) {
    tarch::terminateWithError("The number of ranks has to be positive.");
  }

  const int maximumLevel = validateAndGetMaximumLevel(depth, refinementPlan);

  const auto  planLeafWeights = derivePlanLeafWeights(depth, refinementPlan);
  const auto& refinedCellKeys = planLeafWeights.refinedCellKeys;
  const auto  getLeafWeight =
    [&planLeafWeights](const int level, const tarch::la::Vector<DIMENSIONS, int>& latticeIndex) {
      return planLeafWeights.getLeafWeight(level, latticeIndex);
    };
  const auto rootLattice = tarch::la::Vector<DIMENSIONS, int>(0);
  const auto totalWeight = getLeafWeight(0, rootLattice);
  if (totalWeight < 1 or totalWeight > std::numeric_limits<int>::max()) {
    tarch::terminateWithError("The refinement plan exceeds the tree's 32-bit index range.");
  }
  const auto numberOfActiveRanks = static_cast<int>(std::min<std::int64_t>(numberOfRanks, totalWeight));

  // The rank boundaries live in leaf-count space along the space-filling curve. One
  // walk over the curve places every boundary at the nearest side of the
  // indivisible unit its ideal split falls into.
  std::vector<std::int64_t> rankBoundaries(numberOfActiveRanks + 1, 0);
  rankBoundaries[numberOfActiveRanks] = totalWeight;
  peano4::grid::AutomatonState rootState{};
  rootState.x = _domainOffset;
  rootState.h = _domainWidth;
  {
    int          nextBoundary              = 1;
    std::int64_t leafPrefix                = 0;
    const auto   placeBoundariesWithinUnit = [&](const std::int64_t unitWeight) {
      while (nextBoundary < numberOfActiveRanks
             and static_cast<std::int64_t>(nextBoundary) * totalWeight
               <= (leafPrefix + unitWeight) * numberOfActiveRanks) {
        const auto targetWeightNumerator = static_cast<std::int64_t>(nextBoundary) * totalWeight;
        const auto distanceBefore        = std::abs(leafPrefix * numberOfActiveRanks - targetWeightNumerator);
        const auto distanceAfter = std::abs((leafPrefix + unitWeight) * numberOfActiveRanks - targetWeightNumerator);
        auto       boundary      = distanceBefore < distanceAfter ? leafPrefix : leafPrefix + unitWeight;
        // A boundary at or before its predecessor would leave a rank without
        // leaves, so it moves to the unit's end; only several splits inside
        // one unit can still coincide there.
        if (boundary <= rankBoundaries[nextBoundary - 1]) {
          boundary = leafPrefix + unitWeight;
        }
        rankBoundaries[nextBoundary] = boundary;
        nextBoundary++;
      }
      leafPrefix += unitWeight;
    };
    // A cell with a leaf child below the spawn level is indivisible: Its leaf
    // children coarsen together and restrict onto one coarse face, so they
    // stay on one rank.
    const int  spawnLevel             = deriveSpawnLevel(depth, numberOfRanks);
    const auto hasOnlyRefinedChildren = [&](const int level, const tarch::la::Vector<DIMENSIONS, int>& latticeIndex) {
      if (level + 1 > maximumLevel) {
        return false;
      }
      const auto& childFlags = refinedCellKeys[level + 1];
      for (int childPositionLinearised = 0; childPositionLinearised < ChildrenPerCell; childPositionLinearised++) {
        tarch::la::Vector<DIMENSIONS, int> childLattice(0);
        int                                remainingDigits = childPositionLinearised;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          childLattice(axis) = latticeIndex(axis) * SubdivisionFactor + remainingDigits % SubdivisionFactor;
          remainingDigits /= SubdivisionFactor;
        }
        if (childFlags.find(serialiseLatticeKey(childLattice)) == peano4::grid::LatticeIndexMap::NoEntry) {
          return false;
        }
      }
      return true;
    };
    const auto walkUnits = [&](
                             const auto&                               recurse,
                             const peano4::grid::AutomatonState&       state,
                             const int                                 level,
                             const tarch::la::Vector<DIMENSIONS, int>& latticeIndex
                           ) -> void {
      const auto subtreeWeight = getLeafWeight(level, latticeIndex);
      const bool isIndivisible =
        level + 1 >= depth and level > spawnLevel and not hasOnlyRefinedChildren(level, latticeIndex);
      const bool holdsBoundary = nextBoundary < numberOfActiveRanks
        and static_cast<std::int64_t>(nextBoundary) * totalWeight < (leafPrefix + subtreeWeight) * numberOfActiveRanks;
      if (isIndivisible or not holdsBoundary) {
        placeBoundariesWithinUnit(subtreeWeight);
        return;
      }
      peano4::grid::AutomatonState fineGridStates[ChildrenPerCell];
      peano4::grid::AutomatonState::refineState(state, fineGridStates);
      for (const auto& child : peano4::sfc::ActiveCurve::getChildren(state.curveState)) {
        recurse(
          recurse,
          fineGridStates[child.linearisedPosition],
          level + 1,
          latticeIndex * SubdivisionFactor + child.getPositionWithinParent()
        );
      }
    };
    walkUnits(walkUnits, rootState, 0, rootLattice);
    assertion(leafPrefix == totalWeight, leafPrefix, totalWeight);
  }
  const auto ownerOfLeaf = [&rankBoundaries](const std::int64_t leafNumber) {
    return static_cast<int>(
      std::upper_bound(rankBoundaries.begin() + 1, rankBoundaries.end(), leafNumber) - rankBoundaries.begin() - 1
    );
  };

  std::vector<peano4::topology::TopologyNode> topologyNodes(1);
  const auto materialiseOwnership = [&](
                                      const auto&                               recurse,
                                      const peano4::grid::AutomatonState&       state,
                                      const int                                 level,
                                      const tarch::la::Vector<DIMENSIONS, int>& latticeIndex,
                                      const std::int64_t                        firstLeaf,
                                      const std::int64_t                        numberOfLeavesInSubtree,
                                      const int                                 topologyNode
                                    ) -> void {
    topologyNodes[topologyNode].firstOwner = ownerOfLeaf(firstLeaf);
    topologyNodes[topologyNode].lastOwner  = ownerOfLeaf(firstLeaf + numberOfLeavesInSubtree - 1);
    if (topologyNodes[topologyNode].firstOwner == topologyNodes[topologyNode].lastOwner) {
      return;
    }
    assertion(level < maximumLevel, level, maximumLevel);
    if (topologyNodes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max() - ChildrenPerCell)) {
      tarch::terminateWithError("The rank ownership plan exceeds the tree's index range.");
    }
    const auto childrenBlock                  = static_cast<int>(topologyNodes.size());
    topologyNodes[topologyNode].childrenBlock = childrenBlock;
    topologyNodes.resize(topologyNodes.size() + ChildrenPerCell);

    peano4::grid::AutomatonState fineGridStates[ChildrenPerCell];
    peano4::grid::AutomatonState::refineState(state, fineGridStates);
    auto nextLeaf = firstLeaf;
    for (const auto& child : peano4::sfc::ActiveCurve::getChildren(state.curveState)) {
      const auto childLattice = latticeIndex * SubdivisionFactor + child.getPositionWithinParent();
      const auto childWeight  = getLeafWeight(level + 1, childLattice);
      recurse(
        recurse,
        fineGridStates[child.linearisedPosition],
        level + 1,
        childLattice,
        nextLeaf,
        childWeight,
        childrenBlock + child.linearisedPosition
      );
      nextLeaf += childWeight;
    }
    assertion(nextLeaf == firstLeaf + numberOfLeavesInSubtree, nextLeaf, firstLeaf, numberOfLeavesInSubtree);
  };

  materialiseOwnership(materialiseOwnership, rootState, 0, rootLattice, 0, totalWeight, 0);

  constexpr int    OwnershipPlanHeaderSize = 2;
  constexpr int    TopologyNodeStride      = 3;
  std::vector<int> ownershipPlan(OwnershipPlanHeaderSize + topologyNodes.size() * TopologyNodeStride, 0);
  ownershipPlan[0] = OwnershipPlanMarker;
  ownershipPlan[1] = static_cast<int>(topologyNodes.size());
  for (int nodeIndex = 0; nodeIndex < static_cast<int>(topologyNodes.size()); nodeIndex++) {
    const auto planOffset         = OwnershipPlanHeaderSize + nodeIndex * TopologyNodeStride;
    ownershipPlan[planOffset]     = topologyNodes[nodeIndex].firstOwner;
    ownershipPlan[planOffset + 1] = topologyNodes[nodeIndex].lastOwner;
    ownershipPlan[planOffset + 2] = topologyNodes[nodeIndex].childrenBlock;
  }
  return ownershipPlan;
}

/**
 * Returns the bytes of every array and lattice map the table holds. A hash set counts its
 * keys only.
 */
std::size_t
peano4::topology::Tree::getAllocatedBytes() const noexcept {
  std::size_t result = tarch::getAllocatedBytesOf(_blockSlotOfSpawnCell)
    + tarch::getAllocatedBytesOf(_blockLatticeOfSlot) + tarch::getAllocatedBytesOf(_ownerOfSpawnCell)
    + tarch::getAllocatedBytesOf(_topologyNodes) + tarch::getAllocatedBytesOf(_cells)
    + tarch::getAllocatedBytesOf(_vertices) + tarch::getAllocatedBytesOf(_cellLevelOffsets)
    + tarch::getAllocatedBytesOf(_levelLocalIndexOfCoordinate) + tarch::getAllocatedBytesOf(_vertexLevelOffsets)
    + tarch::getAllocatedBytesOf(_faceLevelOffsets) + tarch::getAllocatedBytesOf(_cellsPerAxisOfLevel)
    + tarch::getAllocatedBytesOf(_sparseCellIndexOfLattice) + tarch::getAllocatedBytesOf(_sparseVertexIndexOfLattice)
    + tarch::getAllocatedBytesOf(_sparseFaceIndexOfLattice) + tarch::getAllocatedBytesOf(_surroundedVertices)
    + tarch::getAllocatedBytesOf(_buildFlagKeys) + tarch::getAllocatedBytesOf(_planRefinedCellKeys)
    + _localBlockKeys.size() * sizeof(std::int64_t);
  for (const auto& indexOfCoordinate : _levelLocalIndexOfCoordinate) {
    result += tarch::getAllocatedBytesOf(indexOfCoordinate);
  }
  for (const auto& surroundedVertices : _surroundedVertices) {
    result += tarch::getAllocatedBytesOf(surroundedVertices);
  }
  for (const auto* const maps :
       {&_sparseCellIndexOfLattice, &_sparseVertexIndexOfLattice, &_buildFlagKeys, &_planRefinedCellKeys}) {
    for (const auto& map : *maps) {
      result += map.getAllocatedBytes();
    }
  }
  for (const auto& mapsOfLevel : _sparseFaceIndexOfLattice) {
    for (const auto& map : mapsOfLevel) {
      result += map.getAllocatedBytes();
    }
  }
  return result;
}
