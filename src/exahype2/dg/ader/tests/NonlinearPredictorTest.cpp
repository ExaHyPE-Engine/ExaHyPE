// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NonlinearPredictorTest.h"

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/ader/NonlinearPredictor.h"
#include "exahype2/dg/ader/PicardOutcome.h"
#include "exahype2/dg/ader/TeamWorkers.h"
#include "exahype2/dg/ader/tests/KernelTestData.h"
#include "exahype2/dg/tests/KernelTestData.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/la/Vector.h"

namespace {
  using Setup      = exahype2::dg::ader::tests::KernelTestSetup;
  using Tables     = exahype2::dg::tests::BasisTableSet;
  using StubLayout = exahype2::dg::ader::tests::StubLayout;
  using CellLayout = Setup::CellLayout;

  template <typename Solver>
  using Predictor = exahype2::dg::ader::NonlinearPredictor<Solver, double>;

  TARGET_DECLARE
  constexpr int NodesPerAxis       = CellLayout::NodesPerAxis;
  constexpr int NumberOfUnknowns   = CellLayout::NumberOfUnknowns;
  constexpr int NumberOfData       = CellLayout::NumberOfData;
  constexpr int OperatorEntries    = NodesPerAxis * NodesPerAxis;
  constexpr int StateStride        = CellLayout::PaddedNumberOfData;
  constexpr int UnknownStride      = CellLayout::PaddedNumberOfUnknowns;
  constexpr int FluxAxisStride     = CellLayout::NodesPerCell * UnknownStride;
  constexpr int GradientAxisStride = CellLayout::NodesPerCell * UnknownStride;
  constexpr int SourceSize         = CellLayout::NodesPerCell * UnknownStride;
  constexpr int CellStateSize      = CellLayout::NodesPerCell * NumberOfData;

  constexpr int    MarkedTimeNode  = 1;
  constexpr double TimeStamp       = 0.75;
  constexpr double TimeStepSize    = 0.125;
  constexpr double PicardTolerance = 1.0e-8;
  constexpr double Slope           = 0.5;

  constexpr double FluxSpeed               = 0.5;
  constexpr double DiffusionSpeed          = 0.25;
  constexpr double ProductSpeed            = 0.75;
  constexpr double SourceStrength          = 1.5;
  constexpr double DiffusiveSourceStrength = 0.5;

  struct PlainFluxTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const double* NOALIAS const                                   state,
      const tarch::la::Vector<DIMENSIONS, double>&                  position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      const double                                                  timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         fluxValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] = (1.0 + direction) * state[unknown] + position(direction) + timeStamp;
      }
    }
  };

  struct DiffusiveFluxTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const double* NOALIAS const                                   state,
      const double* const NOALIAS                                   stateGradients[DIMENSIONS],
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         fluxValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] = (1.0 + direction) * state[unknown];
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          fluxValues[unknown] += (0.5 + axis) * stateGradients[axis][unknown];
        }
      }
    }
  };

  struct SourceTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      const double                                                  timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         sourceValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        sourceValues[unknown] = 2.0 * state[unknown] + timeStamp;
      }
    }
  };

  struct DiffusiveSourceTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void diffusiveSourceTerm(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateGradient,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         sourceValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        sourceValues[unknown] = 0.0;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          sourceValues[unknown] += (1.0 + axis) * stateGradient[axis * NumberOfUnknowns + unknown];
        }
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
      const int                                                     direction,
      double* NOALIAS const                                         productValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        productValues[unknown] = (1.0 + 0.25 * direction) * stateGradient[unknown];
      }
    }
  };

  struct InertSolver final : StubLayout {};
  struct PlainFluxSolver final : StubLayout, PlainFluxTerm {};
  struct DiffusiveFluxSolver final : StubLayout, DiffusiveFluxTerm {};
  struct SourceSolver final : StubLayout, SourceTerm {};
  struct DiffusiveSourceSolver final : StubLayout, DiffusiveSourceTerm {};
  struct ProductSolver final : StubLayout, ProductTerm {};
  struct ProductSourceSolver final : StubLayout, ProductTerm, SourceTerm {};
  struct ProductDiffusiveSourceSolver final : StubLayout, ProductTerm, DiffusiveSourceTerm {};
  struct SourceDiffusiveSourceSolver final : StubLayout, SourceTerm, DiffusiveSourceTerm {};
  struct AllSourcesSolver final : StubLayout, ProductTerm, SourceTerm, DiffusiveSourceTerm {};
  struct FullSolver final : StubLayout, DiffusiveFluxTerm, ProductTerm, SourceTerm, DiffusiveSourceTerm {};

  /**
   * Every term changes a profile that is linear along the first axis at a constant rate.
   */
  struct ProfileSolver final : StubLayout {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const double* NOALIAS const                                   state,
      const double* const NOALIAS                                   stateGradients[DIMENSIONS],
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] =
          FluxSpeed * (1.0 + direction) * state[unknown] + DiffusionSpeed * stateGradients[direction][unknown];
      }
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
      [[maybe_unused]] const double* NOALIAS const                  state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] += SourceStrength;
      }
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void diffusiveSourceTerm(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateGradient,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        double sumOverAxes = 0.0;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          sumOverAxes += stateGradient[axis * NumberOfUnknowns + unknown];
        }
        values[unknown] += DiffusiveSourceStrength * sumOverAxes;
      }
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateGradient,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] += ProductSpeed * (1.0 + direction) * stateGradient[unknown];
      }
    }
  };
  TARGET_DECLARE_END

  using ProfileScratch = Predictor<ProfileSolver>::Scratch;
  using StubScratch    = Predictor<exahype2::dg::ader::tests::StubSolver>::Scratch;

  void fillIdentity(double (&matrix)[OperatorEntries]) {
    for (int entry = 0; entry < OperatorEntries; entry++) {
      matrix[entry] = 0.0;
    }
    for (int index = 0; index < NodesPerAxis; index++) {
      matrix[index * NodesPerAxis + index] = 1.0;
    }
  }

  tarch::la::Vector<DIMENSIONS, double> getCellSize() {
    tarch::la::Vector<DIMENSIONS, double> cellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSize(axis) = 0.5 + 0.25 * axis;
    }
    return cellSize;
  }

  tarch::la::Vector<DIMENSIONS, double> getInverseCellSize() {
    tarch::la::Vector<DIMENSIONS, double> inverseCellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      inverseCellSize(axis) = 1.0 / (0.5 + 0.25 * axis);
    }
    return inverseCellSize;
  }

  tarch::la::Vector<DIMENSIONS, double> getCellCentre() {
    tarch::la::Vector<DIMENSIONS, double> cellCentre;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellCentre(axis) = 1.0 + axis;
    }
    return cellCentre;
  }

  tarch::la::Vector<DIMENSIONS, double> getNodePositionAt(const Tables& tables, const int cellNode) {
    const auto                            cellSize   = getCellSize();
    const auto                            cellCentre = getCellCentre();
    const auto                            nodeIndex  = peano4::dDelinearised(cellNode, NodesPerAxis);
    tarch::la::Vector<DIMENSIONS, double> position;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      position(axis) =
        cellCentre(axis) - 0.5 * cellSize(axis) + tables.quadraturePoints[nodeIndex(axis)] * cellSize(axis);
    }
    return position;
  }

  double getTimePositionOfMarkedNode(const Tables& tables) {
    return TimeStamp + tables.quadraturePoints[MarkedTimeNode] * TimeStepSize;
  }

  double getStateValueAt(const int cellNode, const int value) { return 1.0 + 0.5 * cellNode + value; }

  double getGradientValueAt(const int axis, const int cellNode, const int unknown) {
    return 0.25 + axis + 0.5 * unknown + 0.125 * cellNode;
  }

  void fillSpaceTimeState(double (&spaceTimeState)[CellLayout::SpaceTimeNodes * StateStride]) {
    for (int entry = 0; entry < CellLayout::SpaceTimeNodes * StateStride; entry++) {
      spaceTimeState[entry] = 0.0;
    }
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int spaceTimeNode = MarkedTimeNode * CellLayout::NodesPerCell + cellNode;
      for (int value = 0; value < NumberOfData; value++) {
        spaceTimeState[spaceTimeNode * StateStride + value] = getStateValueAt(cellNode, value);
      }
    }
  }

  void fillStateGradient(double (&stateGradient)[DIMENSIONS * GradientAxisStride]) {
    for (int entry = 0; entry < DIMENSIONS * GradientAxisStride; entry++) {
      stateGradient[entry] = 0.0;
    }
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          stateGradient[axis * GradientAxisStride + cellNode * UnknownStride + unknown] =
            getGradientValueAt(axis, cellNode, unknown);
        }
      }
    }
  }

  double getAveragedStateValueAt(const int cellNode, const int value) { return 1.0 + 0.25 * cellNode + 0.5 * value; }

  double getSpaceTimeStateValueAt(const int timeNode, const int cellNode, const int value) {
    return getAveragedStateValueAt(cellNode, value) + 0.5 * timeNode;
  }

  double getFluxValueAt(const int axis, const int cellNode, const int unknown) {
    return 2.0 + 0.25 * cellNode + axis + 0.5 * unknown;
  }

  void fillFlux(double (&flux)[DIMENSIONS * FluxAxisStride], const double factor) {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
        for (int unknown = 0; unknown < UnknownStride; unknown++) {
          flux[axis * FluxAxisStride + cellNode * UnknownStride + unknown] =
            factor * getFluxValueAt(axis, cellNode, unknown);
        }
      }
    }
  }

  template <int ValuesPerFace>
  std::array<double*, peano4::FacesPerCell> getRowsOf(double (&rows)[peano4::FacesPerCell][ValuesPerFace]) {
    std::array<double*, peano4::FacesPerCell> addresses{};
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      addresses[static_cast<std::size_t>(face)] = rows[face];
    }
    return addresses;
  }

  struct FaceBuffers final {
    FaceBuffers()                              = default;
    FaceBuffers(const FaceBuffers&)            = delete;
    FaceBuffers(FaceBuffers&&)                 = delete;
    FaceBuffers& operator=(const FaceBuffers&) = delete;
    FaceBuffers& operator=(FaceBuffers&&)      = delete;
    ~FaceBuffers()                             = default;

    double                                    state[peano4::FacesPerCell][CellLayout::NodesPerFace * NumberOfData]{};
    double                                    flux[peano4::FacesPerCell][CellLayout::NodesPerFace * NumberOfUnknowns]{};
    std::array<double*, peano4::FacesPerCell> statePointers{getRowsOf(state)};
    std::array<double*, peano4::FacesPerCell> fluxPointers{getRowsOf(flux)};
  };

  int getCellNodeOnFace(const int direction, const int faceNode, const int normalIndex) {
    auto nodeIndex       = peano4::dDelinearisedExclusive(faceNode, NodesPerAxis, direction);
    nodeIndex(direction) = normalIndex;
    return peano4::dLinearised(nodeIndex, NodesPerAxis);
  }

  double getLinearCellStateEntry(const Tables& tables, const int cellNode, const int value, const double cellExtent) {
    return 1.0 + 0.25 * value
      + Slope * cellExtent * tables.quadraturePoints[peano4::dDelinearised(cellNode, NodesPerAxis)(0)];
  }

  void fillLinearCellState(const Tables& tables, double (&cellState)[CellStateSize], const double cellExtent) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = getLinearCellStateEntry(tables, cellNode, value, cellExtent);
      }
    }
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
      const int slot        = cell * peano4::FacesPerCell + face;
      const int stateOffset = slot * Setup::FaceStateCardinality;
      const int fluxOffset  = slot * Setup::FaceFluxCardinality;
      faceState[face]       = faceStates + stateOffset;
      faceFlux[face]        = faceFluxes + fluxOffset;
    }
  }

  /**
   * Runs the predictor of the cells of one team. A thread behind the last cell still calls
   * predict() to reach the barriers of the team.
   */
  struct NonlinearPredictorOfCell final {
    exahype2::dg::BasisTables<double> tables{};
    double*                           cellStates{nullptr};
    double*                           faceStates{nullptr};
    double*                           faceFluxes{nullptr};
    double*                           scratch{nullptr};
    int*                              sweeps{nullptr};
    const double*                     cellCentres{nullptr};
    const double*                     cellSizes{nullptr};
    int                               threadsPerCell{1};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                            team,
      const tarch::accelerator::TeamHandle handle
    ) const {
      exahype2::dg::ader::TeamWorkers workers = exahype2::dg::ader::getWorkers(handle, threadsPerCell);
      const int                       cell    = team * workers.groups + workers.group;
      workers.hasCell                         = workers.hasCell and cell < Setup::NumberOfCells;
      const int pointed                       = workers.hasCell ? cell : 0;
      double*   faceState[peano4::FacesPerCell];
      double*   faceFlux[peano4::FacesPerCell];
      collectFacePointers(pointed, faceStates, faceFluxes, faceState, faceFlux);
      const int                                   coordinateOffset = pointed * DIMENSIONS;
      const tarch::la::Vector<DIMENSIONS, double> cellCentre(cellCentres + coordinateOffset);
      const tarch::la::Vector<DIMENSIONS, double> cellSize(cellSizes + coordinateOffset);
      const int                                   stateOffset   = pointed * Setup::CellStateCardinality;
      const int                                   scratchOffset = pointed * StubScratch::Size;
      const int                                   outcome = Predictor<exahype2::dg::ader::tests::StubSolver>::predict(
        faceState,
        faceFlux,
        cellStates + stateOffset,
        scratch + scratchOffset,
        tables,
        cellCentre,
        cellSize,
        Setup::TimeStamp,
        Setup::TimeStepSize,
        Setup::PicardTolerance,
        workers
      );
      if (workers.hasCell and workers.thread == 0) {
        sweeps[cell] = outcome;
      }
    }
  };
  TARGET_DECLARE_END

  std::vector<int> predictOnDevice(
    const Tables&                         tables,
    const exahype2::dg::tests::CellChain& geometry,
    std::vector<double>&                  cellStates,
    std::vector<double>&                  faceStates,
    std::vector<double>&                  faceFluxes
  ) {
    const exahype2::dg::tests::DeviceBasisTables deviceTables(tables);
    const std::vector<double> scratch(static_cast<std::size_t>(Setup::NumberOfCells) * StubScratch::Size, 0.0);
    std::vector<int>          sweeps(Setup::NumberOfCells, 0);

    const exahype2::dg::tests::DeviceBuffer<double> deviceCellStates(cellStates);
    const exahype2::dg::tests::DeviceBuffer<double> deviceFaceStates(faceStates);
    const exahype2::dg::tests::DeviceBuffer<double> deviceFaceFluxes(faceFluxes);
    const exahype2::dg::tests::DeviceBuffer<double> deviceScratch(scratch);
    const exahype2::dg::tests::DeviceBuffer<int>    deviceSweeps(sweeps);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);

    const int threadsPerTeam = tarch::accelerator::getTeamThreads(CellLayout::NodesPerCell);
    const int threadsPerCell = std::min(CellLayout::NodesPerCell, threadsPerTeam);
    const int cellsPerTeam   = threadsPerTeam / threadsPerCell;
    tarch::accelerator::parallelFor(
      tarch::accelerator::TeamRange(
        (Setup::NumberOfCells + cellsPerTeam - 1) / cellsPerTeam,
        threadsPerTeam,
        Predictor<exahype2::dg::ader::tests::StubSolver>::getTeamScratchBytes(threadsPerTeam)
      ),
      NonlinearPredictorOfCell{
        deviceTables.getView(),
        deviceCellStates.getData(),
        deviceFaceStates.getData(),
        deviceFaceFluxes.getData(),
        deviceScratch.getData(),
        deviceSweeps.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData(),
        threadsPerCell
      }
    );

    deviceCellStates.copyBack(cellStates);
    deviceFaceStates.copyBack(faceStates);
    deviceFaceFluxes.copyBack(faceFluxes);
    deviceSweeps.copyBack(sweeps);
    return sweeps;
  }
} // namespace

exahype2::dg::ader::tests::NonlinearPredictorTest::NonlinearPredictorTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::NonlinearPredictorTest") {}

void
exahype2::dg::ader::tests::NonlinearPredictorTest::run() {
  testMethod(testTheGuessRepeatsTheCellStateOverTime);
  testMethod(testTheRightHandSideOfATimeNodeStartsFromZero);
  testMethod(testAConstantFluxHasNoDivergence);
  testMethod(testALinearFluxSubtractsItsExactDivergence);
  testMethod(testAPicardSystemWithoutTermsReproducesTheInitialState);
  testMethod(testTheResidualCountsTheSquaredDistanceOfTheIterates);
  testMethod(testEveryThreadSplitSumsTheSameResidual);
  testMethod(testAnUnboundedIterateReportsDivergence);
  testMethod(testAnIterateAboveTheFloatRangeReportsDivergence);
  testMethod(testATinyToleranceComparesTheSquaredDistance);
  testMethod(testTheGradientOfAConstantVanishes);
  testMethod(testALinearStateHasTheExactGradient);
  testMethod(testTheFluxIsAskedForOncePerNodeAndAxis);
  testMethod(testTheDiffusiveFluxSeesTheNodalGradients);
  testMethod(testEveryTermCombinationAccumulatesIntoOneBuffer);
  testMethod(testTheAverageOfAConstantIsThatConstant);
  testMethod(testTheFirstTimeNodeRestartsTheRunningAverages);
  testMethod(testEveryAxisBlockKeepsCellNodeOrder);
  testMethod(testTheStateAverageLeavesTheLaterTimeSlicesAlone);
  testMethod(testTheScratchBlocksFollowEachOtherWithoutOverlap);
  testMethod(testATermTheSolverLacksHasZeroExtent);
  testMethod(testThePredictorWritesTheTimeAveragesAndTheFacesAndLeavesTheCellState);
  testMethod(testAZeroSquaredToleranceReportsUnconvergedPicardIteration);
  testMethod(testTheToleranceGrowsToTheRoundingDistanceOfTheCell);
  testMethod(testAToleranceBelowTheRoundingDistanceKeepsTheTimeAverages);
  testMethod(testEveryCellOfADeviceRangeMatchesTheHostPredictor);
}

/**
 * Every time slice of the initial space-time state is the cell state with zero padding lanes.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheGuessRepeatsTheCellStateOverTime() {
  float cellState[CellStateSize];
  for (int entry = 0; entry < CellStateSize; entry++) {
    cellState[entry] = 1.0F + static_cast<float>(entry);
  }

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  for (int entry = 0; entry < CellLayout::SpaceTimeNodes * StateStride; entry++) {
    spaceTimeState[entry] = 9.0;
  }

  Predictor<StubLayout>::initialiseState(spaceTimeState, cellState);

  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
      for (int value = 0; value < NumberOfData; value++) {
        validateNumericalEqualsWithParams3(
          spaceTimeState[spaceTimeNode * StateStride + value],
          static_cast<double>(cellState[cellNode * NumberOfData + value]),
          timeNode,
          cellNode,
          value
        );
      }
      for (int value = NumberOfData; value < StateStride; value++) {
        validateNumericalEqualsWithParams2(spaceTimeState[spaceTimeNode * StateStride + value], 0.0, cellNode, value);
      }
    }
  }
}

/**
 * clearRightHandSide() zeroes one time node and leaves the others unchanged.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheRightHandSideOfATimeNodeStartsFromZero() {
  constexpr int ClearedTimeNode = 1;

  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride];
  for (int entry = 0; entry < CellLayout::SpaceTimeNodes * UnknownStride; entry++) {
    rightHandSide[entry] = 9.0;
  }

  Predictor<StubLayout>::clearRightHandSide(rightHandSide, ClearedTimeNode);

  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    const double expected = timeNode == ClearedTimeNode ? 0.0 : 9.0;
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int spaceTimeNode = cellNode + CellLayout::NodesPerCell * timeNode;
      for (int unknown = 0; unknown < UnknownStride; unknown++) {
        validateEqualsWithParams3(
          rightHandSide[spaceTimeNode * UnknownStride + unknown],
          expected,
          timeNode,
          cellNode,
          unknown
        );
      }
    }
  }
}

/**
 * A constant flux leaves the right-hand side unchanged bitwise.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testAConstantFluxHasNoDivergence() {
  const Tables tables;
  const auto   inverseCellSize = getInverseCellSize();

  // The double nearest to 5/3 is inexact. Every product with it rounds.
  double flux[DIMENSIONS * FluxAxisStride];
  for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
    flux[entry] = 5.0 / 3.0;
  }

  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride];
  for (int entry = 0; entry < CellLayout::SpaceTimeNodes * UnknownStride; entry++) {
    rightHandSide[entry] = 7.0;
  }

  double fluxDerivative[DIMENSIONS * FluxAxisStride];
  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    Predictor<StubLayout>::differentiateFlux(
      fluxDerivative,
      flux,
      exahype2::dg::getDerivativeWeightsOfNodePairs<NodesPerAxis>(tables.stiffnessOperatorTransposed.data()),
      tables.cellQuadratureWeights.data(),
      inverseCellSize,
      0.25,
      timeNode
    );
    Predictor<StubLayout>::subtractFluxDivergence(rightHandSide, fluxDerivative, timeNode);
  }

  for (int entry = 0; entry < CellLayout::SpaceTimeNodes * UnknownStride; entry++) {
    validateEqualsWithParams1(rightHandSide[entry], 7.0, entry);
  }
}

/**
 * A flux that grows linearly along its own axis subtracts its exact derivative from one time slice.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testALinearFluxSubtractsItsExactDivergence() {
  constexpr double FluxSlope = 3.0;

  const Tables tables;
  const auto   inverseCellSize = getInverseCellSize();

  // The flux of an axis is linear along that axis alone. Its derivative is the slope.
  double flux[DIMENSIONS * FluxAxisStride] = {};
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const double reference = tables.quadraturePoints[peano4::dDelinearised(cellNode, NodesPerAxis)(axis)];
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        flux[axis * FluxAxisStride + cellNode * UnknownStride + unknown] =
          (1.0 + unknown) * (2.0 + FluxSlope * (1.0 + axis) * reference);
      }
    }
  }

  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride] = {};

  double fluxDerivative[DIMENSIONS * FluxAxisStride];
  Predictor<StubLayout>::differentiateFlux(
    fluxDerivative,
    flux,
    exahype2::dg::getDerivativeWeightsOfNodePairs<NodesPerAxis>(tables.stiffnessOperatorTransposed.data()),
    tables.cellQuadratureWeights.data(),
    inverseCellSize,
    TimeStepSize,
    MarkedTimeNode
  );
  Predictor<StubLayout>::subtractFluxDivergence(rightHandSide, fluxDerivative, MarkedTimeNode);

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    const int    spaceTimeNode = MarkedTimeNode * CellLayout::NodesPerCell + cellNode;
    const double weight        = tables.quadratureWeights[MarkedTimeNode] * tables.cellQuadratureWeights[cellNode];
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      double expected = 0.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        expected -= weight * TimeStepSize * inverseCellSize(axis) * (1.0 + unknown) * FluxSlope * (1.0 + axis);
      }
      validateNumericalEqualsWithParams2(
        rightHandSide[spaceTimeNode * UnknownStride + unknown],
        expected,
        cellNode,
        unknown
      );
    }
    for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
      validateNumericalEqualsWithParams2(
        rightHandSide[spaceTimeNode * UnknownStride + unknown],
        0.0,
        cellNode,
        unknown
      );
    }
  }

  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    if (timeNode != MarkedTimeNode) {
      for (int entry = 0; entry < CellLayout::NodesPerCell * UnknownStride; entry++) {
        validateNumericalEqualsWithParams2(
          rightHandSide[timeNode * CellLayout::NodesPerCell * UnknownStride + entry],
          0.0,
          timeNode,
          entry
        );
      }
    }
  }
}

/**
 * A Picard system without terms returns the cell state bitwise at every time node.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testAPicardSystemWithoutTermsReproducesTheInitialState() {
  const Tables tables;

  // The double nearest to one third is inexact. Every product with these values rounds.
  double cellState[CellStateSize];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      cellState[cellNode * NumberOfData + value] = 1.0 / 3.0 + 0.5 * cellNode + value;
    }
  }

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  Predictor<StubLayout>::initialiseState(spaceTimeState, cellState);

  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride];
  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    Predictor<StubLayout>::clearRightHandSide(rightHandSide, timeNode);
  }

  double nodeResiduals[CellLayout::NodesPerCell];
  Predictor<StubLayout>::solvePicardSystem(
    spaceTimeState,
    nodeResiduals,
    rightHandSide,
    cellState,
    tables.inversePicardOperator.data(),
    tables.inverseCellQuadratureWeights.data(),
    PicardTolerance * PicardTolerance
  );

  validateEquals(Predictor<StubLayout>::sumNodeResiduals(nodeResiduals), 0.0);

  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int spaceTimeNode = cellNode + CellLayout::NodesPerCell * timeNode;
      for (int value = 0; value < NumberOfData; value++) {
        validateEqualsWithParams3(
          spaceTimeState[spaceTimeNode * StateStride + value],
          cellState[cellNode * NumberOfData + value],
          timeNode,
          cellNode,
          value
        );
      }
    }
  }
}

/**
 * The solve counts the squared distance between the new and the previous iterate of every node in
 * units of the squared tolerance and stores the new iterate. A second solve with the same
 * right-hand side keeps the iterate.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheResidualCountsTheSquaredDistanceOfTheIterates() {
  // Every squared change lies below this power of two. The units of every change are then exact.
  constexpr double SquaredTolerance = 4096.0;
  constexpr double Units            = Predictor<StubLayout>::ResidualUnitsOfTolerance;

  double inversePicardOperator[OperatorEntries];
  fillIdentity(inversePicardOperator);

  double inverseCellQuadratureWeights[CellLayout::NodesPerCell];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    inverseCellQuadratureWeights[cellNode] = 1.0;
  }

  const double cellState[CellStateSize]                                  = {};
  double       spaceTimeState[CellLayout::SpaceTimeNodes * StateStride]  = {};
  double       rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride] = {};
  double       nodeResiduals[CellLayout::NodesPerCell]                   = {};

  double expectedResiduals[CellLayout::NodesPerCell] = {};
  double expectedResidual                            = 0.0;
  double largestSquare                               = 0.0;
  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const double newValue = 1.0 + 0.5 * (cellNode + unknown + timeNode);
        rightHandSide[(timeNode * CellLayout::NodesPerCell + cellNode) * UnknownStride + unknown] = newValue;
        expectedResiduals[cellNode] += newValue * newValue / SquaredTolerance * Units;
        expectedResidual += newValue * newValue / SquaredTolerance;
        largestSquare = std::max(largestSquare, newValue * newValue);
      }
    }
  }
  validate(largestSquare < SquaredTolerance);

  Predictor<StubLayout>::solvePicardSystem(
    spaceTimeState,
    nodeResiduals,
    rightHandSide,
    cellState,
    inversePicardOperator,
    inverseCellQuadratureWeights,
    SquaredTolerance
  );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    validateEqualsWithParams1(nodeResiduals[cellNode], expectedResiduals[cellNode], cellNode);
  }
  validateEquals(Predictor<StubLayout>::sumNodeResiduals(nodeResiduals), expectedResidual);

  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int spaceTimeNode = cellNode + CellLayout::NodesPerCell * timeNode;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(
          spaceTimeState[spaceTimeNode * StateStride + unknown],
          1.0 + 0.5 * (cellNode + unknown + timeNode),
          timeNode,
          cellNode,
          unknown
        );
      }
    }
  }

  Predictor<StubLayout>::solvePicardSystem(
    spaceTimeState,
    nodeResiduals,
    rightHandSide,
    cellState,
    inversePicardOperator,
    inverseCellQuadratureWeights,
    SquaredTolerance
  );

  validateEquals(Predictor<StubLayout>::sumNodeResiduals(nodeResiduals), 0.0);
}

/**
 * The node residuals and their sum stay bitwise equal when three threads split the nodes of the
 * cell or when the nodes and the unknowns run in reversed order.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testEveryThreadSplitSumsTheSameResidual() {
  constexpr int    Threads        = 3;
  constexpr double SplitTolerance = 1.0e4;

  double inversePicardOperator[OperatorEntries];
  fillIdentity(inversePicardOperator);

  double inverseCellQuadratureWeights[CellLayout::NodesPerCell];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    inverseCellQuadratureWeights[cellNode] = 1.0;
  }

  // The double nearest to one third is inexact. Every square and every sum rounds.
  const double cellState[CellStateSize]                                  = {};
  double       rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride] = {};
  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        rightHandSide[(timeNode * CellLayout::NodesPerCell + cellNode) * UnknownStride + unknown] =
          1.0 / 3.0 + 0.5 * (cellNode + unknown + timeNode);
      }
    }
  }

  double oneThreadState[CellLayout::SpaceTimeNodes * StateStride] = {};
  double oneThreadResiduals[CellLayout::NodesPerCell]             = {};
  Predictor<StubLayout>::solvePicardSystem(
    oneThreadState,
    oneThreadResiduals,
    rightHandSide,
    cellState,
    inversePicardOperator,
    inverseCellQuadratureWeights,
    SplitTolerance
  );

  double splitState[CellLayout::SpaceTimeNodes * StateStride] = {};
  double splitResiduals[CellLayout::NodesPerCell]             = {};
  for (int thread = 0; thread < Threads; thread++) {
    exahype2::dg::ader::TeamWorkers workers;
    workers.thread  = thread;
    workers.threads = Threads;
    Predictor<StubLayout>::solvePicardSystem(
      splitState,
      splitResiduals,
      rightHandSide,
      cellState,
      inversePicardOperator,
      inverseCellQuadratureWeights,
      SplitTolerance,
      workers
    );
  }

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    validateEqualsWithParams1(splitResiduals[cellNode], oneThreadResiduals[cellNode], cellNode);
  }
  validateEquals(
    Predictor<StubLayout>::sumNodeResiduals(splitResiduals),
    Predictor<StubLayout>::sumNodeResiduals(oneThreadResiduals)
  );

  double reversedRightHandSide[CellLayout::SpaceTimeNodes * UnknownStride] = {};
  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const int reversedNode = CellLayout::NodesPerCell - 1 - cellNode;
        reversedRightHandSide
          [(timeNode * CellLayout::NodesPerCell + reversedNode) * UnknownStride + NumberOfUnknowns - 1 - unknown] =
            rightHandSide[(timeNode * CellLayout::NodesPerCell + cellNode) * UnknownStride + unknown];
      }
    }
  }
  double reversedState[CellLayout::SpaceTimeNodes * StateStride] = {};
  double reversedResiduals[CellLayout::NodesPerCell]             = {};
  Predictor<StubLayout>::solvePicardSystem(
    reversedState,
    reversedResiduals,
    reversedRightHandSide,
    cellState,
    inversePicardOperator,
    inverseCellQuadratureWeights,
    SplitTolerance
  );
  validateEquals(
    Predictor<StubLayout>::sumNodeResiduals(reversedResiduals),
    Predictor<StubLayout>::sumNodeResiduals(oneThreadResiduals)
  );
  for (int entry = 0; entry < CellLayout::SpaceTimeNodes * StateStride; entry++) {
    validateEqualsWithParams1(splitState[entry], oneThreadState[entry], entry);
  }
}

/**
 * An iterate above the divergence bound and an iterate that is not a number both give an infinite
 * residual. The node returns to the cell state.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testAnUnboundedIterateReportsDivergence() {
  double inversePicardOperator[OperatorEntries];
  fillIdentity(inversePicardOperator);

  double inverseCellQuadratureWeights[CellLayout::NodesPerCell];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    inverseCellQuadratureWeights[cellNode] = 1.0;
  }

  const double cellState[CellStateSize]                                  = {};
  double       spaceTimeState[CellLayout::SpaceTimeNodes * StateStride]  = {};
  double       rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride] = {};
  double       nodeResiduals[CellLayout::NodesPerCell]                   = {};

  rightHandSide[0] = 1e30;
  rightHandSide[1] = 2.0;
  Predictor<StubLayout>::solvePicardSystem(
    spaceTimeState,
    nodeResiduals,
    rightHandSide,
    cellState,
    inversePicardOperator,
    inverseCellQuadratureWeights,
    PicardTolerance * PicardTolerance
  );
  validate(not std::isfinite(nodeResiduals[0]));
  validate(not std::isfinite(Predictor<StubLayout>::sumNodeResiduals(nodeResiduals)));
  validateEquals(spaceTimeState[0], 0.0);
  validateEquals(spaceTimeState[1], 0.0);

  rightHandSide[0] = std::numeric_limits<double>::quiet_NaN();
  Predictor<StubLayout>::solvePicardSystem(
    spaceTimeState,
    nodeResiduals,
    rightHandSide,
    cellState,
    inversePicardOperator,
    inverseCellQuadratureWeights,
    PicardTolerance * PicardTolerance
  );
  validate(not std::isfinite(nodeResiduals[0]));
  validate(not std::isfinite(Predictor<StubLayout>::sumNodeResiduals(nodeResiduals)));
  validateEquals(spaceTimeState[0], 0.0);
}

/**
 * With single-precision computation, an iterate whose squared distance lies above the float range
 * gives an infinite residual and the node returns to the cell state.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testAnIterateAboveTheFloatRangeReportsDivergence() {
  using FloatPredictor = exahype2::dg::ader::NonlinearPredictor<StubLayout, float>;

  float inversePicardOperator[OperatorEntries];
  for (int row = 0; row < NodesPerAxis; row++) {
    for (int column = 0; column < NodesPerAxis; column++) {
      inversePicardOperator[row * NodesPerAxis + column] = row == column ? 1.0F : 0.0F;
    }
  }
  float inverseCellQuadratureWeights[CellLayout::NodesPerCell];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    inverseCellQuadratureWeights[cellNode] = 1.0F;
  }

  const float cellState[CellStateSize]                                  = {};
  float       spaceTimeState[CellLayout::SpaceTimeNodes * StateStride]  = {};
  float       rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride] = {};
  float       nodeResiduals[CellLayout::NodesPerCell]                   = {};

  // The squared distance 9e38 lies above the largest float and below 1e40.
  rightHandSide[0] = 3e19F;
  FloatPredictor::solvePicardSystem(
    spaceTimeState,
    nodeResiduals,
    rightHandSide,
    cellState,
    inversePicardOperator,
    inverseCellQuadratureWeights,
    PicardTolerance * PicardTolerance
  );
  validate(not std::isfinite(nodeResiduals[0]));
  validate(not std::isfinite(FloatPredictor::sumNodeResiduals(nodeResiduals)));
  validateEquals(spaceTimeState[0], 0.0F);
}

/**
 * The square of a change far below one counts as its exact fraction of a squared tolerance of the
 * same size. A squared tolerance below that square rejects the change.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testATinyToleranceComparesTheSquaredDistance() {
  constexpr double Change = 0x1p-500;

  double inversePicardOperator[OperatorEntries];
  fillIdentity(inversePicardOperator);

  double inverseCellQuadratureWeights[CellLayout::NodesPerCell];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    inverseCellQuadratureWeights[cellNode] = 1.0;
  }

  const double cellState[CellStateSize]                                  = {};
  double       rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride] = {};
  double       nodeResiduals[CellLayout::NodesPerCell]                   = {};
  rightHandSide[0]                                                       = Change;

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride] = {};
  Predictor<StubLayout>::solvePicardSystem(
    spaceTimeState,
    nodeResiduals,
    rightHandSide,
    cellState,
    inversePicardOperator,
    inverseCellQuadratureWeights,
    4.0 * Change * Change
  );
  validateEquals(Predictor<StubLayout>::sumNodeResiduals(nodeResiduals), 0.25);

  double rejectingState[CellLayout::SpaceTimeNodes * StateStride] = {};
  Predictor<StubLayout>::solvePicardSystem(
    rejectingState,
    nodeResiduals,
    rightHandSide,
    cellState,
    inversePicardOperator,
    inverseCellQuadratureWeights,
    0.5 * Change * Change
  );
  validate(Predictor<StubLayout>::sumNodeResiduals(nodeResiduals) >= 1.0);
}

/**
 * A constant state has a zero gradient along every axis.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheGradientOfAConstantVanishes() {
  const Tables tables;

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  for (int entry = 0; entry < CellLayout::SpaceTimeNodes * StateStride; entry++) {
    spaceTimeState[entry] = 3.0;
  }

  double stateGradient[DIMENSIONS * GradientAxisStride];
  for (int entry = 0; entry < DIMENSIONS * GradientAxisStride; entry++) {
    stateGradient[entry] = 9.0;
  }

  tarch::la::Vector<DIMENSIONS, double> inverseCellSize;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    inverseCellSize(axis) = 1.0 + axis;
  }

  Predictor<StubLayout>::computeSpaceTimeGradient(
    stateGradient,
    spaceTimeState,
    exahype2::dg::getDerivativeWeightsOfNodePairs<NodesPerAxis>(tables.derivativeOperator.data()),
    inverseCellSize,
    MarkedTimeNode
  );

  for (int entry = 0; entry < DIMENSIONS * GradientAxisStride; entry++) {
    validateNumericalEqualsWithParams1(stateGradient[entry], 0.0, entry);
  }
}

/**
 * A state that grows linearly along every axis has the exact physical derivative along every axis.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testALinearStateHasTheExactGradient() {
  constexpr double StateSlope = 3.0;

  const Tables tables;
  const auto   inverseCellSize = getInverseCellSize();

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride] = {};
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    const auto nodeIndex     = peano4::dDelinearised(cellNode, NodesPerAxis);
    const int  spaceTimeNode = MarkedTimeNode * CellLayout::NodesPerCell + cellNode;
    double     reference     = 0.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      reference += (1.0 + axis) * StateSlope * tables.quadraturePoints[nodeIndex(axis)];
    }
    for (int value = 0; value < NumberOfData; value++) {
      spaceTimeState[spaceTimeNode * StateStride + value] = (1.0 + value) * (2.0 + reference);
    }
  }

  double stateGradient[DIMENSIONS * GradientAxisStride];
  for (int entry = 0; entry < DIMENSIONS * GradientAxisStride; entry++) {
    stateGradient[entry] = 9.0;
  }

  Predictor<StubLayout>::computeSpaceTimeGradient(
    stateGradient,
    spaceTimeState,
    exahype2::dg::getDerivativeWeightsOfNodePairs<NodesPerAxis>(tables.derivativeOperator.data()),
    inverseCellSize,
    MarkedTimeNode
  );

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const double expected = (1.0 + unknown) * (1.0 + axis) * StateSlope * inverseCellSize(axis);
        validateNumericalEqualsWithParams3(
          stateGradient[axis * GradientAxisStride + cellNode * UnknownStride + unknown],
          expected,
          axis,
          cellNode,
          unknown
        );
      }
      for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
        validateNumericalEqualsWithParams3(
          stateGradient[axis * GradientAxisStride + cellNode * UnknownStride + unknown],
          0.0,
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

/**
 * The flux of every node and axis receives the node position and the node time.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheFluxIsAskedForOncePerNodeAndAxis() {
  const Tables tables;

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  fillSpaceTimeState(spaceTimeState);

  double stateGradient[DIMENSIONS * GradientAxisStride];
  fillStateGradient(stateGradient);

  double flux[DIMENSIONS * FluxAxisStride];
  for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
    flux[entry] = 9.0;
  }

  Predictor<PlainFluxSolver>::evaluateSpaceTimeFlux(
    flux,
    spaceTimeState,
    stateGradient,
    tables.quadraturePoints.data(),
    getCellCentre(),
    getCellSize(),
    TimeStamp,
    TimeStepSize,
    MarkedTimeNode
  );

  const double timePosition = getTimePositionOfMarkedNode(tables);

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const double position = getNodePositionAt(tables, cellNode)(axis);
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const double expected = (1.0 + axis) * getStateValueAt(cellNode, unknown) + position + timePosition;
        validateNumericalEqualsWithParams3(
          flux[axis * FluxAxisStride + cellNode * UnknownStride + unknown],
          expected,
          axis,
          cellNode,
          unknown
        );
      }
      for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
        validateNumericalEqualsWithParams3(
          flux[axis * FluxAxisStride + cellNode * UnknownStride + unknown],
          0.0,
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

/**
 * The diffusive flux of a node reads the gradient of every axis at that node.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheDiffusiveFluxSeesTheNodalGradients() {
  const Tables tables;

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  fillSpaceTimeState(spaceTimeState);

  double stateGradient[DIMENSIONS * GradientAxisStride];
  fillStateGradient(stateGradient);

  double flux[DIMENSIONS * FluxAxisStride];
  for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
    flux[entry] = 9.0;
  }

  Predictor<DiffusiveFluxSolver>::evaluateSpaceTimeFlux(
    flux,
    spaceTimeState,
    stateGradient,
    tables.quadraturePoints.data(),
    getCellCentre(),
    getCellSize(),
    TimeStamp,
    TimeStepSize,
    MarkedTimeNode
  );

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        double expected = (1.0 + axis) * getStateValueAt(cellNode, unknown);
        for (int gradientAxis = 0; gradientAxis < DIMENSIONS; gradientAxis++) {
          expected += (0.5 + gradientAxis) * getGradientValueAt(gradientAxis, cellNode, unknown);
        }
        validateNumericalEqualsWithParams3(
          flux[axis * FluxAxisStride + cellNode * UnknownStride + unknown],
          expected,
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

/**
 * Checks the right-hand side and the running source average of Derived against the terms the flags
 * select.
 */
template <typename Derived>
void
exahype2::dg::ader::tests::NonlinearPredictorTest::validateSourceOf(
  const bool evaluateNonconservativeProduct,
  const bool evaluateSource,
  const bool evaluateDiffusiveSource
) {
  constexpr double FirstSourceValue    = 0.25;
  constexpr double AveragedSourceValue = 0.75;

  const Tables tables;

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  fillSpaceTimeState(spaceTimeState);

  double stateGradient[DIMENSIONS * GradientAxisStride];
  fillStateGradient(stateGradient);

  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride];
  for (int entry = 0; entry < CellLayout::SpaceTimeNodes * UnknownStride; entry++) {
    rightHandSide[entry] = 0.5;
  }

  double firstSource[SourceSize];
  double timeAveragedSource[SourceSize];
  for (int entry = 0; entry < SourceSize; entry++) {
    firstSource[entry]        = FirstSourceValue;
    timeAveragedSource[entry] = AveragedSourceValue;
  }

  Predictor<Derived>::evaluateSpaceTimeSource(
    rightHandSide,
    firstSource,
    timeAveragedSource,
    spaceTimeState,
    stateGradient,
    tables.quadraturePoints.data(),
    tables.quadratureWeights.data(),
    tables.cellQuadratureWeights.data(),
    getCellCentre(),
    getCellSize(),
    TimeStamp,
    TimeStepSize,
    MarkedTimeNode
  );

  const double timePosition = getTimePositionOfMarkedNode(tables);
  const double weight       = tables.quadratureWeights[MarkedTimeNode];

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    const int    spaceTimeNode = MarkedTimeNode * CellLayout::NodesPerCell + cellNode;
    const double updateSize    = weight * tables.cellQuadratureWeights[cellNode] * TimeStepSize;
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      double expected = 0.0;
      if (evaluateSource) {
        expected += 2.0 * getStateValueAt(cellNode, unknown) + timePosition;
      }
      if (evaluateDiffusiveSource) {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          expected += (1.0 + axis) * getGradientValueAt(axis, cellNode, unknown);
        }
      }
      if (evaluateNonconservativeProduct) {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          expected -= (1.0 + 0.25 * axis) * getGradientValueAt(axis, cellNode, unknown);
        }
      }
      validateNumericalEqualsWithParams2(
        rightHandSide[spaceTimeNode * UnknownStride + unknown],
        0.5 + updateSize * expected,
        cellNode,
        unknown
      );
      validateNumericalEqualsWithParams2(
        timeAveragedSource[cellNode * UnknownStride + unknown],
        AveragedSourceValue + weight * (expected - FirstSourceValue),
        cellNode,
        unknown
      );
    }
    for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
      validateNumericalEqualsWithParams2(
        rightHandSide[spaceTimeNode * UnknownStride + unknown],
        0.5,
        cellNode,
        unknown
      );
      validateNumericalEqualsWithParams2(
        timeAveragedSource[cellNode * UnknownStride + unknown],
        AveragedSourceValue - weight * FirstSourceValue,
        cellNode,
        unknown
      );
    }
    for (int unknown = 0; unknown < UnknownStride; unknown++) {
      validateEqualsWithParams2(firstSource[cellNode * UnknownStride + unknown], FirstSourceValue, cellNode, unknown);
    }
  }

  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    if (timeNode != MarkedTimeNode) {
      const int firstEntry = timeNode * CellLayout::NodesPerCell * UnknownStride;
      for (int entry = 0; entry < CellLayout::NodesPerCell * UnknownStride; entry++) {
        validateNumericalEqualsWithParams2(rightHandSide[firstEntry + entry], 0.5, timeNode, entry);
      }
    }
  }
}

/**
 * The source, the diffusive source and the negated nonconservative product enter one buffer in every combination.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testEveryTermCombinationAccumulatesIntoOneBuffer() {
  validateSourceOf<SourceSolver>(false, true, false);
  validateSourceOf<ProductSolver>(true, false, false);
  validateSourceOf<DiffusiveSourceSolver>(false, false, true);
  validateSourceOf<ProductSourceSolver>(true, true, false);
  validateSourceOf<ProductDiffusiveSourceSolver>(true, false, true);
  validateSourceOf<SourceDiffusiveSourceSolver>(false, true, true);
  validateSourceOf<AllSourcesSolver>(true, true, true);
}

/**
 * A state, a flux and a source that are constant in time average to their value bitwise.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheAverageOfAConstantIsThatConstant() {
  const Tables tables;

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
      for (int value = 0; value < StateStride; value++) {
        spaceTimeState[spaceTimeNode * StateStride + value] = getAveragedStateValueAt(cellNode, value);
      }
    }
  }

  double stateGradient[DIMENSIONS * GradientAxisStride];
  fillStateGradient(stateGradient);

  double firstFlux[DIMENSIONS * FluxAxisStride];
  double flux[DIMENSIONS * FluxAxisStride];
  fillFlux(firstFlux, 1.0);
  fillFlux(flux, 1.0);

  double timeAveragedFlux[DIMENSIONS * FluxAxisStride]             = {};
  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride] = {};
  double firstSource[SourceSize]                                   = {};
  double timeAveragedSource[SourceSize];
  for (int entry = 0; entry < SourceSize; entry++) {
    timeAveragedSource[entry] = 9.0;
  }

  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    const double* const fluxOfTimeNode = timeNode == 0 ? firstFlux : flux;
    Predictor<PlainFluxSolver>::averageFluxOverTime(
      timeAveragedFlux,
      fluxOfTimeNode,
      firstFlux,
      tables.quadratureWeights[timeNode],
      timeNode
    );
    Predictor<ProductSolver>::evaluateSpaceTimeSource(
      rightHandSide,
      firstSource,
      timeAveragedSource,
      spaceTimeState,
      stateGradient,
      tables.quadraturePoints.data(),
      tables.quadratureWeights.data(),
      tables.cellQuadratureWeights.data(),
      getCellCentre(),
      getCellSize(),
      TimeStamp,
      TimeStepSize,
      timeNode
    );
  }

  Predictor<StubLayout>::averageStateOverTime(spaceTimeState, tables.quadratureWeights.data());

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < StateStride; value++) {
      validateEqualsWithParams2(
        spaceTimeState[cellNode * StateStride + value],
        getAveragedStateValueAt(cellNode, value),
        cellNode,
        value
      );
    }
  }
  for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
    validateEqualsWithParams1(timeAveragedFlux[entry], firstFlux[entry], entry);
  }
  for (int entry = 0; entry < SourceSize; entry++) {
    validateEqualsWithParams1(timeAveragedSource[entry], firstSource[entry], entry);
  }
}

/**
 * The first time node overwrites the running flux and source averages with its own flux and source.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheFirstTimeNodeRestartsTheRunningAverages() {
  const Tables tables;

  double firstFlux[DIMENSIONS * FluxAxisStride];
  fillFlux(firstFlux, 1.0);

  double timeAveragedFlux[DIMENSIONS * FluxAxisStride];
  for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
    timeAveragedFlux[entry] = 9.0;
  }

  Predictor<PlainFluxSolver>::averageFluxOverTime(
    timeAveragedFlux,
    firstFlux,
    firstFlux,
    tables.quadratureWeights[0],
    0
  );

  for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
    validateEqualsWithParams1(timeAveragedFlux[entry], firstFlux[entry], entry);
  }

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  fillSpaceTimeState(spaceTimeState);

  double stateGradient[DIMENSIONS * GradientAxisStride];
  fillStateGradient(stateGradient);

  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride] = {};
  double firstSource[SourceSize];
  double timeAveragedSource[SourceSize];
  for (int entry = 0; entry < SourceSize; entry++) {
    firstSource[entry]        = 9.0;
    timeAveragedSource[entry] = 9.0;
  }

  Predictor<SourceSolver>::evaluateSpaceTimeSource(
    rightHandSide,
    firstSource,
    timeAveragedSource,
    spaceTimeState,
    stateGradient,
    tables.quadraturePoints.data(),
    tables.quadratureWeights.data(),
    tables.cellQuadratureWeights.data(),
    getCellCentre(),
    getCellSize(),
    TimeStamp,
    TimeStepSize,
    0
  );

  // The first time slice of the state is zero. The source of SourceSolver is the time of that slice.
  const double sourceOfFirstTimeNode = TimeStamp + tables.quadraturePoints[0] * TimeStepSize;

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < UnknownStride; unknown++) {
      const int    entry    = cellNode * UnknownStride + unknown;
      const double expected = unknown < NumberOfUnknowns ? sourceOfFirstTimeNode : 0.0;
      validateNumericalEqualsWithParams2(firstSource[entry], expected, cellNode, unknown);
      validateEqualsWithParams2(timeAveragedSource[entry], firstSource[entry], cellNode, unknown);
    }
  }
}

/**
 * The flux average keeps one block per axis in cell node order.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testEveryAxisBlockKeepsCellNodeOrder() {
  constexpr double Weight      = 0.25;
  constexpr double FirstFactor = 1.0;
  constexpr double LaterFactor = 3.0;

  double firstFlux[DIMENSIONS * FluxAxisStride];
  double flux[DIMENSIONS * FluxAxisStride];
  fillFlux(firstFlux, FirstFactor);
  fillFlux(flux, LaterFactor);

  double timeAveragedFlux[DIMENSIONS * FluxAxisStride] = {};
  Predictor<PlainFluxSolver>::averageFluxOverTime(timeAveragedFlux, firstFlux, firstFlux, Weight, 0);
  Predictor<PlainFluxSolver>::averageFluxOverTime(timeAveragedFlux, flux, firstFlux, Weight, MarkedTimeNode);

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < UnknownStride; unknown++) {
        const double value = getFluxValueAt(axis, cellNode, unknown);
        validateNumericalEqualsWithParams3(
          timeAveragedFlux[axis * FluxAxisStride + cellNode * UnknownStride + unknown],
          FirstFactor * value + Weight * (LaterFactor - FirstFactor) * value,
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

/**
 * The state average overwrites the first time slice and leaves the later slices unchanged bitwise.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheStateAverageLeavesTheLaterTimeSlicesAlone() {
  const Tables tables;

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
      for (int value = 0; value < StateStride; value++) {
        spaceTimeState[spaceTimeNode * StateStride + value] = getSpaceTimeStateValueAt(timeNode, cellNode, value);
      }
    }
  }

  Predictor<StubLayout>::averageStateOverTime(spaceTimeState, tables.quadratureWeights.data());

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < StateStride; value++) {
      const double firstValue = getSpaceTimeStateValueAt(0, cellNode, value);
      double       expected   = firstValue;
      for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
        const double difference = getSpaceTimeStateValueAt(timeNode, cellNode, value) - firstValue;
        expected += tables.quadratureWeights[timeNode] * difference;
      }
      validateNumericalEqualsWithParams2(spaceTimeState[cellNode * StateStride + value], expected, cellNode, value);
    }
  }

  for (int timeNode = 1; timeNode < NodesPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
      for (int value = 0; value < StateStride; value++) {
        validateEqualsWithParams3(
          spaceTimeState[spaceTimeNode * StateStride + value],
          getSpaceTimeStateValueAt(timeNode, cellNode, value),
          timeNode,
          cellNode,
          value
        );
      }
    }
  }
}

/**
 * The scratch buffers follow each other without a gap. The time-averaged state overlays the
 * space-time state.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheScratchBlocksFollowEachOtherWithoutOverlap() {
  using Scratch = Predictor<FullSolver>::Scratch;

  validateEquals(Scratch::SpaceTimeStateSize, StateStride * CellLayout::SpaceTimeNodes);
  validateEquals(Scratch::RightHandSideSize, UnknownStride * CellLayout::SpaceTimeNodes);
  validateEquals(Scratch::FluxSize, DIMENSIONS * UnknownStride * CellLayout::NodesPerCell);
  validateEquals(Scratch::FirstFluxSize, DIMENSIONS * UnknownStride * CellLayout::NodesPerCell);
  validateEquals(Scratch::TimeAveragedFluxSize, DIMENSIONS * UnknownStride * CellLayout::NodesPerCell);
  validateEquals(Scratch::FirstSourceSize, UnknownStride * CellLayout::NodesPerCell);
  validateEquals(Scratch::TimeAveragedSourceSize, UnknownStride * CellLayout::NodesPerCell);
  validateEquals(Scratch::StateGradientSize, DIMENSIONS * UnknownStride * CellLayout::NodesPerCell);
  validate(Scratch::NodeResidualSize >= CellLayout::NodesPerCell);
  validate(Scratch::NodeResidualSize < CellLayout::NodesPerCell + PEANO_WITH_SIMDLEN);
  validateEquals(Scratch::NodeResidualSize % PEANO_WITH_SIMDLEN, 0);

  validateEquals(Scratch::SpaceTimeStateOffset, 0);
  validateEquals(Scratch::TimeAveragedStateOffset, Scratch::SpaceTimeStateOffset);
  validateEquals(Scratch::RightHandSideOffset, Scratch::SpaceTimeStateOffset + Scratch::SpaceTimeStateSize);
  validateEquals(Scratch::FluxOffset, Scratch::RightHandSideOffset + Scratch::RightHandSideSize);
  validateEquals(Scratch::FirstFluxOffset, Scratch::FluxOffset + Scratch::FluxSize);
  validateEquals(Scratch::TimeAveragedFluxOffset, Scratch::FirstFluxOffset + Scratch::FirstFluxSize);
  validateEquals(Scratch::FirstSourceOffset, Scratch::TimeAveragedFluxOffset + Scratch::TimeAveragedFluxSize);
  validateEquals(Scratch::TimeAveragedSourceOffset, Scratch::FirstSourceOffset + Scratch::FirstSourceSize);
  validateEquals(Scratch::StateGradientOffset, Scratch::TimeAveragedSourceOffset + Scratch::TimeAveragedSourceSize);
  validateEquals(Scratch::NodeResidualOffset, Scratch::StateGradientOffset + Scratch::StateGradientSize);
  validateEquals(Scratch::Size, Scratch::NodeResidualOffset + Scratch::NodeResidualSize);
}

/**
 * The scratch block reserves zero entries for a term that the solver lacks.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testATermTheSolverLacksHasZeroExtent() {
  using Full          = Predictor<FullSolver>;
  using Bare          = Predictor<InertSolver>;
  using Product       = Predictor<ProductSolver>;
  using DiffusiveFlux = Predictor<DiffusiveFluxSolver>;

  validate(not Bare::EvaluateFlux);
  validateEquals(Bare::Scratch::FluxSize, 0);
  validateEquals(Bare::Scratch::FirstFluxSize, 0);
  validateEquals(Bare::Scratch::TimeAveragedFluxSize, 0);
  validateEquals(Bare::Scratch::FirstSourceSize, 0);
  validateEquals(Bare::Scratch::TimeAveragedSourceSize, 0);
  validateEquals(Bare::Scratch::StateGradientSize, 0);
  validate(not Bare::AccumulatesSource);
  validate(not Bare::NeedsStateGradient);
  validate(Bare::Scratch::Size < Full::Scratch::Size);

  // The nonconservative product enters the source. It needs the source buffers without a source term.
  validate(Product::AccumulatesSource);
  validate(Product::NeedsStateGradient);
  validate(Product::Scratch::FirstSourceSize > 0);
  validate(Product::Scratch::TimeAveragedSourceSize > 0);
  validate(Product::Scratch::StateGradientSize > 0);
  validateEquals(Product::Scratch::FluxSize, 0);

  validate(DiffusiveFlux::EvaluateFlux);
  validate(DiffusiveFlux::NeedsStateGradient);
  validate(not DiffusiveFlux::AccumulatesSource);
  validate(DiffusiveFlux::Scratch::StateGradientSize > 0);
  validateEquals(DiffusiveFlux::Scratch::TimeAveragedSourceSize, 0);
  validate(DiffusiveFlux::Scratch::FluxSize > 0);
}

/**
 * The predictor writes the exact time averages of a linear profile into the scratch and onto the
 * faces and leaves the cell state unchanged bitwise.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::
  testThePredictorWritesTheTimeAveragesAndTheFacesAndLeavesTheCellState() {
  const Tables                                tables;
  const auto                                  cellSize = getCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[CellStateSize];
  fillLinearCellState(tables, cellState, cellSize(0));

  FaceBuffers faces;
  double      scratch[ProfileScratch::Size] = {};

  const int numberOfSweeps = Predictor<ProfileSolver>::predict(
    faces.statePointers.data(),
    faces.fluxPointers.data(),
    cellState,
    scratch,
    tables.getView(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    PicardTolerance
  );

  // The first sweep reaches the solution and the second one confirms it.
  validateEquals(numberOfSweeps, 2);

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      validateEqualsWithParams2(
        cellState[cellNode * NumberOfData + value],
        getLinearCellStateEntry(tables, cellNode, value, cellSize(0)),
        cellNode,
        value
      );
    }
  }

  const double rate          = -FluxSpeed * Slope + SourceStrength + (DiffusiveSourceStrength - ProductSpeed) * Slope;
  const double sourceAverage = SourceStrength + (DiffusiveSourceStrength - ProductSpeed) * Slope;

  double averagedState[CellStateSize];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      const double moved = (value < NumberOfUnknowns) ? 0.5 * TimeStepSize * rate : 0.0;
      averagedState[cellNode * NumberOfData + value] =
        getLinearCellStateEntry(tables, cellNode, value, cellSize(0)) + moved;
    }
  }

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      validateNumericalEqualsWithParams2(
        scratch[ProfileScratch::TimeAveragedStateOffset + cellNode * StateStride + value],
        averagedState[cellNode * NumberOfData + value],
        cellNode,
        value
      );
    }
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams2(
        scratch[ProfileScratch::TimeAveragedSourceOffset + cellNode * UnknownStride + unknown],
        sourceAverage,
        cellNode,
        unknown
      );
      // The predictor leaves the derivative of the time-averaged flux along its axis in the scratch.
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        const double fluxDerivative = axis == 0 ? FluxSpeed * Slope : 0.0;
        validateNumericalEqualsWithParams3(
          scratch[ProfileScratch::TimeAveragedFluxOffset + axis * FluxAxisStride + cellNode * UnknownStride + unknown],
          fluxDerivative,
          axis,
          cellNode,
          unknown
        );
      }
    }
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        double lower = 0.0;
        double upper = 0.0;
        for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
          const int    cellNode  = getCellNodeOnFace(direction, faceNode, normalIndex);
          const double nodeValue = averagedState[cellNode * NumberOfData + value];
          lower += tables.leftFaceCoefficients[normalIndex] * nodeValue;
          upper += tables.rightFaceCoefficients[normalIndex] * nodeValue;
        }
        const int entry = faceNode * NumberOfData + value;
        validateNumericalEqualsWithParams3(faces.state[direction][entry], lower, direction, faceNode, value);
        validateNumericalEqualsWithParams3(
          faces.state[direction + DIMENSIONS][entry],
          upper,
          direction,
          faceNode,
          value
        );
      }
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        double lower = 0.0;
        double upper = 0.0;
        for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
          const int    cellNode  = getCellNodeOnFace(direction, faceNode, normalIndex);
          const double diffusion = (direction == 0) ? DiffusionSpeed * Slope : 0.0;
          const double nodeValue =
            FluxSpeed * (1.0 + direction) * averagedState[cellNode * NumberOfData + unknown] + diffusion;
          lower += tables.leftFaceCoefficients[normalIndex] * nodeValue;
          upper += tables.rightFaceCoefficients[normalIndex] * nodeValue;
        }
        const int entry = faceNode * NumberOfUnknowns + unknown;
        validateNumericalEqualsWithParams3(faces.flux[direction][entry], lower, direction, faceNode, unknown);
        validateNumericalEqualsWithParams3(
          faces.flux[direction + DIMENSIONS][entry],
          upper,
          direction,
          faceNode,
          unknown
        );
      }
    }
  }
}

/**
 * A zero state under a tolerance whose square rounds to zero stops at the sweep limit and returns
 * UnconvergedPicardIteration.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testAZeroSquaredToleranceReportsUnconvergedPicardIteration() {
  // No squared residual falls below zero. A zero state has the rounding distance zero.
  constexpr double UnreachableTolerance = 1.0e-200;

  const Tables                                tables;
  const auto                                  cellSize = getCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  const double cellState[CellStateSize] = {};
  FaceBuffers  faces;
  double       scratch[ProfileScratch::Size] = {};

  const int outcome = Predictor<ProfileSolver>::predict(
    faces.statePointers.data(),
    faces.fluxPointers.data(),
    cellState,
    scratch,
    tables.getView(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    UnreachableTolerance
  );
  validateEquals(outcome, exahype2::dg::ader::UnconvergedPicardIteration);
}

/**
 * The squared tolerance is the larger of the squared Picard tolerance and the squares of two units
 * in the last place of the largest unknown of the cell summed over the values of an iterate. A
 * reflection of the cell state keeps it.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheToleranceGrowsToTheRoundingDistanceOfTheCell() {
  constexpr double ValuesPerIterate         = NumberOfUnknowns * CellLayout::NodesPerCell * NodesPerAxis;
  constexpr int    LastNode                 = CellLayout::NodesPerCell - 1;
  double           cellState[CellStateSize] = {};
  cellState[0]                              = -8.0;
  cellState[NumberOfUnknowns - 1]           = 3.0;
  cellState[LastNode * NumberOfData]        = 12.0;
  const double rounding                     = 2.0 * std::numeric_limits<double>::epsilon() * 12.0;

  validateEquals(Predictor<ProfileSolver>::getSquaredTolerance(cellState, 1.0e-7), 1.0e-7 * 1.0e-7);
  validateEquals(
    Predictor<ProfileSolver>::getSquaredTolerance(cellState, 1.0e-200),
    ValuesPerIterate * rounding * rounding
  );

  double reflectedState[CellStateSize] = {};
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      reflectedState[(LastNode - cellNode) * NumberOfData + value] = cellState[cellNode * NumberOfData + value];
    }
  }
  validateEquals(
    Predictor<ProfileSolver>::getSquaredTolerance(reflectedState, 1.0e-200),
    Predictor<ProfileSolver>::getSquaredTolerance(cellState, 1.0e-200)
  );

  const double zeroState[CellStateSize] = {};
  validateEquals(Predictor<ProfileSolver>::getSquaredTolerance(zeroState, 1.0e-7), 1.0e-7 * 1.0e-7);
}

/**
 * A tolerance below the rounding distance of the iterates stops at that distance or at the sweep
 * limit with the time averages of the converged iteration.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testAToleranceBelowTheRoundingDistanceKeepsTheTimeAverages() {
  constexpr double UnreachableTolerance = 1.0e-200;

  const Tables                                tables;
  const auto                                  cellSize = getCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[CellStateSize];
  fillLinearCellState(tables, cellState, cellSize(0));

  FaceBuffers convergedFaces;
  FaceBuffers unconvergedFaces;
  double      convergedScratch[ProfileScratch::Size]   = {};
  double      unconvergedScratch[ProfileScratch::Size] = {};

  const int convergedOutcome = Predictor<ProfileSolver>::predict(
    convergedFaces.statePointers.data(),
    convergedFaces.fluxPointers.data(),
    cellState,
    convergedScratch,
    tables.getView(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    PicardTolerance
  );
  const int unconvergedOutcome = Predictor<ProfileSolver>::predict(
    unconvergedFaces.statePointers.data(),
    unconvergedFaces.fluxPointers.data(),
    cellState,
    unconvergedScratch,
    tables.getView(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    UnreachableTolerance
  );

  validateWithParams1(convergedOutcome > 0, convergedOutcome);
  validateWithParams1(
    unconvergedOutcome >= convergedOutcome or unconvergedOutcome == exahype2::dg::ader::UnconvergedPicardIteration,
    unconvergedOutcome
  );

  for (int entry = 0; entry < CellLayout::NodesPerCell * StateStride; entry++) {
    validateNumericalEqualsWithParams1(
      unconvergedScratch[ProfileScratch::TimeAveragedStateOffset + entry],
      convergedScratch[ProfileScratch::TimeAveragedStateOffset + entry],
      entry
    );
  }
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfData; entry++) {
      validateNumericalEqualsWithParams2(
        unconvergedFaces.state[face][entry],
        convergedFaces.state[face][entry],
        face,
        entry
      );
    }
    for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfUnknowns; entry++) {
      validateNumericalEqualsWithParams2(
        unconvergedFaces.flux[face][entry],
        convergedFaces.flux[face][entry],
        face,
        entry
      );
    }
  }
}

void
exahype2::dg::ader::tests::NonlinearPredictorTest::testEveryCellOfADeviceRangeMatchesTheHostPredictor() {
  const Tables                         tables;
  const exahype2::dg::tests::CellChain geometry;
  const std::vector<double> cellStates = exahype2::dg::tests::getRamp(Setup::AllCellStatesCardinality, 1.0, 0.003);
  const std::vector<double> faceStates(Setup::AllFaceStatesCardinality, 0.0);
  const std::vector<double> faceFluxes(Setup::AllFaceFluxesCardinality, 0.0);

  std::vector<double>    deviceCellResult      = cellStates;
  std::vector<double>    deviceFaceStateResult = faceStates;
  std::vector<double>    deviceFaceFluxResult  = faceFluxes;
  const std::vector<int> deviceSweepResult =
    predictOnDevice(tables, geometry, deviceCellResult, deviceFaceStateResult, deviceFaceFluxResult);

  std::vector<double> hostCellStates = cellStates;
  std::vector<double> hostFaceStates = faceStates;
  std::vector<double> hostFaceFluxes = faceFluxes;
  std::vector<double> hostScratch(static_cast<std::size_t>(Setup::NumberOfCells) * StubScratch::Size, 0.0);
  const exahype2::dg::BasisTables<double> hostTables = tables.getView();
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    double* faceState[peano4::FacesPerCell];
    double* faceFlux[peano4::FacesPerCell];
    collectFacePointers(cell, hostFaceStates.data(), hostFaceFluxes.data(), faceState, faceFlux);
    const int                                   coordinateOffset = cell * DIMENSIONS;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(geometry.cellCentres.data() + coordinateOffset);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(geometry.cellSizes.data() + coordinateOffset);
    const int                                   stateOffset   = cell * Setup::CellStateCardinality;
    const int                                   scratchOffset = cell * StubScratch::Size;
    const int                                   hostSweeps = Predictor<exahype2::dg::ader::tests::StubSolver>::predict(
      faceState,
      faceFlux,
      hostCellStates.data() + stateOffset,
      hostScratch.data() + scratchOffset,
      hostTables,
      cellCentre,
      cellSize,
      Setup::TimeStamp,
      Setup::TimeStepSize,
      Setup::PicardTolerance
    );
    validateWithParams2(hostSweeps > 0, cell, hostSweeps);
    validateEqualsWithParams1(deviceSweepResult[cell], hostSweeps, cell);
  }

  validateNumericalBuffersEquals(deviceCellResult, hostCellStates);
  validateNumericalBuffersEquals(deviceFaceStateResult, hostFaceStates);
  validateNumericalBuffersEquals(deviceFaceFluxResult, hostFaceFluxes);
}
