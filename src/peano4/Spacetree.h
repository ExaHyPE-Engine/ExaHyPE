// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Partition.h"
#include "peano4/RefinementRequest.h"
#include "peano4/grid/LatticeIndexMap.h"
#include "peano4/topology/Tree.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/la/Vector.h"

namespace peano4 {
  struct Regrid;
  class Spacetree;
} // namespace peano4

/**
 * The adaptive Cartesian grid of the whole domain and the partition this rank owns. Every rank
 * applies the refinement requests of all ranks to one refinement plan, balances the ranks along the
 * space-filling curve and rebuilds the tree and the partition from the plan. A regrid with live data
 * returns the slot moves, which a caller applies to its pools before finishRegrid().
 */
class peano4::Spacetree final {
public:
  /**
   * Counts of the rank's part of the grid after the last regrid.
   */
  struct Statistics final {
    int minimumLevel{std::numeric_limits<int>::max()};
    int maximumLevel{0};
    int numberOfCells{0};
    int numberOfRefinedCells{0};
    int numberOfFaces{0};
    int numberOfHangingFaces{0};
    int numberOfExchangedFaces{0};
    int numberOfStationaryRegrids{0};

    tarch::AllocatedBytes partitionBytes{};
    std::size_t           treeBytes{0};

    tarch::la::Vector<DIMENSIONS, double> minimumCellWidth{
      tarch::la::Vector<DIMENSIONS, double>(std::numeric_limits<double>::max())
    };
  };

  Spacetree(const Spacetree&)            = delete;
  Spacetree(Spacetree&&)                 = delete;
  Spacetree& operator=(const Spacetree&) = delete;
  Spacetree& operator=(Spacetree&&)      = delete;
  ~Spacetree()                           = default;

  static peano4::Spacetree& getInstance();

  void init(
    const tarch::la::Vector<DIMENSIONS, double>& offset,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    const std::bitset<DIMENSIONS>&               periodicBoundaries = std::bitset<DIMENSIONS>()
  );
  void shutdown();

  void scheduleRegularGridConstruction(int targetLevel);
  void markPersistentDataLive();
  void enableDynamicLoadBalancing(bool enable);

  bool regrid(const std::vector<RefinementRequest>& requests, Regrid& regrid);
  void finishRegrid();
  bool requestsChangePlan(const std::vector<RefinementRequest>& requests);

  [[nodiscard]] const Partition& getPartition() const noexcept;
  [[nodiscard]] int getCellAt(int level, const tarch::la::Vector<DIMENSIONS, int>& lattice) const;
  [[nodiscard]] const Statistics& getStatistics() const noexcept;
  [[nodiscard]] const std::bitset<DIMENSIONS>& getPeriodicBoundaries() const noexcept;

  static int deriveSpawnLevel(int targetLevel, int numberOfRanks);

  static constexpr int RepartitionTolerancePercent = 2;

private:
  /**
   * One local cell that the walk along the curve finds: Its index in the tree and its slot in the
   * partition.
   */
  struct CellRecord final {
    int treeCell{-1};
    int slot{-1};
  };

  /**
   * The faces one neighbour rank and this rank hold, as the walk over the cells finds them.
   */
  struct NeighbourFaces final {
    int                        rank{-1};
    std::vector<ExchangedFace> faces{};
  };

  /**
   * The arrays of the partition and the tables that build them, in host memory.
   */
  struct PartitionArrays final {
    int numberOfSlots{0};
    int numberOfFaces{0};
    int minimumLevel{0};
    int maximumLevel{0};

    std::vector<double> widthOfLevel{};

    std::vector<int> cells{};
    std::vector<int> cellRanges{};
    std::vector<int> cellLevel{};
    std::vector<int> cellLattice{};
    std::vector<int> facesOfCell{};
    /**
     * The index of every slot's cell along the space-filling curve over all ranks; -1 for a free slot.
     */
    std::vector<int> cellCurveIndex{};

    std::vector<int>       faces{};
    std::vector<int>       faceRanges{};
    std::vector<int>       faceLevel{};
    std::vector<int>       faceNormal{};
    std::vector<int>       faceLattice{};
    std::vector<int>       cellsOfFace{};
    std::vector<FaceKind>  faceKind{};
    std::vector<Adjacency> faceAdjacency{};

    std::vector<LevelJump> levelJumps{};
    std::vector<int>       levelJumpRanges{};

    std::vector<Neighbour>      neighbours{};
    std::vector<ExchangedFace>  exchangedFaces{};
    std::vector<NeighbourFaces> neighbourFaces{};

    /**
     * Maps the index of every tree face in the partition to its partition face during a build.
     */
    std::vector<int> faceOfTreeFace{};
    /**
     * Maps a coarse face to its level jump during a build.
     */
    std::vector<int> levelJumpOfCoarseFace{};
    /**
     * Maps a lattice key to the slot of its cell, one map per level.
     */
    std::vector<peano4::grid::LatticeIndexMap> cellSlotOfLattice{};

    std::vector<int> freeSlots{};
    std::vector<int> slotsFreedByRegrid{};
  };

  /**
   * The arrays a kernel reads, in device memory. A build without a GPU backend leaves them empty.
   */
  struct DeviceArrays final {
    tarch::accelerator::Buffer<int>       cells{tarch::MemoryLocation::Device};
    tarch::accelerator::Buffer<int>       cellLevel{tarch::MemoryLocation::Device};
    tarch::accelerator::Buffer<int>       cellLattice{tarch::MemoryLocation::Device};
    tarch::accelerator::Buffer<int>       facesOfCell{tarch::MemoryLocation::Device};
    tarch::accelerator::Buffer<int>       faces{tarch::MemoryLocation::Device};
    tarch::accelerator::Buffer<int>       faceLevel{tarch::MemoryLocation::Device};
    tarch::accelerator::Buffer<int>       faceNormal{tarch::MemoryLocation::Device};
    tarch::accelerator::Buffer<int>       faceLattice{tarch::MemoryLocation::Device};
    tarch::accelerator::Buffer<int>       cellsOfFace{tarch::MemoryLocation::Device};
    tarch::accelerator::Buffer<FaceKind>  faceKind{tarch::MemoryLocation::Device};
    tarch::accelerator::Buffer<LevelJump> levelJumps{tarch::MemoryLocation::Device};
    tarch::accelerator::Buffer<double>    widthOfLevel{tarch::MemoryLocation::Device};
  };

  Spacetree() = default;

  static peano4::Spacetree _singleton;

  int _targetLevel{-1};

  tarch::la::Vector<DIMENSIONS, double> _domainOffset{};
  tarch::la::Vector<DIMENSIONS, double> _domainWidth{};
  std::bitset<DIMENSIONS>               _periodicBoundaries{};

  std::unique_ptr<peano4::topology::Tree>              _tree{};
  std::vector<std::unique_ptr<peano4::topology::Tree>> _spareTrees{};
  peano4::topology::Tree::RefinementPlan               _refinementPlan{};
  std::vector<int>                                     _blockOwners{};

  std::vector<RefinementRequest>         _lastEvaluatedRequests{};
  peano4::topology::Tree::RefinementPlan _lastEvaluatedPlanUpdate{};
  bool                                   _lastEvaluationChangedPlan{false};

  bool _persistentDataLive{false};
  bool _persistentDataLiveUniform{false};
  bool _dynamicLoadBalancing{true};
  bool _regridPending{false};

  PartitionArrays _arrays{};
  DeviceArrays    _deviceArrays{};
  Partition       _partition{};
  Partition       _partitionOnDevice{};
  Statistics      _statistics{};

  [[nodiscard]] int deriveSpawnLevel() const;
  std::unique_ptr<peano4::topology::Tree> acquireTree();
  void buildTree(const peano4::topology::Tree::RefinementPlan& refinementPlan);
  bool derivePlanUpdate(
    const std::vector<RefinementRequest>&   requests,
    peano4::topology::Tree::RefinementPlan& updatedPlan
  );
  [[nodiscard]] std::vector<int> deriveOwnersForPlan(const peano4::topology::Tree::RefinementPlan& updatedPlan) const;
  void votePersistentDataLive();
  void deriveStatistics();

  void buildPartition();
  void rebuildPartition(const peano4::topology::Tree& previousTree, Regrid& regrid);
  void collectCells(std::vector<CellRecord>& records) const;
  void assignSlots(std::vector<CellRecord>& records, bool preserveSlots);
  void deriveFaces(const std::vector<CellRecord>& records);
  int registerFace(int treeCell, int faceNumber, FaceKind kind);
  int registerLevelJump(int refinedTreeCell, int faceNumber, int coarseFace);
  void addExchangedFace(int rank, int face, int side);
  void orderLists(const std::vector<CellRecord>& records);
  void numberCellsAlongCurve(const std::vector<CellRecord>& records);
  void assemblePartitions();
  [[nodiscard]] tarch::AllocatedBytes getPartitionBytes() const noexcept;
  [[nodiscard]] std::size_t getTreeBytes() const noexcept;
  [[nodiscard]] std::int64_t getLatticeKey(int level, const tarch::la::Vector<DIMENSIONS, int>& lattice) const;
};
