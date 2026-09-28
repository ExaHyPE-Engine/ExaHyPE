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
 * Stores one cell of a rank-local spacetree view. The children of a refined
 * cell form one contiguous block of ChildrenPerCell entries and childrenBlock names the
 * first of them; the 2^d vertex and 2d face indices point into the vertex and
 * face arrays, so two adjacent cells reference the entity between them by one
 * index.
 */
struct peano4::topology::TreeCell {
  int parent{-1};
  int childrenBlock{-1};
  int level{0};
  int ownerTreeId{-1};

  tarch::la::Vector<DIMENSIONS, int> latticeIndex{0};

  std::array<int, peano4::VerticesPerCell> vertexIndices{};
  std::array<int, peano4::FacesPerCell>    faceIndices{};
};

/**
 * Represents one node of the ownership table. A node covers one cell and stores the
 * contiguous interval of ranks whose curve sections own leaves below it,
 * plus the index of its first child node; a childrenBlock of -1 marks a
 * node without children.
 */
struct peano4::topology::TopologyNode {
  int firstOwner{-1};
  int lastOwner{-1};
  int childrenBlock{-1};
};

/**
 * Stores an index-linked spacetree over flat cell, vertex and face arrays. A
 * caller visits the cells in any order, and concurrent descents into disjoint
 * subtrees write no common state.
 */
class peano4::topology::Tree final {
public:
  static constexpr int RootCell{0};

  /**
   * Stores refined vertex lattice positions per level: Element i holds the flagged
   * vertices of level depth + i. A cell refines when any of its corner
   * vertices is flagged, so the refined region dilates by one cell ring
   * around every flagged vertex.
   */
  using RefinementPlan = std::vector<std::vector<tarch::la::Vector<DIMENSIONS, int>>>;

  Tree(
    int                                          treeId,
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
  const int _id;

  const tarch::la::Vector<DIMENSIONS, double> _domainOffset;
  const tarch::la::Vector<DIMENSIONS, double> _domainWidth;

  const std::bitset<DIMENSIONS> _periodicBoundaries;

  int _depth{-1};
  int _spawnLevel{0};

  /**
   * Marks the finest level with closed-form indices, the base depth; the
   * lattice maps hold every level beyond it. A rank materialising the whole
   * domain stores every level up to it as one regular grid, a partitioned rank
   * stores the levels above its spawn level that way and the levels below it
   * block by block.
   */
  int _denseBoundaryLevel{0};

  /**
   * Maps a spawn-level cell's level-local index onto the slot of its
   * materialised block, or -1. Between the spawn level and the base depth a
   * block holds its cells in the base-ChildrenPerCell digit interleave and its vertices
   * and faces in row-major order. A vertex or face on the boundary of two
   * blocks has a slot in both and belongs to the lower block, whose slot the
   * cells of both blocks reference.
   */
  std::vector<int> _blockSlotOfSpawnCell{};

  /**
   * Stores the spawn-level lattice position of every materialised block in slot order.
   */
  std::vector<tarch::la::Vector<DIMENSIONS, int>> _blockLatticeOfSlot{};

  /**
   * Stores per spawn-level cell the rank that owns its whole subtree, or -1
   * for a subtree several ranks own, so getOwnerOfCellAt() answers a cell
   * below the spawn level with one table read where the topology nodes agree.
   */
  std::vector<int> _ownerOfSpawnCell{};

  std::vector<peano4::topology::TopologyNode> _topologyNodes{};

  /**
   * Stores spawn-level lattice keys of the blocks this rank materialises below the
   * spawn level: Its owned blocks and one ring of neighbours. The set is empty
   * both for a rank that materialises everything and for one that owns no
   * block at all, so _materialisesWholeDomain tells the two apart.
   */
  std::unordered_set<std::int64_t> _localBlockKeys{};

  bool _materialisesWholeDomain{true};

  std::vector<peano4::topology::TreeCell> _cells{};
  std::vector<peano4::grid::GridVertex>   _vertices{};

  /**
   * Stores the first cell index of each level and the final total count.
   */
  std::vector<int> _cellLevelOffsets{};

  /**
   * Maps a lattice coordinate of a dense level onto its contribution to the
   * level-local cell index, per axis: Entry axis * cells per axis + coordinate.
   * The sum over the axes is the base-ChildrenPerCell digit interleave of the lattice
   * position, so getCellAt() reads one entry per axis.
   */
  std::vector<std::vector<int>> _levelLocalIndexOfCoordinate{};

  /**
   * Stores the first vertex index of each level and the final total count.
   */
  std::vector<int> _vertexLevelOffsets{};

  /**
   * Stores the first face index of each level created by the build.
   */
  std::vector<int> _faceLevelOffsets{};

  int _numberOfFaces{0};

  /**
   * Holds the number of cells per axis of every level.
   */
  std::vector<int> _cellsPerAxisOfLevel{};

  /**
   * Sizes one axis field so three coordinates fit into one 64-bit key.
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
   * Stores lattice keys of vertices closed by the surroundedness rule, indexed
   * by level - depth. The derivation answers from the refinement-plan tables and
   * every evaluation round reuses the result.
   */
  mutable std::vector<std::vector<std::int64_t>> _surroundedVertices{};
  mutable bool                                   _surroundedVerticesDerived{false};

  /**
   * Stores flagged vertex lattice keys of one evaluation round, indexed by level - depth.
   */
  using FlaggedKeysPerLevel = std::vector<std::unordered_set<std::int64_t>>;

  /**
   * Stores flag keys of the plan this structure was built from, indexed by
   * level - depth. Structural queries answer from these tables alone, so the
   * plan evaluation needs no materialised cell or vertex.
   */
  std::vector<peano4::grid::LatticeIndexMap> _buildFlagKeys{};

  /**
   * Stores refined-cell lattice keys; element i holds level depth + i.
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
   * Holds the leaf counts of the structure a refinement plan describes: The
   * refined cells of every level, the leaves a regular cell of every level
   * above the base depth holds, and per refined cell the leaves its
   * refinement adds below it.
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
