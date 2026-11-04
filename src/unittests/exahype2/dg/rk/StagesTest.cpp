// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/dg/rk/Stages.h"

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/Kernels.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/exahype2/dg/KernelTestData.h"

namespace {
  using Setup  = exahype2::dg::unittests::KernelTestSetup;
  using Tables = exahype2::dg::unittests::BasisTableSet;

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
   * A solver whose acoustic flux along an axis couples the pressure with the velocity of that axis.
   * A swap of two axes maps the flux onto itself.
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

  constexpr int AcousticUnknowns              = AcousticSolver::NumberOfUnknowns;
  constexpr int AcousticUnknownsPerStage      = StageOf<AcousticSolver>::UnknownsPerStage;
  constexpr int AcousticEstimateValuesPerCell = StageOf<AcousticSolver>::EstimateValuesPerCell;

  /**
   * The stage state of these weights equals the cell state bitwise.
   */
  constexpr std::array<double, NumberOfStages> ZeroWeights{};

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

  /**
   * Returns the cell state plus the weighted estimates of every stage, with the auxiliary variables of the cell state.
   */
  std::array<double, StateValuesPerCell> getWeightedSum(
    const std::array<double, NumberOfStages>& weights,
    const double* const                       cellState,
    const double* const                       estimates
  ) {
    ASSERTION(cellState != nullptr);
    ASSERTION(estimates != nullptr);
    std::array<double, StateValuesPerCell> weightedSum{};
    for (int entry = 0; entry < StateValuesPerCell; entry++) {
      weightedSum[static_cast<std::size_t>(entry)] = cellState[entry];
    }
    for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        for (int stage = 0; stage < NumberOfStages; stage++) {
          weightedSum[static_cast<std::size_t>(cellNode * NumberOfData + unknown)] +=
            weights[static_cast<std::size_t>(stage)]
            * estimates[stage * UnknownsPerStage + cellNode * NumberOfUnknowns + unknown];
        }
      }
    }
    return weightedSum;
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

  template <typename Solver>
  struct FaceHalvesOf final {
    using SolverLayout = exahype2::dg::PaddedLayout<Solver>;

    double  states[peano4::FacesPerCell][SolverLayout::NodesPerFace * SolverLayout::NumberOfData]{};
    double  fluxes[peano4::FacesPerCell][SolverLayout::NodesPerFace * SolverLayout::NumberOfUnknowns]{};
    double* statePointers[peano4::FacesPerCell]{};
    double* fluxPointers[peano4::FacesPerCell]{};

    FaceHalvesOf() {
      for (int face = 0; face < peano4::FacesPerCell; face++) {
        statePointers[face] = states[face];
        fluxPointers[face]  = fluxes[face];
      }
    }
    FaceHalvesOf(const FaceHalvesOf& other)            = delete;
    FaceHalvesOf(FaceHalvesOf&& other)                 = delete;
    FaceHalvesOf& operator=(const FaceHalvesOf& other) = delete;
    FaceHalvesOf& operator=(FaceHalvesOf&& other)      = delete;
    ~FaceHalvesOf()                                    = default;
  };

  /**
   * Opens the stage OpenedStage of one cell of Solver at TimeStamp with the operators of tables and writes its face
   * states and face fluxes into halves.
   */
  template <typename Solver>
  void openStageOfCell(
    const Tables&                                tables,
    const typename StageOf<Solver>::Weights&     weights,
    const double* const                          cellState,
    double* const                                estimates,
    double* const                                stageState,
    FaceHalvesOf<Solver>&                        halves,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  ) {
    StageOf<Solver>::open(
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
  }

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
    const Tables&                             tables,
    const exahype2::dg::unittests::CellChain& geometry,
    const typename StageOf<Solver>::Weights&  weights,
    const std::vector<double>&                cellStates,
    std::vector<double>&                      estimates,
    std::vector<double>&                      faceStates,
    std::vector<double>&                      faceFluxes
  ) {
    const exahype2::dg::unittests::DeviceBasisTables deviceTables(tables);
    const std::vector<double> stageStates(static_cast<std::size_t>(Setup::NumberOfCells) * StateValuesPerCell, 0.0);

    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellStates(cellStates);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceEstimates(estimates);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceStageStates(stageStates);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceFaceStates(faceStates);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceFaceFluxes(faceFluxes);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);

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

  template <typename Solver>
  void openOnHost(
    const Tables&                             tables,
    const exahype2::dg::unittests::CellChain& geometry,
    const typename StageOf<Solver>::Weights&  weights,
    const std::vector<double>&                cellStates,
    std::vector<double>&                      estimates,
    std::vector<double>&                      faceStates,
    std::vector<double>&                      faceFluxes
  ) {
    ASSERTION(
      cellStates.size() == static_cast<std::size_t>(Setup::NumberOfCells) * StateValuesPerCell,
      cellStates.size()
    );
    ASSERTION(
      estimates.size() == static_cast<std::size_t>(Setup::NumberOfCells) * EstimateValuesPerCell,
      estimates.size()
    );
    ASSERTION(
      faceStates.size() == static_cast<std::size_t>(Setup::NumberOfCells) * peano4::FacesPerCell * StateValuesPerFace,
      faceStates.size()
    );
    ASSERTION(
      faceFluxes.size() == static_cast<std::size_t>(Setup::NumberOfCells) * peano4::FacesPerCell * FluxValuesPerFace,
      faceFluxes.size()
    );
    std::vector<double> stageState(StateValuesPerCell, 0.0);
    for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
      double* faceState[peano4::FacesPerCell];
      double* faceFlux[peano4::FacesPerCell];
      collectFacePointers(cell, faceStates.data(), faceFluxes.data(), faceState, faceFlux);
      const int  coordinateOffset = cell * DIMENSIONS;
      const auto cellOffset       = static_cast<std::ptrdiff_t>(cell);
      StageOf<Solver>::open(
        weights,
        cellStates.data() + cellOffset * StateValuesPerCell,
        estimates.data() + cellOffset * EstimateValuesPerCell,
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
  }

  void closeOnDevice(
    const exahype2::dg::unittests::CellChain& geometry,
    const StageOf<FullSolver>::Weights&       weights,
    std::vector<double>&                      cellStates,
    const std::vector<double>&                estimates
  ) {
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellStates(cellStates);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceEstimates(estimates);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);

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

  void closeOnHost(
    const exahype2::dg::unittests::CellChain& geometry,
    const StageOf<FullSolver>::Weights&       weights,
    std::vector<double>&                      cellStates,
    const std::vector<double>&                estimates
  ) {
    ASSERTION(
      cellStates.size() == static_cast<std::size_t>(Setup::NumberOfCells) * StateValuesPerCell,
      cellStates.size()
    );
    ASSERTION(
      estimates.size() == static_cast<std::size_t>(Setup::NumberOfCells) * EstimateValuesPerCell,
      estimates.size()
    );
    for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
      const int  coordinateOffset = cell * DIMENSIONS;
      const auto cellOffset       = static_cast<std::ptrdiff_t>(cell);
      static_cast<void>(StageOf<FullSolver>::close<false>(
        weights,
        cellStates.data() + cellOffset * StateValuesPerCell,
        estimates.data() + cellOffset * EstimateValuesPerCell,
        static_cast<const double*>(nullptr),
        tarch::la::Vector<DIMENSIONS, double>(geometry.cellCentres.data() + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
        TimeStamp,
        TimeStepSize
      ));
    }
  }
} // namespace

UNIT_TEST("The stage state adds every weighted estimate to the unknowns and copies the auxiliary variables") {
  GIVEN("A cell state and estimates that rise with their entry, the weights one half, minus one and two and the sum") {
    const Tables                                tables;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    const auto                                  cellSize = getCellSize();
    double                                      cellState[StateValuesPerCell];
    for (int entry = 0; entry < StateValuesPerCell; entry++) {
      cellState[entry] = 1.0 + 0.5 * entry;
    }
    double estimates[EstimateValuesPerCell];
    for (int entry = 0; entry < EstimateValuesPerCell; entry++) {
      estimates[entry] = 0.25 * entry;
    }
    const StageOf<InertSolver>::Weights weights{0.5, -1.0, 2.0};
    const auto                          weightedSum = getWeightedSum(weights, cellState, estimates);

    WHEN("A stage of the solver without terms opens") {
      double                    stageState[StateValuesPerCell];
      FaceHalvesOf<InertSolver> halves;
      openStageOfCell<InertSolver>(tables, weights, cellState, estimates, stageState, halves, cellCentre, cellSize);

      THEN("The unknowns of the stage state store the sum and its auxiliary variables those of the cell state") {
        for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            const int entry = cellNode * NumberOfData + unknown;
            VALIDATE_NUMERICAL_EQUALS(
              stageState[entry],
              weightedSum[static_cast<std::size_t>(entry)],
              cellNode,
              unknown
            );
          }
          VALIDATE_EQUALS(
            stageState[cellNode * NumberOfData + NumberOfUnknowns],
            cellState[cellNode * NumberOfData + NumberOfUnknowns],
            cellNode
          );
        }
      }
    }
  }
}

UNIT_TEST("A stage with zero weights reproduces the cell state bitwise from estimates of NaN") {
  GIVEN("A cell state that falls with its entry and estimates filled with NaN") {
    const Tables                                tables;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    const auto                                  cellSize = getCellSize();
    double                                      cellState[StateValuesPerCell];
    for (int entry = 0; entry < StateValuesPerCell; entry++) {
      cellState[entry] = 3.0 - 0.125 * entry;
    }
    double estimates[EstimateValuesPerCell];
    for (int entry = 0; entry < EstimateValuesPerCell; entry++) {
      estimates[entry] = std::numeric_limits<double>::quiet_NaN();
    }

    WHEN("A stage of the solver without terms opens with zero weights") {
      double                    stageState[StateValuesPerCell];
      FaceHalvesOf<InertSolver> halves;
      openStageOfCell<InertSolver>(tables, ZeroWeights, cellState, estimates, stageState, halves, cellCentre, cellSize);

      THEN("The stage state equals the cell state bitwise") {
        for (int entry = 0; entry < StateValuesPerCell; entry++) {
          VALIDATE_EQUALS(stageState[entry], cellState[entry], entry);
        }
      }
    }
  }
}

UNIT_TEST("The weighted sum written over the cell state equals the weighted sum written into the stage state") {
  GIVEN("A cell state, a copy of it and estimates that rise with their entry and the weights of Kutta's scheme") {
    const Tables                                tables;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    const auto                                  cellSize = getCellSize();
    double                                      cellState[StateValuesPerCell];
    double                                      closedState[StateValuesPerCell];
    for (int entry = 0; entry < StateValuesPerCell; entry++) {
      cellState[entry]   = 2.0 + 0.5 * entry;
      closedState[entry] = cellState[entry];
    }
    double estimates[EstimateValuesPerCell];
    for (int entry = 0; entry < EstimateValuesPerCell; entry++) {
      estimates[entry] = 1.0 + entry;
    }
    const StageOf<InertSolver>::Weights weights{1.0 / 6.0, 2.0 / 3.0, 1.0 / 6.0};

    WHEN("The step closes over the copy and a stage opens from the cell state") {
      static_cast<void>(StageOf<InertSolver>::close<false>(
        weights,
        closedState,
        estimates,
        static_cast<const double*>(nullptr),
        cellCentre,
        cellSize,
        TimeStamp,
        TimeStepSize
      ));
      double                    stageState[StateValuesPerCell];
      FaceHalvesOf<InertSolver> halves;
      openStageOfCell<InertSolver>(tables, weights, cellState, estimates, stageState, halves, cellCentre, cellSize);

      THEN("Both weighted sums agree") { VALIDATE_NUMERICAL_BUFFERS_EQUALS(closedState, stageState); }
    }
  }
}

UNIT_TEST("The open and the close add every weighted estimate to the cell state with one rounding") {
  GIVEN("A cell state, a copy of it, rising estimates, the weights of Kutta's scheme and their fused sums") {
    const Tables                                tables;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    const auto                                  cellSize = getCellSize();
    double                                      cellState[StateValuesPerCell];
    double                                      closedState[StateValuesPerCell];
    for (int entry = 0; entry < StateValuesPerCell; entry++) {
      cellState[entry]   = 2.0 + 0.5 * entry;
      closedState[entry] = cellState[entry];
    }
    double estimates[EstimateValuesPerCell];
    for (int entry = 0; entry < EstimateValuesPerCell; entry++) {
      estimates[entry] = 1.0 + entry;
    }
    const StageOf<InertSolver>::Weights    weights{1.0 / 6.0, 2.0 / 3.0, 1.0 / 6.0};
    std::array<double, StateValuesPerCell> fusedSum{};
    for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const int entry = cellNode * NumberOfData + unknown;
        double    sum   = cellState[entry];
        for (int stage = 0; stage < NumberOfStages; stage++) {
          sum = std::fma(
            weights[static_cast<std::size_t>(stage)],
            estimates[stage * UnknownsPerStage + cellNode * NumberOfUnknowns + unknown],
            sum
          );
        }
        fusedSum[static_cast<std::size_t>(entry)] = sum;
      }
    }

    WHEN("The step closes over the copy and a stage opens from the cell state") {
      static_cast<void>(StageOf<InertSolver>::close<false>(
        weights,
        closedState,
        estimates,
        static_cast<const double*>(nullptr),
        cellCentre,
        cellSize,
        TimeStamp,
        TimeStepSize
      ));
      double                    stageState[StateValuesPerCell];
      FaceHalvesOf<InertSolver> halves;
      openStageOfCell<InertSolver>(tables, weights, cellState, estimates, stageState, halves, cellCentre, cellSize);

      THEN("Every unknown of the closed state and of the stage state equals its fused sum bitwise") {
        for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            const auto entry = static_cast<std::size_t>(cellNode * NumberOfData + unknown);
            VALIDATE_EQUALS(closedState[entry], fusedSum[entry], cellNode, unknown);
            VALIDATE_EQUALS(stageState[entry], fusedSum[entry], cellNode, unknown);
          }
        }
      }
    }
  }
}

UNIT_TEST("A constant state gives a zero flux divergence bitwise") {
  GIVEN("A cell state that is constant over the cell and estimates of nine") {
    const Tables                                tables;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    const auto                                  cellSize = getCellSize();
    double                                      cellState[StateValuesPerCell];
    for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = 2.0 + value;
      }
    }
    double estimates[EstimateValuesPerCell];
    for (int entry = 0; entry < EstimateValuesPerCell; entry++) {
      estimates[entry] = 9.0;
    }

    WHEN("A stage of the solver with a flux opens with zero weights") {
      double                   stageState[StateValuesPerCell];
      FaceHalvesOf<FluxSolver> halves;
      openStageOfCell<FluxSolver>(tables, ZeroWeights, cellState, estimates, stageState, halves, cellCentre, cellSize);
      const double* const estimate = estimates + OpenedStage * UnknownsPerStage;

      THEN("Every value of the estimate of the opened stage is zero bitwise") {
        for (int entry = 0; entry < UnknownsPerStage; entry++) {
          VALIDATE_EQUALS(estimate[entry], 0.0, entry);
        }
      }
    }
  }
}

UNIT_TEST("A flux that is linear along the first axis subtracts its physical slope times the time step at every node") {
  GIVEN("A cell state that is linear along the first axis with the slope three and zero estimates") {
    const Tables                                tables;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    const auto                                  cellSize = getCellSize();
    double                                      cellState[StateValuesPerCell];
    fillLinearState(tables, cellState, 0, Slope);
    double estimates[EstimateValuesPerCell] = {};

    WHEN("A stage of the solver with a flux opens with zero weights") {
      double                   stageState[StateValuesPerCell];
      FaceHalvesOf<FluxSolver> halves;
      openStageOfCell<FluxSolver>(tables, ZeroWeights, cellState, estimates, stageState, halves, cellCentre, cellSize);
      const double* const estimate = estimates + OpenedStage * UnknownsPerStage;

      THEN("Every value of the estimate is minus the time step times the slope over the extent of the first axis") {
        for (int entry = 0; entry < UnknownsPerStage; entry++) {
          VALIDATE_NUMERICAL_EQUALS(estimate[entry], -TimeStepSize * Slope / cellSize(0), entry);
        }
      }
    }
  }
}

UNIT_TEST("The source of every node enters the estimate times the time step") {
  GIVEN("A cell state that rises with its entry and estimates of nine") {
    const Tables                                tables;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    const auto                                  cellSize = getCellSize();
    double                                      cellState[StateValuesPerCell];
    for (int entry = 0; entry < StateValuesPerCell; entry++) {
      cellState[entry] = 0.5 + 0.25 * entry;
    }
    double estimates[EstimateValuesPerCell];
    for (int entry = 0; entry < EstimateValuesPerCell; entry++) {
      estimates[entry] = 9.0;
    }

    WHEN("A stage of the solver with a source opens with zero weights") {
      double                     stageState[StateValuesPerCell];
      FaceHalvesOf<SourceSolver> halves;
      openStageOfCell<SourceSolver>(
        tables,
        ZeroWeights,
        cellState,
        estimates,
        stageState,
        halves,
        cellCentre,
        cellSize
      );
      const double* const estimate = estimates + OpenedStage * UnknownsPerStage;

      THEN("The estimate of every unknown is the time step times the source of its node") {
        for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            VALIDATE_NUMERICAL_EQUALS(
              estimate[cellNode * NumberOfUnknowns + unknown],
              TimeStepSize * (1.0 + unknown) * cellState[cellNode * NumberOfData + unknown],
              cellNode,
              unknown
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("The product of a state that is linear along the last axis reads the exact physical gradient of that axis") {
  GIVEN("A cell state that is linear along the last axis with the slope three and zero estimates") {
    const Tables                                tables;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    const auto                                  cellSize = getCellSize();
    double                                      cellState[StateValuesPerCell];
    fillLinearState(tables, cellState, DIMENSIONS - 1, Slope);
    double estimates[EstimateValuesPerCell] = {};

    WHEN("A stage of the solver whose product returns the gradient opens with zero weights") {
      double                      stageState[StateValuesPerCell];
      FaceHalvesOf<ProductSolver> halves;
      openStageOfCell<ProductSolver>(
        tables,
        ZeroWeights,
        cellState,
        estimates,
        stageState,
        halves,
        cellCentre,
        cellSize
      );
      const double* const estimate = estimates + OpenedStage * UnknownsPerStage;

      THEN("Every value of the estimate is minus the time step times the slope over the extent of the last axis") {
        for (int entry = 0; entry < UnknownsPerStage; entry++) {
          VALIDATE_NUMERICAL_EQUALS(estimate[entry], -TimeStepSize * Slope / cellSize(DIMENSIONS - 1), entry);
        }
      }
    }
  }
}

UNIT_TEST("A diffusive flux reads the gradient of every axis at the node") {
  GIVEN("A cell state that is linear along the last axis with the slope three and zero estimates") {
    const Tables                                tables;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    const auto                                  cellSize = getCellSize();
    double                                      cellState[StateValuesPerCell];
    fillLinearState(tables, cellState, DIMENSIONS - 1, Slope);
    double estimates[EstimateValuesPerCell] = {};

    WHEN("A stage of the solver whose diffusive flux sums the weighted gradients opens with zero weights") {
      double                        stageState[StateValuesPerCell];
      FaceHalvesOf<DiffusiveSolver> halves;
      openStageOfCell<DiffusiveSolver>(
        tables,
        ZeroWeights,
        cellState,
        estimates,
        stageState,
        halves,
        cellCentre,
        cellSize
      );
      const double* const estimate = estimates + OpenedStage * UnknownsPerStage;

      THEN("The diffusive flux is constant along every axis and the estimate is zero") {
        for (int entry = 0; entry < UnknownsPerStage; entry++) {
          VALIDATE_NUMERICAL_EQUALS(estimate[entry], 0.0, entry);
        }
      }
    }
  }
}

UNIT_TEST("Two cells whose states mirror each other across the first diagonal receive mirrored estimates bitwise") {
  GIVEN("A state of the acoustic system and its mirror image across the first diagonal") {
    const Tables                                tables;
    const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    double                                      cellState[NodesPerCell * AcousticUnknowns];
    double                                      mirroredCellState[NodesPerCell * AcousticUnknowns];
    for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < AcousticUnknowns; unknown++) {
        const double value                               = std::sin(1.0 + cellNode) * std::cos(1.0 + unknown);
        cellState[cellNode * AcousticUnknowns + unknown] = value;
        mirroredCellState[getMirroredNode(cellNode) * AcousticUnknowns + getMirroredUnknown(unknown)] = value;
      }
    }

    WHEN("A stage of the acoustic solver opens with zero weights in both cells") {
      double                       estimates[AcousticEstimateValuesPerCell]{};
      double                       mirroredEstimates[AcousticEstimateValuesPerCell]{};
      double                       stageState[NodesPerCell * AcousticUnknowns];
      double                       mirroredStageState[NodesPerCell * AcousticUnknowns];
      FaceHalvesOf<AcousticSolver> halves;
      FaceHalvesOf<AcousticSolver> mirroredHalves;
      openStageOfCell<AcousticSolver>(
        tables,
        ZeroWeights,
        cellState,
        estimates,
        stageState,
        halves,
        cellCentre,
        cellSize
      );
      openStageOfCell<AcousticSolver>(
        tables,
        ZeroWeights,
        mirroredCellState,
        mirroredEstimates,
        mirroredStageState,
        mirroredHalves,
        cellCentre,
        cellSize
      );
      const double* const estimate         = estimates + OpenedStage * AcousticUnknownsPerStage;
      const double* const mirroredEstimate = mirroredEstimates + OpenedStage * AcousticUnknownsPerStage;

      THEN("Every value of the estimate of the mirror cell equals the mirrored value of the estimate bitwise") {
        for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < AcousticUnknowns; unknown++) {
            VALIDATE_EQUALS(
              mirroredEstimate[getMirroredNode(cellNode) * AcousticUnknowns + getMirroredUnknown(unknown)],
              estimate[cellNode * AcousticUnknowns + unknown],
              cellNode,
              unknown
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("A state that is linear along the first axis extrapolates to its values at the two faces of that axis") {
  GIVEN("A cell state that is linear along the first axis with the slope three and zero estimates") {
    const Tables                                tables;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);
    double                                      cellState[StateValuesPerCell];
    fillLinearState(tables, cellState, 0, Slope);
    double estimates[EstimateValuesPerCell] = {};

    WHEN("A stage of the solver with a flux opens with zero weights") {
      double                   stageState[StateValuesPerCell];
      FaceHalvesOf<FluxSolver> halves;
      openStageOfCell<FluxSolver>(tables, ZeroWeights, cellState, estimates, stageState, halves, cellCentre, cellSize);

      THEN("Each face of that axis stores the state and the flux at its end of the axis") {
        for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
          for (int value = 0; value < NumberOfData; value++) {
            VALIDATE_NUMERICAL_EQUALS(halves.states[0][faceNode * NumberOfData + value], 1.0 + value, faceNode, value);
            VALIDATE_NUMERICAL_EQUALS(
              halves.states[DIMENSIONS][faceNode * NumberOfData + value],
              1.0 + value + Slope,
              faceNode,
              value
            );
          }
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            VALIDATE_NUMERICAL_EQUALS(
              halves.fluxes[0][faceNode * NumberOfUnknowns + unknown],
              1.0 + unknown,
              faceNode,
              unknown
            );
            VALIDATE_NUMERICAL_EQUALS(
              halves.fluxes[DIMENSIONS][faceNode * NumberOfUnknowns + unknown],
              1.0 + unknown + Slope,
              faceNode,
              unknown
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("A constant state opens a stage with a zero estimate and writes itself and its flux onto every face") {
  GIVEN("A constant cell state, zero estimates and the weight one half of the first stage") {
    const Tables                                tables;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);
    double                                      cellState[StateValuesPerCell];
    for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = 4.0 + value;
      }
    }
    double                             estimates[EstimateValuesPerCell] = {};
    const StageOf<FluxSolver>::Weights weights{0.5, 0.0, 0.0};

    WHEN("A stage of the solver with a flux opens") {
      double                   stageState[StateValuesPerCell];
      FaceHalvesOf<FluxSolver> halves;
      openStageOfCell<FluxSolver>(tables, weights, cellState, estimates, stageState, halves, cellCentre, cellSize);
      const double* const estimate = estimates + OpenedStage * UnknownsPerStage;

      THEN("The estimate of the opened stage is zero and every face stores the state and its flux along the normal") {
        for (int entry = 0; entry < UnknownsPerStage; entry++) {
          VALIDATE_EQUALS(estimate[entry], 0.0, entry);
        }
        for (int face = 0; face < peano4::FacesPerCell; face++) {
          for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
            for (int value = 0; value < NumberOfData; value++) {
              VALIDATE_NUMERICAL_EQUALS(
                halves.states[face][faceNode * NumberOfData + value],
                4.0 + value,
                face,
                faceNode,
                value
              );
            }
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              const double expected = (1.0 + face % DIMENSIONS) * (4.0 + unknown);
              VALIDATE_NUMERICAL_EQUALS(
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
    }
  }
}

UNIT_TEST("The face integrals add the Galerkin face integral to the estimate of the given stage only") {
  GIVEN(
    "Estimates that rise with their entry, face fluxes that rise with face and entry and the estimates that the "
    "Galerkin face integral gives"
  ) {
    const Tables tables;
    const auto   cellSize    = getCellSize();
    const auto   faceWeights = getFaceQuadratureWeights(tables);
    double       estimates[EstimateValuesPerCell];
    double       expected[EstimateValuesPerCell];
    for (int entry = 0; entry < EstimateValuesPerCell; entry++) {
      estimates[entry] = 1.0 + 0.25 * entry;
      expected[entry]  = estimates[entry];
    }
    FaceHalvesOf<InertSolver> halves;
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      for (int entry = 0; entry < FluxValuesPerFace; entry++) {
        halves.fluxes[face][entry] = 2.0 + face + 0.5 * entry;
      }
    }
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

    WHEN("The stage adds its face integrals") {
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
      bool stageChanged = false;
      for (int entry = 0; entry < EstimateValuesPerCell; entry++) {
        stageChanged = stageChanged or estimates[entry] != 1.0 + 0.25 * entry;
      }

      THEN("The estimates equal those of the Galerkin face integral and the estimate of the stage changed") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(estimates, expected);
        VALIDATE(stageChanged);
      }
    }
  }
}

UNIT_TEST("The close writes the weighted sum over the cell state and reports the wave speeds of the new state") {
  GIVEN("A cell state, a copy of it, estimates that rise with their entry, the weights of the tableau and the sum") {
    const Tables                                tables;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(UniformCellExtent);
    const auto                                  weights = StageOf<WaveSpeedSolver>::Tableau::Weights;
    double                                      cellState[StateValuesPerCell];
    double                                      silentCellState[StateValuesPerCell];
    for (int entry = 0; entry < StateValuesPerCell; entry++) {
      cellState[entry]       = 2.0 + 0.5 * entry;
      silentCellState[entry] = cellState[entry];
    }
    double estimates[EstimateValuesPerCell];
    for (int entry = 0; entry < EstimateValuesPerCell; entry++) {
      estimates[entry] = 1.0 + 0.125 * entry;
    }
    const auto weightedSum = getWeightedSum(weights, cellState, estimates);

    WHEN("The step closes over the cell state with the report of the wave speeds and over the copy without") {
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

      THEN("Both states store the sum and only the report gives the wave speed plain and per extent") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(cellState, weightedSum);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(silentCellState, weightedSum);
        VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, WaveSpeed);
        VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, DIMENSIONS * WaveSpeed / UniformCellExtent);
        VALIDATE_NUMERICAL_EQUALS(silentStatistics.maxPhysicalEigenvalue, 0.0);
        VALIDATE_NUMERICAL_EQUALS(silentStatistics.maxScaledEigenvalue, 0.0);
      }
    }
  }
}

UNIT_TEST("Every tableau is explicit, its nodes are the row sums of its matrix and its weights sum to one") {
  GIVEN("The Butcher tableaus of the orders one to four") {
    const exahype2::dg::rk::ButcherTableau<1> firstOrder{};
    const exahype2::dg::rk::ButcherTableau<2> secondOrder{};
    const exahype2::dg::rk::ButcherTableau<3> thirdOrder{};
    const exahype2::dg::rk::ButcherTableau<4> fourthOrder{};

    THEN("The matrix of every tableau vanishes on and above its diagonal and fits its nodes and weights") {
      const auto check = [](const auto& tableau) {
        using Tableau    = std::remove_cvref_t<decltype(tableau)>;
        double weightSum = 0.0;
        for (int stage = 0; stage < Tableau::Stages; stage++) {
          weightSum += Tableau::Weights[static_cast<std::size_t>(stage)];
          double rowSum = 0.0;
          for (int earlier = 0; earlier < Tableau::Stages; earlier++) {
            const double entry = Tableau::Matrix[static_cast<std::size_t>(stage)][static_cast<std::size_t>(earlier)];
            rowSum += entry;
            if (earlier >= stage) {
              VALIDATE_EQUALS(entry, 0.0, stage, earlier);
            }
          }
          VALIDATE_NUMERICAL_EQUALS(rowSum, Tableau::Nodes[static_cast<std::size_t>(stage)], stage);
        }
        VALIDATE_NUMERICAL_EQUALS(weightSum, 1.0);
      };
      check(firstOrder);
      check(secondOrder);
      check(thirdOrder);
      check(fourthOrder);
    }
  }
}

UNIT_TEST("The opened stage of every cell of a device range reproduces the serial host loop") {
  GIVEN("The cell chain, cell states and estimates that rise over the chain and the weights of the opened stage") {
    const Tables                             tables;
    const exahype2::dg::unittests::CellChain geometry;
    const auto                weights = StageOf<FullSolver>::Tableau::Matrix[static_cast<std::size_t>(OpenedStage)];
    const std::vector<double> cellStates =
      exahype2::dg::unittests::getRamp(static_cast<std::size_t>(Setup::NumberOfCells) * StateValuesPerCell, 0.8, 0.021);
    const std::vector<double> estimates = exahype2::dg::unittests::getRamp(
      static_cast<std::size_t>(Setup::NumberOfCells) * EstimateValuesPerCell,
      -0.5,
      0.017
    );
    const std::vector<double> faceStates(
      static_cast<std::size_t>(Setup::NumberOfCells) * peano4::FacesPerCell * StateValuesPerFace,
      0.0
    );
    const std::vector<double> faceFluxes(
      static_cast<std::size_t>(Setup::NumberOfCells) * peano4::FacesPerCell * FluxValuesPerFace,
      0.0
    );

    WHEN("Every cell opens the stage of the solver with flux, source and product on the device and on the host") {
      std::vector<double> deviceEstimates  = estimates;
      std::vector<double> deviceFaceStates = faceStates;
      std::vector<double> deviceFaceFluxes = faceFluxes;
      openOnDevice<FullSolver>(
        tables,
        geometry,
        weights,
        cellStates,
        deviceEstimates,
        deviceFaceStates,
        deviceFaceFluxes
      );
      std::vector<double> hostEstimates  = estimates;
      std::vector<double> hostFaceStates = faceStates;
      std::vector<double> hostFaceFluxes = faceFluxes;
      openOnHost<FullSolver>(tables, geometry, weights, cellStates, hostEstimates, hostFaceStates, hostFaceFluxes);

      THEN("Both give the same estimates, face states and face fluxes") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceEstimates, hostEstimates);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceFaceStates, hostFaceStates);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceFaceFluxes, hostFaceFluxes);
      }
    }

    WHEN("Every cell opens the stage of the diffusive solver with source and product on the device and on the host") {
      std::vector<double> deviceEstimates  = estimates;
      std::vector<double> deviceFaceStates = faceStates;
      std::vector<double> deviceFaceFluxes = faceFluxes;
      openOnDevice<DiffusiveFullSolver>(
        tables,
        geometry,
        weights,
        cellStates,
        deviceEstimates,
        deviceFaceStates,
        deviceFaceFluxes
      );
      std::vector<double> hostEstimates  = estimates;
      std::vector<double> hostFaceStates = faceStates;
      std::vector<double> hostFaceFluxes = faceFluxes;
      openOnHost<DiffusiveFullSolver>(
        tables,
        geometry,
        weights,
        cellStates,
        hostEstimates,
        hostFaceStates,
        hostFaceFluxes
      );

      THEN("Both give the same estimates, face states and face fluxes") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceEstimates, hostEstimates);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceFaceStates, hostFaceStates);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceFaceFluxes, hostFaceFluxes);
      }
    }
  }
}

UNIT_TEST("The closed step of every cell of a device range reproduces the serial host loop") {
  GIVEN("The cell chain, cell states and estimates that rise over the chain and the weights of the tableau") {
    const exahype2::dg::unittests::CellChain geometry;
    const auto                               weights = StageOf<FullSolver>::Tableau::Weights;
    const std::vector<double>                cellStates =
      exahype2::dg::unittests::getRamp(static_cast<std::size_t>(Setup::NumberOfCells) * StateValuesPerCell, 1.0, 0.01);
    const std::vector<double> estimates = exahype2::dg::unittests::getRamp(
      static_cast<std::size_t>(Setup::NumberOfCells) * EstimateValuesPerCell,
      -0.5,
      0.017
    );

    WHEN("Every cell closes its step on the device and in a serial host loop") {
      std::vector<double> deviceCellStates = cellStates;
      closeOnDevice(geometry, weights, deviceCellStates, estimates);
      std::vector<double> hostCellStates = cellStates;
      closeOnHost(geometry, weights, hostCellStates, estimates);

      THEN("Both give the same cell states") { VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceCellStates, hostCellStates); }
    }
  }
}
