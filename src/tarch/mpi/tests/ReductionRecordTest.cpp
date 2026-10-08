// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReductionRecordTest.h"

#include "tarch/mpi/Rank.h"
#include "tarch/mpi/ReductionRecord.h"
#include "tarch/tests/TestMacros.h"

namespace {
  /**
   * Returns the double that rank adds. Even ranks add negative and odd ranks add positive values.
   */
  double getDoubleOfRank(const int rank) { return rank % 2 == 0 ? -1.5 - rank : 0.25 * rank; }
} // namespace

tarch::mpi::tests::ReductionRecordTest::ReductionRecordTest() :
  TestCase("tarch::mpi::tests::ReductionRecordTest") {}

void
tarch::mpi::tests::ReductionRecordTest::run() {
  testMethod(testAllreduceReducesEverySlotOverTheRanks);
  testMethod(testDoublesKeepTheirValueWhenEveryRankAddsThemAll);
  testMethod(testTwoPostedRecordsReduceIndependently);
  testMethod(testRecordsOfTheSameLayoutAssignTheSameSlots);
}

/**
 * A record posted after another one reduces its own values, even when it completes first.
 */
void
tarch::mpi::tests::ReductionRecordTest::testTwoPostedRecordsReduceIndependently() {
  const auto& rank      = tarch::mpi::Rank::getInstance();
  const auto  thisRank  = static_cast<std::uint64_t>(rank.getRank());
  const auto  ranks     = static_cast<std::uint64_t>(rank.getNumberOfRanks());
  auto        earlier   = std::make_unique<tarch::mpi::ReductionRecord>();
  auto        later     = std::make_unique<tarch::mpi::ReductionRecord>();
  const int   earlySlot = earlier->addSum(thisRank + 1);
  const int   lateSlot  = later->addMaximum(thisRank + 10);
  earlier->post(rank.getCommunicator());
  later->post(rank.getCommunicator());
  validate(earlier->isPosted() and later->isPosted());

  later->wait();
  earlier->wait();
  validate(not earlier->isPosted() and not later->isPosted());
  validateEquals(earlier->getSum(earlySlot), ranks * (ranks + 1) / 2);
  validateEquals(later->getMaximum(lateSlot), ranks + 9);
}

/**
 * Two records with as many sums, minima and maxima have the same layout, whatever their values.
 */
void
tarch::mpi::tests::ReductionRecordTest::testRecordsOfTheSameLayoutAssignTheSameSlots() {
  tarch::mpi::ReductionRecord first;
  tarch::mpi::ReductionRecord second;
  validate(first.hasLayoutOf(second));
  static_cast<void>(first.addSum(1));
  static_cast<void>(first.addMinimum(2));
  validate(not first.hasLayoutOf(second));
  static_cast<void>(second.addSum(3));
  static_cast<void>(second.addMinimum(4));
  validate(first.hasLayoutOf(second));
  static_cast<void>(second.addMaximum(5));
  validate(not first.hasLayoutOf(second));
}

/**
 * Every slot holds the sum, minimum or maximum of the values that the ranks add to it.
 */
void
tarch::mpi::tests::ReductionRecordTest::testAllreduceReducesEverySlotOverTheRanks() {
  const auto& rank          = tarch::mpi::Rank::getInstance();
  const int   numberOfRanks = rank.getNumberOfRanks();
  const auto  thisRank      = static_cast<std::uint64_t>(rank.getRank());

  tarch::mpi::ReductionRecord record;
  const int                   sumSlot           = record.addSum(thisRank + 1);
  const int                   minimumSlot       = record.addMinimum(thisRank + 5);
  const int                   maximumSlot       = record.addMaximum(thisRank + 5);
  const int                   secondSumSlot     = record.addSum(2);
  const int                   minimumDoubleSlot = record.addMinimumOfDouble(getDoubleOfRank(rank.getRank()));
  const int                   maximumDoubleSlot = record.addMaximumOfDouble(getDoubleOfRank(rank.getRank()));
  const std::array<std::uint64_t, 2> pair{thisRank, 7};
  const int                          firstSumOfPair     = record.addSums(pair);
  const int                          firstMinimumOfPair = record.addMinima(pair);
  const int                          firstMaximumOfPair = record.addMaxima(pair);
  record.allreduce(rank.getCommunicator());

  double expectedMinimum = getDoubleOfRank(0);
  double expectedMaximum = getDoubleOfRank(0);
  for (int other = 1; other < numberOfRanks; other++) {
    expectedMinimum = std::min(expectedMinimum, getDoubleOfRank(other));
    expectedMaximum = std::max(expectedMaximum, getDoubleOfRank(other));
  }
  const auto ranks = static_cast<std::uint64_t>(numberOfRanks);
  validateEquals(sumSlot, 0);
  validateEquals(secondSumSlot, 1);
  validateEquals(record.getSum(sumSlot), ranks * (ranks + 1) / 2);
  validateEquals(record.getSum(secondSumSlot), 2 * ranks);
  validateEquals(record.getMinimum(minimumSlot), std::uint64_t{5});
  validateEquals(record.getMaximum(maximumSlot), ranks + 4);
  validateEquals(record.getMinimumOfDouble(minimumDoubleSlot), expectedMinimum);
  validateEquals(record.getMaximumOfDouble(maximumDoubleSlot), expectedMaximum);
  validateEquals(firstSumOfPair, 2);
  validateEquals(record.getSum(firstSumOfPair), ranks * (ranks - 1) / 2);
  validateEquals(record.getSum(firstSumOfPair + 1), 7 * ranks);
  validateEquals(record.getMinimum(firstMinimumOfPair), std::uint64_t{0});
  validateEquals(record.getMinimum(firstMinimumOfPair + 1), std::uint64_t{7});
  validateEquals(record.getMaximum(firstMaximumOfPair), ranks - 1);
  validateEquals(record.getMaximum(firstMaximumOfPair + 1), std::uint64_t{7});
}

/**
 * A double that every rank adds leaves the reduction bitwise unchanged, signed zeros, subnormal
 * numbers and infinities included.
 */
void
tarch::mpi::tests::ReductionRecordTest::testDoublesKeepTheirValueWhenEveryRankAddsThemAll() {
  constexpr std::array<double, 8> Values{
    -std::numeric_limits<double>::infinity(),
    -3.5,
    -0.0,
    0.0,
    std::numeric_limits<double>::denorm_min(),
    2.0,
    std::numeric_limits<double>::max(),
    std::numeric_limits<double>::infinity()
  };
  tarch::mpi::ReductionRecord record;
  std::array<int, Values.size()> minimumSlots{};
  std::array<int, Values.size()> maximumSlots{};
  for (std::size_t value = 0; value < Values.size(); value++) {
    minimumSlots[value] = record.addMinimumOfDouble(Values[value]);
    maximumSlots[value] = record.addMaximumOfDouble(Values[value]);
  }
  record.allreduce(tarch::mpi::Rank::getInstance().getCommunicator());

  for (std::size_t value = 0; value < Values.size(); value++) {
    const auto expectedBits = std::bit_cast<std::uint64_t>(Values[value]);
    validateEqualsWithParams1(
      std::bit_cast<std::uint64_t>(record.getMinimumOfDouble(minimumSlots[value])),
      expectedBits,
      value
    );
    validateEqualsWithParams1(
      std::bit_cast<std::uint64_t>(record.getMaximumOfDouble(maximumSlots[value])),
      expectedBits,
      value
    );
  }
}
