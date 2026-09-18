// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::sfc::tests {
  class GrammarTest;
} // namespace peano4::sfc::tests

class peano4::sfc::tests::GrammarTest final : public tarch::tests::TestCase {
public:
  GrammarTest();
  GrammarTest(const GrammarTest&)            = delete;
  GrammarTest(GrammarTest&&)                 = delete;
  GrammarTest& operator=(const GrammarTest&) = delete;
  GrammarTest& operator=(GrammarTest&&)      = delete;
  ~GrammarTest() override                    = default;

  void run() override;

private:
  void testStepsNameAxisAndDirection();
  void testProductionSeparatesChildrenAndSteps();
};
