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
 * cells, and childrenBlock is the index of the first. Two adjacent cells point to the same vertex or
 * face index for the entity between them.
 */
struct peano4::topology::TreeCell {
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
 * leaves below the cell, and childrenBlock is the index of the first child node or -1.
 */
struct peano4::topology::TopologyNode {
  int firstOwner{-1};
  int lastOwner{-1};
  int childrenBlock{-1};
};

/**
 * Spacetree over flat cell, vertex and face arrays that reference each other by index. A caller
 * visits the cells in any order, and concurrent descents into disjoint subtrees write no common
 * state.
 */
class peano4::topology::Tree final {
public:
  static constexpr int RootCell{0};

  /**
   * The flagged vertex lattice positions per level, where entry i lists the flags of level
   * depth + i. A cell with a flagged corner refines, so the refined region grows by one cell ring
   * around every flagged vertex.
   */
  using RefinementPlan = std::vector<std::vector<tarch::la::Vector<DIMENSIONS, int>>>;

  Tree(
    int                                          rank,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainWidth,
    const std::bitset<DIMENSIONS>&               periodicBoundaries = std::bitset<DIMENSIONS>()
  );

  Tree(const Tree&)            = delete;
  Tree(Tree&&)                 = delete;
  Tree& operator=(const Tree&) = delete;
  Tree& operator=(Tree&&)      = delete;
  ~Tree()                      = default;

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

  bool ownershipPlanSplitsCellWithLeafChild(
    int                     depth,
    int                     spawnLevel,
    const RefinementPlan&   refinementPlan,
    const std::vector<int>& ownershipPlan
  ) const;

  bool updateRefinementPlan(
    const std::vector<peano4::RefinementRequest>& requests,
    RefinementPlan&                               refinementPlan
  ) const;

  INLINE_METHOD static bool latticeKeyPrecedes(
    const tarch::la::Vector<DIMENSIONS, int>& left,
    const tarch::la::Vector<DIMENSIONS, int>& right
  );

  static constexpr int OwnershipPlanMarker{-2};

  INLINE_METHOD int getCellsPerAxisOfLevel(int level) const;
  INLINE_METHOD int getOwnerOfCellAt(int level, const tarch::la::Vector<DIMENSIONS, int>& latticeIndex) const;

  INLINE_METHOD bool isVertexRefinedInPlan(int level, const tarch::la::Vector<DIMENSIONS, int>& vertexLattice) const;
  bool cellExistsInPlan(int level, const tarch::la::Vector<DIMENSIONS, int>& cellLattice) const;
  bool isCellRefinedInPlan(int level, const tarch::la::Vector<DIMENSIONS, int>& cellLattice) const;
  bool isVertexPersistentInPlan(int level, const tarch::la::Vector<DIMENSIONS, int>& vertexLattice) const;

  INLINE_METHOD bool subtreeTouchesRank(int cellIndex, int rank) const;
  INLINE_METHOD bool subtreeTouchesRankAt(
    int                                       level,
    const tarch::la::Vector<DIMENSIONS, int>& latticeIndex,
    int                                       rank
  ) const;
  INLINE_METHOD bool isSplitBlockAt(const tarch::la::Vector<DIMENSIONS, int>& blockLattice) const;

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

  INLINE_METHOD int getAdjacentCellOwnerAt(
    int                                       level,
    const tarch::la::Vector<DIMENSIONS, int>& vertexLattice,
    int                                       adjacencyEntry
  ) const;
  INLINE_METHOD int getNumberOfLocalAdjacentCellsOfVertexAt(
    int                                       level,
    const tarch::la::Vector<DIMENSIONS, int>& vertexLattice
  ) const;

private:
  const int _rank;

  const tarch::la::Vector<DIMENSIONS, double> _domainOffset;
  const tarch::la::Vector<DIMENSIONS, double> _domainWidth;

  const std::bitset<DIMENSIONS> _periodicBoundaries;

  int _depth{-1};
  int _spawnLevel{0};

  /**
   * The finest level with closed-form indices, the base depth. The lattice maps store every finer
   * level. A partitioned rank stores the levels below its spawn level block by block.
   */
  int _denseBoundaryLevel{0};

  /**
   * Maps the level-local index of a spawn-level cell to the slot of its materialised block, or -1. A
   * block stores its cells in digit interleave and its vertices and faces in row-major order. A
   * vertex or face on the boundary of two blocks belongs to the lower block, whose slot the cells of
   * both blocks reference.
   */
  std::vector<int> _blockSlotOfSpawnCell{};

  /**
   * Stores the spawn-level lattice position of every materialised block in slot order.
   */
  std::vector<tarch::la::Vector<DIMENSIONS, int>> _blockLatticeOfSlot{};

  /**
   * The rank that owns the whole subtree of a spawn-level cell, or -1 for a subtree of several
   * ranks. getOwnerOfCellAt() reads it for a cell below the spawn level.
   */
  std::vector<int> _ownerOfSpawnCell{};

  std::vector<peano4::topology::TopologyNode> _topologyNodes{};

  /**
   * The spawn-level lattice keys of the blocks this rank materialises: Its own blocks and one ring of
   * neighbours. The set is empty for a rank that materialises everything and for a rank without
   * blocks, and _materialisesWholeDomain tells the two apart.
   */
  std::unordered_set<std::int64_t> _localBlockKeys{};

  bool _materialisesWholeDomain{true};

  std::vector<peano4::topology::TreeCell> _cells{};
  std::vector<peano4::grid::GridVertex>   _vertices{};

  /**
   * The first cell index of each level, followed by the total count.
   */
  std::vector<int> _cellLevelOffsets{};

  /**
   * Maps a lattice coordinate of a dense level to its part of the level-local cell index, at entry
   * axis * cells per axis + coordinate. The sum over the axes is the digit interleave of the lattice
   * position, so getCellAt() reads one entry per axis.
   */
  std::vector<std::vector<int>> _levelLocalIndexOfCoordinate{};

  /**
   * The first vertex index of each level, followed by the total count.
   */
  std::vector<int> _vertexLevelOffsets{};

  /**
   * The first face index of each level, followed by the total count.
   */
  std::vector<int> _faceLevelOffsets{};

  int _numberOfFaces{0};

  std::vector<int> _cellsPerAxisOfLevel{};

  /**
   * Bits per axis, so that three coordinates fit into one 64-bit key.
   */
  static constexpr int LatticeKeyBitsPerAxis{21};

  /**
   * Maps cell lattice keys beyond the dense boundary, indexed by level - _denseBoundaryLevel - 1.
   */
  std::vector<peano4::grid::LatticeIndexMap> _sparseCellIndexOfLattice{};

  /**
   * Maps vertex lattice keys beyond the dense boundary, indexed like the cell maps.
   */
  std::vector<peano4::grid::LatticeIndexMap> _sparseVertexIndexOfLattice{};

  /**
   * Maps face lattice keys per normal on each sparse level.
   */
  std::vector<std::array<peano4::grid::LatticeIndexMap, DIMENSIONS>> _sparseFaceIndexOfLattice{};

  /**
   * The lattice keys of the surrounded vertices at index level - depth, which every evaluation round
   * reuses.
   */
  mutable std::vector<std::vector<std::int64_t>> _surroundedVertices{};
  mutable bool                                   _surroundedVerticesDerived{false};

  /**
   * The flagged vertex lattice keys of one evaluation round at index level - depth.
   */
  using FlaggedKeysPerLevel = std::vector<std::unordered_set<std::int64_t>>;

  /**
   * The flag keys of the plan this tree was built from at index level - depth. Structural queries read
   * these tables only, so the plan evaluation needs no materialised cell or vertex.
   */
  std::vector<peano4::grid::LatticeIndexMap> _buildFlagKeys{};

  /**
   * The lattice keys of the refined cells, where entry i lists level depth + i.
   */
  std::vector<peano4::grid::LatticeIndexMap> _planRefinedCellKeys{};

  INLINE_METHOD static tarch::la::Vector<DIMENSIONS, int> deserialiseLatticeKey(std::int64_t latticeKey);
  INLINE_METHOD int getTopologyNodeAt(int level, const tarch::la::Vector<DIMENSIONS, int>& latticeIndex) const;
  INLINE_METHOD bool usesBlockLayout(int level) const;
  INLINE_METHOD int getCellsPerAxisOfBlock(int level) const;
  INLINE_METHOD int getBlockSlotAt(const tarch::la::Vector<DIMENSIONS, int>& blockLattice) const;
  INLINE_METHOD int getInterleavedIndex(int level, const tarch::la::Vector<DIMENSIONS, int>& lattice) const;
  INLINE_METHOD bool isLocalBlockOfCell(int level, const tarch::la::Vector<DIMENSIONS, int>& cellLattice) const;

  bool hasRefinedGoverningVertex(int level, const tarch::la::Vector<DIMENSIONS, int>& vertexLattice) const;

  static std::vector<peano4::grid::LatticeIndexMap> deriveRefinedCellKeyTables(
    int                            depth,
    const std::bitset<DIMENSIONS>& periodicBoundaries,
    const RefinementPlan&          refinementPlan
  );

  void derivePlanRefinedCellKeys(const RefinementPlan& refinementPlan);

  void deriveSurroundedVertices() const;

  static int validateAndGetMaximumLevel(int depth, const RefinementPlan& refinementPlan);

  /**
   * The leaf counts of the grid a refinement plan describes: The refined cells of every level, the
   * leaves below a regular cell of every level above the base depth, and the leaves the refinement of
   * each refined cell adds.
   */
  struct PlanLeafWeights {
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
