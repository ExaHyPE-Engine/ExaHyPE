// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NonlinearPredictorTest.h"

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/ader/NonlinearPredictor.h"
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
  constexpr int FluxAxisStride     = CellLayout::SpaceTimeNodes * UnknownStride;
  constexpr int GradientAxisStride = CellLayout::NodesPerCell * UnknownStride;
  constexpr int AveragedAxisStride = CellLayout::NodesPerCell * UnknownStride;
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

  /**
   * A flux that depends on the state, on the position along its axis and on the time.
   */
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

  /**
   * A diffusive flux that reads the gradient of every axis at the node.
   */
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
  struct FluxSourceSolver final : StubLayout, PlainFluxTerm, SourceTerm {};
  struct FullSolver final : StubLayout, DiffusiveFluxTerm, ProductTerm, SourceTerm, DiffusiveSourceTerm {};

  /**
   * Every term moves a profile that is linear along the first axis at a constant rate.
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
        values[unknown] +=
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

  double getAveragedFluxValueAt(const int axis, const int cellNode, const int unknown) {
    return 2.0 + 0.25 * cellNode + axis + 0.5 * unknown;
  }

  double getAveragedSourceValueAt(const int cellNode, const int unknown) {
    return 3.0 - 0.25 * cellNode + 0.5 * unknown;
  }

  struct SpaceTimeBuffers final {
    double state[CellLayout::SpaceTimeNodes * StateStride]{};
    double flux[DIMENSIONS * FluxAxisStride]{};
    double source[CellLayout::SpaceTimeNodes * UnknownStride]{};
  };

  struct AveragedBuffers final {
    double state[CellLayout::NodesPerCell * StateStride]{};
    double flux[DIMENSIONS * AveragedAxisStride]{};
    double source[AveragedAxisStride]{};
  };

  void fillConstantInTime(SpaceTimeBuffers& buffers) {
    for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
      for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
        const int spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
        for (int value = 0; value < StateStride; value++) {
          buffers.state[spaceTimeNode * StateStride + value] = getAveragedStateValueAt(cellNode, value);
        }
        for (int unknown = 0; unknown < UnknownStride; unknown++) {
          buffers.source[spaceTimeNode * UnknownStride + unknown] = getAveragedSourceValueAt(cellNode, unknown);
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            buffers.flux[axis * FluxAxisStride + spaceTimeNode * UnknownStride + unknown] =
              getAveragedFluxValueAt(axis, cellNode, unknown);
          }
        }
      }
    }
  }

  void fillTargets(AveragedBuffers& buffers, const double value) {
    for (int entry = 0; entry < CellLayout::NodesPerCell * StateStride; entry++) {
      buffers.state[entry] = value;
    }
    for (int entry = 0; entry < DIMENSIONS * AveragedAxisStride; entry++) {
      buffers.flux[entry] = value;
    }
    for (int entry = 0; entry < AveragedAxisStride; entry++) {
      buffers.source[entry] = value;
    }
  }

  /**
   * The face states and face fluxes of one cell, with one pointer per face for the kernels.
   */
  struct FaceBuffers final {
    double  state[peano4::FacesPerCell][CellLayout::NodesPerFace * NumberOfData]{};
    double  flux[peano4::FacesPerCell][CellLayout::NodesPerFace * NumberOfUnknowns]{};
    double* statePointers[peano4::FacesPerCell]{};
    double* fluxPointers[peano4::FacesPerCell]{};

    FaceBuffers() {
      for (int face = 0; face < peano4::FacesPerCell; face++) {
        statePointers[face] = state[face];
        fluxPointers[face]  = flux[face];
      }
    }
  };

  /**
   * Returns the cell node that carries the face node of the face along direction in layer normalIndex.
   */
  int getCellNodeOnFace(const int direction, const int faceNode, const int normalIndex) {
    auto nodeIndex       = peano4::dDelinearisedExclusive(faceNode, NodesPerAxis, direction);
    nodeIndex(direction) = normalIndex;
    return peano4::dLinearised(nodeIndex, NodesPerAxis);
  }

  double getLinearCellStateEntry(const Tables& tables, const int cellNode, const int value, const double cellExtent) {
    return 1.0 + 0.25 * value
      + Slope * cellExtent * tables.quadraturePoints[peano4::dDelinearised(cellNode, NodesPerAxis)(0)];
  }

  TARGET_DECLARE
  /**
   * Points the face arrays of one cell into the flat buffers, one block per cell and face.
   */
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
   * Writes the predictor of one cell into its faces and the number of its Picard sweeps into sweeps.
   */
  struct PredictorOfCell final {
    exahype2::dg::BasisTables<double> tables{};
    double*                           cellStates{nullptr};
    double*                           faceStates{nullptr};
    double*                           faceFluxes{nullptr};
    double*                           timeAveragedScratch{nullptr};
    double*                           picardScratch{nullptr};
    int*                              sweeps{nullptr};
    const double*                     cellCentres{nullptr};
    const double*                     cellSizes{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int cell) const {
      double* faceState[peano4::FacesPerCell];
      double* faceFlux[peano4::FacesPerCell];
      collectFacePointers(cell, faceStates, faceFluxes, faceState, faceFlux);
      const int                                   coordinateOffset = cell * DIMENSIONS;
      const tarch::la::Vector<DIMENSIONS, double> cellCentre(cellCentres + coordinateOffset);
      const tarch::la::Vector<DIMENSIONS, double> cellSize(cellSizes + coordinateOffset);
      const int                                   stateOffset        = cell * Setup::CellStateCardinality;
      const int                                   timeAveragedOffset = cell * StubScratch::TimeAveragedSize;
      const int                                   picardOffset       = cell * StubScratch::PicardSize;
      sweeps[cell] = Predictor<exahype2::dg::ader::tests::StubSolver>::predict(
        faceState,
        faceFlux,
        cellStates + stateOffset,
        timeAveragedScratch + timeAveragedOffset,
        picardScratch + picardOffset,
        tables,
        cellCentre,
        cellSize,
        Setup::TimeStamp,
        Setup::TimeStepSize,
        Setup::PicardTolerance
      );
    }
  };
  TARGET_DECLARE_END

  /**
   * Runs the predictor of every cell on the device and returns the number of Picard sweeps of every cell.
   */
  std::vector<int> predictOnDevice(
    const Tables&                         tables,
    const exahype2::dg::tests::CellChain& geometry,
    std::vector<double>&                  cellStates,
    std::vector<double>&                  faceStates,
    std::vector<double>&                  faceFluxes
  ) {
    const exahype2::dg::tests::DeviceBasisTables deviceTables(tables);
    const std::vector<double>                    timeAveragedScratch(
      static_cast<std::size_t>(Setup::NumberOfCells) * StubScratch::TimeAveragedSize,
      0.0
    );
    const std::vector<double> picardScratch(
      static_cast<std::size_t>(Setup::NumberOfCells) * StubScratch::PicardSize,
      0.0
    );
    std::vector<int> sweeps(Setup::NumberOfCells, 0);

    const exahype2::dg::tests::DeviceBuffer<double> deviceCellStates(cellStates);
    const exahype2::dg::tests::DeviceBuffer<double> deviceFaceStates(faceStates);
    const exahype2::dg::tests::DeviceBuffer<double> deviceFaceFluxes(faceFluxes);
    const exahype2::dg::tests::DeviceBuffer<double> deviceTimeAveragedScratch(timeAveragedScratch);
    const exahype2::dg::tests::DeviceBuffer<double> devicePicardScratch(picardScratch);
    const exahype2::dg::tests::DeviceBuffer<int>    deviceSweeps(sweeps);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);

    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(Setup::NumberOfCells),
      PredictorOfCell{
        deviceTables.getView(),
        deviceCellStates.getData(),
        deviceFaceStates.getData(),
        deviceFaceFluxes.getData(),
        deviceTimeAveragedScratch.getData(),
        devicePicardScratch.getData(),
        deviceSweeps.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData()
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
  testMethod(testTheResidualIsTheSquaredDistanceOfTheIterates);
  testMethod(testAnUnboundedIterateReportsDivergence);
  testMethod(testTheGradientOfAConstantVanishes);
  testMethod(testALinearStateHasTheExactGradient);
  testMethod(testTheFluxIsAskedForOncePerNodeAndAxis);
  testMethod(testTheDiffusiveFluxSeesTheNodalGradients);
  testMethod(testEveryTermCombinationAccumulatesIntoOneBuffer);
  testMethod(testTheAverageOfAConstantIsThatConstant);
  testMethod(testTheTargetIsOverwrittenNotAccumulated);
  testMethod(testEveryAxisBlockKeepsCellNodeOrder);
  testMethod(testTermsTheSolverLacksAreLeftAlone);
  testMethod(testTheScratchBlocksFollowEachOtherWithoutOverlap);
  testMethod(testATermTheSolverLacksHasZeroExtent);
  testMethod(testThePredictorWritesTheTimeAveragesAndTheFacesAndLeavesTheCellState);
  testMethod(testEveryCellOfADeviceRangeMatchesTheHostPredictor);
}

/**
 * Every time slice of the space-time state holds the cell state and every padding lane holds zero.
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
 * The right-hand side of the cleared time node holds zeros, the other time nodes keep their values.
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
 * A flux that is constant along every axis leaves the right-hand side where it is.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testAConstantFluxHasNoDivergence() {
  const Tables tables;
  const auto   inverseCellSize = getInverseCellSize();

  // A constant without an exact binary representation, so a plain product leaves rounding behind.
  double spaceTimeFlux[DIMENSIONS * FluxAxisStride];
  for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
    spaceTimeFlux[entry] = 5.0 / 3.0;
  }

  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride];
  for (int entry = 0; entry < CellLayout::SpaceTimeNodes * UnknownStride; entry++) {
    rightHandSide[entry] = 7.0;
  }

  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    Predictor<StubLayout>::subtractFluxDivergence(
      rightHandSide,
      spaceTimeFlux,
      tables.stiffnessOperatorTransposed.data(),
      tables.cellQuadratureWeights.data(),
      inverseCellSize,
      0.25,
      timeNode
    );
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

  // The flux block of an axis grows linearly along that axis alone, so the nodal derivative of the
  // interpolant is the slope itself.
  double spaceTimeFlux[DIMENSIONS * FluxAxisStride] = {};
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
      for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
        const int    spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
        const double reference     = tables.quadraturePoints[peano4::dDelinearised(cellNode, NodesPerAxis)(axis)];
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          spaceTimeFlux[axis * FluxAxisStride + spaceTimeNode * UnknownStride + unknown] =
            (1.0 + unknown) * (2.0 + FluxSlope * (1.0 + axis) * reference);
        }
      }
    }
  }

  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride] = {};

  Predictor<StubLayout>::subtractFluxDivergence(
    rightHandSide,
    spaceTimeFlux,
    tables.stiffnessOperatorTransposed.data(),
    tables.cellQuadratureWeights.data(),
    inverseCellSize,
    TimeStepSize,
    MarkedTimeNode
  );

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
 * A cell without flux and without source solves to the initial state at every time node.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testAPicardSystemWithoutTermsReproducesTheInitialState() {
  const Tables tables;

  // Values without an exact binary representation, so a plain product leaves rounding behind.
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

  double updatedTimeSlice[NodesPerAxis * UnknownStride] = {};

  const double squaredResidual = Predictor<StubLayout>::solvePicardSystem(
    spaceTimeState,
    updatedTimeSlice,
    rightHandSide,
    cellState,
    tables.inversePicardOperator.data(),
    tables.inverseCellQuadratureWeights.data()
  );

  validateEquals(squaredResidual, 0.0);

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
 * The solve returns the squared distance between the new and the previous iterate and stores the new one.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheResidualIsTheSquaredDistanceOfTheIterates() {
  double inversePicardOperator[OperatorEntries];
  fillIdentity(inversePicardOperator);

  double inverseCellQuadratureWeights[CellLayout::NodesPerCell];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    inverseCellQuadratureWeights[cellNode] = 1.0;
  }

  const double cellState[CellStateSize]                                  = {};
  double       spaceTimeState[CellLayout::SpaceTimeNodes * StateStride]  = {};
  double       rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride] = {};
  double       updatedTimeSlice[NodesPerAxis * UnknownStride]            = {};

  double expectedResidual = 0.0;
  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const double newValue = 1.0 + 0.5 * (cellNode + unknown + timeNode);
        rightHandSide[(timeNode * CellLayout::NodesPerCell + cellNode) * UnknownStride + unknown] = newValue;
        expectedResidual += newValue * newValue;
      }
    }
  }

  const double squaredResidual = Predictor<StubLayout>::solvePicardSystem(
    spaceTimeState,
    updatedTimeSlice,
    rightHandSide,
    cellState,
    inversePicardOperator,
    inverseCellQuadratureWeights
  );

  validateNumericalEquals(squaredResidual, expectedResidual);

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

  const double secondResidual = Predictor<StubLayout>::solvePicardSystem(
    spaceTimeState,
    updatedTimeSlice,
    rightHandSide,
    cellState,
    inversePicardOperator,
    inverseCellQuadratureWeights
  );

  validateNumericalEquals(secondResidual, 0.0);
}

/**
 * An iterate above the divergence bound and an iterate that is not a number both return an infinite distance.
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
  double       updatedTimeSlice[NodesPerAxis * UnknownStride]            = {};

  rightHandSide[0]               = 1e30;
  const double unboundedResidual = Predictor<StubLayout>::solvePicardSystem(
    spaceTimeState,
    updatedTimeSlice,
    rightHandSide,
    cellState,
    inversePicardOperator,
    inverseCellQuadratureWeights
  );
  validate(not std::isfinite(unboundedResidual));

  rightHandSide[0]                = std::numeric_limits<double>::quiet_NaN();
  const double notANumberResidual = Predictor<StubLayout>::solvePicardSystem(
    spaceTimeState,
    updatedTimeSlice,
    rightHandSide,
    cellState,
    inversePicardOperator,
    inverseCellQuadratureWeights
  );
  validate(not std::isfinite(notANumberResidual));
}

/**
 * A state that holds one value at every node has a zero gradient along every axis.
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
    tables.derivativeOperator.data(),
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

  // The state grows linearly along every axis, so the nodal derivative of the interpolant is the slope.
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
    tables.derivativeOperator.data(),
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
 * The flux fills one block per axis of one time slice, and the solver sees the node position and the node time.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheFluxIsAskedForOncePerNodeAndAxis() {
  const Tables tables;

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  fillSpaceTimeState(spaceTimeState);

  double stateGradient[DIMENSIONS * GradientAxisStride];
  fillStateGradient(stateGradient);

  double spaceTimeFlux[DIMENSIONS * FluxAxisStride];
  for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
    spaceTimeFlux[entry] = 9.0;
  }

  Predictor<PlainFluxSolver>::evaluateSpaceTimeFlux(
    spaceTimeFlux,
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
      const int    spaceTimeNode = MarkedTimeNode * CellLayout::NodesPerCell + cellNode;
      const double position      = getNodePositionAt(tables, cellNode)(axis);
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const double expected = (1.0 + axis) * getStateValueAt(cellNode, unknown) + position + timePosition;
        validateNumericalEqualsWithParams3(
          spaceTimeFlux[axis * FluxAxisStride + spaceTimeNode * UnknownStride + unknown],
          expected,
          axis,
          cellNode,
          unknown
        );
      }
      for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
        validateNumericalEqualsWithParams3(
          spaceTimeFlux[axis * FluxAxisStride + spaceTimeNode * UnknownStride + unknown],
          0.0,
          axis,
          cellNode,
          unknown
        );
      }
    }
    for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
      if (timeNode != MarkedTimeNode) {
        const int firstEntry = axis * FluxAxisStride + timeNode * CellLayout::NodesPerCell * UnknownStride;
        for (int entry = 0; entry < CellLayout::NodesPerCell * UnknownStride; entry++) {
          validateNumericalEqualsWithParams3(spaceTimeFlux[firstEntry + entry], 9.0, axis, timeNode, entry);
        }
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

  double spaceTimeFlux[DIMENSIONS * FluxAxisStride];
  for (int entry = 0; entry < DIMENSIONS * FluxAxisStride; entry++) {
    spaceTimeFlux[entry] = 9.0;
  }

  Predictor<DiffusiveFluxSolver>::evaluateSpaceTimeFlux(
    spaceTimeFlux,
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
      const int spaceTimeNode = MarkedTimeNode * CellLayout::NodesPerCell + cellNode;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        double expected = (1.0 + axis) * getStateValueAt(cellNode, unknown);
        for (int gradientAxis = 0; gradientAxis < DIMENSIONS; gradientAxis++) {
          expected += (0.5 + gradientAxis) * getGradientValueAt(gradientAxis, cellNode, unknown);
        }
        validateNumericalEqualsWithParams3(
          spaceTimeFlux[axis * FluxAxisStride + spaceTimeNode * UnknownStride + unknown],
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
 * Runs the source evaluation of Derived on one time slice and compares the source buffer and the
 * right-hand side against the terms the flags name.
 */
template <typename Derived>
void
exahype2::dg::ader::tests::NonlinearPredictorTest::validateSourceOf(
  const bool evaluateNonconservativeProduct,
  const bool evaluateSource,
  const bool evaluateDiffusiveSource
) {
  const Tables tables;

  double spaceTimeState[CellLayout::SpaceTimeNodes * StateStride];
  fillSpaceTimeState(spaceTimeState);

  double stateGradient[DIMENSIONS * GradientAxisStride];
  fillStateGradient(stateGradient);

  double spaceTimeSource[CellLayout::SpaceTimeNodes * UnknownStride];
  double rightHandSide[CellLayout::SpaceTimeNodes * UnknownStride];
  for (int entry = 0; entry < CellLayout::SpaceTimeNodes * UnknownStride; entry++) {
    spaceTimeSource[entry] = 9.0;
    rightHandSide[entry]   = 0.5;
  }

  Predictor<Derived>::evaluateSpaceTimeSource(
    spaceTimeSource,
    rightHandSide,
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

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    const int    spaceTimeNode = MarkedTimeNode * CellLayout::NodesPerCell + cellNode;
    const double updateSize =
      tables.quadratureWeights[MarkedTimeNode] * tables.cellQuadratureWeights[cellNode] * TimeStepSize;
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
        spaceTimeSource[spaceTimeNode * UnknownStride + unknown],
        expected,
        cellNode,
        unknown
      );
      validateNumericalEqualsWithParams2(
        rightHandSide[spaceTimeNode * UnknownStride + unknown],
        0.5 + updateSize * expected,
        cellNode,
        unknown
      );
    }
    for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
      validateNumericalEqualsWithParams2(
        spaceTimeSource[spaceTimeNode * UnknownStride + unknown],
        0.0,
        cellNode,
        unknown
      );
      validateNumericalEqualsWithParams2(
        rightHandSide[spaceTimeNode * UnknownStride + unknown],
        0.5,
        cellNode,
        unknown
      );
    }
  }

  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    if (timeNode != MarkedTimeNode) {
      const int firstEntry = timeNode * CellLayout::NodesPerCell * UnknownStride;
      for (int entry = 0; entry < CellLayout::NodesPerCell * UnknownStride; entry++) {
        validateNumericalEqualsWithParams2(spaceTimeSource[firstEntry + entry], 9.0, timeNode, entry);
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
 * A predictor that holds the same value at every time node averages to that value.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheAverageOfAConstantIsThatConstant() {
  const Tables tables;

  SpaceTimeBuffers spaceTime;
  fillConstantInTime(spaceTime);

  AveragedBuffers averaged;
  fillTargets(averaged, 9.0);

  Predictor<FluxSourceSolver>::averageOverTime(
    averaged.state,
    averaged.flux,
    averaged.source,
    spaceTime.state,
    spaceTime.flux,
    spaceTime.source,
    tables.quadratureWeights.data()
  );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < StateStride; value++) {
      validateEqualsWithParams2(
        averaged.state[cellNode * StateStride + value],
        getAveragedStateValueAt(cellNode, value),
        cellNode,
        value
      );
    }
    for (int unknown = 0; unknown < UnknownStride; unknown++) {
      validateEqualsWithParams2(
        averaged.source[cellNode * UnknownStride + unknown],
        getAveragedSourceValueAt(cellNode, unknown),
        cellNode,
        unknown
      );
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        validateEqualsWithParams3(
          averaged.flux[axis * AveragedAxisStride + cellNode * UnknownStride + unknown],
          getAveragedFluxValueAt(axis, cellNode, unknown),
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

/**
 * The average overwrites the state, the flux and the source of the target and adds nothing to them.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheTargetIsOverwrittenNotAccumulated() {
  const Tables           tables;
  const SpaceTimeBuffers spaceTime;

  AveragedBuffers averaged;
  fillTargets(averaged, 9.0);

  Predictor<FluxSourceSolver>::averageOverTime(
    averaged.state,
    averaged.flux,
    averaged.source,
    spaceTime.state,
    spaceTime.flux,
    spaceTime.source,
    tables.quadratureWeights.data()
  );

  for (int entry = 0; entry < CellLayout::NodesPerCell * StateStride; entry++) {
    validateNumericalEqualsWithParams1(averaged.state[entry], 0.0, entry);
  }
  for (int entry = 0; entry < DIMENSIONS * AveragedAxisStride; entry++) {
    validateNumericalEqualsWithParams1(averaged.flux[entry], 0.0, entry);
  }
  for (int entry = 0; entry < AveragedAxisStride; entry++) {
    validateNumericalEqualsWithParams1(averaged.source[entry], 0.0, entry);
  }
}

/**
 * The averaged flux holds one block per axis, and every block runs in cell node order.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testEveryAxisBlockKeepsCellNodeOrder() {
  double weightOfFirstNodeOnly[NodesPerAxis] = {};
  weightOfFirstNodeOnly[0]                   = 1.0;

  SpaceTimeBuffers spaceTime;
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        spaceTime.flux[axis * FluxAxisStride + cellNode * UnknownStride + unknown] =
          getAveragedFluxValueAt(axis, cellNode, unknown);
      }
    }
  }

  AveragedBuffers averaged;
  Predictor<PlainFluxSolver>::averageOverTime(
    averaged.state,
    averaged.flux,
    averaged.source,
    spaceTime.state,
    spaceTime.flux,
    spaceTime.source,
    weightOfFirstNodeOnly
  );

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(
          averaged.flux[axis * AveragedAxisStride + cellNode * UnknownStride + unknown],
          getAveragedFluxValueAt(axis, cellNode, unknown),
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

/**
 * A solver without flux and without source leaves the target buffers of both untouched.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTermsTheSolverLacksAreLeftAlone() {
  const Tables tables;

  SpaceTimeBuffers spaceTime;
  fillConstantInTime(spaceTime);

  AveragedBuffers averaged;
  fillTargets(averaged, 5.0);

  Predictor<InertSolver>::averageOverTime(
    averaged.state,
    averaged.flux,
    averaged.source,
    spaceTime.state,
    spaceTime.flux,
    spaceTime.source,
    tables.quadratureWeights.data()
  );

  for (int entry = 0; entry < DIMENSIONS * AveragedAxisStride; entry++) {
    validateNumericalEqualsWithParams1(averaged.flux[entry], 5.0, entry);
  }
  for (int entry = 0; entry < AveragedAxisStride; entry++) {
    validateNumericalEqualsWithParams1(averaged.source[entry], 5.0, entry);
  }
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < StateStride; value++) {
      validateNumericalEqualsWithParams2(
        averaged.state[cellNode * StateStride + value],
        getAveragedStateValueAt(cellNode, value),
        cellNode,
        value
      );
    }
  }
}

/**
 * The two blocks of the scratch place every buffer behind its predecessor without a gap.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testTheScratchBlocksFollowEachOtherWithoutOverlap() {
  using Scratch = Predictor<FullSolver>::Scratch;

  validateEquals(Scratch::SpaceTimeStateSize, StateStride * CellLayout::SpaceTimeNodes);
  validateEquals(Scratch::RightHandSideSize, UnknownStride * CellLayout::SpaceTimeNodes);
  validateEquals(Scratch::SpaceTimeFluxSize, DIMENSIONS * UnknownStride * CellLayout::SpaceTimeNodes);
  validateEquals(Scratch::SpaceTimeSourceSize, UnknownStride * CellLayout::SpaceTimeNodes);
  validateEquals(Scratch::StateGradientSize, DIMENSIONS * UnknownStride * CellLayout::NodesPerCell);
  validateEquals(Scratch::UpdatedTimeSliceSize, NodesPerAxis * UnknownStride);

  validateEquals(Scratch::SpaceTimeStateOffset, 0);
  validateEquals(Scratch::RightHandSideOffset, Scratch::SpaceTimeStateOffset + Scratch::SpaceTimeStateSize);
  validateEquals(Scratch::SpaceTimeFluxOffset, Scratch::RightHandSideOffset + Scratch::RightHandSideSize);
  validateEquals(Scratch::SpaceTimeSourceOffset, Scratch::SpaceTimeFluxOffset + Scratch::SpaceTimeFluxSize);
  validateEquals(Scratch::StateGradientOffset, Scratch::SpaceTimeSourceOffset + Scratch::SpaceTimeSourceSize);
  validateEquals(Scratch::UpdatedTimeSliceOffset, Scratch::StateGradientOffset + Scratch::StateGradientSize);
  validateEquals(Scratch::PicardSize, Scratch::UpdatedTimeSliceOffset + Scratch::UpdatedTimeSliceSize);

  validateEquals(Scratch::TimeAveragedStateSize, StateStride * CellLayout::NodesPerCell);
  validateEquals(Scratch::TimeAveragedFluxSize, DIMENSIONS * UnknownStride * CellLayout::NodesPerCell);
  validateEquals(Scratch::TimeAveragedSourceSize, UnknownStride * CellLayout::NodesPerCell);

  validateEquals(Scratch::TimeAveragedStateOffset, 0);
  validateEquals(Scratch::TimeAveragedFluxOffset, Scratch::TimeAveragedStateOffset + Scratch::TimeAveragedStateSize);
  validateEquals(Scratch::TimeAveragedSourceOffset, Scratch::TimeAveragedFluxOffset + Scratch::TimeAveragedFluxSize);
  validateEquals(Scratch::TimeAveragedSize, Scratch::TimeAveragedSourceOffset + Scratch::TimeAveragedSourceSize);
}

/**
 * A term the solver lacks occupies no entry and shortens the block it belongs to.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::testATermTheSolverLacksHasZeroExtent() {
  using Full          = Predictor<FullSolver>;
  using Bare          = Predictor<InertSolver>;
  using Product       = Predictor<ProductSolver>;
  using DiffusiveFlux = Predictor<DiffusiveFluxSolver>;

  validate(not Bare::EvaluateFlux);
  validateEquals(Bare::Scratch::SpaceTimeFluxSize, 0);
  validateEquals(Bare::Scratch::SpaceTimeSourceSize, 0);
  validateEquals(Bare::Scratch::StateGradientSize, 0);
  validateEquals(Bare::Scratch::TimeAveragedFluxSize, 0);
  validateEquals(Bare::Scratch::TimeAveragedSourceSize, 0);
  validate(not Bare::AccumulatesSource);
  validate(not Bare::NeedsStateGradient);
  validate(Bare::Scratch::PicardSize < Full::Scratch::PicardSize);
  validate(Bare::Scratch::TimeAveragedSize < Full::Scratch::TimeAveragedSize);

  // A nonconservative product enters the source buffer, so it claims that buffer without a source term.
  validate(Product::AccumulatesSource);
  validate(Product::NeedsStateGradient);
  validate(Product::Scratch::SpaceTimeSourceSize > 0);
  validate(Product::Scratch::StateGradientSize > 0);
  validateEquals(Product::Scratch::SpaceTimeFluxSize, 0);

  validate(DiffusiveFlux::EvaluateFlux);
  validate(DiffusiveFlux::NeedsStateGradient);
  validate(not DiffusiveFlux::AccumulatesSource);
  validate(DiffusiveFlux::Scratch::StateGradientSize > 0);
  validateEquals(DiffusiveFlux::Scratch::SpaceTimeSourceSize, 0);
  validate(DiffusiveFlux::Scratch::SpaceTimeFluxSize > 0);
}

/**
 * The predictor writes the time averages of a linear profile into the scratch and onto the faces
 * and leaves the cell state to the caller bitwise.
 */
void
exahype2::dg::ader::tests::NonlinearPredictorTest::
  testThePredictorWritesTheTimeAveragesAndTheFacesAndLeavesTheCellState() {
  const Tables                                tables;
  const auto                                  cellSize = getCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[CellStateSize];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      cellState[cellNode * NumberOfData + value] = getLinearCellStateEntry(tables, cellNode, value, cellSize(0));
    }
  }

  FaceBuffers faces;
  double      timeAveragedScratch[ProfileScratch::TimeAveragedSize] = {};
  double      picardScratch[ProfileScratch::PicardSize]             = {};

  const int numberOfSweeps = Predictor<ProfileSolver>::predict(
    faces.statePointers,
    faces.fluxPointers,
    cellState,
    timeAveragedScratch,
    picardScratch,
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
        timeAveragedScratch[ProfileScratch::TimeAveragedStateOffset + cellNode * StateStride + value],
        averagedState[cellNode * NumberOfData + value],
        cellNode,
        value
      );
    }
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams2(
        timeAveragedScratch[ProfileScratch::TimeAveragedSourceOffset + cellNode * UnknownStride + unknown],
        sourceAverage,
        cellNode,
        unknown
      );
      // The predictor leaves the derivative of the time-averaged flux along its axis in the scratch.
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        const double fluxDerivative = axis == 0 ? FluxSpeed * Slope : 0.0;
        validateNumericalEqualsWithParams3(
          timeAveragedScratch
            [ProfileScratch::TimeAveragedFluxOffset + axis * AveragedAxisStride + cellNode * UnknownStride + unknown],
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
 * The predictor of every cell of a device range reproduces the serial host loop.
 */
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
  std::vector<double> hostTimeAveragedScratch(
    static_cast<std::size_t>(Setup::NumberOfCells) * StubScratch::TimeAveragedSize,
    0.0
  );
  std::vector<double> hostPicardScratch(static_cast<std::size_t>(Setup::NumberOfCells) * StubScratch::PicardSize, 0.0);
  const exahype2::dg::BasisTables<double> hostTables = tables.getView();
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    double* faceState[peano4::FacesPerCell];
    double* faceFlux[peano4::FacesPerCell];
    collectFacePointers(cell, hostFaceStates.data(), hostFaceFluxes.data(), faceState, faceFlux);
    const int                                   coordinateOffset = cell * DIMENSIONS;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(geometry.cellCentres.data() + coordinateOffset);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(geometry.cellSizes.data() + coordinateOffset);
    const int                                   stateOffset        = cell * Setup::CellStateCardinality;
    const int                                   timeAveragedOffset = cell * StubScratch::TimeAveragedSize;
    const int                                   picardOffset       = cell * StubScratch::PicardSize;
    const int                                   hostSweeps = Predictor<exahype2::dg::ader::tests::StubSolver>::predict(
      faceState,
      faceFlux,
      hostCellStates.data() + stateOffset,
      hostTimeAveragedScratch.data() + timeAveragedOffset,
      hostPicardScratch.data() + picardOffset,
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
