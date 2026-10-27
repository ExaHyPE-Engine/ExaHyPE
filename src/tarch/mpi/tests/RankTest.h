// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::mpi::tests {
  class RankTest;
} // namespace tarch::mpi::tests

/**
 * Checks the node layout the rank derives. The layout decides the device a rank takes.
 */
class tarch::mpi::tests::RankTest final : public tarch::tests::TestCase {
public:
  RankTest();
  RankTest(const RankTest& other)            = delete;
  RankTest(RankTest&& other)                 = delete;
  RankTest& operator=(const RankTest& other) = delete;
  RankTest& operator=(RankTest&& other)      = delete;
  ~RankTest() override                       = default;

  void run() override;

private:
  void testRankOnNodeMatchesTheSharedMemorySplit();
  void testTheNodeMasterHasNodeRankZero();
};
