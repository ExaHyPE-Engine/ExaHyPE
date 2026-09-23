// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class AMRMatricesTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::AMRMatricesTest final : public tarch::tests::TestCase {
public:
  AMRMatricesTest();
  AMRMatricesTest(const AMRMatricesTest&)            = delete;
  AMRMatricesTest(AMRMatricesTest&&)                 = delete;
  AMRMatricesTest& operator=(const AMRMatricesTest&) = delete;
  AMRMatricesTest& operator=(AMRMatricesTest&&)      = delete;
  ~AMRMatricesTest() override                        = default;

  void run() override;

private:
  void testLagrangeBasisFunctionIsCardinal();
  void testInterpolationEvaluatesTheCoarsePolynomialAtTheChildNodes();
  void testRestrictionIsTheInterpolationTransposedOverTheChildWidth();
  void testWeightedRestrictionPreservesConstants();
  void testMatricesAreMirrorSymmetricToTheLastBit();
  void testAverageMirrorPairsKeepsSymmetricEntries();
  void testTransposePerChildSwapsTheNodeIndices();
  void testOrderZeroMatrices();
};
