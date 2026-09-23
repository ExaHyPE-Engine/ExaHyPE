// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class FixedTimeSteppingTest;
} // namespace exahype2::tests

class exahype2::tests::FixedTimeSteppingTest final : public tarch::tests::TestCase {
public:
  FixedTimeSteppingTest();
  FixedTimeSteppingTest(const FixedTimeSteppingTest&)            = delete;
  FixedTimeSteppingTest(FixedTimeSteppingTest&&)                 = delete;
  FixedTimeSteppingTest& operator=(const FixedTimeSteppingTest&) = delete;
  FixedTimeSteppingTest& operator=(FixedTimeSteppingTest&&)      = delete;
  ~FixedTimeSteppingTest() override                              = default;

  void run() override;

private:
  void testTimeStepSizeIsZeroUntilStored();
};
