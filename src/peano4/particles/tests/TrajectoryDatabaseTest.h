// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::particles::tests {
  class TrajectoryDatabaseTest;
} // namespace peano4::particles::tests

class peano4::particles::tests::TrajectoryDatabaseTest final : public tarch::tests::TestCase {
private:
  void testAppendsChronologicallyWithoutDuplication();
  void testClearRewritesFileFromScratch();

public:
  TrajectoryDatabaseTest();
  ~TrajectoryDatabaseTest() override = default;

  void run() override;
};
