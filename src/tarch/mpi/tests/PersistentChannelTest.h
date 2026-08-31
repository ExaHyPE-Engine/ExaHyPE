// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::mpi::tests {
  class PersistentChannelTest;
} // namespace tarch::mpi::tests

class tarch::mpi::tests::PersistentChannelTest final : public tarch::tests::TestCase {
public:
  PersistentChannelTest();
  PersistentChannelTest(const PersistentChannelTest&)            = delete;
  PersistentChannelTest(PersistentChannelTest&&)                 = delete;
  PersistentChannelTest& operator=(const PersistentChannelTest&) = delete;
  PersistentChannelTest& operator=(PersistentChannelTest&&)      = delete;
  ~PersistentChannelTest() override                              = default;

  void run() override;

private:
  void testRoundTripRepeats();
  void testTransferChangeRebinds();
  void testMoveTransfersOwnership();
};
