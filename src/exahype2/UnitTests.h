// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once
#ifndef _EXAHYPE2_UNIT_TESTS_H_
#define _EXAHYPE2_UNIT_TESTS_H_

#include "tarch/tests/TestCase.h"

namespace exahype2 {
  /**
   * Please destroy after usage
   */
  tarch::tests::TestCase* getUnitTests();
}

#endif
