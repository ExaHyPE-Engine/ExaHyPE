// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class BoundaryConditionsTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::BoundaryConditionsTest final : public tarch::tests::TestCase {
public:
  BoundaryConditionsTest();
  BoundaryConditionsTest(const BoundaryConditionsTest&)            = delete;
  BoundaryConditionsTest(BoundaryConditionsTest&&)                 = delete;
  BoundaryConditionsTest& operator=(const BoundaryConditionsTest&) = delete;
  BoundaryConditionsTest& operator=(BoundaryConditionsTest&&)      = delete;
  ~BoundaryConditionsTest() override                               = default;

  void run() override;

private:
  void testTheOuterHalfIsFilledFromTheInnerHalf();
  void testEveryNodeSeesItsOwnPositionOnTheFace();
  void testANarrowerStorageIsStagedThroughTheComputePrecision();
};
