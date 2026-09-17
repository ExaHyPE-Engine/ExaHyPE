// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::data::tests {
  class PooledRecordTest;
} // namespace peano4::data::tests

/**
 * Checks that a pooled record owns storage until its slot is bound, keeps its
 * values across a binding, and ships its values through the stack exchange.
 */
class peano4::data::tests::PooledRecordTest final : public tarch::tests::TestCase {
public:
  PooledRecordTest();

  void run() override;

private:
  void testUnboundRecordAllocatesClearedStorage();
  void testCopyDuplicatesValues();
  void testMoveTransfersOwnStorage();
  void testBindPoolStorageKeepsOwnValues();
  void testRebindAfterGrowthKeepsValues();
  void testBlankSourceClearsTarget();
  void testReferenceValuesOf();
  void testLoadFollowsRefinement();
  void testExchangeToSelf();
};
