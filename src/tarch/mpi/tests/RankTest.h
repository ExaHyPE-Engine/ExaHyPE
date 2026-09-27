// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::mpi::tests {
  class RankTest;
} // namespace tarch::mpi::tests

/** Checks the node layout the rank derives, which decides the device a rank takes. */
class tarch::mpi::tests::RankTest final : public tarch::tests::TestCase {
public:
  RankTest();
  RankTest(const RankTest&)            = delete;
  RankTest(RankTest&&)                 = delete;
  RankTest& operator=(const RankTest&) = delete;
  RankTest& operator=(RankTest&&)      = delete;
  ~RankTest() override                 = default;

  void run() override;

private:
  void testRankOnNodeMatchesTheSharedMemorySplit();
  void testNodeMasterHoldsNodeRankZero();
};
