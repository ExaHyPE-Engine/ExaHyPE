// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::tests {
  class MigrationExchangeTest;
} // namespace peano4::tests

class peano4::tests::MigrationExchangeTest final : public tarch::tests::TestCase {
public:
  MigrationExchangeTest();
  MigrationExchangeTest(const MigrationExchangeTest&)            = delete;
  MigrationExchangeTest(MigrationExchangeTest&&)                 = delete;
  MigrationExchangeTest& operator=(const MigrationExchangeTest&) = delete;
  MigrationExchangeTest& operator=(MigrationExchangeTest&&)      = delete;
  ~MigrationExchangeTest() override                              = default;

  void run() override;

private:
  void testEveryFilledSlotHoldsTheChunkOfItsCell();
  void testExchangesHoldDistinctTags();
  void testReleaseFreesTheBuffersAndKeepsTheTag();
};
