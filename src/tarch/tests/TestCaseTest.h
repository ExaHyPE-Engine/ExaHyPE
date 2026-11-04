// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::tests {
  class TestCaseTest;
} // namespace tarch::tests

class tarch::tests::TestCaseTest final : public tarch::tests::TestCase {
public:
  TestCaseTest();
  TestCaseTest(const TestCaseTest&)            = delete;
  TestCaseTest(TestCaseTest&&)                 = delete;
  TestCaseTest& operator=(const TestCaseTest&) = delete;
  TestCaseTest& operator=(TestCaseTest&&)      = delete;
  ~TestCaseTest() override                     = default;

  void run() override;

private:
  void testToleranceScalesWithTheLargestMagnitude();
  void testAZeroBufferGetsTheSmallestTolerance();
  void testBuffersWithinTheToleranceAreEqual();
  void testANaNComparesUnequal();
};
