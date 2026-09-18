// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace runner::tests {
  class RuntimeParametersTest;
} // namespace runner::tests

class runner::tests::RuntimeParametersTest final : public tarch::tests::TestCase {
public:
  RuntimeParametersTest();
  RuntimeParametersTest(const RuntimeParametersTest&)            = delete;
  RuntimeParametersTest(RuntimeParametersTest&&)                 = delete;
  RuntimeParametersTest& operator=(const RuntimeParametersTest&) = delete;
  RuntimeParametersTest& operator=(RuntimeParametersTest&&)      = delete;
  ~RuntimeParametersTest() override                              = default;

  void run() override;

private:
  void testNumbersParseFromTheWholeArgument();
  void testTheOutputPathAlwaysEndsInASeparator();
  void testTheMeshSizeDividesOncePerLevel();
  void testDefaultsReadBackAsTheyWereGiven();
  void testBothSpellingsOfTheLoadBalancingSwitchAreConsumed();
  void testTheMaximumTerminalTimeBoundsTheMinimumOne();
};
