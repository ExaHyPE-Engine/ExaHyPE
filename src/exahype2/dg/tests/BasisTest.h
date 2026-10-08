// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::dg::tests {
  class BasisTest;
} // namespace exahype2::dg::tests

class exahype2::dg::tests::BasisTest final : public tarch::tests::TestCase {
public:
  BasisTest();
  BasisTest(const BasisTest&)            = delete;
  BasisTest(BasisTest&&)                 = delete;
  BasisTest& operator=(const BasisTest&) = delete;
  BasisTest& operator=(BasisTest&&)      = delete;
  ~BasisTest() override                  = default;

  void run() override;

private:
  void testNodeCountsFollowTheNumberOfNodesPerAxis();
  void testEveryNodeIsPaddedToTheVectorWidth();
  void testTheNodeCountsFollowTheDimensionCount();
  void testTheBasisTablesPointNowhereUntilTheyAreSet();
  void testGetQuadraturePointMapsTheReferenceNode();
  void testVisitsEveryNodeWithItsQuadratureWeight();
  void testReadsFloatValuesInPlace();
  void testEvaluatePolynomialOrder1();
  void testEvaluatePolynomialOrder2();
  void testTheOrderInvariantProductIgnoresTheFactorOrder();
  void testTheSmallestValuesAreTakenPerEntry();
  void testProductMatchesTheDefinition();
  void testResultIsAccumulatedOrOverwritten();
  void testScalingMultipliesEveryProduct();
  void testStridesAddressTheIntendedSlices();
  void testANarrowerResultIsRoundedOnce();
  void testTheDerivativeOfAConstantLineIsExactlyZero();
  void testTheDerivativeActsOnTheDifferencesFromTheSmallestValue();
  void testGradientOfALinearPolynomialIsItsSlope();
  void testEachBlockHoldsTheDerivativeAlongItsOwnAxis();
  void testNarrowStorageIsDifferentiatedInTheComputePrecision();
  void testTheGradientOfAConstantStateIsExactlyZero();
  void testALinearFluxDifferentiatesToItsPhysicalSlope();
  void testAConstantFluxDifferentiatesToExactZeros();
  void testTwoCallsOverAlternateLinesDifferentiateLikeOneCall();
  void testAPaddedNodeStrideProjectsOnlyTheEntries();
  void testTheLowerAndUpperFaceUseTheirOwnCoefficients();
  void testTheFaceNodesRunWithTheLowestTangentialAxisFastest();
  void testAFieldOfFloatAccumulatesTheFacesInDouble();
  void testOneDimensionalIdentityLeavesDataUnchanged();
  void testTwoDimensionalIdentityLeavesDataUnchanged();
  void testThreeDimensionalIdentityLeavesDataUnchanged();
  void testProjectionPreservesAConstantField();
  void testLagrangeBasisFunctionIsCardinal();
  void testInterpolationEvaluatesTheCoarsePolynomialAtTheChildNodes();
  void testRestrictionIsTheInterpolationTransposedOverTheChildWidth();
  void testWeightedRestrictionPreservesConstants();
  void testMatricesAreMirrorSymmetricToTheLastBit();
  void testAverageMirrorPairsKeepsSymmetricEntries();
  void testTransposePerChildSwapsTheNodeIndices();
  void testOrderZeroMatrices();
  void testFaceInterpolationReproducesALinearProfile();
  void testCellInterpolationReproducesALinearState();
  void testAConstantFaceAndCellInterpolateBitwise();
  void testFaceRestrictionProjectsALinearProfile();
  void testCellRestrictionProjectsALinearState();
  void testConstantFacesAndCellsRestrictBitwise();

  template <int Order>
  void validateAffineInterpolant(const double* nodes);
};
