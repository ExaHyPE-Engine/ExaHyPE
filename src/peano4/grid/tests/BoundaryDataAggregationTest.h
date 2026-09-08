// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class BoundaryDataAggregationTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::BoundaryDataAggregationTest final : public tarch::tests::TestCase {
public:
  BoundaryDataAggregationTest();
  BoundaryDataAggregationTest(const BoundaryDataAggregationTest&)            = delete;
  BoundaryDataAggregationTest(BoundaryDataAggregationTest&&)                 = delete;
  BoundaryDataAggregationTest& operator=(const BoundaryDataAggregationTest&) = delete;
  BoundaryDataAggregationTest& operator=(BoundaryDataAggregationTest&&)      = delete;
  ~BoundaryDataAggregationTest() override                                    = default;

  void run() override;

private:
  void testPrepareIsIdempotentWithinOneGeneration();
  void testPrepareClearsBuffersForTheNextGeneration();
  void testRankExchangesAdvanceGenerationsIndependently();
  void testRebuildRankTableKeepsRecordsForUnchangedRankSet();
  void testRebuildRankTableDiscardsRecordsForChangedRankSet();
  void testRebuildRankTableSkipsKnownTopologyVersion();
};
