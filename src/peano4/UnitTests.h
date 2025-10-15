// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4 {
  /**
   * Construct suite of unit tests for Peano 4
   *
   * Please destroy after usage. If you want to add a new unit test (set) for
   * Peano, you have to add it here. In principle, the order in this routine
   * should not make a difference: Eventually, all tests should pass anyway.
   */
  tarch::tests::TestCase* getUnitTests();
}
