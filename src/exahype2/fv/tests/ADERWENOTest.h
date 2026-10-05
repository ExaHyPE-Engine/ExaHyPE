// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once
#include "tarch/tests/TestCase.h"
namespace exahype2::fv::tests {
  class ADERWENOTest final : public tarch::tests::TestCase {
  public:
    ADERWENOTest();
    void run() override;

  private:
    void testWENOReconstructsPolynomialsExactly();
    void testADERWENOPreservesUniformState();
    void testADERWENOAdvectionConvergesAtHighOrder();
    void testADERWENONcpAdvectionConvergesAtHighOrder();
    void testADERWENOPeriodicAdvectionConservesMass();
    void testDeviceUpdatesMatchHost();
  };
} // namespace exahype2::fv::tests
