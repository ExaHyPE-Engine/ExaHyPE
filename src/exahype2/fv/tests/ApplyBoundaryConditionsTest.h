// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class ApplyBoundaryConditionsTest;
} // namespace exahype2::fv::tests

class exahype2::fv::tests::ApplyBoundaryConditionsTest final : public tarch::tests::TestCase {
public:
  ApplyBoundaryConditionsTest();
  ApplyBoundaryConditionsTest(const ApplyBoundaryConditionsTest&)            = delete;
  ApplyBoundaryConditionsTest(ApplyBoundaryConditionsTest&&)                 = delete;
  ApplyBoundaryConditionsTest& operator=(const ApplyBoundaryConditionsTest&) = delete;
  ApplyBoundaryConditionsTest& operator=(ApplyBoundaryConditionsTest&&)      = delete;
  ~ApplyBoundaryConditionsTest() override                                    = default;

  void run() override;

private:
  void testDomainFaceTakesTheBoundaryValuesAndTheTimeStamps();
  void testPeriodicAxisAndInteriorFaceKeepTheirHalo();
};
