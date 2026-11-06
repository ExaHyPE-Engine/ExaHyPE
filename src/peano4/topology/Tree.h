// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "peano4/grid/GridVertex.h"
#include "peano4/grid/LatticeIndexMap.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace peano4 {
  struct RefinementRequest;
} // namespace peano4

namespace peano4::topology {
  struct TreeCell;
  struct TopologyNode;
  class Tree;

  INLINE_METHOD int getCellsPerAxis(int level);
} // namespace peano4::topology

/**
 * One cell of the tree of a rank. The children of a refined cell are ChildrenPerCell consecutive
 * cells that start at childrenBlock. Two adjacent cells point to the same vertex or face index for
 * the entity between them.
 */
struct peano4::topology::TreeCell final {
  int parent{-1};
  int childrenBlock{-1};
  int level{0};
  int ownerRank{-1};

  tarch::la::Vector<DIMENSIONS, int> latticeIndex{0};

  std::array<int, peano4::VerticesPerCell> vertexIndices{};
  std::array<int, peano4::FacesPerCell>    faceIndices{};
};

/**
 * One node of the ownership table for one cell. The ranks from firstOwner to lastOwner own the
 * leaves below the cell. childrenBlock is the index of the first child node or -1.
 */
struct peano4::topology::TopologyNode final {
  int firstOwner{-1};
  int lastOwner{-1};
  int childrenBlock{-1};
};

/**
 * Spacetree over flat cell, vertex and face arrays that reference each other by index. A caller
 * visits the cells in any order. Concurrent descents into disjoint subtrees write disjoint state.
 */
class peano4::topology::Tree final {
public:
  static constexpr int RootCell{0};

  /**
   * Lists the flagged vertices of every level from the base depth down in ascending order of
   * serialiseLatticeKey().
   */
  using RefinementPlan = std::vector<std::vector<tarch::la::Vector<DIMENSIONS, int>>>;

  Tree(
    int                                          rank,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainWidth,
    const std::bitset<DIMENSIONS>&               periodicBoundaries = std::bitset<DIMENSIONS>()
  );

  Tree(const Tree& other)            = delete;
  Tree(Tree&& other)                 = delete;
  Tree& operator=(const Tree& other) = delete;
  Tree& operator=(Tree&& other)      = delete;
  ~Tree()                            = default;

  void buildRegularGrid(
    int                     depth,
    int                     spawnLevel     = 0,
    int                     numberOfRanks  = 1,
    const RefinementPlan&   refinementPlan = RefinementPlan(),
    const std::vector<int>* ownersOfBlocks = nullptr
  );

  std::vector<int> getOwnersOfBlocks() const;
  static int deriveSpawnLevel(int targetLevel, int numberOfRanks);
  std::vector<int> deriveBalancedBlockOwners(int depth, int numberOfRanks, const RefinementPlan& refinementPlan) const;

  std::vector<std::int64_t> deriveRankLoads(
    int                     depth,
    int                     numberOfRanks,
    const RefinementPlan&   refinementPlan,
    const std::vector<int>& ownershipPlan
  ) const;

  bool ownershipPlanSplitsParentOfLeaf(
    int                     depth,
    int                     spawnLevel,
    const RefinementPlan&   refinementPlan,
    const std::vector<int>& ownershipPlan
  ) const;

  bool updateRefinementPlan(
    const std::vector<peano4::RefinementRequest>& requests,
    RefinementPlan&                               refinementPlan
  ) const;

  static constexpr int OwnershipPlanMarker{-2};

  INLINE_METHOD int getCellsPerAxisOfLevel(int level) const;
  INLINE_METHOD int getOwnerOfCellAt(int level, const tarch::la::Vector<DIMENSIONS, int>& latticeIndex) const;

  bool cellExistsInPlan(int level, const tarch::la::Vector<DIMENSIONS, int>& cellLattice) const;
  bool isCellRefinedInPlan(int level, const tarch::la::Vector<DIMENSIONS, int>& cellLattice) const;

  INLINE_METHOD bool subtreeTouchesRank(int cellIndex, int rank) const;

  INLINE_METHOD const peano4::topology::TreeCell& getCell(int cellIndex) const;
  INLINE_METHOD const peano4::grid::GridVertex& getVertex(int vertexIndex) const;

  INLINE_METHOD int getChild(int cellIndex, int linearisedChildPosition) const;
  INLINE_METHOD int getCellAt(int level, const tarch::la::Vector<DIMENSIONS, int>& latticeIndex) const;
  INLINE_METHOD int getVertexAt(int level, const tarch::la::Vector<DIMENSIONS, int>& latticeIndex) const;
  INLINE_METHOD int getFaceAt(int level, int normal, const tarch::la::Vector<DIMENSIONS, int>& faceLattice) const;
  INLINE_METHOD bool isRefined(int cellIndex) const;

  INLINE_METHOD int getDepth() const;
  INLINE_METHOD std::size_t getNumberOfCells() const;
  INLINE_METHOD std::size_t getNumberOfVertices() const;
  INLINE_METHOD std::size_t getNumberOfFaces() const;
  [[nodiscard]] std::size_t getAllocatedBytes() const noexcept;

  INLINE_METHOD const std::bitset<DIMENSIONS>& getPeriodicBoundaries() const;

  INLINE_METHOD static std::int64_t serialiseLatticeKey(const tarch::la::Vector<DIMENSIONS, int>& latticeIndex);

private:
  const int _rank;

  const tarch::la::Vector<DIMENSIONS, double> _domainOffset;
  const tarch::la::Vector<DIMENSIONS, double> _domainWidth;

  const std::bitset<DIMENSIONS> _periodicBoundaries;

  int _depth{-1};
  int _spawnLevel{0};

  int _denseBoundaryLevel{0};

  std::vector<int> _blockSlotOfSpawnCell{};

  std::vector<tarch::la::Vector<DIMENSIONS, int>> _blockLatticeOfSlot{};

  std::vector<int> _ownerOfSpawnCell{};

  std::vector<peano4::topology::TopologyNode> _topologyNodes{};

  std::unordered_set<std::int64_t> _localBlockKeys{};

  bool _materialisesWholeDomain{true};

  std::vector<peano4::topology::TreeCell> _cells{};
  std::vector<peano4::grid::GridVertex>   _vertices{};

  std::vector<int> _cellLevelOffsets{};

  std::vector<std::vector<int>> _levelLocalIndexOfCoordinate{};

  std::vector<int> _vertexLevelOffsets{};

  std::vector<int> _faceLevelOffsets{};

  int _numberOfFaces{0};

  std::vector<int> _cellsPerAxisOfLevel{};

  static constexpr int LatticeKeyBitsPerAxis{21};

  std::vector<peano4::grid::LatticeIndexMap> _sparseCellIndexOfLattice{};

  std::vector<peano4::grid::LatticeIndexMap> _sparseVertexIndexOfLattice{};

  std::vector<std::array<peano4::grid::LatticeIndexMap, DIMENSIONS>> _sparseFaceIndexOfLattice{};

  mutable std::vector<std::vector<std::int64_t>> _surroundedVertices{};
  mutable bool                                   _surroundedVerticesDerived{false};

  using FlaggedKeysPerLevel = std::vector<std::unordered_set<std::int64_t>>;

  std::vector<peano4::grid::LatticeIndexMap> _buildFlagKeys{};

  std::vector<peano4::grid::LatticeIndexMap> _planRefinedCellKeys{};

  INLINE_METHOD static tarch::la::Vector<DIMENSIONS, int> deserialiseLatticeKey(std::int64_t latticeKey);
  INLINE_METHOD int getTopologyNodeAt(int level, const tarch::la::Vector<DIMENSIONS, int>& latticeIndex) const;
  INLINE_METHOD bool usesBlockLayout(int level) const;
  INLINE_METHOD int getCellsPerAxisOfBlock(int level) const;
  INLINE_METHOD int getBlockSlotAt(const tarch::la::Vector<DIMENSIONS, int>& blockLattice) const;
  INLINE_METHOD int getInterleavedIndex(int level, const tarch::la::Vector<DIMENSIONS, int>& lattice) const;
  INLINE_METHOD bool isLocalBlockOfCell(int level, const tarch::la::Vector<DIMENSIONS, int>& cellLattice) const;
  INLINE_METHOD bool isSplitBlockAt(const tarch::la::Vector<DIMENSIONS, int>& blockLattice) const;

  INLINE_METHOD bool isVertexRefinedInPlan(int level, const tarch::la::Vector<DIMENSIONS, int>& vertexLattice) const;
  bool hasRefinedGoverningVertex(int level, const tarch::la::Vector<DIMENSIONS, int>& vertexLattice) const;

  static std::vector<peano4::grid::LatticeIndexMap> deriveRefinedCellKeyTables(
    int                            depth,
    const std::bitset<DIMENSIONS>& periodicBoundaries,
    const RefinementPlan&          refinementPlan
  );

  void derivePlanRefinedCellKeys(const RefinementPlan& refinementPlan);
  void deriveSurroundedVertices() const;

  static int validateAndGetMaximumLevel(int depth, const RefinementPlan& refinementPlan);

  struct PlanLeafWeights final {
    std::vector<peano4::grid::LatticeIndexMap>                  refinedCellKeys{};
    std::vector<std::unordered_map<std::int64_t, std::int64_t>> adaptiveLeafDeltas{};
    std::vector<std::int64_t>                                   regularLeafWeights{};
    int                                                         depth{0};

    [[nodiscard]] std::int64_t getLeafWeight(int level, const tarch::la::Vector<DIMENSIONS, int>& latticeIndex) const;
  };

  PlanLeafWeights derivePlanLeafWeights(int depth, const RefinementPlan& refinementPlan) const;
  void assignBuildFlagKeys(const RefinementPlan& refinementPlan);
};

#include "peano4/topology/Tree.cpph"
