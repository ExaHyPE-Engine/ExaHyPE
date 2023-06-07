
// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once
#include "tarch/tests/TestCase.h"

namespace exahype2 {
  namespace fv {
    namespace rusanov {
      namespace tests {
        class ApplySplit1DRiemannToPatchTest;
      }
    }
  }
}


class exahype2::fv::rusanov::tests::ApplySplit1DRiemannToPatchTest : public tarch::tests::TestCase {
  private:
    void testMe();
    void testBreakingDam();

  public:
    ApplySplit1DRiemannToPatchTest();

    virtual ~ApplySplit1DRiemannToPatchTest() = default;

    virtual void run() override;
};
