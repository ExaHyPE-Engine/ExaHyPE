// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/tests/TestCase.h"

#include <cmath>
#include <iostream>
#include <fstream>

namespace exahype2 {
  namespace dg {
    namespace rusanov {
      namespace tests {
        class RiemannTest;
      }
    }
  }
}


class exahype2::dg::rusanov::tests::RiemannTest : public tarch::tests::TestCase {
  private:
    void testIntegrateOverRiemannSolutionsAndAddToVolume_GaussLegendre();
  public:
    RiemannTest();

    virtual ~RiemannTest() = default;

    virtual void run() override;
};


