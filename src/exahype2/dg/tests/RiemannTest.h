// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_TESTS_RIEMANN_TEST_H_
#define _EXAHYPE2_DG_TESTS_RIEMANN_TEST_H_


#include "tarch/tests/TestCase.h"

#include <cmath>
#include <iostream>
#include <fstream>

namespace exahype2 {
  namespace dg {
    namespace tests {
      class RiemannTest;
    }
  }
}


class exahype2::dg::tests::RiemannTest : public tarch::tests::TestCase {
  private:
    /**
     * Testing projection to faces by creating fully constant
     * cell and ensuring that the
     * projected values are also the same constant.
     */
    void testProjectVolumetricDataOntoFacesForConstantSolution();

  public:
    RiemannTest();

    virtual ~RiemannTest() = default;

    void run() override;
};


#endif
