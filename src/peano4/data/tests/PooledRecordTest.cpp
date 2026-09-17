// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PooledRecordTest.h"

#include "peano4/data/PooledRecord.h"
#include "peano4/grid/Stack.h"
#include "peano4/grid/StackExchange.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tests/TestMacros.h"

namespace {
  struct TestRecord final : peano4::data::PooledRecord<TestRecord, double, 4, 4> {};

  using Pool = TestRecord::MemoryPool;

  constexpr int Tag = 4322;

  /** Carries the refinement state that load() reads. */
  struct RefinementMarker final {
    bool refined{false};

    [[nodiscard]] bool hasBeenRefined() const { return refined; }
  };

  void fill(TestRecord& record, const double seed) {
    double* const values = record.data();
    for (int index = 0; index < TestRecord::Cardinality; index++) {
      values[index] = seed + index;
    }
  }

  bool holds(const TestRecord& record, const double seed) {
    const double* const values = record.data();
    for (int index = 0; index < TestRecord::Cardinality; index++) {
      if (values[index] != seed + index) {
        return false;
      }
    }
    return true;
  }

  bool readsZero(const TestRecord& record) {
    const double* const values = record.data();
    return std::all_of(values, values + TestRecord::Cardinality, [](const double value) { return value == 0.0; });
  }

  /** Polls both transfers until neither has anything left to do. */
  void completeTransfer(
    peano4::grid::Stack<TestRecord>&            sender,
    peano4::grid::StackExchangeJob<TestRecord>& sendJob,
    peano4::grid::Stack<TestRecord>&            receiver,
    peano4::grid::StackExchangeJob<TestRecord>& receiveJob
  ) {
    bool sendFinished    = false;
    bool receiveFinished = false;
    while (not sendFinished or not receiveFinished) {
      sendFinished    = sendFinished or peano4::grid::tryToFinishSendOrReceive(sender, sendJob);
      receiveFinished = receiveFinished or peano4::grid::tryToFinishSendOrReceive(receiver, receiveJob);
    }
  }
} // namespace

peano4::data::tests::PooledRecordTest::PooledRecordTest() :
  TestCase("peano4::data::tests::PooledRecordTest") {}

void peano4::data::tests::PooledRecordTest::run() {
  testMethod(testUnboundRecordAllocatesClearedStorage);
  testMethod(testCopyDuplicatesValues);
  testMethod(testMoveTransfersOwnStorage);
  testMethod(testBindPoolStorageKeepsOwnValues);
  testMethod(testRebindAfterGrowthKeepsValues);
  testMethod(testBlankSourceClearsTarget);
  testMethod(testReferenceValuesOf);
  testMethod(testLoadFollowsRefinement);
  testMethod(testExchangeToSelf);
}

/**
 * The first data() call of an unbound record hands out cleared private storage.
 */
void peano4::data::tests::PooledRecordTest::testUnboundRecordAllocatesClearedStorage() {
  TestRecord    record;
  double* const values = record.data();

  validate(std::all_of(values, values + TestRecord::Cardinality, [](const double value) { return value == 0.0; }));

  fill(record, 1.0);
  validate(holds(record, 1.0));
}

/**
 * A copy holds the source's values in storage of its own.
 */
void peano4::data::tests::PooledRecordTest::testCopyDuplicatesValues() {
  TestRecord source;
  fill(source, 2.0);

  const TestRecord copy(source);

  validate(holds(copy, 2.0));
  validate(copy.data() != std::as_const(source).data());
  fill(source, 3.0);
  validate(holds(copy, 2.0));
}

/**
 * A move hands the private storage over without copying the values.
 */
void peano4::data::tests::PooledRecordTest::testMoveTransfersOwnStorage() {
  TestRecord source;
  fill(source, 3.0);
  const double* const values = source.data();

  const TestRecord moved(std::move(source));

  validate(moved.data() == values);
  validate(holds(moved, 3.0));
}

/**
 * Binding hands every slot its chunk and moves values written before the
 * binding into the chunk.
 */
void peano4::data::tests::PooledRecordTest::testBindPoolStorageKeepsOwnValues() {
  std::vector<TestRecord> slots(3);
  fill(slots[1], 5.0);

  TestRecord::bindPoolStorage(slots);

  Pool& pool = TestRecord::getMemoryPool();
  validateEquals(pool.size(), std::size_t{3});
  for (int slot = 0; slot < 3; slot++) {
    validate(std::as_const(slots[slot]).data() == pool.chunk(static_cast<Pool::SlotIndex>(slot)));
  }
  validate(readsZero(slots[0]));
  validate(holds(slots[1], 5.0));
  validate(readsZero(slots[2]));

  slots.clear();
  pool.clear();
}

/**
 * A binding after the slot vector grew keeps the bound values and hands the
 * new slots cleared chunks.
 */
void peano4::data::tests::PooledRecordTest::testRebindAfterGrowthKeepsValues() {
  std::vector<TestRecord> slots(3);
  TestRecord::bindPoolStorage(slots);
  fill(slots[2], 7.0);

  slots.resize(64);
  TestRecord::bindPoolStorage(slots);

  Pool& pool = TestRecord::getMemoryPool();
  validateEquals(pool.size(), std::size_t{64});
  validate(std::as_const(slots[2]).data() == pool.chunk(2));
  validate(holds(slots[2], 7.0));
  validate(readsZero(slots[0]));
  validate(readsZero(slots[63]));

  slots.clear();
  pool.clear();
}

/**
 * Assigning a blank record clears a bound target and keeps its chunk.
 */
void peano4::data::tests::PooledRecordTest::testBlankSourceClearsTarget() {
  std::vector<TestRecord> slots(2);
  TestRecord::bindPoolStorage(slots);
  fill(slots[0], 9.0);

  slots[0] = slots[1];

  validate(readsZero(slots[0]));
  validate(std::as_const(slots[0]).data() == TestRecord::getMemoryPool().chunk(0));

  slots.clear();
  TestRecord::getMemoryPool().clear();
}

/**
 * A referencing record reads the chunk of the referenced record.
 */
void peano4::data::tests::PooledRecordTest::testReferenceValuesOf() {
  std::vector<TestRecord> slots(1);
  TestRecord::bindPoolStorage(slots);
  fill(slots[0], 4.0);
  TestRecord alias;

  alias.referenceValuesOf(slots[0]);

  validate(std::as_const(alias).data() == std::as_const(slots[0]).data());
  validate(holds(alias, 4.0));

  slots.clear();
  TestRecord::getMemoryPool().clear();
}

/**
 * load() keeps the data of an entity the grid has not refined.
 */
void peano4::data::tests::PooledRecordTest::testLoadFollowsRefinement() {
  validate(TestRecord::load(RefinementMarker{false}));
  validate(not TestRecord::load(RefinementMarker{true}));
}

/**
 * The values of a stack arrive on the receiving stack, and a blank record
 * arrives as zeros.
 */
void peano4::data::tests::PooledRecordTest::testExchangeToSelf() {
  const auto& rank      = tarch::mpi::Rank::getInstance();
  const int   localRank = rank.getRank();

  peano4::grid::Stack<TestRecord>            sender;
  peano4::grid::Stack<TestRecord>            receiver;
  peano4::grid::StackExchangeJob<TestRecord> sendJob;
  peano4::grid::StackExchangeJob<TestRecord> receiveJob;
  sender.resize(3);
  fill(sender[0], 1.0);
  fill(sender[2], 2.0);

  peano4::grid::startReceive(receiver, receiveJob, localRank, Tag, rank.getCommunicator(), 3);
  peano4::grid::startSend(sender, sendJob, localRank, Tag, rank.getCommunicator());
  completeTransfer(sender, sendJob, receiver, receiveJob);

  validate(sender.empty());
  validateEquals(receiver.size(), std::size_t{3});
  validate(holds(receiver[0], 1.0));
  validate(readsZero(receiver[1]));
  validate(holds(receiver[2], 2.0));
}
