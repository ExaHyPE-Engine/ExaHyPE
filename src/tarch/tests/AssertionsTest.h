// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::tests {
  class AssertionsTest;
} // namespace tarch::tests

class tarch::tests::AssertionsTest final : public tarch::tests::TestCase {
public:
  AssertionsTest();
  AssertionsTest(const AssertionsTest& other)            = delete;
  AssertionsTest(AssertionsTest&& other)                 = delete;
  AssertionsTest& operator=(const AssertionsTest& other) = delete;
  AssertionsTest& operator=(AssertionsTest&& other)      = delete;
  ~AssertionsTest() override                             = default;

  void run() override;

private:
  void testAConstexprFunctionCarriesAnAssertion();
};
