// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::amr::tests {
  class MultiresolutionDetailTest;
} // namespace exahype2::amr::tests

/**
 * Pins the defining properties of the within-patch affine-fit detail used as a
 * dynamic-AMR indicator:
 *  - a constant field has detail 0;
 *  - any affine field has detail close to 0 (the fit is exact), so smooth
 *    gradients do not refine;
 *  - a discontinuity has an O(1) detail, so shocks refine;
 *  - a patch and every reflection and axis swap of it yield the bitwise-identical
 *    detail, so a mirror-symmetric scenario derives identical refinement commands
 *    on both sides of its symmetry plane.
 */
class exahype2::amr::tests::MultiresolutionDetailTest final : public tarch::tests::TestCase {
public:
  MultiresolutionDetailTest();
  MultiresolutionDetailTest(const MultiresolutionDetailTest&)            = delete;
  MultiresolutionDetailTest(MultiresolutionDetailTest&&)                 = delete;
  MultiresolutionDetailTest& operator=(const MultiresolutionDetailTest&) = delete;
  MultiresolutionDetailTest& operator=(MultiresolutionDetailTest&&)      = delete;
  ~MultiresolutionDetailTest() override                                  = default;

  void run() override;

private:
  void testConstantFieldHasZeroDetail();
  void testAffineFieldHasZeroDetail();
  void testStepFieldHasLargeDetail();
  void testDetailIsBitwiseInvariantUnderReflectionsAndSwaps();
  void testNonFiniteValueRefines();
  void testCustomNodesFitTheirOwnAffineField();
  void testStridedPatchWithIndicator();
  void testSingleNodePatchHasZeroDetail();
};
