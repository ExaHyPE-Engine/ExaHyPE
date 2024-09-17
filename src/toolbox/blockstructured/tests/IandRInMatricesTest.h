// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"

namespace toolbox {
  namespace blockstructured {
    namespace tests {
      class IandRInMatricesTest;
    }
  }
}


class toolbox::blockstructured::tests::IandRInMatricesTest: public tarch::tests::TestCase {
  private:

    /**
     * The matrix interpolation and restriction scheme is tested by comparing the results to the tensor product scheme.
     */
    void compareInterpolateLinearCaseToTensorProductPatch();
    void compareRestrictLinearCaseToTensorProductPatch();

  public:

    /**
     * Cosntructor.
     */
    IandRInMatricesTest();

    /**
     * Destructor, empty.
     */
    virtual ~IandRInMatricesTest() = default;

    /**
     * This routine is triggered by the TestCaseCollection
     */
    virtual void run() override;
};

