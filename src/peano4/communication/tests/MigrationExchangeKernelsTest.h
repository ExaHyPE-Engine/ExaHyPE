// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::communication::tests {
  class MigrationExchangeKernelsTest;
} // namespace peano4::communication::tests

/**
 * Checks that the migration kernels fill a child from its parent, a parent from its children, the
 * send buffer from every kind of sent cell and the received slots from the receive buffer.
 */
class peano4::communication::tests::MigrationExchangeKernelsTest final : public tarch::tests::TestCase {
public:
  MigrationExchangeKernelsTest();
  MigrationExchangeKernelsTest(const MigrationExchangeKernelsTest& other)            = delete;
  MigrationExchangeKernelsTest(MigrationExchangeKernelsTest&& other)                 = delete;
  MigrationExchangeKernelsTest& operator=(const MigrationExchangeKernelsTest& other) = delete;
  MigrationExchangeKernelsTest& operator=(MigrationExchangeKernelsTest&& other)      = delete;
  ~MigrationExchangeKernelsTest() override                                           = default;

  void run() override;

private:
  void testProlongFillsTheChildFromItsParent();
  void testRestrictFillsTheParentFromItsChildren();
  void testGatherTakesEveryKindOfSentCell();
  void testPreparationReachesEveryWorkItemOfItsChunk();
  void testScatterWritesTheReceivedChunks();
};
