// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::mpi::tests {
  class RequestGroupTest;
} // namespace tarch::mpi::tests

class tarch::mpi::tests::RequestGroupTest final : public tarch::tests::TestCase {
public:
  RequestGroupTest();
  RequestGroupTest(const RequestGroupTest&)            = delete;
  RequestGroupTest(RequestGroupTest&&)                 = delete;
  RequestGroupTest& operator=(const RequestGroupTest&) = delete;
  RequestGroupTest& operator=(RequestGroupTest&&)      = delete;
  ~RequestGroupTest() override                         = default;

  void run() override;

private:
  void testWaitAllCompletesPostedTransfers();
  void testEmptyGroupNeedsNoWait();
  void testClearedGroupStartsOver();
};
