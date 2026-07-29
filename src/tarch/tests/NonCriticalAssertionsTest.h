// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::tests {
  class NonCriticalAssertionsTest;
} // namespace tarch::tests

class tarch::tests::NonCriticalAssertionsTest final : public tarch::tests::TestCase {
public:
  NonCriticalAssertionsTest();

  void run() override;

private:
  void testNoAssertionIsNotAdopted();
};
