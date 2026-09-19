// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PredictorScratchTest.h"

#include "exahype2/dg/ader/PredictorScratch.h"

namespace {
  constexpr int NumberOfDofsPerAxis        = 3;
  constexpr int NumberOfUnknowns           = 2;
  constexpr int NumberOfAuxiliaryVariables = 1;

  using CellLayout = exahype2::dg::ader::Layout<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables>;

  using FullNonlinearScratch = exahype2::dg::ader::NonlinearPredictorScratch<
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    true,
    true,
    true,
    true>;
  using BareNonlinearScratch = exahype2::dg::ader::NonlinearPredictorScratch<
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    false,
    false,
    false,
    false>;
  using ProductOnlyScratch = exahype2::dg::ader::NonlinearPredictorScratch<
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    true,
    false,
    false,
    false>;
  using DiffusiveFluxOnlyScratch = exahype2::dg::ader::NonlinearPredictorScratch<
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    false,
    false,
    true,
    false>;

  using FullLinearScratch = exahype2::dg::ader::
    LinearPredictorScratch<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, true, true, true>;
  using BareLinearScratch = exahype2::dg::ader::
    LinearPredictorScratch<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, false, false, false>;

  constexpr int NumberOfBasisTables = 11;

  const double StubTables[NumberOfBasisTables] = {};
} // namespace

exahype2::dg::ader::tests::PredictorScratchTest::PredictorScratchTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::PredictorScratchTest") {}

void exahype2::dg::ader::tests::PredictorScratchTest::run() {
  testMethod(testEveryNodeIsPaddedToTheVectorWidth);
  testMethod(testTheNodeCountsFollowTheDimensionCount);
  testMethod(testNonlinearBlocksFollowEachOtherWithoutOverlap);
  testMethod(testLinearBlocksFollowEachOtherWithoutOverlap);
  testMethod(testSwitchedOffTermsHaveZeroExtent);
  testMethod(testTheBasisTablesPointNowhereUntilTheyAreSet);
}

void exahype2::dg::ader::tests::PredictorScratchTest::testEveryNodeIsPaddedToTheVectorWidth() {
  validateEquals(CellLayout::NumberOfData, NumberOfUnknowns + NumberOfAuxiliaryVariables);

  validateEquals(CellLayout::PaddedNumberOfUnknowns % PEANO_WITH_SIMDLEN, 0);
  validate(CellLayout::PaddedNumberOfUnknowns >= NumberOfUnknowns);
  validate(CellLayout::PaddedNumberOfUnknowns - NumberOfUnknowns < PEANO_WITH_SIMDLEN);

  validateEquals(CellLayout::PaddedNumberOfData % PEANO_WITH_SIMDLEN, 0);
  validate(CellLayout::PaddedNumberOfData >= CellLayout::NumberOfData);
  validate(CellLayout::PaddedNumberOfData - CellLayout::NumberOfData < PEANO_WITH_SIMDLEN);

  using ExactWidthLayout = exahype2::dg::ader::Layout<NumberOfDofsPerAxis, PEANO_WITH_SIMDLEN, 0>;
  validateEquals(ExactWidthLayout::PaddedNumberOfUnknowns, PEANO_WITH_SIMDLEN);
  validateEquals(ExactWidthLayout::PaddedNumberOfData, PEANO_WITH_SIMDLEN);

  using OneAboveWidthLayout = exahype2::dg::ader::Layout<NumberOfDofsPerAxis, PEANO_WITH_SIMDLEN + 1, 0>;
  validateEquals(OneAboveWidthLayout::PaddedNumberOfUnknowns, 2 * PEANO_WITH_SIMDLEN);
}

void exahype2::dg::ader::tests::PredictorScratchTest::testTheNodeCountsFollowTheDimensionCount() {
  int expectedNodesPerCell = 1;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    expectedNodesPerCell *= NumberOfDofsPerAxis;
  }

  constexpr int ExpectedNodesPerAxisIn3D = (DIMENSIONS > 2) ? NumberOfDofsPerAxis : 1;
  validateEquals(CellLayout::NodesPerAxisIn3D, ExpectedNodesPerAxisIn3D);
  validateEquals(CellLayout::NodesPerFace, expectedNodesPerCell / NumberOfDofsPerAxis);
  validateEquals(CellLayout::NodesPerCell, expectedNodesPerCell);
  validateEquals(CellLayout::SpaceTimeNodes, expectedNodesPerCell * NumberOfDofsPerAxis);
  validateEquals(CellLayout::NodesPerCell, CellLayout::NodesPerFace * NumberOfDofsPerAxis);
}

void exahype2::dg::ader::tests::PredictorScratchTest::testNonlinearBlocksFollowEachOtherWithoutOverlap() {
  validateEquals(FullNonlinearScratch::SpaceTimeStateSize, CellLayout::PaddedNumberOfData * CellLayout::SpaceTimeNodes);
  validateEquals(
    FullNonlinearScratch::RightHandSideSize,
    CellLayout::PaddedNumberOfUnknowns * CellLayout::SpaceTimeNodes
  );
  validateEquals(
    FullNonlinearScratch::SpaceTimeFluxSize,
    DIMENSIONS * CellLayout::PaddedNumberOfUnknowns * CellLayout::SpaceTimeNodes
  );
  validateEquals(
    FullNonlinearScratch::SpaceTimeSourceSize,
    CellLayout::PaddedNumberOfUnknowns * CellLayout::SpaceTimeNodes
  );
  validateEquals(
    FullNonlinearScratch::StateGradientSize,
    DIMENSIONS * CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell
  );
  validateEquals(FullNonlinearScratch::UpdatedTimeSliceSize, NumberOfDofsPerAxis * CellLayout::PaddedNumberOfUnknowns);

  validateEquals(FullNonlinearScratch::SpaceTimeStateOffset, 0);
  validateEquals(
    FullNonlinearScratch::RightHandSideOffset,
    FullNonlinearScratch::SpaceTimeStateOffset + FullNonlinearScratch::SpaceTimeStateSize
  );
  validateEquals(
    FullNonlinearScratch::SpaceTimeFluxOffset,
    FullNonlinearScratch::RightHandSideOffset + FullNonlinearScratch::RightHandSideSize
  );
  validateEquals(
    FullNonlinearScratch::SpaceTimeSourceOffset,
    FullNonlinearScratch::SpaceTimeFluxOffset + FullNonlinearScratch::SpaceTimeFluxSize
  );
  validateEquals(
    FullNonlinearScratch::StateGradientOffset,
    FullNonlinearScratch::SpaceTimeSourceOffset + FullNonlinearScratch::SpaceTimeSourceSize
  );
  validateEquals(
    FullNonlinearScratch::UpdatedTimeSliceOffset,
    FullNonlinearScratch::StateGradientOffset + FullNonlinearScratch::StateGradientSize
  );
  validateEquals(
    FullNonlinearScratch::PicardSize,
    FullNonlinearScratch::UpdatedTimeSliceOffset + FullNonlinearScratch::UpdatedTimeSliceSize
  );

  validateEquals(
    FullNonlinearScratch::TimeAveragedStateSize,
    CellLayout::PaddedNumberOfData * CellLayout::NodesPerCell
  );
  validateEquals(
    FullNonlinearScratch::TimeAveragedFluxSize,
    DIMENSIONS * CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell
  );
  validateEquals(
    FullNonlinearScratch::TimeAveragedSourceSize,
    CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell
  );
  validateEquals(FullNonlinearScratch::VolumeCoefficientsSize, NumberOfDofsPerAxis * NumberOfDofsPerAxis);

  validateEquals(FullNonlinearScratch::TimeAveragedStateOffset, 0);
  validateEquals(
    FullNonlinearScratch::TimeAveragedFluxOffset,
    FullNonlinearScratch::TimeAveragedStateOffset + FullNonlinearScratch::TimeAveragedStateSize
  );
  validateEquals(
    FullNonlinearScratch::TimeAveragedSourceOffset,
    FullNonlinearScratch::TimeAveragedFluxOffset + FullNonlinearScratch::TimeAveragedFluxSize
  );
  validateEquals(
    FullNonlinearScratch::VolumeCoefficientsOffset,
    FullNonlinearScratch::TimeAveragedSourceOffset + FullNonlinearScratch::TimeAveragedSourceSize
  );
  validateEquals(
    FullNonlinearScratch::TimeAveragedSize,
    FullNonlinearScratch::VolumeCoefficientsOffset + FullNonlinearScratch::VolumeCoefficientsSize
  );
}

void exahype2::dg::ader::tests::PredictorScratchTest::testLinearBlocksFollowEachOtherWithoutOverlap() {
  validateEquals(FullLinearScratch::StateSize, CellLayout::PaddedNumberOfData * CellLayout::NodesPerCell);
  validateEquals(FullLinearScratch::TimeAveragedStateSize, CellLayout::PaddedNumberOfData * CellLayout::NodesPerCell);
  validateEquals(
    FullLinearScratch::FluxSize,
    DIMENSIONS * CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell
  );
  validateEquals(
    FullLinearScratch::TimeAveragedFluxSize,
    DIMENSIONS * CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell
  );
  validateEquals(FullLinearScratch::SourceSize, CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell);
  validateEquals(
    FullLinearScratch::TimeAveragedSourceSize,
    CellLayout::PaddedNumberOfUnknowns * CellLayout::NodesPerCell
  );
  validateEquals(FullLinearScratch::StateGradientSize, CellLayout::PaddedNumberOfUnknowns * NumberOfDofsPerAxis);
  validateEquals(
    FullLinearScratch::PointSourceSize,
    (NumberOfDofsPerAxis + 1) * CellLayout::NodesPerCell * CellLayout::PaddedNumberOfUnknowns
  );

  validateEquals(FullLinearScratch::StateOffset, 0);
  validateEquals(
    FullLinearScratch::TimeAveragedStateOffset,
    FullLinearScratch::StateOffset + FullLinearScratch::StateSize
  );
  validateEquals(
    FullLinearScratch::FluxOffset,
    FullLinearScratch::TimeAveragedStateOffset + FullLinearScratch::TimeAveragedStateSize
  );
  validateEquals(
    FullLinearScratch::TimeAveragedFluxOffset,
    FullLinearScratch::FluxOffset + FullLinearScratch::FluxSize
  );
  validateEquals(
    FullLinearScratch::SourceOffset,
    FullLinearScratch::TimeAveragedFluxOffset + FullLinearScratch::TimeAveragedFluxSize
  );
  validateEquals(
    FullLinearScratch::TimeAveragedSourceOffset,
    FullLinearScratch::SourceOffset + FullLinearScratch::SourceSize
  );
  validateEquals(
    FullLinearScratch::StateGradientOffset,
    FullLinearScratch::TimeAveragedSourceOffset + FullLinearScratch::TimeAveragedSourceSize
  );
  validateEquals(
    FullLinearScratch::PointSourceOffset,
    FullLinearScratch::StateGradientOffset + FullLinearScratch::StateGradientSize
  );
  validateEquals(FullLinearScratch::Size, FullLinearScratch::PointSourceOffset + FullLinearScratch::PointSourceSize);
}

void exahype2::dg::ader::tests::PredictorScratchTest::testSwitchedOffTermsHaveZeroExtent() {
  validateEquals(BareNonlinearScratch::SpaceTimeFluxSize, 0);
  validateEquals(BareNonlinearScratch::SpaceTimeSourceSize, 0);
  validateEquals(BareNonlinearScratch::StateGradientSize, 0);
  validateEquals(BareNonlinearScratch::TimeAveragedFluxSize, 0);
  validateEquals(BareNonlinearScratch::TimeAveragedSourceSize, 0);
  validate(not BareNonlinearScratch::AccumulatesSource);
  validate(not BareNonlinearScratch::NeedsStateGradient);
  validate(BareNonlinearScratch::PicardSize < FullNonlinearScratch::PicardSize);
  validate(BareNonlinearScratch::TimeAveragedSize < FullNonlinearScratch::TimeAveragedSize);

  // A nonconservative product enters the source buffer, so it claims that buffer without a source term.
  validate(ProductOnlyScratch::AccumulatesSource);
  validate(ProductOnlyScratch::NeedsStateGradient);
  validate(ProductOnlyScratch::SpaceTimeSourceSize > 0);
  validate(ProductOnlyScratch::StateGradientSize > 0);
  validateEquals(ProductOnlyScratch::SpaceTimeFluxSize, 0);

  validate(DiffusiveFluxOnlyScratch::NeedsStateGradient);
  validate(not DiffusiveFluxOnlyScratch::AccumulatesSource);
  validate(DiffusiveFluxOnlyScratch::StateGradientSize > 0);
  validateEquals(DiffusiveFluxOnlyScratch::SpaceTimeSourceSize, 0);
  validate(DiffusiveFluxOnlyScratch::SpaceTimeFluxSize > 0);

  validateEquals(BareLinearScratch::SourceSize, 0);
  validateEquals(BareLinearScratch::TimeAveragedSourceSize, 0);
  validateEquals(BareLinearScratch::StateGradientSize, 0);
  validateEquals(BareLinearScratch::PointSourceSize, 0);
  validate(BareLinearScratch::Size < FullLinearScratch::Size);
  validateEquals(BareLinearScratch::Size, BareLinearScratch::StateGradientOffset);
}

void exahype2::dg::ader::tests::PredictorScratchTest::testTheBasisTablesPointNowhereUntilTheyAreSet() {
  const exahype2::dg::ader::BasisTables<double> emptyTables;
  validateEquals(emptyTables.quadraturePoints, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.quadratureWeights, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.inverseQuadratureWeights, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.cellQuadratureWeights, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.inverseCellQuadratureWeights, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.stiffnessOperator, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.stiffnessOperatorTransposed, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.derivativeOperator, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.inversePicardOperator, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.leftFaceCoefficients, static_cast<const double*>(nullptr));
  validateEquals(emptyTables.rightFaceCoefficients, static_cast<const double*>(nullptr));

  exahype2::dg::ader::BasisTables<double> tables;
  tables.quadraturePoints             = StubTables + 0;
  tables.quadratureWeights            = StubTables + 1;
  tables.inverseQuadratureWeights     = StubTables + 2;
  tables.cellQuadratureWeights        = StubTables + 3;
  tables.inverseCellQuadratureWeights = StubTables + 4;
  tables.stiffnessOperator            = StubTables + 5;
  tables.stiffnessOperatorTransposed  = StubTables + 6;
  tables.derivativeOperator           = StubTables + 7;
  tables.inversePicardOperator        = StubTables + 8;
  tables.leftFaceCoefficients         = StubTables + 9;
  tables.rightFaceCoefficients        = StubTables + 10;

  validateEquals(tables.quadraturePoints, StubTables + 0);
  validateEquals(tables.quadratureWeights, StubTables + 1);
  validateEquals(tables.inverseQuadratureWeights, StubTables + 2);
  validateEquals(tables.cellQuadratureWeights, StubTables + 3);
  validateEquals(tables.inverseCellQuadratureWeights, StubTables + 4);
  validateEquals(tables.stiffnessOperator, StubTables + 5);
  validateEquals(tables.stiffnessOperatorTransposed, StubTables + 6);
  validateEquals(tables.derivativeOperator, StubTables + 7);
  validateEquals(tables.inversePicardOperator, StubTables + 8);
  validateEquals(tables.leftFaceCoefficients, StubTables + 9);
  validateEquals(tables.rightFaceCoefficients, StubTables + 10);

  const exahype2::dg::ader::BasisTables<float> narrowTables;
  validateEquals(narrowTables.quadraturePoints, static_cast<const float*>(nullptr));
}
