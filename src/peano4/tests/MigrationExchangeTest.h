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
  MigrationExchangeTest(const MigrationExchangeTest& other)            = delete;
  MigrationExchangeTest(MigrationExchangeTest&& other)                 = delete;
  MigrationExchangeTest& operator=(const MigrationExchangeTest& other) = delete;
  MigrationExchangeTest& operator=(MigrationExchangeTest&& other)      = delete;
  ~MigrationExchangeTest() override                                    = default;

  void run() override;

private:
  void testEveryFilledSlotStoresTheChunkOfItsCell();
  void testAPreparingTransferFillsEverySlotWithTheChunkOfItsCell();
  void testExchangesGetDistinctTags();
  void testReleaseFreesTheBuffersAndKeepsTheTag();
};
