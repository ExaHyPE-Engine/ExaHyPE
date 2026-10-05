// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class RefinementRequestsTest;
} // namespace exahype2::tests

class exahype2::tests::RefinementRequestsTest final : public tarch::tests::TestCase {
public:
  RefinementRequestsTest();
  RefinementRequestsTest(const RefinementRequestsTest&)            = delete;
  RefinementRequestsTest(RefinementRequestsTest&&)                 = delete;
  RefinementRequestsTest& operator=(const RefinementRequestsTest&) = delete;
  RefinementRequestsTest& operator=(RefinementRequestsTest&&)      = delete;
  ~RefinementRequestsTest() override                               = default;

  void run() override;

private:
  void testKeepVotesRaiseNoRequest();
  void testARefineVoteBecomesARequestOnEveryRank();
  void testAnEraseStandsOnceEveryChildVotes();
  void testRefineWinsAndKeepBeatsErase();
};
