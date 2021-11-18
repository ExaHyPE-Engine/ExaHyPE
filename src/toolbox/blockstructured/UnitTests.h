// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_BLOCKSTRUCTURED_UNIT_TESTS_H_
#define _TOOLBOX_BLOCKSTRUCTURED_UNIT_TESTS_H_


#include "tarch/tests/TestCase.h"


namespace toolbox {
  namespace blockstructured {
    /**
     * Please destroy after usage
     */
    tarch::tests::TestCase* getUnitTests();
  }
}

#endif
