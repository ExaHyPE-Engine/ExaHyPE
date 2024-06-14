
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/tests/TestCase.h"


namespace exahype2 {
  namespace fd {
    namespace tests {
      class CCZ4KernelTest;
    }
  }
}


class exahype2::fd::tests::CCZ4KernelTest : public tarch::tests::TestCase {
  private:
    void AppleWithAppleTest();

    void prepareFieldData(
      double** g, 
      double& phi, 
      double** K, 
      double& trK,
      double& Theta,
      double* Gamma, 
      double& alpha, 
      double* beta,
      double* b,
      double x, double y, double z
    );

  public:
    CCZ4KernelTest();

    /**
     * This routine is triggered by the TestCaseCollection
     */
    virtual void run();
};

