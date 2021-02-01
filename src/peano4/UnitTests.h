// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_UNIT_TESTS_H_
#define _PEANO4_UNIT_TESTS_H_


#include "tarch/tests/TestCase.h"


namespace peano4 {
  /**
   * Please destroy after usage
   */
  tarch::tests::TestCase* getUnitTests();
}

#endif
