// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LinearPredictorTest.h"

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/ader/LinearPredictor.h"
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
  using Predictor = exahype2::dg::ader::LinearPredictor<Solver, double>;

  TARGET_DECLARE
  constexpr int NodesPerAxis              = CellLayout::NodesPerAxis;
  constexpr int NumberOfUnknowns          = CellLayout::NumberOfUnknowns;
  constexpr int NumberOfData              = CellLayout::NumberOfData;
  constexpr int NumberOfPointSources      = 2;
  constexpr int NumberOfSelectableSources = 3;
  constexpr int StateStride               = CellLayout::PaddedNumberOfData;
  constexpr int UnknownStride             = CellLayout::PaddedNumberOfUnknowns;
  constexpr int AxisStride                = CellLayout::NodesPerCell * UnknownStride;
  constexpr int StateSize                 = CellLayout::NodesPerCell * StateStride;
  constexpr int SampleSize                = (NodesPerAxis + 1) * CellLayout::NodesPerCell * UnknownStride;
  constexpr int CellStateSize             = CellLayout::NodesPerCell * NumberOfData;

  constexpr double TimeStamp    = 0.5;
  constexpr double TimeStepSize = 0.25;
  constexpr double Slope        = 3.0;
  constexpr double MarkerValue  = 7.0;

  constexpr double ProfileSlope       = 0.5;
  constexpr double LinearFluxSpeed    = 0.5;
  constexpr double LinearProductSpeed = 0.25;
  constexpr double LinearSourceRate   = 0.75;

  struct FluxTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         fluxValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] += (1.0 + direction) * state[unknown];
      }
    }
  };

  struct ProductTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateGradient,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         productValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        productValues[unknown] += (1.0 + direction) * stateGradient[unknown];
      }
    }
  };

  struct MaterialTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void multiplyMaterialParameterMatrix(
      [[maybe_unused]] const double* NOALIAS const                  state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] *= 2.0 + direction;
      }
    }
  };

  struct SourceTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         sourceValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        sourceValues[unknown] += 3.0 * state[unknown];
      }
    }
  };

  INLINE_METHOD GPU_CALLABLE_METHOD double getPointSourceForce(
    const int    unknown,
    const int    pointSource,
    const double timePosition,
    const double stateValue
  ) {
    return 1.0 + unknown + pointSource + timePosition + stateValue;
  }

  struct PointSourceTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void pointSource(
      const double* NOALIAS const                  state,
      [[maybe_unused]] const double* NOALIAS const nodePosition,
      const double                                 timeStamp,
      [[maybe_unused]] const double                timeStepSize,
      double* NOALIAS const                        forceVector,
      const int                                    pointSource
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        forceVector[unknown] = getPointSourceForce(unknown, pointSource, timeStamp, state[unknown]);
      }
    }
  };

  struct InertSolver final : StubLayout {};
  struct SourceSolver final : StubLayout, SourceTerm {};
  struct MarkerSolver final : StubLayout, FluxTerm, ProductTerm, MaterialTerm, SourceTerm {};
  struct PointSourceSolver final : StubLayout, PointSourceTerm {};
  struct ProductSolver final : StubLayout, ProductTerm {};
  struct FullSolver final : StubLayout, FluxTerm, ProductTerm, MaterialTerm, SourceTerm, PointSourceTerm {};

  /**
   * A linear system that transports a profile along the first axis and lets it grow.
   */
  struct ProfileSolver final : StubLayout {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] += LinearFluxSpeed * (1.0 + direction) * state[unknown];
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
        values[unknown] += LinearProductSpeed * (1.0 + direction) * stateGradient[unknown];
      }
    }

    static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] += LinearSourceRate * state[unknown];
      }
    }
  };
  TARGET_DECLARE_END

  using MarkerScratch  = Predictor<MarkerSolver>::Scratch;
  using ProfileScratch = Predictor<ProfileSolver>::Scratch;
  using ProductScratch = Predictor<ProductSolver>::Scratch;
  using StubScratch    = Predictor<exahype2::dg::ader::tests::StubSolver>::Scratch;

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

  double getVaryingStateEntry(const int cellNode, const int value) { return 1.0 + 0.125 * (cellNode + value); }

  void fillVaryingState(double (&state)[StateSize]) {
    for (int entry = 0; entry < StateSize; entry++) {
      state[entry] = 0.0;
    }
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        state[cellNode * StateStride + value] = getVaryingStateEntry(cellNode, value);
      }
    }
  }

  void fillStateLinearAlongAxis(const Tables& tables, double (&state)[StateSize], const int markedAxis) {
    for (int entry = 0; entry < StateSize; entry++) {
      state[entry] = 0.0;
    }
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const double reference = tables.quadraturePoints[peano4::dDelinearised(cellNode, NodesPerAxis)(markedAxis)];
      for (int value = 0; value < NumberOfData; value++) {
        state[cellNode * StateStride + value] = 1.0 + value + Slope * reference;
      }
    }
  }

  double getReferenceCoordinate(const int pointSource, const int axis) {
    return (pointSource == 0) ? 0.2 + 0.1 * axis : 0.7 - 0.15 * axis;
  }

  double getTimePositionOf(const Tables& tables, const int timeSample) {
    return (timeSample == 0) ? TimeStamp : TimeStamp + TimeStepSize * tables.quadraturePoints[timeSample - 1];
  }

  struct PointSourceTable final {
    double locations[NumberOfSelectableSources][DIMENSIONS]{};

    void place(const int pointSource, const tarch::la::Vector<DIMENSIONS, double>& location) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        locations[pointSource][axis] = location(axis);
      }
    }

    void placeInside(
      const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      const tarch::la::Vector<DIMENSIONS, double>& cellSize
    ) {
      for (int pointSource = 0; pointSource < NumberOfPointSources; pointSource++) {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          locations[pointSource][axis] =
            cellCentre(axis) + cellSize(axis) * (getReferenceCoordinate(pointSource, axis) - 0.5);
        }
      }
    }

    [[nodiscard]] const double* getData() const { return &locations[0][0]; }
  };

  int selectInCellOfUnitSize(
    const PointSourceTable& table,
    int (&pointSourceIndices)[NumberOfSelectableSources],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre
  ) {
    const tarch::la::Vector<DIMENSIONS, double> cellSize(1.0);
    return Predictor<StubLayout>::selectPointSources(pointSourceIndices, table.getData(), cellCentre, cellSize);
  }

  double getSeedOfState(const int entry) { return 0.25 + 0.125 * entry; }

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

  int getCellNodeOnFace(const int direction, const int faceNode, const int normalIndex) {
    auto nodeIndex       = peano4::dDelinearisedExclusive(faceNode, NodesPerAxis, direction);
    nodeIndex(direction) = normalIndex;
    return peano4::dLinearised(nodeIndex, NodesPerAxis);
  }

  double getBaseValue(const int value) { return 1.0 + 0.25 * value; }

  double getLinearCellStateEntry(const Tables& tables, const int cellNode, const int value, const double cellExtent) {
    return getBaseValue(value)
      + ProfileSlope * cellExtent * tables.quadraturePoints[peano4::dDelinearised(cellNode, NodesPerAxis)(0)];
  }

  /**
   * The Taylor coefficients of the profile system and the factors they enter the time average with.
   */
  struct TaylorCoefficients final {
    double offset[NodesPerAxis][NumberOfUnknowns]{};
    double slope[NodesPerAxis][NumberOfUnknowns]{};
    double factor[NodesPerAxis]{};
  };

  TaylorCoefficients getTaylorCoefficients(const double transportSpeed, const double sourceRate) {
    TaylorCoefficients coefficients;
    coefficients.factor[0] = 1.0;
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      coefficients.offset[0][unknown] = getBaseValue(unknown);
      coefficients.slope[0][unknown]  = ProfileSlope;
    }
    double factor = 1.0;
    for (int order = 1; order < NodesPerAxis; order++) {
      factor *= TimeStepSize / (order + 1);
      coefficients.factor[order] = factor;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        coefficients.offset[order][unknown] = sourceRate * coefficients.offset[order - 1][unknown]
          - transportSpeed * coefficients.slope[order - 1][unknown];
        coefficients.slope[order][unknown] = sourceRate * coefficients.slope[order - 1][unknown];
      }
    }
    return coefficients;
  }

  /**
   * Puts source zero at the centre of cell one and source one on the right face of cell four.
   */
  std::vector<double> getPointSourceLocations(const exahype2::dg::tests::CellChain& geometry) {
    constexpr int SourceZeroCell = 1;
    constexpr int SourceOneCell  = 4;
    constexpr int ZeroOffset     = SourceZeroCell * DIMENSIONS;
    constexpr int OneOffset      = SourceOneCell * DIMENSIONS;

    std::vector<double> locations(Setup::LocationCardinality);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      locations[axis]              = geometry.cellCentres[ZeroOffset + axis];
      locations[DIMENSIONS + axis] = geometry.cellCentres[OneOffset + axis];
    }
    locations[DIMENSIONS] += 0.5 * geometry.cellSizes[OneOffset];
    return locations;
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

  struct SelectPointSourcesOfCell final {
    const double* locations{nullptr};
    const double* cellCentres{nullptr};
    const double* cellSizes{nullptr};
    int*          indices{nullptr};
    int*          counts{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int cell) const {
      const int coordinateOffset = cell * DIMENSIONS;
      const int indexOffset      = cell * Setup::NumberOfPointSources;
      int       pointSourceIndices[Setup::NumberOfPointSources];
      for (int entry = 0; entry < Setup::NumberOfPointSources; entry++) {
        pointSourceIndices[entry] = -1;
      }
      counts[cell] = Predictor<exahype2::dg::ader::tests::StubSolver>::selectPointSources(
        pointSourceIndices,
        locations,
        tarch::la::Vector<DIMENSIONS, double>(cellCentres + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset)
      );
      for (int entry = 0; entry < Setup::NumberOfPointSources; entry++) {
        indices[indexOffset + entry] = pointSourceIndices[entry];
      }
    }
  };

  /**
   * Runs the predictor of the cells of one team. A thread behind the last cell still calls
   * predict() to reach the barriers of the team.
   */
  template <typename StoragePrecision>
  struct LinearPredictorOfCell final {
    exahype2::dg::BasisTables<double> tables{};
    const double*                     locations{nullptr};
    StoragePrecision*                 cellStates{nullptr};
    double*                           faceStates{nullptr};
    double*                           faceFluxes{nullptr};
    double*                           scratch{nullptr};
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
      int                                         pointSourceIndices[Setup::NumberOfPointSources];
      const int numberOfPointSources = Predictor<exahype2::dg::ader::tests::StubSolver>::selectPointSources(
        pointSourceIndices,
        locations,
        cellCentre,
        cellSize
      );
      Predictor<exahype2::dg::ader::tests::StubSolver>::predict(
        faceState,
        faceFlux,
        cellStates + stateOffset,
        scratch + scratchOffset,
        pointSourceIndices,
        numberOfPointSources,
        locations,
        tables,
        cellCentre,
        cellSize,
        Setup::TimeStamp,
        Setup::TimeStepSize,
        workers
      );
    }
  };
  TARGET_DECLARE_END

  void selectPointSourcesOfEveryCell(
    const exahype2::dg::tests::CellChain& geometry,
    const std::vector<double>&            locations,
    std::vector<int>&                     indices,
    std::vector<int>&                     counts
  ) {
    const exahype2::dg::tests::DeviceBuffer<double> deviceLocations(locations);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::tests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::tests::DeviceBuffer<int>    deviceIndices(indices);
    const exahype2::dg::tests::DeviceBuffer<int>    deviceCounts(counts);

    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(Setup::NumberOfCells),
      SelectPointSourcesOfCell{
        deviceLocations.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData(),
        deviceIndices.getData(),
        deviceCounts.getData()
      }
    );

    deviceIndices.copyBack(indices);
    deviceCounts.copyBack(counts);
  }

  template <typename StoragePrecision>
  void predictOnDevice(
    const Tables&                         tables,
    const exahype2::dg::tests::CellChain& geometry,
    const std::vector<double>&            locations,
    std::vector<StoragePrecision>&        cellStates,
    std::vector<double>&                  faceStates,
    std::vector<double>&                  faceFluxes
  ) {
    const exahype2::dg::tests::DeviceBasisTables deviceTables(tables);
    const std::vector<double> scratch(static_cast<std::size_t>(Setup::NumberOfCells) * StubScratch::Size, 0.0);

    const exahype2::dg::tests::DeviceBuffer<double>           deviceLocations(locations);
    const exahype2::dg::tests::DeviceBuffer<StoragePrecision> deviceCellStates(cellStates);
    const exahype2::dg::tests::DeviceBuffer<double>           deviceFaceStates(faceStates);
    const exahype2::dg::tests::DeviceBuffer<double>           deviceFaceFluxes(faceFluxes);
    const exahype2::dg::tests::DeviceBuffer<double>           deviceScratch(scratch);
    const exahype2::dg::tests::DeviceBuffer<double>           deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::tests::DeviceBuffer<double>           deviceCellSizes(geometry.cellSizes);

    const int threadsPerTeam = tarch::accelerator::getTeamThreads(CellLayout::NodesPerCell);
    const int threadsPerCell = std::min(CellLayout::NodesPerCell, threadsPerTeam);
    const int cellsPerTeam   = threadsPerTeam / threadsPerCell;
    tarch::accelerator::parallelFor(
      tarch::accelerator::TeamRange((Setup::NumberOfCells + cellsPerTeam - 1) / cellsPerTeam, threadsPerTeam, 0),
      LinearPredictorOfCell<StoragePrecision>{
        deviceTables.getView(),
        deviceLocations.getData(),
        deviceCellStates.getData(),
        deviceFaceStates.getData(),
        deviceFaceFluxes.getData(),
        deviceScratch.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData(),
        threadsPerCell
      }
    );

    deviceCellStates.copyBack(cellStates);
    deviceFaceStates.copyBack(faceStates);
    deviceFaceFluxes.copyBack(faceFluxes);
  }
} // namespace

exahype2::dg::ader::tests::LinearPredictorTest::LinearPredictorTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::LinearPredictorTest") {}

void
exahype2::dg::ader::tests::LinearPredictorTest::run() {
  testMethod(testTheSeedCopiesTheCellStateAndClearsThePadding);
  testMethod(testTheTimeAverageStartsFromTheState);
  testMethod(testTheFluxIsClearedBeforeATaylorStep);
  testMethod(testTheLastPassEvaluatesTheTermsOfTheTimeAveragedState);
  testMethod(testTheTimeAveragedStateGrowsByTheGivenFactor);
  testMethod(testTheNextDerivativeIsTheSourceMinusTheFluxDivergence);
  testMethod(testTheFluxFillsEveryNodeAndAxis);
  testMethod(testAConstantStateLeavesTheNonconservativeProductAtZero);
  testMethod(testALinearStateGivesTheExactNonconservativeProduct);
  testMethod(testTheMaterialParameterMatrixActsOnEveryAxis);
  testMethod(testTheSourceOverwritesTheUnknownsOfEveryNode);
  testMethod(testASourceInsideIsSelected);
  testMethod(testASourceOutsideIsRejected);
  testMethod(testASourceOnAFaceBelongsToBothCells);
  testMethod(testSelectionKeepsTheTableOrder);
  testMethod(testTheBasisFunctionIsOneAtItsOwnNodeAndZeroAtTheOthers);
  testMethod(testTheBasisFunctionsReproduceALinearFunction);
  testMethod(testNoPointSourceLeavesTheSamplesEmpty);
  testMethod(testASourceIsSpreadByTheBasisAndScaledByTheWeightAndVolume);
  testMethod(testTheTimeIntegralSkipsTheSampleAtTheStart);
  testMethod(testTheDerivativeCoefficientsWeightTheUnknownsOnly);
  testMethod(testEveryAxisReceivesTheSameTimeAverage);
  testMethod(testTheScratchBlocksFollowEachOtherWithoutOverlap);
  testMethod(testATermTheSolverLacksHasZeroExtent);
  testMethod(testThePredictorWritesTheTimeAveragesAndTheFacesAndLeavesTheCellState);
  testMethod(testASolverWithoutFluxGetsZeroFaceFluxes);
  testMethod(testEveryCellOfADeviceRangeSelectsItsPointSources);
  testMethod(testEveryCellOfADeviceRangeMatchesTheHostPredictor<double>);
  testMethod(testEveryCellOfADeviceRangeMatchesTheHostPredictor<float>);
}

/**
 * The zeroth Taylor coefficient is the cell state with zero padding lanes.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testTheSeedCopiesTheCellStateAndClearsThePadding() {
  double cellState[CellStateSize];
  for (int entry = 0; entry < CellStateSize; entry++) {
    cellState[entry] = 1.0 + 0.125 * entry;
  }

  double state[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    state[entry] = MarkerValue;
  }

  Predictor<StubLayout>::initialiseState(state, cellState);

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      validateNumericalEqualsWithParams2(
        state[cellNode * StateStride + value],
        cellState[cellNode * NumberOfData + value],
        cellNode,
        value
      );
    }
    for (int value = NumberOfData; value < StateStride; value++) {
      validateNumericalEqualsWithParams2(state[cellNode * StateStride + value], 0.0, cellNode, value);
    }
  }
}

/**
 * The time average starts at the zeroth Taylor coefficient.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testTheTimeAverageStartsFromTheState() {
  double state[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    state[entry] = 1.0 + 0.5 * entry;
  }

  double timeAveragedState[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    timeAveragedState[entry] = MarkerValue;
  }

  Predictor<StubLayout>::seedTimeAveragedState(timeAveragedState, state);

  for (int entry = 0; entry < StateSize; entry++) {
    validateNumericalEqualsWithParams1(timeAveragedState[entry], state[entry], entry);
  }
}

void
exahype2::dg::ader::tests::LinearPredictorTest::testTheFluxIsClearedBeforeATaylorStep() {
  double flux[DIMENSIONS * AxisStride];
  for (int entry = 0; entry < DIMENSIONS * AxisStride; entry++) {
    flux[entry] = MarkerValue;
  }

  Predictor<StubLayout>::clearFluxes(flux);

  for (int entry = 0; entry < DIMENSIONS * AxisStride; entry++) {
    validateNumericalEqualsWithParams1(flux[entry], 0.0, entry);
  }
}

/**
 * After predict(), the flux block stores the terms of the time-averaged state.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testTheLastPassEvaluatesTheTermsOfTheTimeAveragedState() {
  const Tables                                tables;
  const auto                                  cellSize        = getCellSize();
  const auto                                  inverseCellSize = getInverseCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[CellStateSize];
  for (int entry = 0; entry < CellStateSize; entry++) {
    cellState[entry] = 1.0 + 0.125 * entry;
  }

  FaceBuffers faces;
  double      scratch[MarkerScratch::Size] = {};

  Predictor<MarkerSolver>::predict(
    faces.statePointers,
    faces.fluxPointers,
    cellState,
    scratch,
    nullptr,
    0,
    nullptr,
    tables.getView(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  const double* const timeAveragedState = scratch + MarkerScratch::TimeAveragedStateOffset;

  double flux[DIMENSIONS * AxisStride] = {};
  Predictor<MarkerSolver>::evaluateFlux(flux, timeAveragedState, cellCentre, cellSize, TimeStamp, TimeStepSize);
  exahype2::dg::differentiateFluxInPlace<MarkerSolver>(flux, tables.derivativeOperator.data(), inverseCellSize);
  Predictor<MarkerSolver>::addNonconservativeProduct(
    flux,
    timeAveragedState,
    tables.derivativeOperator.data(),
    inverseCellSize,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );
  Predictor<MarkerSolver>::applyMaterialParameterMatrix(
    flux,
    timeAveragedState,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  for (int entry = 0; entry < DIMENSIONS * AxisStride; entry++) {
    validateNumericalEqualsWithParams1(scratch[MarkerScratch::FluxOffset + entry], flux[entry], entry);
  }
}

/**
 * A Taylor coefficient adds its unknowns to the time-averaged state and leaves the auxiliary
 * variables unchanged.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testTheTimeAveragedStateGrowsByTheGivenFactor() {
  double state[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    state[entry] = 1.0 + 0.125 * entry;
  }

  double timeAveragedState[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    timeAveragedState[entry] = MarkerValue;
  }

  Predictor<StubLayout>::accumulateTimeAveragedState(timeAveragedState, state, 0.5);

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const int entry = cellNode * StateStride + unknown;
      validateNumericalEqualsWithParams2(timeAveragedState[entry], MarkerValue + 0.5 * state[entry], cellNode, unknown);
    }
    for (int value = NumberOfUnknowns; value < StateStride; value++) {
      const int entry = cellNode * StateStride + value;
      validateNumericalEqualsWithParams2(timeAveragedState[entry], MarkerValue, cellNode, value);
    }
  }
}

/**
 * The next Taylor coefficient is the source minus the flux divergence, or the negative divergence
 * without a source.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testTheNextDerivativeIsTheSourceMinusTheFluxDivergence() {
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);

  double flux[DIMENSIONS * AxisStride] = {};
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        flux[axis * AxisStride + cellNode * UnknownStride + unknown] = 1.0 + axis;
      }
    }
  }

  double sumOverAxes = 0.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    sumOverAxes += 1.0 + axis;
  }

  double initialState[StateSize];
  fillVaryingState(initialState);

  double withSource[StateSize];
  fillVaryingState(withSource);
  double withoutSource[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    withoutSource[entry] = MarkerValue;
  }

  Predictor<SourceSolver>::overwriteStateWithLinearSource(withSource, cellCentre, cellSize, TimeStamp, TimeStepSize);
  Predictor<SourceSolver>::addFluxesToTimeDerivative(withSource, flux);
  Predictor<InertSolver>::addFluxesToTimeDerivative(withoutSource, flux);

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const int entry = cellNode * StateStride + unknown;
      validateNumericalEqualsWithParams2(withSource[entry], 3.0 * initialState[entry] - sumOverAxes, cellNode, unknown);
      validateNumericalEqualsWithParams2(withoutSource[entry], -sumOverAxes, cellNode, unknown);
    }
    for (int value = NumberOfUnknowns; value < StateStride; value++) {
      const int entry = cellNode * StateStride + value;
      validateNumericalEqualsWithParams2(withSource[entry], initialState[entry], cellNode, value);
      validateNumericalEqualsWithParams2(withoutSource[entry], MarkerValue, cellNode, value);
    }
  }
}

/**
 * evaluateFlux() writes the flux of every node into the block of its axis.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testTheFluxFillsEveryNodeAndAxis() {
  double state[StateSize];
  fillVaryingState(state);

  double flux[DIMENSIONS * AxisStride] = {};

  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);

  Predictor<MarkerSolver>::evaluateFlux(flux, state, cellCentre, cellSize, TimeStamp, TimeStepSize);

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int base = axis * AxisStride + cellNode * UnknownStride;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(
          flux[base + unknown],
          (1.0 + axis) * getVaryingStateEntry(cellNode, unknown),
          axis,
          cellNode,
          unknown
        );
      }
      for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
        validateNumericalEqualsWithParams3(flux[base + unknown], 0.0, axis, cellNode, unknown);
      }
    }
  }
}

/**
 * A constant state adds an exactly zero nonconservative product.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testAConstantStateLeavesTheNonconservativeProductAtZero() {
  const Tables tables;

  // A rounding error shows for a constant without an exact binary representation.
  double state[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    state[entry] = 4.0 / 3.0;
  }

  double flux[DIMENSIONS * AxisStride] = {};

  const auto                                  inverseCellSize = getInverseCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);

  Predictor<MarkerSolver>::addNonconservativeProduct(
    flux,
    state,
    tables.derivativeOperator.data(),
    inverseCellSize,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  for (int entry = 0; entry < DIMENSIONS * AxisStride; entry++) {
    validateEqualsWithParams1(flux[entry], 0.0, entry);
  }
}

/**
 * A state that is linear along one axis adds the product of its exact slope to the flux of that axis alone.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testALinearStateGivesTheExactNonconservativeProduct() {
  const Tables tables;

  const auto                                  inverseCellSize = getInverseCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);

  for (int markedAxis = 0; markedAxis < DIMENSIONS; markedAxis++) {
    double state[StateSize];
    fillStateLinearAlongAxis(tables, state, markedAxis);

    double flux[DIMENSIONS * AxisStride] = {};

    Predictor<MarkerSolver>::addNonconservativeProduct(
      flux,
      state,
      tables.derivativeOperator.data(),
      inverseCellSize,
      cellCentre,
      cellSize,
      TimeStamp,
      TimeStepSize
    );

    for (int axis = 0; axis < DIMENSIONS; axis++) {
      const double expected = (axis == markedAxis) ? (1.0 + axis) * Slope * inverseCellSize(axis) : 0.0;
      for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
        const int base = axis * AxisStride + cellNode * UnknownStride;
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          validateNumericalEqualsWithParams3(flux[base + unknown], expected, markedAxis, axis, cellNode);
        }
        for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
          validateNumericalEqualsWithParams3(flux[base + unknown], 0.0, markedAxis, axis, cellNode);
        }
      }
    }
  }
}

/**
 * The material parameter matrix scales the unknowns of every node and axis and leaves the padding
 * lanes unchanged.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testTheMaterialParameterMatrixActsOnEveryAxis() {
  double state[StateSize];
  fillVaryingState(state);

  double flux[DIMENSIONS * AxisStride];
  for (int entry = 0; entry < DIMENSIONS * AxisStride; entry++) {
    flux[entry] = 1.0 + 0.125 * entry;
  }

  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);

  Predictor<MarkerSolver>::applyMaterialParameterMatrix(flux, state, cellCentre, cellSize, TimeStamp, TimeStepSize);

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int base = axis * AxisStride + cellNode * UnknownStride;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(
          flux[base + unknown],
          (2.0 + axis) * (1.0 + 0.125 * (base + unknown)),
          axis,
          cellNode,
          unknown
        );
      }
      for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
        validateNumericalEqualsWithParams3(
          flux[base + unknown],
          1.0 + 0.125 * (base + unknown),
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

/**
 * The source overwrites the unknowns of every node and leaves the auxiliary variables and the
 * padding lanes unchanged.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testTheSourceOverwritesTheUnknownsOfEveryNode() {
  double state[StateSize];
  fillVaryingState(state);

  double initialState[StateSize];
  fillVaryingState(initialState);

  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);

  Predictor<MarkerSolver>::overwriteStateWithLinearSource(state, cellCentre, cellSize, TimeStamp, TimeStepSize);

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    const int base = cellNode * StateStride;
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams2(
        state[base + unknown],
        3.0 * getVaryingStateEntry(cellNode, unknown),
        cellNode,
        unknown
      );
    }
    for (int value = NumberOfUnknowns; value < StateStride; value++) {
      validateNumericalEqualsWithParams2(state[base + value], initialState[base + value], cellNode, value);
    }
  }
}

/**
 * A point source inside the cell raises the count by one and writes its table index.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testASourceInsideIsSelected() {
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.5);
  PointSourceTable                            table;
  table.place(0, cellCentre);
  table.place(1, tarch::la::Vector<DIMENSIONS, double>(5.0));
  table.place(2, tarch::la::Vector<DIMENSIONS, double>(-5.0));

  int       pointSourceIndices[NumberOfSelectableSources] = {-1, -1, -1};
  const int count = selectInCellOfUnitSize(table, pointSourceIndices, cellCentre);

  validateEquals(count, 1);
  validateEquals(pointSourceIndices[0], 0);
  validateEquals(pointSourceIndices[1], -1);
  validateEquals(pointSourceIndices[2], -1);
}

/**
 * The selection skips a point source outside the cell along one axis.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testASourceOutsideIsRejected() {
  tarch::la::Vector<DIMENSIONS, double> beyondTheLastAxis(0.5);
  beyondTheLastAxis(DIMENSIONS - 1) = 1.5;
  PointSourceTable table;
  table.place(0, beyondTheLastAxis);
  table.place(1, tarch::la::Vector<DIMENSIONS, double>(-0.5));
  table.place(2, tarch::la::Vector<DIMENSIONS, double>(5.0));

  int       pointSourceIndices[NumberOfSelectableSources] = {-1, -1, -1};
  const int count = selectInCellOfUnitSize(table, pointSourceIndices, tarch::la::Vector<DIMENSIONS, double>(0.5));

  validateEquals(count, 0);
  for (int entry = 0; entry < NumberOfSelectableSources; entry++) {
    validateEqualsWithParams1(pointSourceIndices[entry], -1, entry);
  }
}

void
exahype2::dg::ader::tests::LinearPredictorTest::testASourceOnAFaceBelongsToBothCells() {
  PointSourceTable table;
  table.place(0, tarch::la::Vector<DIMENSIONS, double>(0.0));
  table.place(1, tarch::la::Vector<DIMENSIONS, double>(1.0));
  table.place(2, tarch::la::Vector<DIMENSIONS, double>(5.0));

  int       lowerIndices[NumberOfSelectableSources] = {-1, -1, -1};
  const int lowerCount = selectInCellOfUnitSize(table, lowerIndices, tarch::la::Vector<DIMENSIONS, double>(0.5));
  int       upperIndices[NumberOfSelectableSources] = {-1, -1, -1};
  const int upperCount = selectInCellOfUnitSize(table, upperIndices, tarch::la::Vector<DIMENSIONS, double>(1.5));

  validateEquals(lowerCount, 2);
  validateEquals(lowerIndices[0], 0);
  validateEquals(lowerIndices[1], 1);
  validateEquals(upperCount, 1);
  validateEquals(upperIndices[0], 1);
}

/**
 * The indices follow the order of the point source table of the solver.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testSelectionKeepsTheTableOrder() {
  PointSourceTable table;
  table.place(0, tarch::la::Vector<DIMENSIONS, double>(0.25));
  table.place(1, tarch::la::Vector<DIMENSIONS, double>(5.0));
  table.place(2, tarch::la::Vector<DIMENSIONS, double>(0.75));

  int       pointSourceIndices[NumberOfSelectableSources] = {-1, -1, -1};
  const int count = selectInCellOfUnitSize(table, pointSourceIndices, tarch::la::Vector<DIMENSIONS, double>(0.5));

  validateEquals(count, 2);
  validateEquals(pointSourceIndices[0], 0);
  validateEquals(pointSourceIndices[1], 2);
  validateEquals(pointSourceIndices[2], -1);
}

void
exahype2::dg::ader::tests::LinearPredictorTest::testTheBasisFunctionIsOneAtItsOwnNodeAndZeroAtTheOthers() {
  const Tables tables;
  for (int index = 0; index < NodesPerAxis; index++) {
    for (int node = 0; node < NodesPerAxis; node++) {
      const double value = Predictor<StubLayout>::evaluateLagrangeBasis(
        tables.quadraturePoints.data(),
        index,
        tables.quadraturePoints[node]
      );
      const double expected = (index == node) ? 1.0 : 0.0;
      validateNumericalEqualsWithParams2(value, expected, index, node);
    }
  }
}

void
exahype2::dg::ader::tests::LinearPredictorTest::testTheBasisFunctionsReproduceALinearFunction() {
  const Tables tables;
  for (int sample = 0; sample <= 8; sample++) {
    const double position = 0.125 * sample;
    double       constant = 0.0;
    double       linear   = 0.0;
    for (int index = 0; index < NodesPerAxis; index++) {
      const double value =
        Predictor<StubLayout>::evaluateLagrangeBasis(tables.quadraturePoints.data(), index, position);
      constant += value;
      linear += value * tables.quadraturePoints[index];
    }
    validateNumericalEqualsWithParams1(constant, 1.0, sample);
    validateNumericalEqualsWithParams1(linear, position, sample);
  }
}

/**
 * An empty list of point sources leaves every sample at zero.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testNoPointSourceLeavesTheSamplesEmpty() {
  const Tables                                tables;
  const auto                                  cellSize = getCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  PointSourceTable                            table;
  table.placeInside(cellCentre, cellSize);

  const double state[StateSize]                         = {};
  const int    pointSourceIndices[NumberOfPointSources] = {0, 1};

  double pointSourceValues[SampleSize];
  for (int entry = 0; entry < SampleSize; entry++) {
    pointSourceValues[entry] = 9.0;
  }

  Predictor<PointSourceSolver>::evaluatePointSources(
    pointSourceValues,
    state,
    pointSourceIndices,
    0,
    table.getData(),
    tables.quadraturePoints.data(),
    tables.inverseCellQuadratureWeights.data(),
    getInverseCellSize(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  for (int entry = 0; entry < SampleSize; entry++) {
    validateNumericalEqualsWithParams1(pointSourceValues[entry], 0.0, entry);
  }
}

/**
 * Every node receives the force of each listed source times the basis functions at the source,
 * divided by the quadrature weight of the node and the cell volume.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testASourceIsSpreadByTheBasisAndScaledByTheWeightAndVolume() {
  const Tables                                tables;
  const auto                                  cellSize = getCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  PointSourceTable                            table;
  table.placeInside(cellCentre, cellSize);

  double cellVolume = 1.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    cellVolume *= cellSize(axis);
  }

  double state[StateSize] = {};
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      state[cellNode * StateStride + value] = 0.5 + 0.01 * (cellNode * NumberOfData + value);
    }
  }

  const int pointSourceIndices[NumberOfPointSources] = {1, 0};

  double pointSourceValues[SampleSize];
  for (int entry = 0; entry < SampleSize; entry++) {
    pointSourceValues[entry] = 9.0;
  }

  Predictor<PointSourceSolver>::evaluatePointSources(
    pointSourceValues,
    state,
    pointSourceIndices,
    NumberOfPointSources,
    table.getData(),
    tables.quadraturePoints.data(),
    tables.inverseCellQuadratureWeights.data(),
    getInverseCellSize(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  for (int timeSample = 0; timeSample < NodesPerAxis + 1; timeSample++) {
    const double timePosition = getTimePositionOf(tables, timeSample);
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const auto nodeIndex = peano4::dDelinearised(cellNode, NodesPerAxis);
      for (int unknown = 0; unknown < UnknownStride; unknown++) {
        double expected = 0.0;
        if (unknown < NumberOfUnknowns) {
          for (int entry = 0; entry < NumberOfPointSources; entry++) {
            const int pointSource = pointSourceIndices[entry];
            double    spread      = 1.0;
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              spread *= Predictor<StubLayout>::evaluateLagrangeBasis(
                tables.quadraturePoints.data(),
                nodeIndex(axis),
                getReferenceCoordinate(pointSource, axis)
              );
            }
            const double stateValue = state[cellNode * StateStride + unknown];
            expected += spread * tables.inverseCellQuadratureWeights[cellNode] / cellVolume
              * getPointSourceForce(unknown, pointSource, timePosition, stateValue);
          }
        }
        validateNumericalEqualsWithParams3(
          pointSourceValues[(timeSample * CellLayout::NodesPerCell + cellNode) * UnknownStride + unknown],
          expected,
          timeSample,
          cellNode,
          unknown
        );
      }
    }
  }
}

void
exahype2::dg::ader::tests::LinearPredictorTest::testTheTimeIntegralSkipsTheSampleAtTheStart() {
  const Tables tables;

  double pointSourceValues[SampleSize];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < UnknownStride; unknown++) {
      pointSourceValues[cellNode * UnknownStride + unknown] = 1000.0;
    }
  }
  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < UnknownStride; unknown++) {
        pointSourceValues[((timeNode + 1) * CellLayout::NodesPerCell + cellNode) * UnknownStride + unknown] =
          1.0 + unknown + timeNode + 0.25 * cellNode;
      }
    }
  }

  double state[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    state[entry] = getSeedOfState(entry);
  }

  Predictor<StubLayout>::addPointSourcesToState(
    state,
    pointSourceValues,
    tables.quadratureWeights.data(),
    TimeStepSize
  );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < StateStride; value++) {
      const int entry    = cellNode * StateStride + value;
      double    expected = getSeedOfState(entry);
      if (value < NumberOfUnknowns) {
        for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
          expected += TimeStepSize * tables.quadratureWeights[timeNode] * (1.0 + value + timeNode + 0.25 * cellNode);
        }
      }
      validateNumericalEqualsWithParams2(state[entry], expected, cellNode, value);
    }
  }
}

/**
 * The derivative coefficients weight the samples into the unknowns and leave the auxiliary
 * variables and the padding lanes unchanged.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testTheDerivativeCoefficientsWeightTheUnknownsOnly() {
  double derivativeCoefficients[NodesPerAxis];
  double coefficientSum = 0.0;
  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    derivativeCoefficients[timeNode] = 0.5 + timeNode;
    coefficientSum += derivativeCoefficients[timeNode];
  }

  // Every time sample of a node stores the same value. The expected sum is coefficientSum times that
  // value.
  double pointSourceValues[SampleSize];
  for (int timeSample = 0; timeSample < NodesPerAxis + 1; timeSample++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < UnknownStride; unknown++) {
        pointSourceValues[(timeSample * CellLayout::NodesPerCell + cellNode) * UnknownStride + unknown] =
          (unknown < NumberOfUnknowns) ? (1.0 + unknown + 0.25 * cellNode) : 100.0;
      }
    }
  }

  double state[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    state[entry] = getSeedOfState(entry);
  }

  Predictor<StubLayout>::addPointSourceDerivativeToState(state, pointSourceValues, derivativeCoefficients);

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < StateStride; value++) {
      const int entry    = cellNode * StateStride + value;
      double    expected = getSeedOfState(entry);
      if (value < NumberOfUnknowns) {
        expected += coefficientSum * (1.0 + value + 0.25 * cellNode);
      }
      validateNumericalEqualsWithParams2(state[entry], expected, cellNode, value);
    }
  }
}

/**
 * Every axis of the time-averaged flux receives the same time integral divided by the number of axes.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testEveryAxisReceivesTheSameTimeAverage() {
  const Tables tables;

  double pointSourceValues[SampleSize];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < UnknownStride; unknown++) {
      pointSourceValues[cellNode * UnknownStride + unknown] = 1000.0;
    }
  }
  for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < UnknownStride; unknown++) {
        pointSourceValues[((timeNode + 1) * CellLayout::NodesPerCell + cellNode) * UnknownStride + unknown] =
          1.0 + unknown + timeNode + 0.25 * cellNode;
      }
    }
  }

  double timeAveragedFlux[DIMENSIONS * AxisStride];
  for (int entry = 0; entry < DIMENSIONS * AxisStride; entry++) {
    timeAveragedFlux[entry] = 0.05 * entry;
  }

  Predictor<StubLayout>::addPointSourcesToTimeAveragedFlux(
    timeAveragedFlux,
    pointSourceValues,
    tables.quadratureWeights.data()
  );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < UnknownStride; unknown++) {
      double timeAverage = 0.0;
      for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
        timeAverage += tables.quadratureWeights[timeNode] / DIMENSIONS * (1.0 + unknown + timeNode + 0.25 * cellNode);
      }
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        const int entry = axis * AxisStride + cellNode * UnknownStride + unknown;
        validateNumericalEqualsWithParams3(
          timeAveragedFlux[entry],
          0.05 * entry + timeAverage,
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

void
exahype2::dg::ader::tests::LinearPredictorTest::testTheScratchBlocksFollowEachOtherWithoutOverlap() {
  using Scratch = Predictor<FullSolver>::Scratch;

  validateEquals(Scratch::StateSize, StateStride * CellLayout::NodesPerCell);
  validateEquals(Scratch::TimeAveragedStateSize, StateStride * CellLayout::NodesPerCell);
  validateEquals(Scratch::FluxSize, DIMENSIONS * UnknownStride * CellLayout::NodesPerCell);
  validateEquals(Scratch::PointSourceSize, (NodesPerAxis + 1) * CellLayout::NodesPerCell * UnknownStride);

  validateEquals(Scratch::StateOffset, 0);
  validateEquals(Scratch::TimeAveragedStateOffset, Scratch::StateOffset + Scratch::StateSize);
  validateEquals(Scratch::FluxOffset, Scratch::TimeAveragedStateOffset + Scratch::TimeAveragedStateSize);
  validateEquals(Scratch::PointSourceOffset, Scratch::FluxOffset + Scratch::FluxSize);
  validateEquals(Scratch::Size, Scratch::PointSourceOffset + Scratch::PointSourceSize);
}

/**
 * The scratch block of a solver reserves zero entries for a term that the solver does not declare.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testATermTheSolverLacksHasZeroExtent() {
  using Full = Predictor<FullSolver>;
  using Bare = Predictor<InertSolver>;

  validate(Full::EvaluateFlux);
  validate(Full::EvaluateNonconservativeProduct);
  validate(Full::EvaluateSource);
  validate(Full::EvaluateMaterialParameters);
  validate(Full::EvaluatePointSources);
  validate(Full::AccumulatesFlux);
  validate(Full::AccumulatesSource);

  validate(not Bare::EvaluateFlux);
  validate(not Bare::AccumulatesFlux);
  validate(not Bare::AccumulatesSource);
  validateEquals(Bare::Scratch::PointSourceSize, 0);
  validate(Bare::Scratch::Size < Full::Scratch::Size);
  validateEquals(Bare::Scratch::Size, Bare::Scratch::PointSourceOffset);
}

/**
 * The predictor writes the exact time averages of a linear profile into the scratch and onto the
 * faces and leaves the cell state unchanged bitwise.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::
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
  double      scratch[ProfileScratch::Size] = {};

  Predictor<ProfileSolver>::predict(
    faces.statePointers,
    faces.fluxPointers,
    cellState,
    scratch,
    nullptr,
    0,
    nullptr,
    tables.getView(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

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

  const double             transportSpeed = LinearFluxSpeed + LinearProductSpeed;
  const TaylorCoefficients coefficients   = getTaylorCoefficients(transportSpeed, LinearSourceRate);

  double averagedFlux[NumberOfUnknowns] = {};
  for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
    for (int order = 0; order < NodesPerAxis; order++) {
      averagedFlux[unknown] += coefficients.factor[order] * transportSpeed * coefficients.slope[order][unknown];
    }
  }

  double averagedState[CellStateSize];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    const double reference = tables.quadraturePoints[peano4::dDelinearised(cellNode, NodesPerAxis)(0)];
    for (int value = 0; value < NumberOfData; value++) {
      double average = getLinearCellStateEntry(tables, cellNode, value, cellSize(0));
      if (value < NumberOfUnknowns) {
        average = 0.0;
        for (int order = 0; order < NodesPerAxis; order++) {
          average += coefficients.factor[order]
            * (coefficients.offset[order][value] + coefficients.slope[order][value] * cellSize(0) * reference);
        }
      }
      averagedState[cellNode * NumberOfData + value] = average;
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
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        const double expectedFlux = (axis == 0) ? averagedFlux[unknown] : 0.0;
        validateNumericalEqualsWithParams3(
          scratch[ProfileScratch::FluxOffset + axis * AxisStride + cellNode * UnknownStride + unknown],
          expectedFlux,
          axis,
          cellNode,
          unknown
        );
      }
    }
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    const double fluxSpeed = LinearFluxSpeed * (1.0 + direction);
    for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
      double lower[NumberOfData] = {};
      double upper[NumberOfData] = {};
      for (int value = 0; value < NumberOfData; value++) {
        for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
          const double nodeValue =
            averagedState[getCellNodeOnFace(direction, faceNode, normalIndex) * NumberOfData + value];
          lower[value] += tables.leftFaceCoefficients[normalIndex] * nodeValue;
          upper[value] += tables.rightFaceCoefficients[normalIndex] * nodeValue;
        }
        const int entry = faceNode * NumberOfData + value;
        validateNumericalEqualsWithParams3(faces.state[direction][entry], lower[value], direction, faceNode, value);
        validateNumericalEqualsWithParams3(
          faces.state[direction + DIMENSIONS][entry],
          upper[value],
          direction,
          faceNode,
          value
        );
      }
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const int entry = faceNode * NumberOfUnknowns + unknown;
        validateNumericalEqualsWithParams3(
          faces.flux[direction][entry],
          fluxSpeed * lower[unknown],
          direction,
          faceNode,
          unknown
        );
        validateNumericalEqualsWithParams3(
          faces.flux[direction + DIMENSIONS][entry],
          fluxSpeed * upper[unknown],
          direction,
          faceNode,
          unknown
        );
      }
    }
  }
}

void
exahype2::dg::ader::tests::LinearPredictorTest::testASolverWithoutFluxGetsZeroFaceFluxes() {
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
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfUnknowns; entry++) {
      faces.flux[face][entry] = MarkerValue;
    }
  }
  double scratch[ProductScratch::Size] = {};

  validate(not Predictor<ProductSolver>::EvaluateFlux);
  validate(Predictor<ProductSolver>::EvaluateNonconservativeProduct);
  Predictor<ProductSolver>::predict(
    faces.statePointers,
    faces.fluxPointers,
    cellState,
    scratch,
    nullptr,
    0,
    nullptr,
    tables.getView(),
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfUnknowns; entry++) {
      validateEqualsWithParams2(faces.flux[face][entry], 0.0, face, entry);
    }
  }
}

/**
 * The point source selection of every cell of a device range reproduces the serial host loop.
 */
void
exahype2::dg::ader::tests::LinearPredictorTest::testEveryCellOfADeviceRangeSelectsItsPointSources() {
  const exahype2::dg::tests::CellChain geometry;
  const std::vector<double>            locations = getPointSourceLocations(geometry);
  std::vector<int>                     deviceIndicesResult(Setup::AllIndicesCardinality, -1);
  std::vector<int>                     deviceCountsResult(Setup::NumberOfCells, -1);
  selectPointSourcesOfEveryCell(geometry, locations, deviceIndicesResult, deviceCountsResult);

  int selectedSources = 0;
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    const int                                   coordinateOffset = cell * DIMENSIONS;
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(geometry.cellCentres.data() + coordinateOffset);
    const tarch::la::Vector<DIMENSIONS, double> cellSize(geometry.cellSizes.data() + coordinateOffset);
    int                                         pointSourceIndices[Setup::NumberOfPointSources];
    for (int entry = 0; entry < Setup::NumberOfPointSources; entry++) {
      pointSourceIndices[entry] = -1;
    }
    const int count = Predictor<exahype2::dg::ader::tests::StubSolver>::selectPointSources(
      pointSourceIndices,
      locations.data(),
      cellCentre,
      cellSize
    );
    selectedSources += count;
    validateEqualsWithParams1(deviceCountsResult[cell], count, cell);
    const int indexOffset = cell * Setup::NumberOfPointSources;
    for (int entry = 0; entry < Setup::NumberOfPointSources; entry++) {
      validateEqualsWithParams2(deviceIndicesResult[indexOffset + entry], pointSourceIndices[entry], cell, entry);
    }
  }
  validateEquals(selectedSources, Setup::NumberOfPointSources);
}

/**
 * The predictor of every cell of a device range reproduces the serial host loop.
 */
template <typename StoragePrecision>
void
exahype2::dg::ader::tests::LinearPredictorTest::testEveryCellOfADeviceRangeMatchesTheHostPredictor() {
  const Tables                         tables;
  const exahype2::dg::tests::CellChain geometry;
  const std::vector<double>            locations = getPointSourceLocations(geometry);
  const auto initialState = exahype2::dg::tests::getRamp(Setup::AllCellStatesCardinality, 0.5, 0.002);
  const std::vector<StoragePrecision> cellStates(initialState.begin(), initialState.end());
  const std::vector<double>           faceStates(Setup::AllFaceStatesCardinality, 0.0);
  const std::vector<double>           faceFluxes(Setup::AllFaceFluxesCardinality, 0.0);

  std::vector<StoragePrecision> deviceCellResult      = cellStates;
  std::vector<double>           deviceFaceStateResult = faceStates;
  std::vector<double>           deviceFaceFluxResult  = faceFluxes;
  predictOnDevice(tables, geometry, locations, deviceCellResult, deviceFaceStateResult, deviceFaceFluxResult);

  std::vector<double> hostCellStates(cellStates.begin(), cellStates.end());
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
    int                                         pointSourceIndices[Setup::NumberOfPointSources];
    const int numberOfPointSources = Predictor<exahype2::dg::ader::tests::StubSolver>::selectPointSources(
      pointSourceIndices,
      locations.data(),
      cellCentre,
      cellSize
    );
    Predictor<exahype2::dg::ader::tests::StubSolver>::predict(
      faceState,
      faceFlux,
      hostCellStates.data() + stateOffset,
      hostScratch.data() + scratchOffset,
      pointSourceIndices,
      numberOfPointSources,
      locations.data(),
      hostTables,
      cellCentre,
      cellSize,
      Setup::TimeStamp,
      Setup::TimeStepSize
    );
  }

  validateNumericalBuffersEquals(deviceCellResult, hostCellStates);
  validateNumericalBuffersEquals(deviceFaceStateResult, hostFaceStates);
  validateNumericalBuffersEquals(deviceFaceFluxResult, hostFaceFluxes);
}
