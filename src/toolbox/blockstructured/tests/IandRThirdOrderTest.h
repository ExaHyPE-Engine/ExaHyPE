// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/logging/Log.h"
#include "tarch/tests/TestCase.h"

namespace toolbox {
  namespace blockstructured {
    namespace tests {
      class IandRThirdOrderTest;
    } // namespace tests
  }   // namespace blockstructured
} // namespace toolbox


class toolbox::blockstructured::tests::IandRThirdOrderTest:
  public tarch::tests::TestCase {
private:
  /**
   * The third order interpolation and restriction scheme is tested by comparing
   * the results to the tensor product scheme for linear source data.
   */
  void compareInterpolateLinearCaseToTensorProduct();
  void compareRestrictLinearCaseToTensorProduct();

public:
  /**
   * Cosntructor.
   */
  IandRThirdOrderTest();

  /**
   * Destructor, empty.
   */
  virtual ~IandRThirdOrderTest() = default;

  /**
   * This routine is triggered by the TestCaseCollection
   */
  virtual void run() override;
};
