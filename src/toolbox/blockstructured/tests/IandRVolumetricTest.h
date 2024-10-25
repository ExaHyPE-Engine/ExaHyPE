// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/logging/Log.h"
#include "tarch/tests/TestCase.h"

namespace toolbox {
  namespace blockstructured {
    namespace tests {
      class IandRVolumetricTest;
    } // namespace tests
  }   // namespace blockstructured
} // namespace toolbox


class toolbox::blockstructured::tests::IandRVolumetricTest:
  public tarch::tests::TestCase {
private:
  void testMatrixInterpolation();
  void testSecondOrderInterpolation();

public:
  /**
   * Cosntructor.
   */
  IandRVolumetricTest();

  /**
   * Destructor, empty.
   */
  virtual ~IandRVolumetricTest() = default;

  /**
   * This routine is triggered by the TestCaseCollection
   */
  virtual void run() override;
};
