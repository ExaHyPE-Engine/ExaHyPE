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

  void run() override;

private:
  void testPrepareArmsContributorCountdown();
  void testPrepareIsIdempotentWithinOneGeneration();
  void testPrepareClearsBuffersForTheNextGeneration();
  void testChangedRankSetDiscardsRecords();
  void testAppendOutgoingMovesPayloadAndRecordsSegment();
};
