// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::mpi::tests {
  class ReductionRecordTest;
} // namespace tarch::mpi::tests

/**
 * Checks that one allreduce sums, minimises and maximises every slot of a record over the ranks.
 */
class tarch::mpi::tests::ReductionRecordTest final : public tarch::tests::TestCase {
public:
  ReductionRecordTest();
  ReductionRecordTest(const ReductionRecordTest&)            = delete;
  ReductionRecordTest(ReductionRecordTest&&)                 = delete;
  ReductionRecordTest& operator=(const ReductionRecordTest&) = delete;
  ReductionRecordTest& operator=(ReductionRecordTest&&)      = delete;
  ~ReductionRecordTest() override                            = default;

  void run() override;

private:
  void testAllreduceReducesEverySlotOverTheRanks();
  void testDoublesKeepTheirValueWhenEveryRankAddsThemAll();
};
