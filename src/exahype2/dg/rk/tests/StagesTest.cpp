// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StagesTest.h"

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/Kernels.h"
#include "exahype2/dg/rk/Stages.h"
#include "exahype2/dg/tests/KernelTestData.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/la/Vector.h"

namespace {
  using Setup  = exahype2::dg::tests::KernelTestSetup;
  using Tables = exahype2::dg::tests::BasisTableSet;

  TARGET_DECLARE
  /**
   * The layout constants the stub solvers of the Runge-Kutta tests derive from.
   */
  struct StubLayout {
    static constexpr int DGOrder                    = 2;
    static constexpr int RungeKuttaOrder            = 3;
    static constexpr int NumberOfUnknowns           = 2;
    static constexpr int NumberOfAuxiliaryVariables = 1;
  };

  template <typename Solver>
  using StageOf = exahype2::dg::rk::Stage<Solver, double>;

  using CellLayout = exahype2::dg::PaddedLayout<StubLayout>;

  constexpr int    NodesPerAxis          = CellLayout::NodesPerAxis;
  constexpr int    NodesPerCell          = CellLayout::NodesPerCell;
  constexpr int    NodesPerFace          = CellLayout::NodesPerFace;
  constexpr int    NumberOfUnknowns      = CellLayout::NumberOfUnknowns;
  constexpr int    NumberOfData          = CellLayout::NumberOfData;
  constexpr int    NumberOfStages        = StageOf<StubLayout>::Tableau::Stages;
  constexpr int    UnknownsPerStage      = StageOf<StubLayout>::UnknownsPerStage;
  constexpr int    EstimateValuesPerCell = StageOf<StubLayout>::EstimateValuesPerCell;
  constexpr int    StateValuesPerCell    = StageOf<StubLayout>::StateValuesPerCell;
  constexpr int    StateValuesPerFace    = NodesPerFace * NumberOfData;
  constexpr int    FluxValuesPerFace     = NodesPerFace * NumberOfUnknowns;
  constexpr int    OpenedStage           = 1;
  constexpr double TimeStamp             = 0.5;
  constexpr double TimeStepSize          = 0.125;
  constexpr double Slope                 = 3.0;
  constexpr double WaveSpeed             = 3.0;
  constexpr double UniformCellExtent     = 0.5;

  struct FluxTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         fluxValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] = (1.0 + direction) * state[unknown];
      }
    }
  };

  struct DiffusiveFluxTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* const NOALIAS                                   stateGradients[DIMENSIONS],
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    direction,
      double* NOALIAS const                                         fluxValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] = 0.0;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          fluxValues[unknown] += (1.0 + axis) * stateGradients[axis][unknown];
        }
      }
    }
  };

  struct SourceTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         sourceValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        sourceValues[unknown] = (1.0 + unknown) * state[unknown];
      }
    }
  };

  struct ProductTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateGradient,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    direction,
      double* NOALIAS const                                         productValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        productValues[unknown] = stateGradient[unknown];
      }
    }
  };

  /**
   * A wave speed that is nonzero only at the end of the time step of the tests.
   */
  struct WaveSpeedTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
      [[maybe_unused]] const double* NOALIAS const                  state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      const double                                                  timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    direction
    ) {
      return timeStamp == TimeStamp + TimeStepSize ? WaveSpeed : 0.0;
    }
  };

  struct InertSolver final : StubLayout {};
  struct FluxSolver final : StubLayout, FluxTerm {};
  struct SourceSolver final : StubLayout, SourceTerm {};
  struct ProductSolver final : StubLayout, ProductTerm {};
  struct DiffusiveSolver final : StubLayout, DiffusiveFluxTerm {};
  struct FullSolver final : StubLayout, FluxTerm, SourceTerm, ProductTerm {};
  struct DiffusiveFullSolver final : StubLayout, DiffusiveFluxTerm, SourceTerm, ProductTerm {};
  struct WaveSpeedSolver final : StubLayout, WaveSpeedTerm {};

  /**
   * A solver whose acoustic flux along an axis couples the pressure with the velocity of that axis,
   * so a swap of two axes maps the flux onto itself.
   */
  struct AcousticSolver final {
    static constexpr int DGOrder                    = StubLayout::DGOrder;
    static constexpr int RungeKuttaOrder            = StubLayout::RungeKuttaOrder;
    static constexpr int NumberOfUnknowns           = 1 + DIMENSIONS;
    static constexpr int NumberOfAuxiliaryVariables = 0;

    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         fluxValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] = 0.0;
      }
      fluxValues[0]             = state[1 + direction];
      fluxValues[1 + direction] = state[0];
    }
  };
  TARGET_DECLARE_END

  constexpr int AcousticUnknowns = AcousticSolver::NumberOfUnknowns;

  /**
   * Returns the node that node moves to when the first two axes swap.
   */
  int getMirroredNode(const int node) {
    const auto                         index = peano4::dDelinearised(node, NodesPerAxis);
    tarch::la::Vector<DIMENSIONS, int> mirrored(index);
    mirrored(0) = index(1);
    mirrored(1) = index(0);
    return peano4::dLinearised(mirrored, NodesPerAxis);
  }

  /**
   * Returns the unknown that unknown moves to when the first two axes swap.
   */
  int getMirroredUnknown(const int unknown) {
    if (unknown == 1) {
      return 2;
    }
    if (unknown == 2) {
      return 1;
    }
    return unknown;
  }

  tarch::la::Vector<DIMENSIONS, double> getCellSize() {
    tarch::la::Vector<DIMENSIONS, double> cellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSize(axis) = 0.5 + 0.25 * axis;
    }
    return cellSize;
  }

  /**
   * Fills the state with a profile that is linear along one axis and constant along the others.
   */
  void fillLinearState(const Tables& tables, double* const cellState, const int axis, const double slope) {
    for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
      const double reference = tables.quadraturePoints[peano4::dDelinearised(cellNode, NodesPerAxis)(axis)];
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = 1.0 + value + slope * reference;
      }
    }
  }

  std::array<double, NodesPerFace> getFaceQuadratureWeights(const Tables& tables) {
    std::array<double, NodesPerFace> weights{};
    for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
      int    index  = faceNode;
      double weight = 1.0;
      for (int axis = 0; axis < DIMENSIONS - 1; axis++) {
        weight *= tables.quadratureWeights[index % NodesPerAxis];
        index /= NodesPerAxis;
      }
      weights[static_cast<std::size_t>(faceNode)] = weight;
    }
    return weights;
  }

  struct FaceHalves final {
    double  states[peano4::FacesPerCell][StateValuesPerFace]{};
    double  fluxes[peano4::FacesPerCell][FluxValuesPerFace]{};
    double* statePointers[peano4::FacesPerCell]{};
    double* fluxPointers[peano4::FacesPerCell]{};

    FaceHalves() {
      for (int face = 0; face < peano4::FacesPerCell; face++) {
        statePointers[face] = states[face];
        fluxPointers[face]  = fluxes[face];
      }
    }
  };

  TARGET_DECLARE
  INLINE_METHOD GPU_CALLABLE_METHOD void collectFacePointers(
    const int     cell,
    double* const faceStates,
    double* const faceFluxes,
    double* (&faceState)[peano4::FacesPerCell],
    double* (&faceFlux)[peano4::FacesPerCell]
  ) {
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      const auto slot = static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + face;
      faceState[face] = faceStates + slot * StateValuesPerFace;
      faceFlux[face]  = faceFluxes + slot * FluxValuesPerFace;
    }
  }

  template <typename Solver>
  struct OpenStageOfCell final {
    typename StageOf<Solver>::Weights weights{};
    exahype2::dg::BasisTables<double> tables{};
    const double*                     cellStates{nullptr};
    double*                           estimates{nullptr};
    double*                           stageStates{nullptr};
    double*                           faceStates{nullptr};
    double*                           faceFluxes{nullptr};
    const double*                     cellCentres{nullptr};
    const double*                     cellSizes{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int cell) const {
      double* faceState[peano4::FacesPerCell];
      double* faceFlux[peano4::FacesPerCell];
      collectFacePointers(cell, faceStates, faceFluxes, faceState, faceFlux);
      const int  coordinateOffset = cell * DIMENSIONS;
      const auto cellOffset       = static_cast<std::ptrdiff_t>(cell);
      StageOf<Solver>::open(
        weights,
        cellStates + cellOffset * StateValuesPerCell,
        estimates + cellOffset * EstimateValuesPerCell,
        stageStates + cellOffset * StateValuesPerCell,
        OpenedStage,
        faceState,
        faceFlux,
        tables.quadraturePoints,
        tables.derivativeOperator,
        tables.leftFaceCoefficients,
        tables.rightFaceCoefficients,
        tarch::la::Vector<DIMENSIONS, double>(cellCentres + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
        TimeStamp,
        TimeStepSize
      );
    }
  };

  struct CloseStepOfCell final {
    StageOf<FullSolver>::Weights weights{};
    double*                      cellStates{nullptr};
    const double*                estimates{nullptr};
    const double*                cellCentres{nullptr};
    const double*                cellSizes{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int cell) const {
      const int  coordinateOffset = cell * DIMENSIONS;
      const auto cellOffset       = static_cast<std::ptrdiff_t>(cell);
      static_cast<void>(StageOf<FullSolver>::close<false>(
        weights,
        cellStates + cellOffset * StateValuesPerCell,
        estimates + cellOffset * EstimateValuesPerCell,
        static_cast<const double*>(nullptr),
        tarch::la::Vector<DIMENSIONS, double>(cellCentres + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
        TimeStamp,
        TimeStepSize
      ));
    }
  };
  TARGET_DECLARE_END

  template <typename Solver>
  void openOnDevice(
    const Tables&                            tables,
    const exahype2::dg::tests::CellChain&    geometry,
    const typename StageOf<Solver>::Weights& weights,
    const std::vector<double>&               cellStates,
    std::vector<double>&                     estimates,
    std::vector<double>&                     faceStates,
    std::vector<double>&                     faceFluxes
  ) {
    const exahype2::dg::tests::DeviceBasisTables deviceTables(tables);
    const std::vector<double> stageStates(static_cast<std::size_t>(Setup::NumberOfCells) * StateValuesPerCell, 0.0);

    const exahype2::dg::tests::DeviceBuffer<double> deviceCellStates(cellStates);
    const exahype2::dg::tests::DeviceBuffer<double> deviceEstimates(estimates);
    const exahype2::dg::tests::DeviceBuffer<double> deviceStageStates(stageStates);
    const exahype2::dg::tests::DeviceBuffer<double> deviceFaceStates(faceStates);
    const exahype2::dg::tests::DeviceBuffer<double> deviceFaceFluxes(faceFluxes);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);

    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(Setup::NumberOfCells),
      OpenStageOfCell<Solver>{
        weights,
        deviceTables.getView(),
        deviceCellStates.getData(),
        deviceEstimates.getData(),
        deviceStageStates.getData(),
        deviceFaceStates.getData(),
        deviceFaceFluxes.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData()
      }
    );

    deviceEstimates.copyBack(estimates);
    deviceFaceStates.copyBack(faceStates);
    deviceFaceFluxes.copyBack(faceFluxes);
  }

  void closeOnDevice(
    const exahype2::dg::tests::CellChain& geometry,
    const StageOf<FullSolver>::Weights&   weights,
    std::vector<double>&                  cellStates,
    const std::vector<double>&            estimates
  ) {
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellStates(cellStates);
    const exahype2::dg::tests::DeviceBuffer<double> deviceEstimates(estimates);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);

    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(Setup::NumberOfCells),
      CloseStepOfCell{
        weights,
        deviceCellStates.getData(),
        deviceEstimates.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData()
      }
    );

    deviceCellStates.copyBack(cellStates);
  }
} // namespace

exahype2::dg::rk::tests::StagesTest::StagesTest() :
  tarch::tests::TestCase("exahype2::dg::rk::tests::StagesTest") {}

void
exahype2::dg::rk::tests::StagesTest::run() {
  testMethod(testTheCombinationWeighsTheEstimatesAndCopiesTheAuxiliaryVariables);
  testMethod(testAZeroWeightSkipsAnUnwrittenEstimate);
  testMethod(testTheCombinationWritesInPlace);
  testMethod(testAConstantFluxGivesZerosBitwise);
  testMethod(testTheDivergenceOfALinearFluxIsItsSlope);
  testMethod(testTheSourceTermIsScaledByTheTimeStep);
  testMethod(testTheNonconservativeProductSeesTheNodalGradient);
  testMethod(testTheDiffusiveFluxSeesTheNodalGradients);
  testMethod(testMirroredCellsReceiveMirroredEstimates);
  testMethod(testTheFaceStatesOfALinearStateAreItsValuesAtTheFaces);
  testMethod(testAConstantStateOpensAStageWithAZeroEstimateAndConstantFaceStates);
  testMethod(testTheFaceIntegralsReachTheEstimateOfTheStageAlone);
  testMethod(testTheCloseWritesTheWeightedSumInPlaceAndReportsTheWaveSpeeds);
  testMethod(testTheButcherTableausAreConsistent);
  testMethod(testEveryCellOfADeviceRangeOpensAStageLikeTheHost);
  testMethod(testEveryCellOfADeviceRangeOpensADiffusiveStageLikeTheHost);
  testMethod(testEveryCellOfADeviceRangeClosesTheStepLikeTheHost);
}

/**
 * The combination adds every weighted estimate to the unknowns and copies the auxiliary variables.
 */
void
exahype2::dg::rk::tests::StagesTest::testTheCombinationWeighsTheEstimatesAndCopiesTheAuxiliaryVariables() {
  double cellState[StateValuesPerCell];
  for (int entry = 0; entry < StateValuesPerCell; entry++) {
    cellState[entry] = 1.0 + 0.5 * entry;
  }
  double estimates[EstimateValuesPerCell];
  for (int entry = 0; entry < EstimateValuesPerCell; entry++) {
    estimates[entry] = 0.25 * entry;
  }
  const StageOf<InertSolver>::Weights weights{0.5, -1.0, 2.0};
  double                              target[StateValuesPerCell];

  StageOf<InertSolver>::combineEstimates(weights, cellState, estimates, target);

  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      double expected = cellState[cellNode * NumberOfData + unknown];
      for (int stage = 0; stage < NumberOfStages; stage++) {
        expected += weights[static_cast<std::size_t>(stage)]
          * estimates[stage * UnknownsPerStage + cellNode * NumberOfUnknowns + unknown];
      }
      validateNumericalEqualsWithParams2(target[cellNode * NumberOfData + unknown], expected, cellNode, unknown);
    }
    validateEqualsWithParams1(
      target[cellNode * NumberOfData + NumberOfUnknowns],
      cellState[cellNode * NumberOfData + NumberOfUnknowns],
      cellNode
    );
  }
}

/**
 * A stage with zero weights reproduces the cell state bitwise and reads no estimate.
 */
void
exahype2::dg::rk::tests::StagesTest::testAZeroWeightSkipsAnUnwrittenEstimate() {
  double cellState[StateValuesPerCell];
  for (int entry = 0; entry < StateValuesPerCell; entry++) {
    cellState[entry] = 3.0 - 0.125 * entry;
  }
  double estimates[EstimateValuesPerCell];
  for (int entry = 0; entry < EstimateValuesPerCell; entry++) {
    estimates[entry] = std::numeric_limits<double>::quiet_NaN();
  }
  const StageOf<InertSolver>::Weights weights{0.0, 0.0, 0.0};
  double                              target[StateValuesPerCell];

  StageOf<InertSolver>::combineEstimates(weights, cellState, estimates, target);

  for (int entry = 0; entry < StateValuesPerCell; entry++) {
    validateEqualsWithParams1(target[entry], cellState[entry], entry);
  }
}

/**
 * The combination into the cell state itself equals the combination into another target.
 */
void
exahype2::dg::rk::tests::StagesTest::testTheCombinationWritesInPlace() {
  double cellState[StateValuesPerCell];
  double reference[StateValuesPerCell];
  for (int entry = 0; entry < StateValuesPerCell; entry++) {
    cellState[entry] = 2.0 + 0.5 * entry;
    reference[entry] = cellState[entry];
  }
  double estimates[EstimateValuesPerCell];
  for (int entry = 0; entry < EstimateValuesPerCell; entry++) {
    estimates[entry] = 1.0 + entry;
  }
  const StageOf<InertSolver>::Weights weights{1.0 / 6.0, 2.0 / 3.0, 1.0 / 6.0};
  double                              target[StateValuesPerCell];

  StageOf<InertSolver>::combineEstimates(weights, reference, estimates, target);
  StageOf<InertSolver>::combineEstimates(weights, cellState, estimates, cellState);

  for (int entry = 0; entry < StateValuesPerCell; entry++) {
    validateEqualsWithParams1(cellState[entry], target[entry], entry);
  }
}

/**
 * A constant state gives a zero flux divergence bitwise.
 */
void
exahype2::dg::rk::tests::StagesTest::testAConstantFluxGivesZerosBitwise() {
  const Tables                                tables;
  const auto                                  cellSize = getCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[StateValuesPerCell];
  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      cellState[cellNode * NumberOfData + value] = 2.0 + value;
    }
  }
  double estimate[UnknownsPerStage];
  for (int entry = 0; entry < UnknownsPerStage; entry++) {
    estimate[entry] = 9.0;
  }

  StageOf<FluxSolver>::volumeIntegral(
    cellState,
    estimate,
    tables.quadraturePoints.data(),
    tables.derivativeOperator.data(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  for (int entry = 0; entry < UnknownsPerStage; entry++) {
    validateEqualsWithParams1(estimate[entry], 0.0, entry);
  }
}

/**
 * A flux that is linear along the first axis subtracts its physical slope times the time step at every node.
 */
void
exahype2::dg::rk::tests::StagesTest::testTheDivergenceOfALinearFluxIsItsSlope() {
  const Tables                                tables;
  const auto                                  cellSize = getCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[StateValuesPerCell];
  fillLinearState(tables, cellState, 0, Slope);
  double estimate[UnknownsPerStage] = {};

  StageOf<FluxSolver>::volumeIntegral(
    cellState,
    estimate,
    tables.quadraturePoints.data(),
    tables.derivativeOperator.data(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  // Only the flux along the first axis varies, and its physical slope is the reference slope
  // over the cell extent.
  const double expected = -TimeStepSize * Slope / cellSize(0);
  for (int entry = 0; entry < UnknownsPerStage; entry++) {
    validateNumericalEqualsWithParams1(estimate[entry], expected, entry);
  }
}

/**
 * The source of every node enters the estimate times the time step.
 */
void
exahype2::dg::rk::tests::StagesTest::testTheSourceTermIsScaledByTheTimeStep() {
  const Tables                                tables;
  const auto                                  cellSize = getCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[StateValuesPerCell];
  for (int entry = 0; entry < StateValuesPerCell; entry++) {
    cellState[entry] = 0.5 + 0.25 * entry;
  }
  double estimate[UnknownsPerStage];
  for (int entry = 0; entry < UnknownsPerStage; entry++) {
    estimate[entry] = 9.0;
  }

  StageOf<SourceSolver>::volumeIntegral(
    cellState,
    estimate,
    tables.quadraturePoints.data(),
    tables.derivativeOperator.data(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const double expected = TimeStepSize * (1.0 + unknown) * cellState[cellNode * NumberOfData + unknown];
      validateNumericalEqualsWithParams2(estimate[cellNode * NumberOfUnknowns + unknown], expected, cellNode, unknown);
    }
  }
}

/**
 * The product of a state that is linear along the last axis reads the exact physical gradient of that axis.
 */
void
exahype2::dg::rk::tests::StagesTest::testTheNonconservativeProductSeesTheNodalGradient() {
  const Tables                                tables;
  const auto                                  cellSize = getCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[StateValuesPerCell];
  fillLinearState(tables, cellState, DIMENSIONS - 1, Slope);
  double estimate[UnknownsPerStage] = {};

  StageOf<ProductSolver>::volumeIntegral(
    cellState,
    estimate,
    tables.quadraturePoints.data(),
    tables.derivativeOperator.data(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  // The stub returns the gradient, which is the physical slope along the last axis and zero
  // along the others, and the kernel subtracts it times the time step.
  const double expected = -TimeStepSize * Slope / cellSize(DIMENSIONS - 1);
  for (int entry = 0; entry < UnknownsPerStage; entry++) {
    validateNumericalEqualsWithParams1(estimate[entry], expected, entry);
  }
}

/**
 * A diffusive flux reads the gradient of every axis at the node.
 */
void
exahype2::dg::rk::tests::StagesTest::testTheDiffusiveFluxSeesTheNodalGradients() {
  const Tables                                tables;
  const auto                                  cellSize = getCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[StateValuesPerCell];
  fillLinearState(tables, cellState, DIMENSIONS - 1, Slope);
  double estimate[UnknownsPerStage] = {};

  StageOf<DiffusiveSolver>::volumeIntegral(
    cellState,
    estimate,
    tables.quadraturePoints.data(),
    tables.derivativeOperator.data(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  // Only the gradient along the last axis is non-zero, so the stub returns the same constant
  // flux along every axis, whose divergence is zero.
  for (int entry = 0; entry < UnknownsPerStage; entry++) {
    validateNumericalEqualsWithParams1(estimate[entry], 0.0, entry);
  }
}

/**
 * Two cells whose states mirror each other across the first diagonal receive mirrored estimates bitwise.
 */
void
exahype2::dg::rk::tests::StagesTest::testMirroredCellsReceiveMirroredEstimates() {
  const Tables                                tables;
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[NodesPerCell * AcousticUnknowns];
  double mirroredCellState[NodesPerCell * AcousticUnknowns];
  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < AcousticUnknowns; unknown++) {
      const double value                               = std::sin(1.0 + cellNode) * std::cos(1.0 + unknown);
      cellState[cellNode * AcousticUnknowns + unknown] = value;
      mirroredCellState[getMirroredNode(cellNode) * AcousticUnknowns + getMirroredUnknown(unknown)] = value;
    }
  }

  double estimate[NodesPerCell * AcousticUnknowns];
  double mirroredEstimate[NodesPerCell * AcousticUnknowns];
  StageOf<AcousticSolver>::volumeIntegral(
    cellState,
    estimate,
    tables.quadraturePoints.data(),
    tables.derivativeOperator.data(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );
  StageOf<AcousticSolver>::volumeIntegral(
    mirroredCellState,
    mirroredEstimate,
    tables.quadraturePoints.data(),
    tables.derivativeOperator.data(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < AcousticUnknowns; unknown++) {
      validateEqualsWithParams2(
        mirroredEstimate[getMirroredNode(cellNode) * AcousticUnknowns + getMirroredUnknown(unknown)],
        estimate[cellNode * AcousticUnknowns + unknown],
        cellNode,
        unknown
      );
    }
  }
}

/**
 * A state that is linear along the first axis extrapolates to its values at the two faces of that axis.
 */
void
exahype2::dg::rk::tests::StagesTest::testTheFaceStatesOfALinearStateAreItsValuesAtTheFaces() {
  const Tables                                tables;
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);

  double stageState[StateValuesPerCell];
  fillLinearState(tables, stageState, 0, Slope);
  FaceHalves halves;

  StageOf<FluxSolver>::projectOntoFaces(
    stageState,
    halves.statePointers,
    halves.fluxPointers,
    tables.leftFaceCoefficients.data(),
    tables.rightFaceCoefficients.data(),
    tables.quadraturePoints.data(),
    tables.derivativeOperator.data(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      validateNumericalEqualsWithParams2(
        halves.states[0][faceNode * NumberOfData + value],
        1.0 + value,
        faceNode,
        value
      );
      validateNumericalEqualsWithParams2(
        halves.states[DIMENSIONS][faceNode * NumberOfData + value],
        1.0 + value + Slope,
        faceNode,
        value
      );
    }
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams2(
        halves.fluxes[0][faceNode * NumberOfUnknowns + unknown],
        1.0 + unknown,
        faceNode,
        unknown
      );
      validateNumericalEqualsWithParams2(
        halves.fluxes[DIMENSIONS][faceNode * NumberOfUnknowns + unknown],
        1.0 + unknown + Slope,
        faceNode,
        unknown
      );
    }
  }
}

/**
 * A constant state opens a stage with a zero estimate and writes itself and its flux onto every face.
 */
void
exahype2::dg::rk::tests::StagesTest::testAConstantStateOpensAStageWithAZeroEstimateAndConstantFaceStates() {
  const Tables                                tables;
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);

  double cellState[StateValuesPerCell];
  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      cellState[cellNode * NumberOfData + value] = 4.0 + value;
    }
  }
  double                             estimates[EstimateValuesPerCell] = {};
  const StageOf<FluxSolver>::Weights weights{0.5, 0.0, 0.0};
  FaceHalves                         halves;
  double                             stageState[StateValuesPerCell];

  StageOf<FluxSolver>::open(
    weights,
    cellState,
    estimates,
    stageState,
    OpenedStage,
    halves.statePointers,
    halves.fluxPointers,
    tables.quadraturePoints.data(),
    tables.derivativeOperator.data(),
    tables.leftFaceCoefficients.data(),
    tables.rightFaceCoefficients.data(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  for (int entry = 0; entry < UnknownsPerStage; entry++) {
    validateEqualsWithParams1(estimates[OpenedStage * UnknownsPerStage + entry], 0.0, entry);
  }
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        validateNumericalEqualsWithParams3(
          halves.states[face][faceNode * NumberOfData + value],
          4.0 + value,
          face,
          faceNode,
          value
        );
      }
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const double expected = (1.0 + face % DIMENSIONS) * (4.0 + unknown);
        validateNumericalEqualsWithParams3(
          halves.fluxes[face][faceNode * NumberOfUnknowns + unknown],
          expected,
          face,
          faceNode,
          unknown
        );
      }
    }
  }
}

/**
 * The face integrals change the estimate of the given stage like exahype2::dg::addFaceIntegrals()
 * and leave the other stages unchanged.
 */
void
exahype2::dg::rk::tests::StagesTest::testTheFaceIntegralsReachTheEstimateOfTheStageAlone() {
  const Tables tables;
  const auto   cellSize    = getCellSize();
  const auto   faceWeights = getFaceQuadratureWeights(tables);

  double estimates[EstimateValuesPerCell];
  double expected[EstimateValuesPerCell];
  for (int entry = 0; entry < EstimateValuesPerCell; entry++) {
    estimates[entry] = 1.0 + 0.25 * entry;
    expected[entry]  = estimates[entry];
  }
  FaceHalves halves;
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int entry = 0; entry < FluxValuesPerFace; entry++) {
      halves.fluxes[face][entry] = 2.0 + face + 0.5 * entry;
    }
  }

  StageOf<InertSolver>::addFaceIntegrals(
    estimates,
    OpenedStage,
    halves.fluxPointers,
    faceWeights.data(),
    tables.inverseCellQuadratureWeights.data(),
    tables.leftFaceCoefficients.data(),
    tables.rightFaceCoefficients.data(),
    cellSize,
    TimeStepSize
  );
  exahype2::dg::addFaceIntegrals<InertSolver, NumberOfUnknowns>(
    expected + static_cast<std::ptrdiff_t>(OpenedStage) * UnknownsPerStage,
    halves.fluxPointers,
    faceWeights.data(),
    tables.inverseCellQuadratureWeights.data(),
    tables.leftFaceCoefficients.data(),
    tables.rightFaceCoefficients.data(),
    cellSize,
    TimeStepSize
  );

  bool stageChanged = false;
  for (int entry = 0; entry < EstimateValuesPerCell; entry++) {
    validateEqualsWithParams1(estimates[entry], expected[entry], entry);
    stageChanged = stageChanged or estimates[entry] != 1.0 + 0.25 * entry;
  }
  validate(stageChanged);
}

/**
 * close() adds the weighted estimates to the cell state in place and reports the wave speeds of the
 * result.
 */
void
exahype2::dg::rk::tests::StagesTest::testTheCloseWritesTheWeightedSumInPlaceAndReportsTheWaveSpeeds() {
  const Tables                                tables;
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(UniformCellExtent);
  const auto                                  weights = StageOf<WaveSpeedSolver>::Tableau::Weights;

  double cellState[StateValuesPerCell];
  double silentCellState[StateValuesPerCell];
  double expected[StateValuesPerCell];
  for (int entry = 0; entry < StateValuesPerCell; entry++) {
    cellState[entry]       = 2.0 + 0.5 * entry;
    silentCellState[entry] = cellState[entry];
  }
  double estimates[EstimateValuesPerCell];
  for (int entry = 0; entry < EstimateValuesPerCell; entry++) {
    estimates[entry] = 1.0 + 0.125 * entry;
  }
  StageOf<WaveSpeedSolver>::combineEstimates(weights, cellState, estimates, expected);

  const auto statistics = StageOf<WaveSpeedSolver>::close<true>(
    weights,
    cellState,
    estimates,
    tables.quadraturePoints.data(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );
  const auto silentStatistics = StageOf<WaveSpeedSolver>::close<false>(
    weights,
    silentCellState,
    estimates,
    tables.quadraturePoints.data(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  for (int entry = 0; entry < StateValuesPerCell; entry++) {
    validateEqualsWithParams1(cellState[entry], expected[entry], entry);
    validateEqualsWithParams1(silentCellState[entry], expected[entry], entry);
  }
  validateNumericalEquals(statistics.maxPhysicalEigenvalue, WaveSpeed);
  validateNumericalEquals(statistics.maxScaledEigenvalue, DIMENSIONS * WaveSpeed / UniformCellExtent);
  validateNumericalEquals(silentStatistics.maxPhysicalEigenvalue, 0.0);
  validateNumericalEquals(silentStatistics.maxScaledEigenvalue, 0.0);
}

/**
 * Every tableau is explicit, its nodes are the row sums of its matrix and its weights sum to one.
 */
void
exahype2::dg::rk::tests::StagesTest::testTheButcherTableausAreConsistent() {
  const auto check = [this](const auto& tableau) {
    using Tableau    = std::remove_cvref_t<decltype(tableau)>;
    double weightSum = 0.0;
    for (int stage = 0; stage < Tableau::Stages; stage++) {
      weightSum += Tableau::Weights[static_cast<std::size_t>(stage)];
      double rowSum = 0.0;
      for (int earlier = 0; earlier < Tableau::Stages; earlier++) {
        const double entry = Tableau::Matrix[static_cast<std::size_t>(stage)][static_cast<std::size_t>(earlier)];
        rowSum += entry;
        if (earlier >= stage) {
          validateEqualsWithParams2(entry, 0.0, stage, earlier);
        }
      }
      validateNumericalEqualsWithParams1(rowSum, Tableau::Nodes[static_cast<std::size_t>(stage)], stage);
    }
    validateNumericalEquals(weightSum, 1.0);
  };
  check(exahype2::dg::rk::ButcherTableau<1>{});
  check(exahype2::dg::rk::ButcherTableau<2>{});
  check(exahype2::dg::rk::ButcherTableau<3>{});
  check(exahype2::dg::rk::ButcherTableau<4>{});
}

/**
 * The opened stage of every cell of a device range reproduces the serial host loop.
 */
void
exahype2::dg::rk::tests::StagesTest::testEveryCellOfADeviceRangeOpensAStageLikeTheHost() {
  validateEveryCellOpensLikeTheHost<FullSolver>();
}

/**
 * The opened stage of a diffusive system of every cell of a device range reproduces the serial host loop.
 */
void
exahype2::dg::rk::tests::StagesTest::testEveryCellOfADeviceRangeOpensADiffusiveStageLikeTheHost() {
  validateEveryCellOpensLikeTheHost<DiffusiveFullSolver>();
}

/**
 * Checks that opening a stage of Derived on the device reproduces the serial host loop.
 */
template <typename Derived>
void
exahype2::dg::rk::tests::StagesTest::validateEveryCellOpensLikeTheHost() {
  const Tables                         tables;
  const exahype2::dg::tests::CellChain geometry;
  const auto                weights = StageOf<Derived>::Tableau::Matrix[static_cast<std::size_t>(OpenedStage)];
  const std::vector<double> cellStates =
    exahype2::dg::tests::getRamp(static_cast<std::size_t>(Setup::NumberOfCells) * StateValuesPerCell, 0.8, 0.021);
  const std::vector<double> estimates =
    exahype2::dg::tests::getRamp(static_cast<std::size_t>(Setup::NumberOfCells) * EstimateValuesPerCell, -0.5, 0.017);
  const std::vector<double> faceStates(
    static_cast<std::size_t>(Setup::NumberOfCells) * peano4::FacesPerCell * StateValuesPerFace,
    0.0
  );
  const std::vector<double> faceFluxes(
    static_cast<std::size_t>(Setup::NumberOfCells) * peano4::FacesPerCell * FluxValuesPerFace,
    0.0
  );

  std::vector<double> deviceEstimates  = estimates;
  std::vector<double> deviceFaceStates = faceStates;
  std::vector<double> deviceFaceFluxes = faceFluxes;
  openOnDevice<Derived>(tables, geometry, weights, cellStates, deviceEstimates, deviceFaceStates, deviceFaceFluxes);

  std::vector<double> hostEstimates  = estimates;
  std::vector<double> hostFaceStates = faceStates;
  std::vector<double> hostFaceFluxes = faceFluxes;
  std::vector<double> stageState(StateValuesPerCell, 0.0);
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    double* faceState[peano4::FacesPerCell];
    double* faceFlux[peano4::FacesPerCell];
    collectFacePointers(cell, hostFaceStates.data(), hostFaceFluxes.data(), faceState, faceFlux);
    const int  coordinateOffset = cell * DIMENSIONS;
    const auto cellOffset       = static_cast<std::ptrdiff_t>(cell);
    StageOf<Derived>::open(
      weights,
      cellStates.data() + cellOffset * StateValuesPerCell,
      hostEstimates.data() + cellOffset * EstimateValuesPerCell,
      stageState.data(),
      OpenedStage,
      faceState,
      faceFlux,
      tables.quadraturePoints.data(),
      tables.derivativeOperator.data(),
      tables.leftFaceCoefficients.data(),
      tables.rightFaceCoefficients.data(),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellCentres.data() + coordinateOffset),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
      TimeStamp,
      TimeStepSize
    );
  }

  validateNumericalBuffersEquals(deviceEstimates, hostEstimates);
  validateNumericalBuffersEquals(deviceFaceStates, hostFaceStates);
  validateNumericalBuffersEquals(deviceFaceFluxes, hostFaceFluxes);
}

/**
 * The closed step of every cell of a device range reproduces the serial host loop.
 */
void
exahype2::dg::rk::tests::StagesTest::testEveryCellOfADeviceRangeClosesTheStepLikeTheHost() {
  const exahype2::dg::tests::CellChain geometry;
  const auto                           weights = StageOf<FullSolver>::Tableau::Weights;
  const std::vector<double>            cellStates =
    exahype2::dg::tests::getRamp(static_cast<std::size_t>(Setup::NumberOfCells) * StateValuesPerCell, 1.0, 0.01);
  const std::vector<double> estimates =
    exahype2::dg::tests::getRamp(static_cast<std::size_t>(Setup::NumberOfCells) * EstimateValuesPerCell, -0.5, 0.017);

  std::vector<double> deviceCellStates = cellStates;
  closeOnDevice(geometry, weights, deviceCellStates, estimates);

  std::vector<double> hostCellStates = cellStates;
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    const int  coordinateOffset = cell * DIMENSIONS;
    const auto cellOffset       = static_cast<std::ptrdiff_t>(cell);
    static_cast<void>(StageOf<FullSolver>::close<false>(
      weights,
      hostCellStates.data() + cellOffset * StateValuesPerCell,
      estimates.data() + cellOffset * EstimateValuesPerCell,
      static_cast<const double*>(nullptr),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellCentres.data() + coordinateOffset),
      tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
      TimeStamp,
      TimeStepSize
    ));
  }

  validateNumericalBuffersEquals(deviceCellStates, hostCellStates);
}
