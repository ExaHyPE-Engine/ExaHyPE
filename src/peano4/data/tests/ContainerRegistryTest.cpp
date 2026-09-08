// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ContainerRegistryTest.h"

#include "peano4/data/ContainerRegistry.h"

namespace {
  /**
   * Records which registry operations reached it and in which order the
   * registry visited the containers. The pool operations visit the containers
   * as concurrent tasks, so the recording claims its position in a pre-sized
   * buffer through an atomic cursor.
   */
  class RecordingContainer final : public peano4::data::ManagedContainer {
  public:
    RecordingContainer(
      const int         containerId,
      const bool        supportsReplication,
      std::vector<int>& visitOrder,
      std::atomic<int>& visitCursor
    ) :
      _containerId(containerId),
      _supportsReplication(supportsReplication),
      _visitOrder(visitOrder),
      _visitCursor(visitCursor) {
      peano4::data::ContainerRegistry::getInstance().attachContainer(this);
    }
    RecordingContainer(const RecordingContainer&)            = delete;
    RecordingContainer(RecordingContainer&&)                 = delete;
    RecordingContainer& operator=(const RecordingContainer&) = delete;
    RecordingContainer& operator=(RecordingContainer&&)      = delete;
    ~RecordingContainer() override { peano4::data::ContainerRegistry::getInstance().detachContainer(this); }

    int getManagedContainerId() const override { return _containerId; }

    bool supportsSubtreeTaskReplication() const override { return _supportsReplication; }

    /**
     * Every argument of the last call of each operation.
     */
    struct RecordedArguments {
      std::size_t                      resizeCellCount{0};
      std::size_t                      resizeFaceCount{0};
      std::size_t                      resizeVertexCount{0};
      std::vector<std::pair<int, int>> migratedCellSlotPairs{};
      std::vector<std::pair<int, int>> migratedFaceSlotPairs{};
      std::vector<std::pair<int, int>> migratedVertexSlotPairs{};
      std::size_t                      migrateCellCount{0};
      std::size_t                      migrateFaceCount{0};
      std::size_t                      migrateVertexCount{0};
      std::vector<std::pair<int, int>> replicatedFacePairs{};
      std::vector<std::pair<int, int>> replicatedVertexPairs{};
      /** Calls per chunk number of the last replication; every chunk arrives once. */
      std::vector<int>                replicatedChunkCalls{};
      std::vector<std::vector<int>>   packedFaceLists{};
      std::vector<std::vector<int>>   packedVertexLists{};
      std::map<int, std::vector<int>> sendCellSlots{};
      std::map<int, std::vector<int>> receiveCellSlots{};
      std::map<int, std::vector<int>> sendFaceSlots{};
      std::map<int, std::vector<int>> receiveFaceSlots{};
      std::map<int, std::vector<int>> sendVertexSlots{};
      std::map<int, std::vector<int>> receiveVertexSlots{};
      std::vector<std::vector<int>>   finishedFaceLists{};
      std::vector<std::vector<int>>   finishedVertexLists{};
      std::vector<int>                faceFrozenSlotBases{};
      std::vector<int>                vertexFrozenSlotBases{};
      int                             numberOfLocalSlots{-1};
    };

    void initialiseSerialSlot() override { record(); }

    void resizePoolForTree(
      const std::size_t cellCount,
      const std::size_t faceCount,
      const std::size_t vertexCount
    ) override {
      _arguments.resizeCellCount   = cellCount;
      _arguments.resizeFaceCount   = faceCount;
      _arguments.resizeVertexCount = vertexCount;
      record();
    }

    void migratePoolForTree(
      const std::vector<std::pair<int, int>>& cellSlotPairs,
      const std::vector<std::pair<int, int>>& faceSlotPairs,
      const std::vector<std::pair<int, int>>& vertexSlotPairs,
      const std::size_t                       cellCount,
      const std::size_t                       faceCount,
      const std::size_t                       vertexCount
    ) override {
      _arguments.migratedCellSlotPairs   = cellSlotPairs;
      _arguments.migratedFaceSlotPairs   = faceSlotPairs;
      _arguments.migratedVertexSlotPairs = vertexSlotPairs;
      _arguments.migrateCellCount        = cellCount;
      _arguments.migrateFaceCount        = faceCount;
      _arguments.migrateVertexCount      = vertexCount;
      record();
    }

    /**
     * The chunks of one replication arrive concurrently, so a lock guards the
     * record, and chunk zero alone counts as the visit.
     */
    void replicateInterfaceSlots(
      const std::vector<std::pair<int, int>>& facePairs,
      const std::vector<std::pair<int, int>>& vertexPairs,
      const int                               chunkNumber,
      const int                               numberOfChunks
    ) override {
      const std::scoped_lock lock(_replicationMutex);
      if (_arguments.replicatedChunkCalls.size() != static_cast<std::size_t>(numberOfChunks)) {
        _arguments.replicatedChunkCalls.assign(numberOfChunks, 0);
      }
      _arguments.replicatedChunkCalls[chunkNumber]++;
      _arguments.replicatedFacePairs   = facePairs;
      _arguments.replicatedVertexPairs = vertexPairs;
      if (chunkNumber == 0) {
        record();
      }
    }

    void packBoundarySlots(
      const std::vector<std::vector<int>>& facePackLists,
      const std::vector<std::vector<int>>& vertexPackLists
    ) override {
      _arguments.packedFaceLists   = facePackLists;
      _arguments.packedVertexLists = vertexPackLists;
      record();
    }

    void transferPoolSlotsForTree(
      const std::map<int, std::vector<int>>& sendCellSlots,
      const std::map<int, std::vector<int>>& receiveCellSlots,
      const std::map<int, std::vector<int>>& sendFaceSlots,
      const std::map<int, std::vector<int>>& receiveFaceSlots,
      const std::map<int, std::vector<int>>& sendVertexSlots,
      const std::map<int, std::vector<int>>& receiveVertexSlots
    ) override {
      _arguments.sendCellSlots      = sendCellSlots;
      _arguments.receiveCellSlots   = receiveCellSlots;
      _arguments.sendFaceSlots      = sendFaceSlots;
      _arguments.receiveFaceSlots   = receiveFaceSlots;
      _arguments.sendVertexSlots    = sendVertexSlots;
      _arguments.receiveVertexSlots = receiveVertexSlots;
      record();
    }

    void finishBoundaryExchange(
      const std::vector<std::vector<int>>& facePackLists,
      const std::vector<std::vector<int>>& vertexPackLists,
      const std::vector<int>&              faceFrozenSlotBases,
      const std::vector<int>&              vertexFrozenSlotBases
    ) override {
      _arguments.finishedFaceLists     = facePackLists;
      _arguments.finishedVertexLists   = vertexPackLists;
      _arguments.faceFrozenSlotBases   = faceFrozenSlotBases;
      _arguments.vertexFrozenSlotBases = vertexFrozenSlotBases;
      record();
    }

    void prepareLocalSlots(const int numberOfSlots) override {
      _arguments.numberOfLocalSlots = numberOfSlots;
      record();
    }

    int                      getNumberOfCalls() const { return _numberOfCalls.load(std::memory_order_relaxed); }
    const RecordedArguments& getArguments() const { return _arguments; }

  private:
    const int         _containerId;
    const bool        _supportsReplication;
    std::vector<int>& _visitOrder;
    std::atomic<int>& _visitCursor;
    std::atomic<int>  _numberOfCalls{0};
    RecordedArguments _arguments{};
    std::mutex        _replicationMutex{};

    void record() {
      _numberOfCalls.fetch_add(1, std::memory_order_relaxed);
      const auto position = _visitCursor.fetch_add(1, std::memory_order_relaxed);
      if (position < static_cast<int>(_visitOrder.size())) {
        _visitOrder[position] = _containerId;
      }
    }
  };

  /** Runs every registry operation once, in the order a sweep drives them. */
  void runEveryRegistryOperation() {
    auto& registry = peano4::data::ContainerRegistry::getInstance();
    registry.initialiseSerialSlots();
    registry.resizePoolsForTree(1, 1, 1);
    registry.migratePoolsForTree({}, {}, {}, 1, 1, 1);
    registry.replicateInterfaceSlots({}, {});
    registry.packBoundarySlots({}, {});
    registry.transferPoolsForTree({}, {}, {}, {}, {}, {});
    registry.finishBoundaryExchange({}, {}, {}, {});
    registry.prepareLocalSlots(1);
  }

  constexpr int NumberOfRegistryOperations = 8;
} // namespace

peano4::data::tests::ContainerRegistryTest::ContainerRegistryTest() :
  TestCase("peano4::data::tests::ContainerRegistryTest") {}

/**
 * The registry visits the containers in container-id order, whatever order they
 * registered in.
 */
void peano4::data::tests::ContainerRegistryTest::testContainersAreVisitedInContainerIdOrder() {
  std::vector<int> visitOrder(3, -1);
  std::atomic<int> visitCursor{0};
  {
    RecordingContainer thirdContainer(4200002, true, visitOrder, visitCursor);
    RecordingContainer firstContainer(4200000, true, visitOrder, visitCursor);
    RecordingContainer secondContainer(4200001, true, visitOrder, visitCursor);

    peano4::data::ContainerRegistry::getInstance().prepareLocalSlots(1);
  }

  const std::vector<int> expectedOrder{4200000, 4200001, 4200002};
  validateEquals(visitCursor.load(), static_cast<int>(expectedOrder.size()));
  for (std::size_t entry = 0; entry < expectedOrder.size(); entry++) {
    validateEquals(visitOrder[entry], expectedOrder[entry]);
  }
}

/**
 * Every registry operation reaches every registered container exactly once per
 * call.
 */
void peano4::data::tests::ContainerRegistryTest::testEveryOperationReachesEveryContainerOnce() {
  constexpr int      NumberOfRounds = 32;
  std::vector<int>   visitOrder(2 * NumberOfRounds * NumberOfRegistryOperations, -1);
  std::atomic<int>   visitCursor{0};
  RecordingContainer cellContainer(4200010, true, visitOrder, visitCursor);
  RecordingContainer faceContainer(4200011, true, visitOrder, visitCursor);

  for (int round = 0; round < NumberOfRounds; round++) {
    runEveryRegistryOperation();
  }

  validateEquals(cellContainer.getNumberOfCalls(), NumberOfRounds * NumberOfRegistryOperations);
  validateEquals(faceContainer.getNumberOfCalls(), NumberOfRounds * NumberOfRegistryOperations);
  validateEquals(visitCursor.load(), 2 * NumberOfRounds * NumberOfRegistryOperations);
}

/**
 * supportsSubtreeTaskReplication() is a conjunction over all registered
 * containers.
 */
void peano4::data::tests::ContainerRegistryTest::testSubtreeTaskReplicationNeedsEveryContainer() {
  auto&            registry = peano4::data::ContainerRegistry::getInstance();
  std::vector<int> visitOrder(2, -1);
  std::atomic<int> visitCursor{0};

  {
    RecordingContainer replicatingContainer(4200020, true, visitOrder, visitCursor);
    validate(registry.supportsSubtreeTaskReplication());
    {
      RecordingContainer exclusiveContainer(4200021, false, visitOrder, visitCursor);
      validate(not registry.supportsSubtreeTaskReplication());
    }
    validate(registry.supportsSubtreeTaskReplication());
  }
}

/**
 * A destroyed container leaves no entry in the registry.
 */
void peano4::data::tests::ContainerRegistryTest::testDetachedContainersAreNoLongerVisited() {
  std::vector<int>   visitOrder(4, -1);
  std::atomic<int>   visitCursor{0};
  RecordingContainer survivingContainer(4200030, true, visitOrder, visitCursor);

  {
    RecordingContainer temporaryContainer(4200031, true, visitOrder, visitCursor);
    peano4::data::ContainerRegistry::getInstance().prepareLocalSlots(1);
    validateEquals(temporaryContainer.getNumberOfCalls(), 1);
  }

  visitCursor.store(0);
  peano4::data::ContainerRegistry::getInstance().prepareLocalSlots(1);
  validateEquals(visitCursor.load(), 1);
  validateEquals(visitOrder[0], 4200030);
}

/**
 * Each parameter carries a value of its own, so a pair swapped anywhere along
 * the forwarding shows up as a wrong value.
 */
void peano4::data::tests::ContainerRegistryTest::testEveryOperationForwardsItsArgumentsUnchanged() {
  auto&            registry = peano4::data::ContainerRegistry::getInstance();
  std::vector<int> visitOrder(NumberOfRegistryOperations, -1);
  std::atomic<int> visitCursor{0};

  RecordingContainer container(4200040, true, visitOrder, visitCursor);

  registry.resizePoolsForTree(11, 22, 33);
  registry.migratePoolsForTree({{1, 2}}, {{3, 4}}, {{5, 6}}, 44, 55, 66);
  registry.replicateInterfaceSlots({{7, 8}}, {{9, 10}});
  registry.packBoundarySlots({{71}}, {{72}});
  registry.transferPoolsForTree({{1, {101}}}, {{2, {102}}}, {{3, {103}}}, {{4, {104}}}, {{5, {105}}}, {{6, {106}}});
  registry.finishBoundaryExchange({{81}}, {{82}}, {83}, {84});
  registry.prepareLocalSlots(9);

  const auto& arguments = container.getArguments();

  validateEquals(arguments.resizeCellCount, std::size_t{11});
  validateEquals(arguments.resizeFaceCount, std::size_t{22});
  validateEquals(arguments.resizeVertexCount, std::size_t{33});

  validateEquals(arguments.migratedCellSlotPairs.size(), std::size_t{1});
  validateEquals(arguments.migratedCellSlotPairs[0].first, 1);
  validateEquals(arguments.migratedFaceSlotPairs[0].first, 3);
  validateEquals(arguments.migratedVertexSlotPairs[0].first, 5);
  validateEquals(arguments.migrateCellCount, std::size_t{44});
  validateEquals(arguments.migrateFaceCount, std::size_t{55});
  validateEquals(arguments.migrateVertexCount, std::size_t{66});

  validateEquals(arguments.replicatedFacePairs[0].first, 7);
  validateEquals(arguments.replicatedVertexPairs[0].first, 9);
  validate(not arguments.replicatedChunkCalls.empty());
  for (const int chunkCalls : arguments.replicatedChunkCalls) {
    validateEquals(chunkCalls, 1);
  }

  validateEquals(arguments.packedFaceLists[0][0], 71);
  validateEquals(arguments.packedVertexLists[0][0], 72);

  validateEquals(arguments.sendCellSlots.at(1)[0], 101);
  validateEquals(arguments.receiveCellSlots.at(2)[0], 102);
  validateEquals(arguments.sendFaceSlots.at(3)[0], 103);
  validateEquals(arguments.receiveFaceSlots.at(4)[0], 104);
  validateEquals(arguments.sendVertexSlots.at(5)[0], 105);
  validateEquals(arguments.receiveVertexSlots.at(6)[0], 106);

  validateEquals(arguments.finishedFaceLists[0][0], 81);
  validateEquals(arguments.finishedVertexLists[0][0], 82);
  validateEquals(arguments.faceFrozenSlotBases[0], 83);
  validateEquals(arguments.vertexFrozenSlotBases[0], 84);

  validateEquals(arguments.numberOfLocalSlots, 9);
}

void peano4::data::tests::ContainerRegistryTest::run() {
  testMethod(testContainersAreVisitedInContainerIdOrder);
  testMethod(testEveryOperationReachesEveryContainerOnce);
  testMethod(testSubtreeTaskReplicationNeedsEveryContainer);
  testMethod(testDetachedContainersAreNoLongerVisited);
  testMethod(testEveryOperationForwardsItsArgumentsUnchanged);
}
