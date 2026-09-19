// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class FaceIntegralTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::FaceIntegralTest final : public tarch::tests::TestCase {
public:
  FaceIntegralTest();
  FaceIntegralTest(const FaceIntegralTest&)            = delete;
  FaceIntegralTest(FaceIntegralTest&&)                 = delete;
  FaceIntegralTest& operator=(const FaceIntegralTest&) = delete;
  FaceIntegralTest& operator=(FaceIntegralTest&&)      = delete;
  ~FaceIntegralTest() override                         = default;

  void run() override;

private:
  void testZeroFluxLeavesUpdateUntouched();
  void testEachCellNodeReceivesItsOwnFaceNode();
  void testUpdateStridesByTheUnknowns();
  void testOrientationFlipsSignOnlyForTheNonlinearForm();
  void testWeightsAndCoefficientsEnterAsAProduct();
};
