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

  void run() override;

private:
  void testRoundTripRepeats();
  void testTransferChangeRebinds();
  void testMoveTransfersOwnership();
};
