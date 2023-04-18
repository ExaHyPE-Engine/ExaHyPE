
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/tests/TestCase.h"


namespace exahype2 {
  namespace fd {
    namespace tests {
      class SommerfeldBCTest;
    }
  }
}


class exahype2::fd::tests::SommerfeldBCTest : public tarch::tests::TestCase {
  private:
    void flatsolutiontest();

  public:
    SommerfeldBCTest();

    /**
     * This routine is triggered by the TestCaseCollection
     */
    virtual void run();
};
