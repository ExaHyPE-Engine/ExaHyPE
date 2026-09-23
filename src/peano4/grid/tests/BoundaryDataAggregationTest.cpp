// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "BoundaryDataAggregationTest.h"

#include "peano4/grid/BoundaryDataAggregation.h"
#include "peano4/grid/Stack.h"

namespace {
  using Aggregation = peano4::grid::BoundaryDataAggregation<peano4::grid::Stack<double>>;
} // namespace

peano4::grid::tests::BoundaryDataAggregationTest::BoundaryDataAggregationTest() :
  tarch::tests::TestCase("peano4::grid::tests::BoundaryDataAggregationTest") {}

void peano4::grid::tests::BoundaryDataAggregationTest::run() {
  testMethod(testPrepareIsIdempotentWithinOneGeneration);
  testMethod(testPrepareClearsBuffersForTheNextGeneration);
  testMethod(testRankExchangesAdvanceGenerationsIndependently);
  testMethod(testRebuildRankTableKeepsRecordsForUnchangedRankSet);
  testMethod(testRebuildRankTableDiscardsRecordsForChangedRankSet);
  testMethod(testRebuildRankTableSkipsKnownTopologyVersion);
}

/** A second prepare of the same sweep keeps the packed data. */
void peano4::grid::tests::BoundaryDataAggregationTest::testPrepareIsIdempotentWithinOneGeneration() {
  Aggregation aggregation;
  aggregation.rebuildRankTable(1, {2});
  aggregation.prepareRankExchange(1, 2);

  aggregation.getRankExchange(2).outgoingPayload.push(0.25);

  aggregation.prepareRankExchange(1, 2);
  validateEquals(aggregation.getRankExchange(2).outgoingPayload.size(), 1);
  validateEquals(aggregation.getRankExchange(2).preparedGeneration, 1);
}

/** The next sweep starts with empty buffers, while the record itself survives. */
void peano4::grid::tests::BoundaryDataAggregationTest::testPrepareClearsBuffersForTheNextGeneration() {
  Aggregation aggregation;
  aggregation.rebuildRankTable(1, {4});
  aggregation.prepareRankExchange(1, 4);

  aggregation.getRankExchange(4).outgoingPayload.push(0.5);
  aggregation.getRankExchange(4).transferComplete.store(true);
  validateEquals(aggregation.getRankExchange(4).outgoingPayload.size(), 1);

  aggregation.prepareRankExchange(2, 4);
  validateEquals(aggregation.getRankExchange(4).preparedGeneration, 2);
  validate(aggregation.getRankExchange(4).outgoingPayload.empty());
  validate(not aggregation.getRankExchange(4).transferComplete.load());
}

/** One rank pair may already pack the next sweep while another still describes the previous one. */
void peano4::grid::tests::BoundaryDataAggregationTest::testRankExchangesAdvanceGenerationsIndependently() {
  Aggregation aggregation;
  aggregation.rebuildRankTable(1, {1, 5});
  aggregation.prepareRankExchange(1, 1);
  aggregation.prepareRankExchange(1, 5);

  aggregation.prepareRankExchange(2, 1);
  validateEquals(aggregation.getRankExchange(1).preparedGeneration, 2);
  validateEquals(aggregation.getRankExchange(5).preparedGeneration, 1);
}

/** A new topology whose rank set is unchanged keeps the records and their grown capacity. */
void peano4::grid::tests::BoundaryDataAggregationTest::testRebuildRankTableKeepsRecordsForUnchangedRankSet() {
  Aggregation aggregation;
  aggregation.rebuildRankTable(1, {1});
  aggregation.prepareRankExchange(1, 1);

  aggregation.getRankExchange(1).outgoingPayload.push(2.0);

  aggregation.rebuildRankTable(2, {1});
  validateEquals(aggregation.getRankExchange(1).outgoingPayload.size(), 1);
  validateEquals(aggregation.getRankExchange(1).preparedGeneration, 1);
}

/** A changed rank set discards the records. */
void peano4::grid::tests::BoundaryDataAggregationTest::testRebuildRankTableDiscardsRecordsForChangedRankSet() {
  Aggregation aggregation;
  aggregation.rebuildRankTable(1, {1});
  aggregation.prepareRankExchange(1, 1);

  aggregation.getRankExchange(1).outgoingPayload.push(2.0);

  aggregation.rebuildRankTable(2, {1, 2});
  validate(aggregation.getRankExchange(1).outgoingPayload.empty());
  validateEquals(aggregation.getRankExchange(1).preparedGeneration, -1);
  validateEquals(aggregation.getRankExchange(2).preparedGeneration, -1);
}

/** Reusing the current topology version leaves the existing records unchanged. */
void peano4::grid::tests::BoundaryDataAggregationTest::testRebuildRankTableSkipsKnownTopologyVersion() {
  Aggregation aggregation;
  aggregation.rebuildRankTable(1, {1});
  aggregation.prepareRankExchange(1, 1);

  aggregation.rebuildRankTable(1, {1, 2});
  validateEquals(aggregation.getRankExchange(1).preparedGeneration, 1);
}
