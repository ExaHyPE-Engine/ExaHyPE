// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace toolbox::blockstructured::tests {
  class MultiresolutionDetailTest;
} // namespace toolbox::blockstructured::tests

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
class toolbox::blockstructured::tests::MultiresolutionDetailTest final : public tarch::tests::TestCase {
private:
  void testConstantFieldHasZeroDetail();
  void testAffineFieldHasZeroDetail();
  void testStepFieldHasLargeDetail();
  void testDetailIsBitwiseInvariantUnderReflectionsAndSwaps();
  void testNonFiniteValueRefines();
  void testCustomNodesFitTheirOwnAffineField();
  void testStridedPatchWithIndicator();
  void testSingleNodePatchHasZeroDetail();

public:
  MultiresolutionDetailTest();
  ~MultiresolutionDetailTest() override = default;

  void run() override;
};
