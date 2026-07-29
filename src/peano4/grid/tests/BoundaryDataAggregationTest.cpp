// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "BoundaryDataAggregationTest.h"

#include "peano4/grid/BoundaryDataAggregation.h"
#include "peano4/stacks/Stack.h"

namespace {
  using Aggregation = peano4::grid::BoundaryDataAggregation<peano4::stacks::Stack<double>>;
} // namespace

peano4::grid::tests::BoundaryDataAggregationTest::BoundaryDataAggregationTest() :
  TestCase("peano4::grid::tests::BoundaryDataAggregationTest") {}

void peano4::grid::tests::BoundaryDataAggregationTest::run() {
  testMethod(testPrepareArmsContributorCountdown);
  testMethod(testPrepareIsIdempotentWithinOneGeneration);
  testMethod(testPrepareClearsBuffersForTheNextGeneration);
  testMethod(testChangedRankSetDiscardsRecords);
  testMethod(testAppendOutgoingMovesPayloadAndRecordsSegment);
}

/** prepare() arms every rank's countdown with its contributor count. */
void peano4::grid::tests::BoundaryDataAggregationTest::testPrepareArmsContributorCountdown() {
  Aggregation aggregation;
  aggregation.prepare(1, {1, 3}, {2, 5});

  validate(aggregation.isPreparedFor(1));
  validate(not aggregation.isPreparedFor(2));
  validateEquals(aggregation.getRankExchange(1).outstandingContributors.load(), 2);
  validateEquals(aggregation.getRankExchange(3).outstandingContributors.load(), 5);
}

/** A second prepare() of the same sweep leaves the countdown alone, so late trees cannot re-arm it. */
void peano4::grid::tests::BoundaryDataAggregationTest::testPrepareIsIdempotentWithinOneGeneration() {
  Aggregation aggregation;
  aggregation.prepare(1, {2}, {3});
  aggregation.getRankExchange(2).outstandingContributors.fetch_sub(1);

  aggregation.prepare(1, {2}, {3});
  validateEquals(aggregation.getRankExchange(2).outstandingContributors.load(), 2);
}

/** The next sweep starts with empty buffers, while the records themselves survive an unchanged rank set. */
void peano4::grid::tests::BoundaryDataAggregationTest::testPrepareClearsBuffersForTheNextGeneration() {
  Aggregation aggregation;
  aggregation.prepare(1, {4}, {1});

  peano4::stacks::Stack<double> source;
  source.push(0.5);
  aggregation.appendOutgoing(aggregation.getRankExchange(4), 10, 20, source);
  validateEquals(aggregation.getRankExchange(4).outgoingPayload.size(), 1);
  validateEquals(aggregation.getRankExchange(4).outgoingSegments.size(), 1);

  aggregation.prepare(2, {4}, {1});
  validate(aggregation.isPreparedFor(2));
  validate(aggregation.getRankExchange(4).outgoingPayload.empty());
  validate(aggregation.getRankExchange(4).outgoingSegments.empty());
  validate(aggregation.getRankExchange(4).incomingSegments.empty());
}

/** A changed rank set discards the records, which only happens when the partition changes. */
void peano4::grid::tests::BoundaryDataAggregationTest::testChangedRankSetDiscardsRecords() {
  Aggregation aggregation;
  aggregation.prepare(1, {1}, {1});

  peano4::stacks::Stack<double> source;
  source.push(2.0);
  aggregation.appendOutgoing(aggregation.getRankExchange(1), 0, 6, source);

  aggregation.prepare(2, {1, 2}, {1, 1});
  validate(aggregation.getRankExchange(1).outgoingPayload.empty());
  validate(aggregation.getRankExchange(1).outgoingSegments.empty());
  validateEquals(aggregation.getRankExchange(2).outstandingContributors.load(), 1);
}

/** appendOutgoing() moves the tree's data into the rank buffer and describes it by one segment. */
void peano4::grid::tests::BoundaryDataAggregationTest::testAppendOutgoingMovesPayloadAndRecordsSegment() {
  Aggregation aggregation;
  aggregation.prepare(1, {7}, {2});

  peano4::stacks::Stack<double> firstSource;
  firstSource.push(1.25);
  firstSource.push(2.5);
  aggregation.appendOutgoing(aggregation.getRankExchange(7), 3, 9, firstSource);

  peano4::stacks::Stack<double> secondSource;
  secondSource.push(4.0);
  aggregation.appendOutgoing(aggregation.getRankExchange(7), 5, 11, secondSource);

  auto& exchange = aggregation.getRankExchange(7);
  validate(firstSource.empty());
  validate(secondSource.empty());
  validateEquals(exchange.outgoingPayload.size(), 3);
  validateNumericalEquals(exchange.outgoingPayload[0], 1.25);
  validateNumericalEquals(exchange.outgoingPayload[1], 2.5);
  validateNumericalEquals(exchange.outgoingPayload[2], 4.0);

  validateEquals(exchange.outgoingSegments.size(), 2);
  validateEquals(exchange.outgoingSegments[0].senderTreeId, 3);
  validateEquals(exchange.outgoingSegments[0].receiverTreeId, 9);
  validateEquals(exchange.outgoingSegments[0].numberOfElements, 2);
  validateEquals(exchange.outgoingSegments[1].senderTreeId, 5);
  validateEquals(exchange.outgoingSegments[1].receiverTreeId, 11);
  validateEquals(exchange.outgoingSegments[1].numberOfElements, 1);
}
