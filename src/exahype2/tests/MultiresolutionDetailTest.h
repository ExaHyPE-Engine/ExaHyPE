// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class MultiresolutionDetailTest;
} // namespace exahype2::tests

class exahype2::tests::MultiresolutionDetailTest final : public tarch::tests::TestCase {
public:
  MultiresolutionDetailTest();
  MultiresolutionDetailTest(const MultiresolutionDetailTest& other)            = delete;
  MultiresolutionDetailTest(MultiresolutionDetailTest&& other)                 = delete;
  MultiresolutionDetailTest& operator=(const MultiresolutionDetailTest& other) = delete;
  MultiresolutionDetailTest& operator=(MultiresolutionDetailTest&& other)      = delete;
  ~MultiresolutionDetailTest() override                                        = default;

  void run() override;

private:
  void testVolumeCentresAreEquidistant();
  void testConstantFieldHasZeroDetail();
  void testAffineFieldHasZeroDetail();
  void testStepFieldHasLargeDetail();
  void testDetailIsBitwiseInvariantUnderReflectionsAndSwaps();
  void testNonFiniteValueRefines();
  void testCustomNodesFitTheirOwnAffineField();
  void testStridedPatchWithIndicator();
  void testSingleNodePatchHasZeroDetail();
  void testRefinementCommandFromDetail();
  void testRefineOnDetail();
  void testRefineOnDetailRunsOnTheDevice();
};
