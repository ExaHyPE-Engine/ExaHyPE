// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once
#include "tarch/tests/TestCase.h"
namespace exahype2::fv::tests {
  class WENOTest final : public tarch::tests::TestCase {
  public:
    WENOTest();
    void run() override;

  private:
    void testWENOReconstructsPolynomialsExactly();
  };
} // namespace exahype2::fv::tests
