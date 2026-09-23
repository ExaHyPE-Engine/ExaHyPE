// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelsOnDeviceTest.h"

#include "exahype2/dg/rk/LinearCombination.h"
#include "exahype2/dg/rk/VolumeIntegral.h"
#include "exahype2/dg/tests/DeviceKernelFixtures.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/accelerator.h"

namespace {
  using Setup = exahype2::dg::tests::DeviceKernelSetup;
} // namespace

exahype2::dg::rk::tests::KernelsOnDeviceTest::KernelsOnDeviceTest() :
  tarch::tests::TestCase("exahype2::dg::rk::tests::KernelsOnDeviceTest") {}

void exahype2::dg::rk::tests::KernelsOnDeviceTest::run() {
  testMethod(testLinearCombinationOfEstimates);
  testMethod(testVolumeIntegral);
  testMethod(testDiffusiveVolumeIntegral);
}

void exahype2::dg::rk::tests::KernelsOnDeviceTest::validateBuffersMatch(
  const std::vector<double>& deviceResult,
  const std::vector<double>& hostResult,
  const std::string&         bufferName
) {
  validateEquals(deviceResult.size(), hostResult.size());
  for (std::size_t entry = 0; entry < deviceResult.size(); entry++) {
    validateNumericalEqualsWithParams2(deviceResult[entry], hostResult[entry], bufferName, entry);
  }
}

void exahype2::dg::rk::tests::KernelsOnDeviceTest::testLinearCombinationOfEstimates() {
  const exahype2::dg::tests::KernelOperators operators;
  const std::vector<double> cellStates = exahype2::dg::tests::makeRamp(Setup::AllCellStatesCardinality, 1.0, 0.01);
  const std::vector<double> rightHandSideEstimates =
    exahype2::dg::tests::makeRamp(Setup::AllRightHandSidesCardinality, -0.5, 0.017);
  const std::vector<double> stageStates(Setup::AllCellStatesCardinality, 0.0);

  const exahype2::dg::tests::DeviceBuffer<double> deviceCellStates(cellStates);
  const exahype2::dg::tests::DeviceBuffer<double> deviceRightHandSideEstimates(rightHandSideEstimates);
  const exahype2::dg::tests::DeviceBuffer<double> deviceStageStates(stageStates);
  const exahype2::dg::tests::DeviceBuffer<double> deviceCoefficients(operators.rungeKuttaCoefficients);

  const double* const onDeviceCellStates             = deviceCellStates.data();
  const double* const onDeviceRightHandSideEstimates = deviceRightHandSideEstimates.data();
  double* const       onDeviceStageStates            = deviceStageStates.data();
  const double* const onDeviceCoefficients           = deviceCoefficients.data();
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(Setup::NumberOfCells),
    [=] GPU_CALLABLE_LAMBDA(const int cell) {
      const int cellStateOffset     = cell * Setup::CellStateCardinality;
      const int rightHandSideOffset = cell * Setup::RightHandSideCardinality;
      exahype2::dg::rk::linearCombinationOfEstimates<
        Setup::RungeKuttaSteps,
        Setup::DofsPerAxis,
        Setup::NumberOfUnknowns,
        Setup::NumberOfAuxiliaryVariables>(
        Setup::RungeKuttaStep,
        onDeviceCellStates + cellStateOffset,
        onDeviceRightHandSideEstimates + rightHandSideOffset,
        onDeviceStageStates + cellStateOffset,
        onDeviceCoefficients
      );
    }
  );

  std::vector<double> deviceResult(stageStates.size());
  deviceStageStates.copyBack(deviceResult);

  std::vector<double> hostResult = stageStates;
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    const int cellStateOffset     = cell * Setup::CellStateCardinality;
    const int rightHandSideOffset = cell * Setup::RightHandSideCardinality;
    exahype2::dg::rk::linearCombinationOfEstimates<
      Setup::RungeKuttaSteps,
      Setup::DofsPerAxis,
      Setup::NumberOfUnknowns,
      Setup::NumberOfAuxiliaryVariables>(
      Setup::RungeKuttaStep,
      cellStates.data() + cellStateOffset,
      rightHandSideEstimates.data() + rightHandSideOffset,
      hostResult.data() + cellStateOffset,
      operators.rungeKuttaCoefficients.data()
    );
  }

  validateBuffersMatch(deviceResult, hostResult, "stageStates");
}

void exahype2::dg::rk::tests::KernelsOnDeviceTest::testVolumeIntegral() { runVolumeIntegral<false>(); }

void exahype2::dg::rk::tests::KernelsOnDeviceTest::testDiffusiveVolumeIntegral() { runVolumeIntegral<true>(); }

/**
 * Runs the volume integral over the cell chain on the device and on the host.
 * The method is public, as nvcc forbids an extended __device__ lambda inside a
 * private member function.
 */
template <bool DiffusiveFlux>
void exahype2::dg::rk::tests::KernelsOnDeviceTest::runVolumeIntegral() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  const std::vector<double> cellStates = exahype2::dg::tests::makeRamp(Setup::AllCellStatesCardinality, 0.8, 0.021);
  const std::vector<double> rightHandSideEstimates(Setup::AllRightHandSidesCardinality, 0.0);

  const exahype2::dg::tests::DeviceBuffer<double> deviceCellStates(cellStates);
  const exahype2::dg::tests::DeviceBuffer<double> deviceRightHandSideEstimates(rightHandSideEstimates);
  const exahype2::dg::tests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
  const exahype2::dg::tests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);
  const exahype2::dg::tests::DeviceBuffer<double> deviceQuadratureNodes(operators.quadratureNodes);
  const exahype2::dg::tests::DeviceBuffer<double> deviceMassMatrixDiagonal(operators.massMatrixDiagonal);
  const exahype2::dg::tests::DeviceBuffer<double> deviceStiffnessMatrix(operators.stiffnessMatrix);
  const exahype2::dg::tests::DeviceBuffer<double> deviceDerivativeOperator(operators.derivativeOperator);

  const double* const onDeviceCellStates             = deviceCellStates.data();
  double* const       onDeviceRightHandSideEstimates = deviceRightHandSideEstimates.data();
  const double* const onDeviceCellCentres            = deviceCellCentres.data();
  const double* const onDeviceCellSizes              = deviceCellSizes.data();
  const double* const onDeviceQuadratureNodes        = deviceQuadratureNodes.data();
  const double* const onDeviceMassMatrixDiagonal     = deviceMassMatrixDiagonal.data();
  const double* const onDeviceStiffnessMatrix        = deviceStiffnessMatrix.data();
  const double* const onDeviceDerivativeOperator     = deviceDerivativeOperator.data();
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(Setup::NumberOfCells),
    [=] GPU_CALLABLE_LAMBDA(const int cell) {
      const int cellStateOffset  = cell * Setup::CellStateCardinality;
      const int cellUpdateOffset = cell * Setup::RightHandSideCardinality + Setup::RungeKuttaStepOffset;
      const int coordinateOffset = cell * DIMENSIONS;
      exahype2::dg::rk::volumeIntegral<
        exahype2::dg::tests::StubSolver,
        Setup::DofsPerAxis,
        Setup::NumberOfUnknowns,
        Setup::NumberOfAuxiliaryVariables,
        false,
        true,
        DiffusiveFlux,
        true,
        true>(
        onDeviceCellStates + cellStateOffset,
        onDeviceRightHandSideEstimates + cellUpdateOffset,
        onDeviceQuadratureNodes,
        onDeviceMassMatrixDiagonal,
        onDeviceStiffnessMatrix,
        onDeviceDerivativeOperator,
        tarch::la::Vector<DIMENSIONS, double>(onDeviceCellCentres + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(onDeviceCellSizes + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize
      );
    }
  );

  std::vector<double> deviceResult(rightHandSideEstimates.size());
  deviceRightHandSideEstimates.copyBack(deviceResult);

  std::vector<double> hostResult = rightHandSideEstimates;
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    const int cellStateOffset  = cell * Setup::CellStateCardinality;
    const int cellUpdateOffset = cell * Setup::RightHandSideCardinality + Setup::RungeKuttaStepOffset;
    const int coordinateOffset = cell * DIMENSIONS;
    exahype2::dg::rk::volumeIntegral<
      exahype2::dg::tests::StubSolver,
      Setup::DofsPerAxis,
      Setup::NumberOfUnknowns,
      Setup::NumberOfAuxiliaryVariables,
      false,
      true,
      DiffusiveFlux,
      true,
      true>(
      cellStates.data() + cellStateOffset,
      hostResult.data() + cellUpdateOffset,
      operators.quadratureNodes.data(),
      operators.massMatrixDiagonal.data(),
      operators.stiffnessMatrix.data(),
      operators.derivativeOperator.data(),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellCentres.data() + coordinateOffset),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
      Setup::TimeStamp,
      Setup::TimeStepSize
    );
  }

  validateBuffersMatch(deviceResult, hostResult, "rightHandSideEstimates");
}
