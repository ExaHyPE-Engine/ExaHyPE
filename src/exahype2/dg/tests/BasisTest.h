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
  BasisTest(const BasisTest& other)            = delete;
  BasisTest(BasisTest&& other)                 = delete;
  BasisTest& operator=(const BasisTest& other) = delete;
  BasisTest& operator=(BasisTest&& other)      = delete;
  ~BasisTest() override                        = default;

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
  void testTheReferencesAndBoundsFollowTheValueRanges();
  void testTheLargestProductOfEntriesIsThePowerOfTheLargestMagnitude();
  void testProductMatchesTheDefinition();
  void testResultIsAccumulatedOrOverwritten();
  void testScalingMultipliesEveryProduct();
  void testStridesAddressTheIntendedSlices();
  void testANarrowerResultIsRoundedOnce();
  void testThePairWeightsRecombineToTheOperatorRows();
  void testTheDerivativeOfAConstantLineIsExactlyZero();
  void testTheDerivativeActsOnTheDifferencesFromTheCentreNode();
  void testADerivativeInPlaceEqualsADerivativeIntoAnotherBuffer();
  void testTheDerivativeOfAReflectedLineIsTheReflectedDerivative();
  void testGradientOfALinearPolynomialIsItsSlope();
  void testEachBlockStoresTheDerivativeAlongItsOwnAxis();
  void testNarrowStorageIsDifferentiatedInTheComputePrecision();
  void testTheGradientOfAConstantStateIsExactlyZero();
  void testALinearFluxDifferentiatesToItsPhysicalSlope();
  void testAConstantFluxDifferentiatesToExactZeros();
  void testTwoCallsOverAlternateLinesDifferentiateLikeOneCall();
  void testAPaddedNodeStrideProjectsOnlyTheEntries();
  void testTheLowerAndUpperFaceUseTheirOwnCoefficients();
  void testTheFacesOfAReflectedFieldAreTheSwappedFaces();
  void testTheFaceNodesRunWithTheLowestTangentialAxisFastest();
  void testAFieldOfFloatAccumulatesTheFacesInDouble();
  void testThreadsThatDivideTheFaceItemsWriteThePairedFaces();
  void testTheLinesOfAnAxisRunThroughTheFaceNodes();
  void testOneDimensionalIdentityLeavesDataUnchanged();
  void testTwoDimensionalIdentityLeavesDataUnchanged();
  void testThreeDimensionalIdentityLeavesDataUnchanged();
  void testProjectionPreservesAConstantField();
  void testTheProjectionOfASwappedFieldIsTheSwappedProjection();
  void testTheProjectionOfAReflectedFieldIsTheReflectedProjection();
  void testBothProjectionRulesProjectTheSameField();
  void testEverySplitOfTheStagesAmongThreadsProjectsLikeOneThread();
  void testLagrangeBasisFunctionIsCardinal();
  void testTheLagrangeBasisFunctionsReproduceALinearFunction();
  void testTheMirrorBasisFunctionAtTheNegatedOffsetReturnsTheSameBits();
  void testInterpolationEvaluatesTheCoarsePolynomialAtTheChildNodes();
  void testEquidistantInterpolationEvaluatesThePolynomialFromFaceToFace();
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
  void testANegatedFieldTransfersToTheNegatedFieldBitwise();

  template <int Order>
  void validateAffineInterpolant(const double* nodes);

  template <int Nodes>
  void validateReflectedDerivative(const double* operatorMatrix);

  template <int Nodes>
  void validateReflectedFaces(const double* leftCoefficients, const double* rightCoefficients);

  template <int Nodes>
  void validateMirroredBasisFunctions();
};
