// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelsOnDeviceTest.h"

#include "exahype2/dg/BoundaryConditions.h"
#include "exahype2/dg/BoundaryExtrapolation.h"
#include "exahype2/dg/EigenvalueStatistics.h"
#include "exahype2/dg/FaceIntegral.h"
#include "exahype2/dg/Riemann.h"
#include "exahype2/dg/tests/DeviceKernelFixtures.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/accelerator.h"

namespace {
  using Setup = exahype2::dg::tests::DeviceKernelSetup;

  TARGET_DECLARE
  struct MaxEigenvalueReduction final {
    using ValueType = exahype2::dg::MaxEigenvalueStatistics;

    static GPU_CALLABLE_METHOD ValueType identity() { return ValueType{}; }

    static GPU_CALLABLE_METHOD ValueType combine(const ValueType left, const ValueType right) {
      ValueType result;
      result.maxPhysicalEigenvalue = (left.maxPhysicalEigenvalue > right.maxPhysicalEigenvalue)
        ? left.maxPhysicalEigenvalue
        : right.maxPhysicalEigenvalue;
      result.maxScaledEigenvalue =
        (left.maxScaledEigenvalue > right.maxScaledEigenvalue) ? left.maxScaledEigenvalue : right.maxScaledEigenvalue;
      return result;
    }
  };
  TARGET_DECLARE_END
} // namespace

exahype2::dg::tests::KernelsOnDeviceTest::KernelsOnDeviceTest() :
  tarch::tests::TestCase("exahype2::dg::tests::KernelsOnDeviceTest") {}

void exahype2::dg::tests::KernelsOnDeviceTest::run() {
  testMethod(testExtrapolateStateToFaces);
  testMethod(testExtrapolateFluxToFaces);
  testMethod(testEvaluateFluxOnFace);
  testMethod(testApplyBoundaryConditions);
  testMethod(testSolveRiemannProblem);
  testMethod(testSolveLinearRiemannProblem);
  testMethod(testAddFaceIntegral);
  testMethod(testReduceMaxEigenvalues);
}

void exahype2::dg::tests::KernelsOnDeviceTest::validateBuffersMatch(
  const std::vector<double>& deviceResult,
  const std::vector<double>& hostResult,
  const std::string&         bufferName
) {
  validateEquals(deviceResult.size(), hostResult.size());
  for (std::size_t entry = 0; entry < deviceResult.size(); entry++) {
    validateNumericalEqualsWithParams2(deviceResult[entry], hostResult[entry], bufferName, entry);
  }
}

void exahype2::dg::tests::KernelsOnDeviceTest::testExtrapolateStateToFaces() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  const std::vector<double> cellStates = exahype2::dg::tests::makeRamp(Setup::AllCellStatesCardinality, 0.4, 0.013);
  const std::vector<double> faceStates(geometry.faceStateEntries(), 0.0);

  const exahype2::dg::tests::DeviceBuffer<double>        deviceCellStates(cellStates);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceStates(faceStates);
  const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.faceIndices);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceLeftFaceCoefficients(operators.leftFaceCoefficients);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceRightFaceCoefficients(operators.rightFaceCoefficients);

  const double* const        onDeviceCellStates            = deviceCellStates.data();
  double* const              onDeviceFaceStates            = deviceFaceStates.data();
  const std::uint64_t* const onDeviceFaceIndices           = deviceFaceIndices.data();
  const double* const        onDeviceLeftFaceCoefficients  = deviceLeftFaceCoefficients.data();
  const double* const        onDeviceRightFaceCoefficients = deviceRightFaceCoefficients.data();
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(Setup::NumberOfCells),
    [=] GPU_CALLABLE_LAMBDA(const int cell) {
      // A cell writes the half of each of its faces that lies on its own side, so
      // the two cells at a face never touch the same entries.
      double* cellFaces[peano4::FacesPerCell];
      for (int face = 0; face < peano4::FacesPerCell; face++) {
        cellFaces[face] = onDeviceFaceStates
          + onDeviceFaceIndices[cell * peano4::FacesPerCell + face] * Setup::FaceStateCardinality
          + (face < DIMENSIONS ? Setup::FaceSideStateCardinality : 0);
      }
      const int cellStateOffset = cell * Setup::CellStateCardinality;
      exahype2::dg::
        extrapolateStateToFaces<Setup::DofsPerAxis, Setup::NumberOfUnknowns, Setup::NumberOfAuxiliaryVariables>(
          cellFaces,
          onDeviceCellStates + cellStateOffset,
          onDeviceLeftFaceCoefficients,
          onDeviceRightFaceCoefficients
        );
    }
  );

  std::vector<double> deviceResult(faceStates.size());
  deviceFaceStates.copyBack(deviceResult);

  std::vector<double> hostResult = faceStates;
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    double* cellFaces[peano4::FacesPerCell];
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      cellFaces[face] = hostResult.data()
        + geometry.faceIndices[cell * peano4::FacesPerCell + face] * Setup::FaceStateCardinality
        + (face < DIMENSIONS ? Setup::FaceSideStateCardinality : 0);
    }
    const int cellStateOffset = cell * Setup::CellStateCardinality;
    exahype2::dg::
      extrapolateStateToFaces<Setup::DofsPerAxis, Setup::NumberOfUnknowns, Setup::NumberOfAuxiliaryVariables>(
        cellFaces,
        cellStates.data() + cellStateOffset,
        operators.leftFaceCoefficients.data(),
        operators.rightFaceCoefficients.data()
      );
  }

  validateBuffersMatch(deviceResult, hostResult, "faceStates");
}

void exahype2::dg::tests::KernelsOnDeviceTest::testExtrapolateFluxToFaces() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  const std::vector<double> cellStates = exahype2::dg::tests::makeRamp(Setup::AllCellStatesCardinality, 0.3, 0.017);
  const std::vector<double> faceFluxes(geometry.faceFluxEntries(), 0.0);

  const exahype2::dg::tests::DeviceBuffer<double>        deviceCellStates(cellStates);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceFluxes(faceFluxes);
  const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.faceIndices);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceCellCentres(geometry.cellCentres);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceLeftFaceCoefficients(operators.leftFaceCoefficients);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceRightFaceCoefficients(operators.rightFaceCoefficients);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceQuadratureNodes(operators.quadratureNodes);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceDerivativeOperator(operators.derivativeOperator);

  const double* const        onDeviceCellStates            = deviceCellStates.data();
  double* const              onDeviceFaceFluxes            = deviceFaceFluxes.data();
  const std::uint64_t* const onDeviceFaceIndices           = deviceFaceIndices.data();
  const double* const        onDeviceCellCentres           = deviceCellCentres.data();
  const double* const        onDeviceCellSizes             = deviceCellSizes.data();
  const double* const        onDeviceLeftFaceCoefficients  = deviceLeftFaceCoefficients.data();
  const double* const        onDeviceRightFaceCoefficients = deviceRightFaceCoefficients.data();
  const double* const        onDeviceQuadratureNodes       = deviceQuadratureNodes.data();
  const double* const        onDeviceDerivativeOperator    = deviceDerivativeOperator.data();
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(Setup::NumberOfCells),
    [=] GPU_CALLABLE_LAMBDA(const int cell) {
      double* cellFaces[peano4::FacesPerCell];
      for (int face = 0; face < peano4::FacesPerCell; face++) {
        cellFaces[face] = onDeviceFaceFluxes
          + onDeviceFaceIndices[cell * peano4::FacesPerCell + face] * Setup::FaceFluxCardinality
          + (face < DIMENSIONS ? Setup::FaceSideFluxCardinality : 0);
      }
      const int cellStateOffset  = cell * Setup::CellStateCardinality;
      const int coordinateOffset = cell * DIMENSIONS;
      exahype2::dg::extrapolateFluxToFaces<
        exahype2::dg::tests::StubSolver,
        Setup::DofsPerAxis,
        Setup::NumberOfUnknowns,
        Setup::NumberOfAuxiliaryVariables,
        true,
        true>(
        cellFaces,
        onDeviceCellStates + cellStateOffset,
        onDeviceLeftFaceCoefficients,
        onDeviceRightFaceCoefficients,
        onDeviceQuadratureNodes,
        onDeviceDerivativeOperator,
        tarch::la::Vector<DIMENSIONS, double>(onDeviceCellCentres + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(onDeviceCellSizes + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize
      );
    }
  );

  std::vector<double> deviceResult(faceFluxes.size());
  deviceFaceFluxes.copyBack(deviceResult);

  std::vector<double> hostResult = faceFluxes;
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    double* cellFaces[peano4::FacesPerCell];
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      cellFaces[face] = hostResult.data()
        + geometry.faceIndices[cell * peano4::FacesPerCell + face] * Setup::FaceFluxCardinality
        + (face < DIMENSIONS ? Setup::FaceSideFluxCardinality : 0);
    }
    const int cellStateOffset  = cell * Setup::CellStateCardinality;
    const int coordinateOffset = cell * DIMENSIONS;
    exahype2::dg::extrapolateFluxToFaces<
      exahype2::dg::tests::StubSolver,
      Setup::DofsPerAxis,
      Setup::NumberOfUnknowns,
      Setup::NumberOfAuxiliaryVariables,
      true,
      true>(
      cellFaces,
      cellStates.data() + cellStateOffset,
      operators.leftFaceCoefficients.data(),
      operators.rightFaceCoefficients.data(),
      operators.quadratureNodes.data(),
      operators.derivativeOperator.data(),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellCentres.data() + coordinateOffset),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
      Setup::TimeStamp,
      Setup::TimeStepSize
    );
  }

  validateBuffersMatch(deviceResult, hostResult, "faceFluxes");
}

void exahype2::dg::tests::KernelsOnDeviceTest::testEvaluateFluxOnFace() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  const std::vector<double> faceStates = exahype2::dg::tests::makeRamp(geometry.faceStateEntries(), 0.7, 0.009);
  const std::vector<double> faceFluxes(geometry.faceFluxEntries(), 0.0);

  const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceStates(faceStates);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceFluxes(faceFluxes);
  const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.workFaceIndices);
  const exahype2::dg::tests::DeviceBuffer<std::uint8_t>  deviceFaceNumbers(geometry.workFaceNumbers);
  const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceCellIndices(geometry.workCellIndices);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceCellCentres(geometry.cellCentres);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceQuadratureNodes(operators.quadratureNodes);

  const double* const        onDeviceFaceStates      = deviceFaceStates.data();
  double* const              onDeviceFaceFluxes      = deviceFaceFluxes.data();
  const std::uint64_t* const onDeviceFaceIndices     = deviceFaceIndices.data();
  const std::uint8_t* const  onDeviceFaceNumbers     = deviceFaceNumbers.data();
  const std::uint64_t* const onDeviceCellIndices     = deviceCellIndices.data();
  const double* const        onDeviceCellCentres     = deviceCellCentres.data();
  const double* const        onDeviceCellSizes       = deviceCellSizes.data();
  const double* const        onDeviceQuadratureNodes = deviceQuadratureNodes.data();
  // Every work item evaluates one side of one work face, so the items write
  // disjoint halves of the flux buffer.
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(2 * geometry.numberOfWorkFaces()),
    [=] GPU_CALLABLE_LAMBDA(const int item) {
      const int   face             = item / 2;
      const int   side             = item % 2;
      const int   faceNumber       = onDeviceFaceNumbers[face];
      const int   cell             = onDeviceCellIndices[face];
      const int   sideFluxOffset   = side * Setup::FaceSideFluxCardinality;
      const int   coordinateOffset = cell * DIMENSIONS;
      const auto* faceState        = onDeviceFaceStates + onDeviceFaceIndices[face] * Setup::FaceStateCardinality;
      auto*       faceFlux         = onDeviceFaceFluxes + onDeviceFaceIndices[face] * Setup::FaceFluxCardinality;
      exahype2::dg::evaluateFluxOnFace<
        exahype2::dg::tests::StubSolver,
        Setup::DofsPerAxis,
        Setup::NumberOfUnknowns,
        Setup::NumberOfAuxiliaryVariables,
        true,
        true>(
        faceFlux + sideFluxOffset,
        faceState,
        faceState + Setup::FaceSideStateCardinality,
        side,
        onDeviceQuadratureNodes,
        exahype2::dg::tests::faceCentreOf(cell, faceNumber, onDeviceCellCentres, onDeviceCellSizes),
        tarch::la::Vector<DIMENSIONS, double>(onDeviceCellSizes + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize,
        faceNumber % DIMENSIONS
      );
    }
  );

  std::vector<double> deviceResult(faceFluxes.size());
  deviceFaceFluxes.copyBack(deviceResult);

  std::vector<double> hostResult = faceFluxes;
  for (int face = 0; face < geometry.numberOfWorkFaces(); face++) {
    for (int side = 0; side < 2; side++) {
      const int   faceNumber       = geometry.workFaceNumbers[face];
      const int   cell             = geometry.workCellIndices[face];
      const int   sideFluxOffset   = side * Setup::FaceSideFluxCardinality;
      const int   coordinateOffset = cell * DIMENSIONS;
      const auto* faceState        = faceStates.data() + geometry.workFaceIndices[face] * Setup::FaceStateCardinality;
      auto*       faceFlux         = hostResult.data() + geometry.workFaceIndices[face] * Setup::FaceFluxCardinality;
      exahype2::dg::evaluateFluxOnFace<
        exahype2::dg::tests::StubSolver,
        Setup::DofsPerAxis,
        Setup::NumberOfUnknowns,
        Setup::NumberOfAuxiliaryVariables,
        true,
        true>(
        faceFlux + sideFluxOffset,
        faceState,
        faceState + Setup::FaceSideStateCardinality,
        side,
        operators.quadratureNodes.data(),
        exahype2::dg::tests::faceCentreOf(cell, faceNumber, geometry.cellCentres.data(), geometry.cellSizes.data()),
        tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize,
        faceNumber % DIMENSIONS
      );
    }
  }

  validateBuffersMatch(deviceResult, hostResult, "faceFluxes");
}

void exahype2::dg::tests::KernelsOnDeviceTest::testApplyBoundaryConditions() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  const std::vector<double> faceStates = exahype2::dg::tests::makeRamp(geometry.faceStateEntries(), 0.2, 0.011);

  const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceStates(faceStates);
  const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.workFaceIndices);
  const exahype2::dg::tests::DeviceBuffer<std::uint8_t>  deviceFaceNumbers(geometry.workFaceNumbers);
  const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceCellIndices(geometry.workCellIndices);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceCellCentres(geometry.cellCentres);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceQuadratureNodes(operators.quadratureNodes);

  double* const              onDeviceFaceStates      = deviceFaceStates.data();
  const std::uint64_t* const onDeviceFaceIndices     = deviceFaceIndices.data();
  const std::uint8_t* const  onDeviceFaceNumbers     = deviceFaceNumbers.data();
  const std::uint64_t* const onDeviceCellIndices     = deviceCellIndices.data();
  const double* const        onDeviceCellCentres     = deviceCellCentres.data();
  const double* const        onDeviceCellSizes       = deviceCellSizes.data();
  const double* const        onDeviceQuadratureNodes = deviceQuadratureNodes.data();
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(geometry.numberOfWorkFaces()),
    [=] GPU_CALLABLE_LAMBDA(const int face) {
      const int faceNumber       = onDeviceFaceNumbers[face];
      const int cell             = onDeviceCellIndices[face];
      const int coordinateOffset = cell * DIMENSIONS;
      exahype2::dg::applyBoundaryConditions<
        exahype2::dg::tests::StubSolver,
        Setup::DofsPerAxis,
        Setup::NumberOfUnknowns,
        Setup::NumberOfAuxiliaryVariables>(
        onDeviceFaceStates + onDeviceFaceIndices[face] * Setup::FaceStateCardinality,
        onDeviceQuadratureNodes,
        exahype2::dg::tests::faceCentreOf(cell, faceNumber, onDeviceCellCentres, onDeviceCellSizes),
        tarch::la::Vector<DIMENSIONS, double>(onDeviceCellSizes + coordinateOffset),
        Setup::TimeStamp,
        faceNumber
      );
    }
  );

  std::vector<double> deviceResult(faceStates.size());
  deviceFaceStates.copyBack(deviceResult);

  std::vector<double> hostResult = faceStates;
  for (int face = 0; face < geometry.numberOfWorkFaces(); face++) {
    const int faceNumber       = geometry.workFaceNumbers[face];
    const int cell             = geometry.workCellIndices[face];
    const int coordinateOffset = cell * DIMENSIONS;
    exahype2::dg::applyBoundaryConditions<
      exahype2::dg::tests::StubSolver,
      Setup::DofsPerAxis,
      Setup::NumberOfUnknowns,
      Setup::NumberOfAuxiliaryVariables>(
      hostResult.data() + geometry.workFaceIndices[face] * Setup::FaceStateCardinality,
      operators.quadratureNodes.data(),
      exahype2::dg::tests::faceCentreOf(cell, faceNumber, geometry.cellCentres.data(), geometry.cellSizes.data()),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
      Setup::TimeStamp,
      faceNumber
    );
  }

  validateBuffersMatch(deviceResult, hostResult, "faceStates");
}

void exahype2::dg::tests::KernelsOnDeviceTest::testSolveRiemannProblem() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  const std::vector<double> faceStates = exahype2::dg::tests::makeRamp(geometry.faceStateEntries(), 0.6, 0.007);
  // The nonlinear solver reads the extrapolated fluxes before it overwrites them.
  const std::vector<double> faceFluxes = exahype2::dg::tests::makeRamp(geometry.faceFluxEntries(), -0.3, 0.019);

  const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceStates(faceStates);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceFluxes(faceFluxes);
  const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.workFaceIndices);
  const exahype2::dg::tests::DeviceBuffer<std::uint8_t>  deviceFaceNumbers(geometry.workFaceNumbers);
  const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceCellIndices(geometry.workCellIndices);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceCellCentres(geometry.cellCentres);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceQuadratureWeights(operators.faceQuadratureWeights);

  const double* const        onDeviceFaceStates            = deviceFaceStates.data();
  double* const              onDeviceFaceFluxes            = deviceFaceFluxes.data();
  const std::uint64_t* const onDeviceFaceIndices           = deviceFaceIndices.data();
  const std::uint8_t* const  onDeviceFaceNumbers           = deviceFaceNumbers.data();
  const std::uint64_t* const onDeviceCellIndices           = deviceCellIndices.data();
  const double* const        onDeviceCellCentres           = deviceCellCentres.data();
  const double* const        onDeviceCellSizes             = deviceCellSizes.data();
  const double* const        onDeviceFaceQuadratureWeights = deviceFaceQuadratureWeights.data();
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(geometry.numberOfWorkFaces()),
    [=] GPU_CALLABLE_LAMBDA(const int face) {
      const int   faceNumber       = onDeviceFaceNumbers[face];
      const int   cell             = onDeviceCellIndices[face];
      const int   coordinateOffset = cell * DIMENSIONS;
      const auto* faceState        = onDeviceFaceStates + onDeviceFaceIndices[face] * Setup::FaceStateCardinality;
      auto*       faceFlux         = onDeviceFaceFluxes + onDeviceFaceIndices[face] * Setup::FaceFluxCardinality;
      exahype2::dg::solveRiemannProblem<
        exahype2::dg::tests::StubSolver,
        Setup::DofsPerAxis,
        Setup::NumberOfUnknowns,
        Setup::NumberOfAuxiliaryVariables,
        true>(
        faceFlux,
        faceFlux + Setup::FaceSideFluxCardinality,
        faceState,
        faceState + Setup::FaceSideStateCardinality,
        onDeviceFaceQuadratureWeights,
        exahype2::dg::tests::faceCentreOf(cell, faceNumber, onDeviceCellCentres, onDeviceCellSizes),
        tarch::la::Vector<DIMENSIONS, double>(onDeviceCellSizes + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize,
        faceNumber % DIMENSIONS
      );
    }
  );

  std::vector<double> deviceResult(faceFluxes.size());
  deviceFaceFluxes.copyBack(deviceResult);

  std::vector<double> hostResult = faceFluxes;
  for (int face = 0; face < geometry.numberOfWorkFaces(); face++) {
    const int   faceNumber       = geometry.workFaceNumbers[face];
    const int   cell             = geometry.workCellIndices[face];
    const int   coordinateOffset = cell * DIMENSIONS;
    const auto* faceState        = faceStates.data() + geometry.workFaceIndices[face] * Setup::FaceStateCardinality;
    auto*       faceFlux         = hostResult.data() + geometry.workFaceIndices[face] * Setup::FaceFluxCardinality;
    exahype2::dg::solveRiemannProblem<
      exahype2::dg::tests::StubSolver,
      Setup::DofsPerAxis,
      Setup::NumberOfUnknowns,
      Setup::NumberOfAuxiliaryVariables,
      true>(
      faceFlux,
      faceFlux + Setup::FaceSideFluxCardinality,
      faceState,
      faceState + Setup::FaceSideStateCardinality,
      operators.faceQuadratureWeights.data(),
      exahype2::dg::tests::faceCentreOf(cell, faceNumber, geometry.cellCentres.data(), geometry.cellSizes.data()),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
      Setup::TimeStamp,
      Setup::TimeStepSize,
      faceNumber % DIMENSIONS
    );
  }

  validateBuffersMatch(deviceResult, hostResult, "faceFluxes");
}

void exahype2::dg::tests::KernelsOnDeviceTest::testSolveLinearRiemannProblem() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  const std::vector<double> faceStates = exahype2::dg::tests::makeRamp(geometry.faceStateEntries(), 0.9, 0.005);
  const std::vector<double> faceFluxes(geometry.faceFluxEntries(), 0.0);

  const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceStates(faceStates);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceFluxes(faceFluxes);
  const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.workFaceIndices);
  const exahype2::dg::tests::DeviceBuffer<std::uint8_t>  deviceFaceNumbers(geometry.workFaceNumbers);
  const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceCellIndices(geometry.workCellIndices);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceCellCentres(geometry.cellCentres);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceQuadratureWeights(operators.faceQuadratureWeights);

  const double* const        onDeviceFaceStates            = deviceFaceStates.data();
  double* const              onDeviceFaceFluxes            = deviceFaceFluxes.data();
  const std::uint64_t* const onDeviceFaceIndices           = deviceFaceIndices.data();
  const std::uint8_t* const  onDeviceFaceNumbers           = deviceFaceNumbers.data();
  const std::uint64_t* const onDeviceCellIndices           = deviceCellIndices.data();
  const double* const        onDeviceCellCentres           = deviceCellCentres.data();
  const double* const        onDeviceCellSizes             = deviceCellSizes.data();
  const double* const        onDeviceFaceQuadratureWeights = deviceFaceQuadratureWeights.data();
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(geometry.numberOfWorkFaces()),
    [=] GPU_CALLABLE_LAMBDA(const int face) {
      const int   faceNumber       = onDeviceFaceNumbers[face];
      const int   cell             = onDeviceCellIndices[face];
      const int   coordinateOffset = cell * DIMENSIONS;
      const auto* faceState        = onDeviceFaceStates + onDeviceFaceIndices[face] * Setup::FaceStateCardinality;
      auto*       faceFlux         = onDeviceFaceFluxes + onDeviceFaceIndices[face] * Setup::FaceFluxCardinality;
      exahype2::dg::solveLinearRiemannProblem<
        exahype2::dg::tests::StubSolver,
        Setup::DofsPerAxis,
        Setup::NumberOfUnknowns,
        Setup::NumberOfAuxiliaryVariables,
        true,
        true,
        true>(
        faceFlux,
        faceFlux + Setup::FaceSideFluxCardinality,
        faceState,
        faceState + Setup::FaceSideStateCardinality,
        onDeviceFaceQuadratureWeights,
        exahype2::dg::tests::faceCentreOf(cell, faceNumber, onDeviceCellCentres, onDeviceCellSizes),
        tarch::la::Vector<DIMENSIONS, double>(onDeviceCellSizes + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize,
        faceNumber % DIMENSIONS
      );
    }
  );

  std::vector<double> deviceResult(faceFluxes.size());
  deviceFaceFluxes.copyBack(deviceResult);

  std::vector<double> hostResult = faceFluxes;
  for (int face = 0; face < geometry.numberOfWorkFaces(); face++) {
    const int   faceNumber       = geometry.workFaceNumbers[face];
    const int   cell             = geometry.workCellIndices[face];
    const int   coordinateOffset = cell * DIMENSIONS;
    const auto* faceState        = faceStates.data() + geometry.workFaceIndices[face] * Setup::FaceStateCardinality;
    auto*       faceFlux         = hostResult.data() + geometry.workFaceIndices[face] * Setup::FaceFluxCardinality;
    exahype2::dg::solveLinearRiemannProblem<
      exahype2::dg::tests::StubSolver,
      Setup::DofsPerAxis,
      Setup::NumberOfUnknowns,
      Setup::NumberOfAuxiliaryVariables,
      true,
      true,
      true>(
      faceFlux,
      faceFlux + Setup::FaceSideFluxCardinality,
      faceState,
      faceState + Setup::FaceSideStateCardinality,
      operators.faceQuadratureWeights.data(),
      exahype2::dg::tests::faceCentreOf(cell, faceNumber, geometry.cellCentres.data(), geometry.cellSizes.data()),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
      Setup::TimeStamp,
      Setup::TimeStepSize,
      faceNumber % DIMENSIONS
    );
  }

  validateBuffersMatch(deviceResult, hostResult, "faceFluxes");
}

void exahype2::dg::tests::KernelsOnDeviceTest::testAddFaceIntegral() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  // The lift accumulates, so both paths have to start from the same estimates.
  const std::vector<double> rightHandSideEstimates =
    exahype2::dg::tests::makeRamp(Setup::AllRightHandSidesCardinality, 0.1, 0.003);
  const std::vector<double> faceFluxes = exahype2::dg::tests::makeRamp(geometry.faceFluxEntries(), -0.2, 0.009);

  const exahype2::dg::tests::DeviceBuffer<double>        deviceRightHandSideEstimates(rightHandSideEstimates);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceFluxes(faceFluxes);
  const exahype2::dg::tests::DeviceBuffer<std::uint64_t> deviceFaceIndices(geometry.faceIndices);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceCellSizes(geometry.cellSizes);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceFaceQuadratureWeights(operators.faceQuadratureWeights);
  const exahype2::dg::tests::DeviceBuffer<double>        deviceInverseCellQuadratureWeights(
    operators.inverseCellQuadratureWeights
  );
  const exahype2::dg::tests::DeviceBuffer<double> deviceLeftFaceCoefficients(operators.leftFaceCoefficients);
  const exahype2::dg::tests::DeviceBuffer<double> deviceRightFaceCoefficients(operators.rightFaceCoefficients);

  double* const              onDeviceRightHandSideEstimates       = deviceRightHandSideEstimates.data();
  const double* const        onDeviceFaceFluxes                   = deviceFaceFluxes.data();
  const std::uint64_t* const onDeviceFaceIndices                  = deviceFaceIndices.data();
  const double* const        onDeviceCellSizes                    = deviceCellSizes.data();
  const double* const        onDeviceFaceQuadratureWeights        = deviceFaceQuadratureWeights.data();
  const double* const        onDeviceInverseCellQuadratureWeights = deviceInverseCellQuadratureWeights.data();
  const double* const        onDeviceLeftFaceCoefficients         = deviceLeftFaceCoefficients.data();
  const double* const        onDeviceRightFaceCoefficients        = deviceRightFaceCoefficients.data();
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(Setup::NumberOfCells),
    [=] GPU_CALLABLE_LAMBDA(const int cell) {
      const int   cellUpdateOffset = cell * Setup::RightHandSideCardinality + Setup::RungeKuttaStepOffset;
      const int   coordinateOffset = cell * DIMENSIONS;
      auto* const cellUpdate       = onDeviceRightHandSideEstimates + cellUpdateOffset;
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        const double inverseCellSizeAlongNormal = 1.0 / onDeviceCellSizes[coordinateOffset + direction];

        const auto* const lowerFaceFlux = onDeviceFaceFluxes
          + onDeviceFaceIndices[cell * peano4::FacesPerCell + direction] * Setup::FaceFluxCardinality
          + Setup::FaceSideFluxCardinality;
        exahype2::dg::addFaceIntegral<Setup::DofsPerAxis, Setup::NumberOfUnknowns, false, double, double>(
          cellUpdate,
          lowerFaceFlux,
          onDeviceFaceQuadratureWeights,
          onDeviceInverseCellQuadratureWeights,
          onDeviceLeftFaceCoefficients,
          onDeviceRightFaceCoefficients,
          direction,
          0,
          inverseCellSizeAlongNormal,
          Setup::TimeStepSize
        );

        const auto* const upperFaceFlux = onDeviceFaceFluxes
          + onDeviceFaceIndices[cell * peano4::FacesPerCell + direction + DIMENSIONS] * Setup::FaceFluxCardinality;
        exahype2::dg::addFaceIntegral<Setup::DofsPerAxis, Setup::NumberOfUnknowns, false, double, double>(
          cellUpdate,
          upperFaceFlux,
          onDeviceFaceQuadratureWeights,
          onDeviceInverseCellQuadratureWeights,
          onDeviceLeftFaceCoefficients,
          onDeviceRightFaceCoefficients,
          direction,
          1,
          inverseCellSizeAlongNormal,
          Setup::TimeStepSize
        );
      }
    }
  );

  std::vector<double> deviceResult(rightHandSideEstimates.size());
  deviceRightHandSideEstimates.copyBack(deviceResult);

  std::vector<double> hostResult = rightHandSideEstimates;
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    const int   cellUpdateOffset = cell * Setup::RightHandSideCardinality + Setup::RungeKuttaStepOffset;
    const int   coordinateOffset = cell * DIMENSIONS;
    auto* const cellUpdate       = hostResult.data() + cellUpdateOffset;
    for (int direction = 0; direction < DIMENSIONS; direction++) {
      const double inverseCellSizeAlongNormal = 1.0 / geometry.cellSizes[coordinateOffset + direction];

      const auto* const lowerFaceFlux = faceFluxes.data()
        + geometry.faceIndices[cell * peano4::FacesPerCell + direction] * Setup::FaceFluxCardinality
        + Setup::FaceSideFluxCardinality;
      exahype2::dg::addFaceIntegral<Setup::DofsPerAxis, Setup::NumberOfUnknowns, false, double, double>(
        cellUpdate,
        lowerFaceFlux,
        operators.faceQuadratureWeights.data(),
        operators.inverseCellQuadratureWeights.data(),
        operators.leftFaceCoefficients.data(),
        operators.rightFaceCoefficients.data(),
        direction,
        0,
        inverseCellSizeAlongNormal,
        Setup::TimeStepSize
      );

      const auto* const upperFaceFlux = faceFluxes.data()
        + geometry.faceIndices[cell * peano4::FacesPerCell + direction + DIMENSIONS] * Setup::FaceFluxCardinality;
      exahype2::dg::addFaceIntegral<Setup::DofsPerAxis, Setup::NumberOfUnknowns, false, double, double>(
        cellUpdate,
        upperFaceFlux,
        operators.faceQuadratureWeights.data(),
        operators.inverseCellQuadratureWeights.data(),
        operators.leftFaceCoefficients.data(),
        operators.rightFaceCoefficients.data(),
        direction,
        1,
        inverseCellSizeAlongNormal,
        Setup::TimeStepSize
      );
    }
  }

  validateBuffersMatch(deviceResult, hostResult, "rightHandSideEstimates");
}

void exahype2::dg::tests::KernelsOnDeviceTest::testReduceMaxEigenvalues() {
  const exahype2::dg::tests::KernelOperators operators;
  const exahype2::dg::tests::CellChain       geometry;
  const std::vector<double> cellStates = exahype2::dg::tests::makeRamp(Setup::AllCellStatesCardinality, 0.25, 0.031);

  const exahype2::dg::tests::DeviceBuffer<double> deviceCellStates(cellStates);
  const exahype2::dg::tests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
  const exahype2::dg::tests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);
  const exahype2::dg::tests::DeviceBuffer<double> deviceQuadratureNodes(operators.quadratureNodes);

  const double* const onDeviceCellStates      = deviceCellStates.data();
  const double* const onDeviceCellCentres     = deviceCellCentres.data();
  const double* const onDeviceCellSizes       = deviceCellSizes.data();
  const double* const onDeviceQuadratureNodes = deviceQuadratureNodes.data();

  exahype2::dg::MaxEigenvalueStatistics deviceResult = MaxEigenvalueReduction::identity();
  tarch::accelerator::parallelReduce(
    tarch::accelerator::Range(Setup::NumberOfCells),
    [=] GPU_CALLABLE_LAMBDA(const int cell, MaxEigenvalueStatistics& maximum) {
      const int cellStateOffset  = cell * Setup::CellStateCardinality;
      const int coordinateOffset = cell * DIMENSIONS;
      maximum                    = MaxEigenvalueReduction::combine(
        maximum,
        exahype2::dg::reduceMaxEigenvalues<
          exahype2::dg::tests::StubSolver,
          Setup::DofsPerAxis,
          Setup::NumberOfUnknowns,
          Setup::NumberOfAuxiliaryVariables>(
          onDeviceCellStates + cellStateOffset,
          onDeviceQuadratureNodes,
          tarch::la::Vector<DIMENSIONS, double>(onDeviceCellCentres + coordinateOffset),
          tarch::la::Vector<DIMENSIONS, double>(onDeviceCellSizes + coordinateOffset),
          Setup::TimeStamp,
          Setup::TimeStepSize
        )
      );
    },
    tarch::accelerator::Custom<MaxEigenvalueReduction>{deviceResult}
  );

  exahype2::dg::MaxEigenvalueStatistics hostResult = MaxEigenvalueReduction::identity();
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    const int cellStateOffset  = cell * Setup::CellStateCardinality;
    const int coordinateOffset = cell * DIMENSIONS;
    hostResult                 = MaxEigenvalueReduction::combine(
      hostResult,
      exahype2::dg::reduceMaxEigenvalues<
        exahype2::dg::tests::StubSolver,
        Setup::DofsPerAxis,
        Setup::NumberOfUnknowns,
        Setup::NumberOfAuxiliaryVariables>(
        cellStates.data() + cellStateOffset,
        operators.quadratureNodes.data(),
        tarch::la::Vector<DIMENSIONS, double>(geometry.cellCentres.data() + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
        Setup::TimeStamp,
        Setup::TimeStepSize
      )
    );
  }

  // A maximum ignores the order in which the tree combines the cells, so the
  // parallel reduction has to land on exactly the serial value.
  validateNumericalEquals(deviceResult.maxScaledEigenvalue, hostResult.maxScaledEigenvalue);
  validateNumericalEquals(deviceResult.maxPhysicalEigenvalue, hostResult.maxPhysicalEigenvalue);
  validate(hostResult.maxPhysicalEigenvalue > 0.0);
}
