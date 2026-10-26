// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/dg/ader/LinearPredictor.h"

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/ader/TeamWorkers.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/exahype2/dg/KernelTestData.h"
#include "unittests/exahype2/dg/ader/KernelTestData.h"

namespace {
  using Setup             = exahype2::dg::ader::unittests::KernelTestSetup;
  using Tables            = exahype2::dg::unittests::BasisTableSet;
  using StubLayout        = exahype2::dg::ader::unittests::StubLayout;
  using RiemannSolverTerm = exahype2::dg::ader::unittests::RiemannSolverTerm;
  using CellLayout        = Setup::CellLayout;

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
  constexpr int CellStateSize             = CellLayout::NodesPerCell * NumberOfData;

  constexpr double TimeStamp    = 0.5;
  constexpr double TimeStepSize = 0.25;
  constexpr double Slope        = 3.0;
  constexpr double MarkerValue  = 7.0;
  constexpr double SourceRate   = 3.0;

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
        fluxValues[unknown] = (1.0 + direction) * state[unknown];
      }
    }
  };

  /**
   * Every Taylor step keeps the auxiliary variable and evaluates the same flux.
   */
  struct AuxiliaryFluxTerm {
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
        fluxValues[unknown] = (1.0 + direction) * state[NumberOfUnknowns];
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
        sourceValues[unknown] += SourceRate * state[unknown];
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
  struct FluxSolver final : StubLayout, FluxTerm {};
  struct FluxMaterialSolver final : StubLayout, FluxTerm, MaterialTerm {};
  struct ProductSolver final : StubLayout, ProductTerm, RiemannSolverTerm {};
  struct AuxiliaryFluxSolver final : StubLayout, AuxiliaryFluxTerm {};
  struct AuxiliaryFluxSourceSolver final : StubLayout, AuxiliaryFluxTerm, SourceTerm {};
  struct MarkerSolver final : StubLayout, FluxTerm, ProductTerm, MaterialTerm, SourceTerm {};
  struct PointSourceSolver final : StubLayout, PointSourceTerm {};
  struct AuxiliaryFluxPointSourceSolver final : StubLayout, AuxiliaryFluxTerm, PointSourceTerm {};
  struct FullSolver final : StubLayout, FluxTerm, ProductTerm, MaterialTerm, SourceTerm, PointSourceTerm {};

  /**
   * A linear system that transports a profile along the first axis and lets it grow.
   */
  struct ProfileSolver final : StubLayout, RiemannSolverTerm {
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
        values[unknown] = LinearFluxSpeed * (1.0 + direction) * state[unknown];
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

  using InertScratch                    = Predictor<InertSolver>::Scratch;
  using SourceScratch                   = Predictor<SourceSolver>::Scratch;
  using FluxScratch                     = Predictor<FluxSolver>::Scratch;
  using FluxMaterialScratch             = Predictor<FluxMaterialSolver>::Scratch;
  using ProductScratch                  = Predictor<ProductSolver>::Scratch;
  using AuxiliaryFluxScratch            = Predictor<AuxiliaryFluxSolver>::Scratch;
  using AuxiliaryFluxSourceScratch      = Predictor<AuxiliaryFluxSourceSolver>::Scratch;
  using MarkerScratch                   = Predictor<MarkerSolver>::Scratch;
  using PointSourceScratch              = Predictor<PointSourceSolver>::Scratch;
  using AuxiliaryFluxPointSourceScratch = Predictor<AuxiliaryFluxPointSourceSolver>::Scratch;
  using ProfileScratch                  = Predictor<ProfileSolver>::Scratch;
  using StubScratch                     = Predictor<exahype2::dg::ader::unittests::StubSolver>::Scratch;

  tarch::la::Vector<DIMENSIONS, double> getCellCentre() { return tarch::la::Vector<DIMENSIONS, double>(1.0); }

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

  double getCellVolume() {
    const auto cellSize   = getCellSize();
    double     cellVolume = 1.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellVolume *= cellSize(axis);
    }
    return cellVolume;
  }

  double getVaryingStateEntry(const int cellNode, const int value) { return 1.0 + 0.125 * (cellNode + value); }

  void fillVaryingCellState(double (&cellState)[CellStateSize]) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = getVaryingStateEntry(cellNode, value);
      }
    }
  }

  /**
   * Returns one lane of a node of the padded state that stores the cell state: The cell state in a
   * data lane and zero in a padding lane.
   */
  double getPaddedEntry(const double (&cellState)[CellStateSize], const int cellNode, const int lane) {
    return (lane < NumberOfData) ? cellState[cellNode * NumberOfData + lane] : 0.0;
  }

  /**
   * Returns the distances of a node from the lower corner of the cell summed over the axes, a
   * function whose derivative along every axis is one.
   */
  double getCoordinateSum(const Tables& tables, const int cellNode) {
    const auto nodeIndex     = peano4::dDelinearised(cellNode, NodesPerAxis);
    const auto cellSize      = getCellSize();
    double     coordinateSum = 0.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      coordinateSum += cellSize(axis) * tables.quadraturePoints[nodeIndex(axis)];
    }
    return coordinateSum;
  }

  /**
   * Fills the unknowns with varying values and the auxiliary variable with the coordinate sum of its node.
   */
  void fillCellStateWithCoordinateSum(const Tables& tables, double (&cellState)[CellStateSize]) {
    fillVaryingCellState(cellState);
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      cellState[cellNode * NumberOfData + NumberOfUnknowns] = getCoordinateSum(tables, cellNode);
    }
  }

  void fillCellStateLinearAlongAxis(const Tables& tables, double (&cellState)[CellStateSize], const int markedAxis) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const double reference = tables.quadraturePoints[peano4::dDelinearised(cellNode, NodesPerAxis)(markedAxis)];
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = 1.0 + value + Slope * reference;
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
    FaceBuffers(const FaceBuffers& other)            = delete;
    FaceBuffers(FaceBuffers&& other)                 = delete;
    FaceBuffers& operator=(const FaceBuffers& other) = delete;
    FaceBuffers& operator=(FaceBuffers&& other)      = delete;
    ~FaceBuffers()                                   = default;
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
   * Returns the last Taylor coefficient of an unknown whose next coefficient is sourceRate times the
   * coefficient before it plus increment.
   */
  double getLastTaylorCoefficient(const double cellValue, const double sourceRate, const double increment) {
    double coefficient = cellValue;
    for (int order = 1; order < NodesPerAxis; order++) {
      coefficient = sourceRate * coefficient + increment;
    }
    return coefficient;
  }

  /**
   * Returns the time average of the Taylor series of an unknown whose next coefficient is sourceRate
   * times the coefficient before it plus increment. The coefficient of order k enters with the time
   * step size to the power k divided by the factorial of k + 1.
   */
  double getTimeAverageOfTaylorSeries(const double zerothCoefficient, const double sourceRate, const double increment) {
    double coefficient = zerothCoefficient;
    double factor      = 1.0;
    double average     = 0.0;
    for (int order = 0; order < NodesPerAxis; order++) {
      average += factor * coefficient;
      coefficient = sourceRate * coefficient + increment;
      factor *= TimeStepSize / (order + 2);
    }
    return average;
  }

  /**
   * Runs the predictor of Solver on the cell at the unit position with the listed point sources.
   */
  template <typename Solver>
  void predict(
    const Tables&       tables,
    const double* const cellState,
    FaceBuffers&        faces,
    double* const       scratch,
    const int* const    pointSourceIndices   = nullptr,
    const int           numberOfPointSources = 0,
    const double* const pointSourceLocations = nullptr
  ) {
    Predictor<Solver>::predict(
      faces.statePointers,
      faces.fluxPointers,
      cellState,
      scratch,
      pointSourceIndices,
      numberOfPointSources,
      pointSourceLocations,
      tables.getView(),
      getCellCentre(),
      getCellSize(),
      TimeStamp,
      TimeStepSize
    );
  }

  /**
   * Returns the terms of Solver at the time-averaged state in the layout of the flux block: The flux
   * differentiated along its axis plus the nonconservative product, multiplied by the material
   * parameter matrix.
   */
  template <typename Solver>
  std::vector<double> getTermsOf(const Tables& tables, const double* const timeAveragedState) {
    const auto          cellCentre      = getCellCentre();
    const auto          cellSize        = getCellSize();
    const auto          inverseCellSize = getInverseCellSize();
    std::vector<double> terms(DIMENSIONS * AxisStride, 0.0);
    if constexpr (Predictor<Solver>::EvaluateFlux) {
      for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          Solver::flux(
            timeAveragedState + cellNode * StateStride,
            cellCentre,
            cellSize,
            TimeStamp,
            TimeStepSize,
            axis,
            terms.data() + axis * AxisStride + cellNode * UnknownStride
          );
        }
      }
      exahype2::dg::differentiateFluxInPlace<Solver>(terms.data(), tables.derivativeOperator.data(), inverseCellSize);
    }
    if constexpr (Predictor<Solver>::EvaluateNonconservativeProduct) {
      std::vector<double> gradient(DIMENSIONS * AxisStride, 0.0);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int lane = 0; lane < UnknownStride; lane++) {
            gradient[axis * AxisStride + cellNode * UnknownStride + lane] =
              timeAveragedState[cellNode * StateStride + lane];
          }
        }
      }
      exahype2::dg::differentiateFluxInPlace<Solver>(
        gradient.data(),
        tables.derivativeOperator.data(),
        inverseCellSize
      );
      for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          double productValues[UnknownStride] = {};
          Solver::nonconservativeProduct(
            timeAveragedState + cellNode * StateStride,
            gradient.data() + axis * AxisStride + cellNode * UnknownStride,
            cellCentre,
            cellSize,
            TimeStamp,
            TimeStepSize,
            axis,
            productValues
          );
          for (int lane = 0; lane < UnknownStride; lane++) {
            terms[axis * AxisStride + cellNode * UnknownStride + lane] += productValues[lane];
          }
        }
      }
    }
    if constexpr (Predictor<Solver>::EvaluateMaterialParameters) {
      for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          Solver::multiplyMaterialParameterMatrix(
            timeAveragedState + cellNode * StateStride,
            cellCentre,
            cellSize,
            TimeStamp,
            TimeStepSize,
            axis,
            terms.data() + axis * AxisStride + cellNode * UnknownStride
          );
        }
      }
    }
    return terms;
  }

  /**
   * Returns the index of one lane of a node in a time sample of the point source samples.
   */
  int getSampleIndex(const int timeSample, const int cellNode, const int lane) {
    return (timeSample * CellLayout::NodesPerCell + cellNode) * UnknownStride + lane;
  }

  /**
   * Returns the product of the basis functions of a node at the one point source of a prediction on
   * a cell state of zeros: The first sample of the node divided by the force of table entry zero at
   * the start of the step, by the inverse quadrature weight of the node and by the inverse cell volume.
   */
  double getBasisValueOfSample(const Tables& tables, const std::vector<double>& scratch, const int cellNode) {
    const double sample = scratch[PointSourceScratch::PointSourceOffset + getSampleIndex(0, cellNode, 0)];
    const double force  = getPointSourceForce(0, 0, TimeStamp, 0.0);
    return sample * getCellVolume() / (tables.inverseCellQuadratureWeights[cellNode] * force);
  }

  /**
   * Returns the sample of one node and lane at a time position. The sample sums the force of every
   * listed point source times the basis function of the node at the source over the quadrature weight
   * of the node and the cell volume. A padding lane returns zero.
   */
  double getSampleOf(
    const Tables& tables,
    const double (&cellState)[CellStateSize],
    const int (&pointSourceIndices)[NumberOfPointSources],
    const double timePosition,
    const int    cellNode,
    const int    lane
  ) {
    const auto nodeIndex = peano4::dDelinearised(cellNode, NodesPerAxis);
    double     sample    = 0.0;
    if (lane < NumberOfUnknowns) {
      for (int entry = 0; entry < NumberOfPointSources; entry++) {
        const int pointSource = pointSourceIndices[entry];
        double    spread      = 1.0;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          spread *= exahype2::dg::getLagrangeBasisFunction1D<NodesPerAxis>(
            tables.quadraturePoints.data(),
            nodeIndex(axis),
            getReferenceCoordinate(pointSource, axis) - 0.5
          );
        }
        const double stateValue = cellState[cellNode * NumberOfData + lane];
        sample += spread * tables.inverseCellQuadratureWeights[cellNode] / getCellVolume()
          * getPointSourceForce(lane, pointSource, timePosition, stateValue);
      }
    }
    return sample;
  }

  /**
   * Puts source zero at the centre of cell one and source one on the right face of cell four.
   */
  std::vector<double> getPointSourceLocations(const exahype2::dg::unittests::CellChain& geometry) {
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
      counts[cell] = Predictor<exahype2::dg::ader::unittests::StubSolver>::selectPointSources(
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
      const int numberOfPointSources = Predictor<exahype2::dg::ader::unittests::StubSolver>::selectPointSources(
        pointSourceIndices,
        locations,
        cellCentre,
        cellSize
      );
      Predictor<exahype2::dg::ader::unittests::StubSolver>::predict(
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
    const exahype2::dg::unittests::CellChain& geometry,
    const std::vector<double>&                locations,
    std::vector<int>&                         indices,
    std::vector<int>&                         counts
  ) {
    const exahype2::dg::unittests::DeviceBuffer<double> deviceLocations(locations);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);
    const exahype2::dg::unittests::DeviceBuffer<int>    deviceIndices(indices);
    const exahype2::dg::unittests::DeviceBuffer<int>    deviceCounts(counts);

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
    const Tables&                             tables,
    const exahype2::dg::unittests::CellChain& geometry,
    const std::vector<double>&                locations,
    std::vector<StoragePrecision>&            cellStates,
    std::vector<double>&                      faceStates,
    std::vector<double>&                      faceFluxes
  ) {
    const exahype2::dg::unittests::DeviceBasisTables deviceTables(tables);
    const std::vector<double> scratch(static_cast<std::size_t>(Setup::NumberOfCells) * StubScratch::Size, 0.0);

    const exahype2::dg::unittests::DeviceBuffer<double>           deviceLocations(locations);
    const exahype2::dg::unittests::DeviceBuffer<StoragePrecision> deviceCellStates(cellStates);
    const exahype2::dg::unittests::DeviceBuffer<double>           deviceFaceStates(faceStates);
    const exahype2::dg::unittests::DeviceBuffer<double>           deviceFaceFluxes(faceFluxes);
    const exahype2::dg::unittests::DeviceBuffer<double>           deviceScratch(scratch);
    const exahype2::dg::unittests::DeviceBuffer<double>           deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double>           deviceCellSizes(geometry.cellSizes);

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

  void predictOnHost(
    const Tables&                             tables,
    const exahype2::dg::unittests::CellChain& geometry,
    const std::vector<double>&                locations,
    std::vector<double>&                      cellStates,
    std::vector<double>&                      faceStates,
    std::vector<double>&                      faceFluxes
  ) {
    std::vector<double> scratch(static_cast<std::size_t>(Setup::NumberOfCells) * StubScratch::Size, 0.0);
    const exahype2::dg::BasisTables<double> hostTables = tables.getView();
    for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
      double* faceState[peano4::FacesPerCell];
      double* faceFlux[peano4::FacesPerCell];
      collectFacePointers(cell, faceStates.data(), faceFluxes.data(), faceState, faceFlux);
      const int                                   coordinateOffset = cell * DIMENSIONS;
      const tarch::la::Vector<DIMENSIONS, double> cellCentre(geometry.cellCentres.data() + coordinateOffset);
      const tarch::la::Vector<DIMENSIONS, double> cellSize(geometry.cellSizes.data() + coordinateOffset);
      const int                                   stateOffset   = cell * Setup::CellStateCardinality;
      const int                                   scratchOffset = cell * StubScratch::Size;
      int                                         pointSourceIndices[Setup::NumberOfPointSources];
      const int numberOfPointSources = Predictor<exahype2::dg::ader::unittests::StubSolver>::selectPointSources(
        pointSourceIndices,
        locations.data(),
        cellCentre,
        cellSize
      );
      Predictor<exahype2::dg::ader::unittests::StubSolver>::predict(
        faceState,
        faceFlux,
        cellStates.data() + stateOffset,
        scratch.data() + scratchOffset,
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
  }
} // namespace

UNIT_TEST("A system without terms starts the time average from the cell state and clears the flux of every step") {
  GIVEN("Basis tables, a varying cell state and a scratch block filled with a marker value") {
    const Tables tables;
    double       cellState[CellStateSize];
    for (int entry = 0; entry < CellStateSize; entry++) {
      cellState[entry] = 1.0 + 0.125 * entry;
    }
    std::vector<double> scratch(InertScratch::Size, MarkerValue);

    WHEN("The predictor of a system without terms runs") {
      FaceBuffers faces;
      predict<InertSolver>(tables, cellState, faces, scratch.data());
      const double* const timeAveragedState = scratch.data() + InertScratch::TimeAveragedStateOffset;
      const double* const flux              = scratch.data() + InertScratch::FluxOffset;

      THEN("The time-averaged state stores the cell state and a zero in every padding lane") {
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int value = 0; value < NumberOfData; value++) {
            VALIDATE_NUMERICAL_EQUALS(
              timeAveragedState[cellNode * StateStride + value],
              cellState[cellNode * NumberOfData + value],
              cellNode,
              value
            );
          }
          for (int value = NumberOfData; value < StateStride; value++) {
            VALIDATE_NUMERICAL_EQUALS(timeAveragedState[cellNode * StateStride + value], 0.0, cellNode, value);
          }
        }
      }

      THEN("Every entry of the flux equals zero") {
        for (int entry = 0; entry < DIMENSIONS * AxisStride; entry++) {
          VALIDATE_NUMERICAL_EQUALS(flux[entry], 0.0, entry);
        }
      }
    }
  }
}

UNIT_TEST("After the prediction the flux stores the terms of the time-averaged state") {
  GIVEN("Basis tables and a varying cell state") {
    const Tables tables;
    double       cellState[CellStateSize];
    for (int entry = 0; entry < CellStateSize; entry++) {
      cellState[entry] = 1.0 + 0.125 * entry;
    }

    WHEN("The predictor of a flux, a product, a material parameter matrix and a source runs") {
      FaceBuffers         faces;
      std::vector<double> scratch(MarkerScratch::Size, 0.0);
      predict<MarkerSolver>(tables, cellState, faces, scratch.data());
      const double* const timeAveragedState = scratch.data() + MarkerScratch::TimeAveragedStateOffset;
      const double* const flux              = scratch.data() + MarkerScratch::FluxOffset;

      THEN("Every entry of the flux equals the terms evaluated at the time-averaged state") {
        const std::vector<double> terms = getTermsOf<MarkerSolver>(tables, timeAveragedState);
        for (int entry = 0; entry < DIMENSIONS * AxisStride; entry++) {
          VALIDATE_NUMERICAL_EQUALS(flux[entry], terms[entry], entry);
        }
      }
    }
  }
}

UNIT_TEST("A linear source overwrites the unknowns of every Taylor coefficient and grows their time average") {
  GIVEN("Basis tables and a varying cell state") {
    const Tables tables;
    double       cellState[CellStateSize];
    fillVaryingCellState(cellState);

    WHEN("The predictor of a source that triples the state runs") {
      FaceBuffers         faces;
      std::vector<double> scratch(SourceScratch::Size, 0.0);
      predict<SourceSolver>(tables, cellState, faces, scratch.data());
      const double* const lastCoefficient   = scratch.data() + SourceScratch::StateOffset;
      const double* const timeAveragedState = scratch.data() + SourceScratch::TimeAveragedStateOffset;

      THEN("Every Taylor step overwrites the unknowns alone with their source") {
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            VALIDATE_NUMERICAL_EQUALS(
              lastCoefficient[cellNode * StateStride + unknown],
              getLastTaylorCoefficient(getVaryingStateEntry(cellNode, unknown), SourceRate, 0.0),
              cellNode,
              unknown
            );
          }
          for (int value = NumberOfUnknowns; value < StateStride; value++) {
            VALIDATE_NUMERICAL_EQUALS(
              lastCoefficient[cellNode * StateStride + value],
              getPaddedEntry(cellState, cellNode, value),
              cellNode,
              value
            );
          }
        }
      }

      THEN("The time average scales the unknowns alone by the Taylor series of the source") {
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            VALIDATE_NUMERICAL_EQUALS(
              timeAveragedState[cellNode * StateStride + unknown],
              getTimeAverageOfTaylorSeries(getVaryingStateEntry(cellNode, unknown), SourceRate, 0.0),
              cellNode,
              unknown
            );
          }
          for (int value = NumberOfUnknowns; value < StateStride; value++) {
            VALIDATE_NUMERICAL_EQUALS(
              timeAveragedState[cellNode * StateStride + value],
              getPaddedEntry(cellState, cellNode, value),
              cellNode,
              value
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("The next Taylor coefficient is the source minus the flux divergence or the negated divergence alone") {
  GIVEN("Basis tables and a cell state whose auxiliary variable sums the node coordinates") {
    const Tables tables;
    double       cellState[CellStateSize];
    fillCellStateWithCoordinateSum(tables, cellState);

    WHEN("The predictors of a flux of the auxiliary variable with and without a source that triples the state run") {
      FaceBuffers         facesWithSource;
      std::vector<double> scratchWithSource(AuxiliaryFluxSourceScratch::Size, 0.0);
      predict<AuxiliaryFluxSourceSolver>(tables, cellState, facesWithSource, scratchWithSource.data());
      FaceBuffers         facesWithoutSource;
      std::vector<double> scratchWithoutSource(AuxiliaryFluxScratch::Size, 0.0);
      predict<AuxiliaryFluxSolver>(tables, cellState, facesWithoutSource, scratchWithoutSource.data());
      const double* const withSource    = scratchWithSource.data() + AuxiliaryFluxSourceScratch::StateOffset;
      const double* const withoutSource = scratchWithoutSource.data() + AuxiliaryFluxScratch::StateOffset;

      THEN("Every Taylor step applies the source to the unknowns alone and subtracts the flux divergence") {
        double fluxDivergence = 0.0;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          fluxDivergence += 1.0 + axis;
        }
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            const int    entry   = cellNode * StateStride + unknown;
            const double initial = cellState[cellNode * NumberOfData + unknown];
            VALIDATE_NUMERICAL_EQUALS(
              withSource[entry],
              getLastTaylorCoefficient(initial, SourceRate, -fluxDivergence),
              cellNode,
              unknown
            );
            VALIDATE_NUMERICAL_EQUALS(withoutSource[entry], -fluxDivergence, cellNode, unknown);
          }
          for (int value = NumberOfUnknowns; value < StateStride; value++) {
            const int    entry   = cellNode * StateStride + value;
            const double initial = getPaddedEntry(cellState, cellNode, value);
            VALIDATE_NUMERICAL_EQUALS(withSource[entry], initial, cellNode, value);
            VALIDATE_NUMERICAL_EQUALS(withoutSource[entry], initial, cellNode, value);
          }
        }
      }
    }
  }
}

UNIT_TEST("The flux of every node and axis reaches the block of that axis") {
  GIVEN("Basis tables and a varying cell state") {
    const Tables tables;
    double       cellState[CellStateSize];
    fillVaryingCellState(cellState);

    WHEN("The predictor of a flux that scales the state by one plus the axis number runs") {
      FaceBuffers         faces;
      std::vector<double> scratch(FluxScratch::Size, 0.0);
      predict<FluxSolver>(tables, cellState, faces, scratch.data());
      const double* const timeAveragedState = scratch.data() + FluxScratch::TimeAveragedStateOffset;
      const double* const flux              = scratch.data() + FluxScratch::FluxOffset;

      THEN("Every node and axis stores its differentiated flux of the time-averaged state and zero padding lanes") {
        const std::vector<double> differentiatedFlux = getTermsOf<FluxSolver>(tables, timeAveragedState);
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
            const int base = axis * AxisStride + cellNode * UnknownStride;
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              VALIDATE_NUMERICAL_EQUALS(
                flux[base + unknown],
                differentiatedFlux[base + unknown],
                axis,
                cellNode,
                unknown
              );
            }
            for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
              VALIDATE_NUMERICAL_EQUALS(flux[base + unknown], 0.0, axis, cellNode, unknown);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("A constant state adds an exactly zero nonconservative product") {
  GIVEN("Basis tables and a cell state of four thirds in every entry") {
    const Tables tables;
    // Four thirds rounds in binary. An inexact cancellation leaves a nonzero product.
    double cellState[CellStateSize];
    for (int entry = 0; entry < CellStateSize; entry++) {
      cellState[entry] = 4.0 / 3.0;
    }

    WHEN("The predictor of a product that scales the gradient by one plus the axis number runs") {
      FaceBuffers         faces;
      std::vector<double> scratch(ProductScratch::Size, 0.0);
      predict<ProductSolver>(tables, cellState, faces, scratch.data());
      const double* const flux = scratch.data() + ProductScratch::FluxOffset;

      THEN("Every entry of the flux equals zero bitwise") {
        for (int entry = 0; entry < DIMENSIONS * AxisStride; entry++) {
          VALIDATE_EQUALS(flux[entry], 0.0, entry);
        }
      }
    }
  }
}

UNIT_TEST("A state that is linear along one axis adds the product of its exact slope to the flux of that axis alone") {
  GIVEN("Basis tables and for every axis a cell state that is linear along that axis with the slope three") {
    const Tables tables;
    double       cellStateAlongAxis[DIMENSIONS][CellStateSize];
    for (int markedAxis = 0; markedAxis < DIMENSIONS; markedAxis++) {
      fillCellStateLinearAlongAxis(tables, cellStateAlongAxis[markedAxis], markedAxis);
    }

    WHEN("The predictor of a product that scales the gradient by one plus the axis number runs on every state") {
      std::vector<std::vector<double>> scratchOfMarkedAxis(DIMENSIONS, std::vector<double>(ProductScratch::Size, 0.0));
      for (int markedAxis = 0; markedAxis < DIMENSIONS; markedAxis++) {
        FaceBuffers faces;
        predict<ProductSolver>(tables, cellStateAlongAxis[markedAxis], faces, scratchOfMarkedAxis[markedAxis].data());
      }

      THEN("The marked axis stores one plus its number times the physical slope and every other entry stores zero") {
        const auto inverseCellSize = getInverseCellSize();
        for (int markedAxis = 0; markedAxis < DIMENSIONS; markedAxis++) {
          const double* const flux = scratchOfMarkedAxis[markedAxis].data() + ProductScratch::FluxOffset;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            const double expected = (axis == markedAxis) ? (1.0 + axis) * Slope * inverseCellSize(axis) : 0.0;
            for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
              const int base = axis * AxisStride + cellNode * UnknownStride;
              for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
                VALIDATE_NUMERICAL_EQUALS(flux[base + unknown], expected, markedAxis, axis, cellNode);
              }
              for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
                VALIDATE_NUMERICAL_EQUALS(flux[base + unknown], 0.0, markedAxis, axis, cellNode);
              }
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The material parameter matrix multiplies the unknowns of every node and axis and leaves the padding lanes") {
  GIVEN("Basis tables and a varying cell state") {
    const Tables tables;
    double       cellState[CellStateSize];
    fillVaryingCellState(cellState);

    WHEN("The predictor of a flux and of a material parameter matrix that scales it by two plus the axis number runs") {
      FaceBuffers         faces;
      std::vector<double> scratch(FluxMaterialScratch::Size, 0.0);
      predict<FluxMaterialSolver>(tables, cellState, faces, scratch.data());
      const double* const timeAveragedState = scratch.data() + FluxMaterialScratch::TimeAveragedStateOffset;
      const double* const flux              = scratch.data() + FluxMaterialScratch::FluxOffset;

      THEN("Every node and axis stores two plus the axis number times its differentiated flux and zero padding lanes") {
        const std::vector<double> differentiatedFlux = getTermsOf<FluxSolver>(tables, timeAveragedState);
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
            const int base = axis * AxisStride + cellNode * UnknownStride;
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              VALIDATE_NUMERICAL_EQUALS(
                flux[base + unknown],
                (2.0 + axis) * differentiatedFlux[base + unknown],
                axis,
                cellNode,
                unknown
              );
            }
            for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
              VALIDATE_NUMERICAL_EQUALS(
                flux[base + unknown],
                differentiatedFlux[base + unknown],
                axis,
                cellNode,
                unknown
              );
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("A point source inside the cell raises the count by one and writes its table index") {
  GIVEN("The unit cell, a point source at its centre and two point sources far outside") {
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.5);
    PointSourceTable                            table;
    table.place(0, cellCentre);
    table.place(1, tarch::la::Vector<DIMENSIONS, double>(5.0));
    table.place(2, tarch::la::Vector<DIMENSIONS, double>(-5.0));

    WHEN("The unit cell selects its point sources") {
      int       pointSourceIndices[NumberOfSelectableSources] = {-1, -1, -1};
      const int count = selectInCellOfUnitSize(table, pointSourceIndices, cellCentre);

      THEN("The count is one and the first index is the point source at the centre") {
        VALIDATE_EQUALS(count, 1);
        VALIDATE_EQUALS(pointSourceIndices[0], 0);
        VALIDATE_EQUALS(pointSourceIndices[1], -1);
        VALIDATE_EQUALS(pointSourceIndices[2], -1);
      }
    }
  }
}

UNIT_TEST("A point source that leaves the cell along one axis keeps the count zero and every index -1") {
  GIVEN("A point source outside the unit cell along its last axis alone and two outside along every axis") {
    tarch::la::Vector<DIMENSIONS, double> beyondTheLastAxis(0.5);
    beyondTheLastAxis(DIMENSIONS - 1) = 1.5;
    PointSourceTable table;
    table.place(0, beyondTheLastAxis);
    table.place(1, tarch::la::Vector<DIMENSIONS, double>(-0.5));
    table.place(2, tarch::la::Vector<DIMENSIONS, double>(5.0));

    WHEN("The unit cell selects its point sources") {
      int       pointSourceIndices[NumberOfSelectableSources] = {-1, -1, -1};
      const int count = selectInCellOfUnitSize(table, pointSourceIndices, tarch::la::Vector<DIMENSIONS, double>(0.5));

      THEN("The count is zero and every index stays unset") {
        VALIDATE_EQUALS(count, 0);
        for (int entry = 0; entry < NumberOfSelectableSources; entry++) {
          VALIDATE_EQUALS(pointSourceIndices[entry], -1, entry);
        }
      }
    }
  }
}

UNIT_TEST("A point source on a face of two cells appears in the list of both cells") {
  GIVEN("Point sources at the lower and at the upper corner of the unit cell and one far outside") {
    PointSourceTable table;
    table.place(0, tarch::la::Vector<DIMENSIONS, double>(0.0));
    table.place(1, tarch::la::Vector<DIMENSIONS, double>(1.0));
    table.place(2, tarch::la::Vector<DIMENSIONS, double>(5.0));

    WHEN("The unit cell and the cell beyond its upper corner select their point sources") {
      int       lowerIndices[NumberOfSelectableSources] = {-1, -1, -1};
      const int lowerCount = selectInCellOfUnitSize(table, lowerIndices, tarch::la::Vector<DIMENSIONS, double>(0.5));
      int       upperIndices[NumberOfSelectableSources] = {-1, -1, -1};
      const int upperCount = selectInCellOfUnitSize(table, upperIndices, tarch::la::Vector<DIMENSIONS, double>(1.5));

      THEN("The unit cell lists both corners and the cell beyond lists the upper corner that both cells touch") {
        VALIDATE_EQUALS(lowerCount, 2);
        VALIDATE_EQUALS(lowerIndices[0], 0);
        VALIDATE_EQUALS(lowerIndices[1], 1);
        VALIDATE_EQUALS(upperCount, 1);
        VALIDATE_EQUALS(upperIndices[0], 1);
      }
    }
  }
}

UNIT_TEST("The indices follow the order of the point source table of the solver") {
  GIVEN("Point sources at one quarter, far outside and at three quarters of the unit cell") {
    PointSourceTable table;
    table.place(0, tarch::la::Vector<DIMENSIONS, double>(0.25));
    table.place(1, tarch::la::Vector<DIMENSIONS, double>(5.0));
    table.place(2, tarch::la::Vector<DIMENSIONS, double>(0.75));

    WHEN("The unit cell selects its point sources") {
      int       pointSourceIndices[NumberOfSelectableSources] = {-1, -1, -1};
      const int count = selectInCellOfUnitSize(table, pointSourceIndices, tarch::la::Vector<DIMENSIONS, double>(0.5));

      THEN("The count is two and the indices list the first and the last point source in table order") {
        VALIDATE_EQUALS(count, 2);
        VALIDATE_EQUALS(pointSourceIndices[0], 0);
        VALIDATE_EQUALS(pointSourceIndices[1], 2);
        VALIDATE_EQUALS(pointSourceIndices[2], -1);
      }
    }
  }
}

UNIT_TEST("The basis function of a node returns one at that node and zero at every other node") {
  GIVEN("Basis tables and a cell state of zeros") {
    const Tables tables;
    const double cellState[CellStateSize] = {};

    WHEN("A point source sits at every node in turn and the predictor spreads its force over the cell") {
      std::vector<double> basisValues(static_cast<std::size_t>(CellLayout::NodesPerCell) * CellLayout::NodesPerCell);
      for (int sourceNode = 0; sourceNode < CellLayout::NodesPerCell; sourceNode++) {
        PointSourceTable table;
        table.place(
          0,
          exahype2::dg::getQuadraturePoint(
            getCellCentre(),
            getCellSize(),
            peano4::dDelinearised(sourceNode, NodesPerAxis),
            tables.quadraturePoints.data()
          )
        );
        const int           pointSourceIndices[1] = {0};
        FaceBuffers         faces;
        std::vector<double> scratch(PointSourceScratch::Size, 0.0);
        predict<PointSourceSolver>(tables, cellState, faces, scratch.data(), pointSourceIndices, 1, table.getData());
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          basisValues[sourceNode * CellLayout::NodesPerCell + cellNode] =
            getBasisValueOfSample(tables, scratch, cellNode);
        }
      }

      THEN("The basis functions of every node give one at the node of the point source and zero elsewhere") {
        for (int sourceNode = 0; sourceNode < CellLayout::NodesPerCell; sourceNode++) {
          for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
            const double expected = (cellNode == sourceNode) ? 1.0 : 0.0;
            VALIDATE_NUMERICAL_EQUALS(
              basisValues[sourceNode * CellLayout::NodesPerCell + cellNode],
              expected,
              sourceNode,
              cellNode
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("The basis functions sum to one and, weighted with the node coordinates, return the point of evaluation") {
  GIVEN("Basis tables, a cell state of zeros and nine points from zero to one on the diagonal of the reference cell") {
    constexpr int    NumberOfPoints = 9;
    constexpr double PointSpacing   = 0.125;
    const Tables     tables;
    const double     cellState[CellStateSize] = {};

    WHEN("A point source sits at every point in turn and the predictor spreads its force over the cell") {
      const auto cellCentre = getCellCentre();
      const auto cellSize   = getCellSize();
      double     basisSums[NumberOfPoints]{};
      double     coordinateSums[NumberOfPoints][DIMENSIONS]{};
      for (int point = 0; point < NumberOfPoints; point++) {
        tarch::la::Vector<DIMENSIONS, double> location;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          location(axis) = cellCentre(axis) + cellSize(axis) * (PointSpacing * point - 0.5);
        }
        PointSourceTable table;
        table.place(0, location);
        const int           pointSourceIndices[1] = {0};
        FaceBuffers         faces;
        std::vector<double> scratch(PointSourceScratch::Size, 0.0);
        predict<PointSourceSolver>(tables, cellState, faces, scratch.data(), pointSourceIndices, 1, table.getData());
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          const double basisValue = getBasisValueOfSample(tables, scratch, cellNode);
          const auto   nodeIndex  = peano4::dDelinearised(cellNode, NodesPerAxis);
          basisSums[point] += basisValue;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            coordinateSums[point][axis] += basisValue * tables.quadraturePoints[nodeIndex(axis)];
          }
        }
      }

      THEN("The basis functions sum to one and their sum weighted with the node coordinates gives the point") {
        for (int point = 0; point < NumberOfPoints; point++) {
          VALIDATE_NUMERICAL_EQUALS(basisSums[point], 1.0, point);
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            VALIDATE_NUMERICAL_EQUALS(coordinateSums[point][axis], PointSpacing * point, point, axis);
          }
        }
      }
    }
  }
}

UNIT_TEST("An empty list of point sources keeps the time averages of the cell state") {
  GIVEN("Basis tables, a varying cell state, two point sources inside the cell and samples of nine") {
    const Tables tables;
    double       cellState[CellStateSize];
    fillVaryingCellState(cellState);
    PointSourceTable table;
    table.placeInside(getCellCentre(), getCellSize());
    const int           pointSourceIndices[NumberOfPointSources] = {0, 1};
    std::vector<double> scratch(PointSourceScratch::Size, 0.0);
    for (int entry = 0; entry < PointSourceScratch::PointSourceSize; entry++) {
      scratch[PointSourceScratch::PointSourceOffset + entry] = 9.0;
    }

    WHEN("The predictor of the point sources runs with an empty list") {
      FaceBuffers faces;
      predict<PointSourceSolver>(tables, cellState, faces, scratch.data(), pointSourceIndices, 0, table.getData());
      const double* const timeAveragedState = scratch.data() + PointSourceScratch::TimeAveragedStateOffset;
      const double* const flux              = scratch.data() + PointSourceScratch::FluxOffset;

      THEN("The time-averaged state stores the padded cell state and the flux stores zeros") {
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int value = 0; value < StateStride; value++) {
            VALIDATE_NUMERICAL_EQUALS(
              timeAveragedState[cellNode * StateStride + value],
              getPaddedEntry(cellState, cellNode, value),
              cellNode,
              value
            );
          }
        }
        for (int entry = 0; entry < DIMENSIONS * AxisStride; entry++) {
          VALIDATE_NUMERICAL_EQUALS(flux[entry], 0.0, entry);
        }
      }
    }
  }
}

UNIT_TEST("The point sources inside a cell reach the samples, the Taylor coefficients and the time-averaged state") {
  GIVEN("Basis tables, a cell state, two point sources inside the cell listed backwards and samples of nine") {
    const Tables tables;
    double       cellState[CellStateSize];
    for (int entry = 0; entry < CellStateSize; entry++) {
      cellState[entry] = 0.5 + 0.01 * entry;
    }
    PointSourceTable table;
    table.placeInside(getCellCentre(), getCellSize());
    const int           pointSourceIndices[NumberOfPointSources] = {1, 0};
    std::vector<double> scratch(PointSourceScratch::Size, 0.0);
    for (int entry = 0; entry < PointSourceScratch::PointSourceSize; entry++) {
      scratch[PointSourceScratch::PointSourceOffset + entry] = 9.0;
    }

    WHEN("The predictor of both point sources runs") {
      FaceBuffers faces;
      predict<PointSourceSolver>(
        tables,
        cellState,
        faces,
        scratch.data(),
        pointSourceIndices,
        NumberOfPointSources,
        table.getData()
      );
      const double* const samples           = scratch.data() + PointSourceScratch::PointSourceOffset;
      const double* const lastCoefficient   = scratch.data() + PointSourceScratch::StateOffset;
      const double* const timeAveragedState = scratch.data() + PointSourceScratch::TimeAveragedStateOffset;

      THEN("A node receives the force of every source times its basis functions over its weight and the volume") {
        for (int timeSample = 0; timeSample < NodesPerAxis + 1; timeSample++) {
          const double timePosition = getTimePositionOf(tables, timeSample);
          for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
            for (int lane = 0; lane < UnknownStride; lane++) {
              VALIDATE_NUMERICAL_EQUALS(
                samples[getSampleIndex(timeSample, cellNode, lane)],
                getSampleOf(tables, cellState, pointSourceIndices, timePosition, cellNode, lane),
                timeSample,
                cellNode,
                lane
              );
            }
          }
        }
      }

      THEN("The derivative coefficients weight the samples into the unknowns of the last Taylor coefficient alone") {
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            double weightedSamples = 0.0;
            for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
              weightedSamples +=
                tables.leftFaceCoefficients[timeNode] * samples[getSampleIndex(timeNode, cellNode, unknown)];
            }
            VALIDATE_NUMERICAL_EQUALS(
              lastCoefficient[cellNode * StateStride + unknown],
              weightedSamples,
              cellNode,
              unknown
            );
          }
          for (int value = NumberOfUnknowns; value < StateStride; value++) {
            VALIDATE_NUMERICAL_EQUALS(
              lastCoefficient[cellNode * StateStride + value],
              getPaddedEntry(cellState, cellNode, value),
              cellNode,
              value
            );
          }
        }
      }

      THEN("The time average starts from the time integral of the samples without the one at the start of the step") {
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            double timeIntegral = 0.0;
            for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
              timeIntegral += TimeStepSize * tables.quadratureWeights[timeNode]
                * samples[getSampleIndex(timeNode + 1, cellNode, unknown)];
            }
            const int    entry             = cellNode * StateStride + unknown;
            const double zerothCoefficient = cellState[cellNode * NumberOfData + unknown] + timeIntegral;
            VALIDATE_NUMERICAL_EQUALS(
              timeAveragedState[entry],
              getTimeAverageOfTaylorSeries(zerothCoefficient, 0.0, lastCoefficient[entry]),
              cellNode,
              unknown
            );
          }
          for (int value = NumberOfUnknowns; value < StateStride; value++) {
            VALIDATE_NUMERICAL_EQUALS(
              timeAveragedState[cellNode * StateStride + value],
              getPaddedEntry(cellState, cellNode, value),
              cellNode,
              value
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("Every axis of the time-averaged flux receives the same time integral divided by the number of axes") {
  GIVEN("Basis tables, a cell state whose auxiliary variable sums the node coordinates and two inner point sources") {
    const Tables tables;
    double       cellState[CellStateSize];
    fillCellStateWithCoordinateSum(tables, cellState);
    PointSourceTable table;
    table.placeInside(getCellCentre(), getCellSize());
    const int pointSourceIndices[NumberOfPointSources] = {0, 1};

    WHEN("The predictor of a flux of the auxiliary variable and of both point sources runs") {
      FaceBuffers         faces;
      std::vector<double> scratch(AuxiliaryFluxPointSourceScratch::Size, 0.0);
      predict<AuxiliaryFluxPointSourceSolver>(
        tables,
        cellState,
        faces,
        scratch.data(),
        pointSourceIndices,
        NumberOfPointSources,
        table.getData()
      );
      const double* const samples = scratch.data() + AuxiliaryFluxPointSourceScratch::PointSourceOffset;
      const double* const flux    = scratch.data() + AuxiliaryFluxPointSourceScratch::FluxOffset;

      THEN("Every axis adds the time integral of the samples over the number of axes to one plus the axis number") {
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int lane = 0; lane < UnknownStride; lane++) {
            double timeAverage = 0.0;
            for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
              timeAverage +=
                tables.quadratureWeights[timeNode] / DIMENSIONS * samples[getSampleIndex(timeNode + 1, cellNode, lane)];
            }
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              const double fluxOfTheTerm = (lane < NumberOfUnknowns) ? 1.0 + axis : 0.0;
              VALIDATE_NUMERICAL_EQUALS(
                flux[axis * AxisStride + cellNode * UnknownStride + lane],
                fluxOfTheTerm + timeAverage,
                axis,
                cellNode,
                lane
              );
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The block of the scratch places every buffer behind its predecessor without a gap") {
  GIVEN("The scratch layout of a solver with every term and point sources") {
    using Scratch = Predictor<FullSolver>::Scratch;

    THEN("Every buffer reserves the entries of its quantity and starts where its predecessor ends") {
      VALIDATE_EQUALS(Scratch::StateSize, StateStride * CellLayout::NodesPerCell);
      VALIDATE_EQUALS(Scratch::TimeAveragedStateSize, StateStride * CellLayout::NodesPerCell);
      VALIDATE_EQUALS(Scratch::FluxSize, DIMENSIONS * UnknownStride * CellLayout::NodesPerCell);
      VALIDATE_EQUALS(Scratch::PointSourceSize, (NodesPerAxis + 1) * CellLayout::NodesPerCell * UnknownStride);

      VALIDATE_EQUALS(Scratch::StateOffset, 0);
      VALIDATE_EQUALS(Scratch::TimeAveragedStateOffset, Scratch::StateOffset + Scratch::StateSize);
      VALIDATE_EQUALS(Scratch::FluxOffset, Scratch::TimeAveragedStateOffset + Scratch::TimeAveragedStateSize);
      VALIDATE_EQUALS(Scratch::PointSourceOffset, Scratch::FluxOffset + Scratch::FluxSize);
      VALIDATE_EQUALS(Scratch::Size, Scratch::PointSourceOffset + Scratch::PointSourceSize);
    }
  }
}

UNIT_TEST("The scratch block of a solver reserves zero entries for a term that the solver does not declare") {
  GIVEN("The predictors of a solver with every term and of a solver without terms") {
    using Full = Predictor<FullSolver>;
    using Bare = Predictor<InertSolver>;

    THEN("The full solver evaluates and accumulates every term") {
      VALIDATE(Full::EvaluateFlux);
      VALIDATE(Full::EvaluateNonconservativeProduct);
      VALIDATE(Full::EvaluateSource);
      VALIDATE(Full::EvaluateMaterialParameters);
      VALIDATE(Full::EvaluatePointSources);
      VALIDATE(Full::AccumulatesFlux);
      VALIDATE(Full::AccumulatesSource);
    }

    THEN("The solver without terms skips every term and its block ends at the offset of the point sources") {
      VALIDATE(not Bare::EvaluateFlux);
      VALIDATE(not Bare::AccumulatesFlux);
      VALIDATE(not Bare::AccumulatesSource);
      VALIDATE_EQUALS(Bare::Scratch::PointSourceSize, 0);
      VALIDATE(Bare::Scratch::Size < Full::Scratch::Size);
      VALIDATE_EQUALS(Bare::Scratch::Size, Bare::Scratch::PointSourceOffset);
    }
  }
}

UNIT_TEST("The predictor writes the time averages of the Taylor series of a linear profile and keeps the cell state") {
  GIVEN("Basis tables and a cell whose state is linear along the first axis") {
    const Tables tables;
    const auto   cellSize = getCellSize();
    double       cellState[CellStateSize];
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = getLinearCellStateEntry(tables, cellNode, value, cellSize(0));
      }
    }

    WHEN("The predictor of a transporting flux and product and of a growing source runs") {
      FaceBuffers         faces;
      std::vector<double> scratch(ProfileScratch::Size, 0.0);
      predict<ProfileSolver>(tables, cellState, faces, scratch.data());
      const double* const timeAveragedState = scratch.data() + ProfileScratch::TimeAveragedStateOffset;
      const double* const flux              = scratch.data() + ProfileScratch::FluxOffset;

      THEN("The cell state keeps every value bitwise") {
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int value = 0; value < NumberOfData; value++) {
            VALIDATE_EQUALS(
              cellState[cellNode * NumberOfData + value],
              getLinearCellStateEntry(tables, cellNode, value, cellSize(0)),
              cellNode,
              value
            );
          }
        }
      }

      THEN("The scratch stores the time averages and the faces store the extrapolated averages and their fluxes") {
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
            VALIDATE_NUMERICAL_EQUALS(
              timeAveragedState[cellNode * StateStride + value],
              averagedState[cellNode * NumberOfData + value],
              cellNode,
              value
            );
          }
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              const double expectedFlux = (axis == 0) ? averagedFlux[unknown] : 0.0;
              VALIDATE_NUMERICAL_EQUALS(
                flux[axis * AxisStride + cellNode * UnknownStride + unknown],
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
              VALIDATE_NUMERICAL_EQUALS(faces.state[direction][entry], lower[value], direction, faceNode, value);
              VALIDATE_NUMERICAL_EQUALS(
                faces.state[direction + DIMENSIONS][entry],
                upper[value],
                direction,
                faceNode,
                value
              );
            }
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              const int entry = faceNode * NumberOfUnknowns + unknown;
              VALIDATE_NUMERICAL_EQUALS(
                faces.flux[direction][entry],
                fluxSpeed * lower[unknown],
                direction,
                faceNode,
                unknown
              );
              VALIDATE_NUMERICAL_EQUALS(
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
    }
  }
}

UNIT_TEST("A solver without flux gets zero face fluxes") {
  GIVEN("Basis tables, a cell whose state is linear along the first axis and face fluxes of a marker value") {
    const Tables tables;
    const auto   cellSize = getCellSize();
    double       cellState[CellStateSize];
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

    WHEN("The predictor of a solver with a nonconservative product and without flux runs") {
      std::vector<double> scratch(ProductScratch::Size, 0.0);
      predict<ProductSolver>(tables, cellState, faces, scratch.data());

      THEN("The predictor evaluates the product only and every face flux is zero") {
        VALIDATE(not Predictor<ProductSolver>::EvaluateFlux);
        VALIDATE(Predictor<ProductSolver>::EvaluateNonconservativeProduct);
        for (int face = 0; face < peano4::FacesPerCell; face++) {
          for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfUnknowns; entry++) {
            VALIDATE_EQUALS(faces.flux[face][entry], 0.0, face, entry);
          }
        }
      }
    }
  }
}

UNIT_TEST("A solver without a Riemann solver leaves the face fluxes as they are") {
  GIVEN("Basis tables, a cell whose state is linear along the first axis and face fluxes of a marker value") {
    const Tables tables;
    const auto   cellSize = getCellSize();
    double       cellState[CellStateSize];
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

    WHEN("The predictor of a solver with a flux and without a Riemann solver runs") {
      std::vector<double> scratch(MarkerScratch::Size, 0.0);
      predict<MarkerSolver>(tables, cellState, faces, scratch.data());

      THEN("The predictor evaluates the flux, skips the face fluxes and every face flux keeps the marker value") {
        VALIDATE(Predictor<MarkerSolver>::EvaluateFlux);
        VALIDATE(not Predictor<MarkerSolver>::WritesFaceFluxes);
        for (int face = 0; face < peano4::FacesPerCell; face++) {
          for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfUnknowns; entry++) {
            VALIDATE_EQUALS(faces.flux[face][entry], MarkerValue, face, entry);
          }
        }
      }
    }
  }
}

UNIT_TEST("The point source selection of every cell of a device range reproduces the serial host loop") {
  GIVEN("A chain of cells, a point source at the centre of cell one and one on the right face of cell four") {
    const exahype2::dg::unittests::CellChain geometry;
    const std::vector<double>                locations = getPointSourceLocations(geometry);

    WHEN("Every cell selects its point sources on the device and in a serial host loop") {
      std::vector<int> deviceIndicesResult(Setup::AllIndicesCardinality, -1);
      std::vector<int> deviceCountsResult(Setup::NumberOfCells, -1);
      selectPointSourcesOfEveryCell(geometry, locations, deviceIndicesResult, deviceCountsResult);

      std::vector<int> hostIndices(Setup::AllIndicesCardinality, -1);
      std::vector<int> hostCounts(Setup::NumberOfCells, -1);
      int              selectedSources = 0;
      for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
        const int                                   coordinateOffset = cell * DIMENSIONS;
        const tarch::la::Vector<DIMENSIONS, double> cellCentre(geometry.cellCentres.data() + coordinateOffset);
        const tarch::la::Vector<DIMENSIONS, double> cellSize(geometry.cellSizes.data() + coordinateOffset);
        int                                         pointSourceIndices[Setup::NumberOfPointSources];
        for (int entry = 0; entry < Setup::NumberOfPointSources; entry++) {
          pointSourceIndices[entry] = -1;
        }
        hostCounts[cell] = Predictor<exahype2::dg::ader::unittests::StubSolver>::selectPointSources(
          pointSourceIndices,
          locations.data(),
          cellCentre,
          cellSize
        );
        selectedSources += hostCounts[cell];
        for (int entry = 0; entry < Setup::NumberOfPointSources; entry++) {
          hostIndices[cell * Setup::NumberOfPointSources + entry] = pointSourceIndices[entry];
        }
      }

      THEN("Every cell lists the point sources of the host loop and the host loop selects both point sources") {
        for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
          VALIDATE_EQUALS(deviceCountsResult[cell], hostCounts[cell], cell);
          const int indexOffset = cell * Setup::NumberOfPointSources;
          for (int entry = 0; entry < Setup::NumberOfPointSources; entry++) {
            VALIDATE_EQUALS(deviceIndicesResult[indexOffset + entry], hostIndices[indexOffset + entry], cell, entry);
          }
        }
        VALIDATE_EQUALS(selectedSources, Setup::NumberOfPointSources);
      }
    }
  }
}

UNIT_TEST("The predictor of every cell of a device range reproduces the serial host loop") {
  GIVEN("Basis tables, a chain of cells with a ramp of cell states, two point sources and zeroed face buffers") {
    const Tables                             tables;
    const exahype2::dg::unittests::CellChain geometry;
    const std::vector<double>                locations = getPointSourceLocations(geometry);
    const std::vector<double>                cellStates =
      exahype2::dg::unittests::getRamp(Setup::AllCellStatesCardinality, 0.5, 0.002);
    const std::vector<double> faceStates(Setup::AllFaceStatesCardinality, 0.0);
    const std::vector<double> faceFluxes(Setup::AllFaceFluxesCardinality, 0.0);

    WHEN("Every cell runs the predictor on the device and in a serial host loop") {
      std::vector<double> deviceCellResult      = cellStates;
      std::vector<double> deviceFaceStateResult = faceStates;
      std::vector<double> deviceFaceFluxResult  = faceFluxes;
      predictOnDevice(tables, geometry, locations, deviceCellResult, deviceFaceStateResult, deviceFaceFluxResult);

      std::vector<double> hostCellStates = cellStates;
      std::vector<double> hostFaceStates = faceStates;
      std::vector<double> hostFaceFluxes = faceFluxes;
      predictOnHost(tables, geometry, locations, hostCellStates, hostFaceStates, hostFaceFluxes);

      THEN("The device and the host agree in the cell states, the face states and the face fluxes") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceCellResult, hostCellStates);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceFaceStateResult, hostFaceStates);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceFaceFluxResult, hostFaceFluxes);
      }
    }

    WHEN("Every cell runs the predictor on the device with cell states in single precision and in a serial host loop") {
      const std::vector<float> narrowCellStates(cellStates.begin(), cellStates.end());
      std::vector<float>       deviceCellResult      = narrowCellStates;
      std::vector<double>      deviceFaceStateResult = faceStates;
      std::vector<double>      deviceFaceFluxResult  = faceFluxes;
      predictOnDevice(tables, geometry, locations, deviceCellResult, deviceFaceStateResult, deviceFaceFluxResult);
      const std::vector<double> deviceCellStates(deviceCellResult.begin(), deviceCellResult.end());

      std::vector<double> hostCellStates(narrowCellStates.begin(), narrowCellStates.end());
      std::vector<double> hostFaceStates = faceStates;
      std::vector<double> hostFaceFluxes = faceFluxes;
      predictOnHost(tables, geometry, locations, hostCellStates, hostFaceStates, hostFaceFluxes);

      THEN("The device and the host agree in the cell states, the face states and the face fluxes") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceCellStates, hostCellStates);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceFaceStateResult, hostFaceStates);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceFaceFluxResult, hostFaceFluxes);
      }
    }
  }
}
