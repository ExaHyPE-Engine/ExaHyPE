// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/dg/ader/NonlinearPredictor.h"

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/ader/PicardOutcome.h"
#include "exahype2/dg/ader/TeamWorkers.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/Range.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/exahype2/dg/KernelTestData.h"
#include "unittests/exahype2/dg/ader/KernelTestData.h"

namespace {
  using Setup      = exahype2::dg::ader::unittests::KernelTestSetup;
  using Tables     = exahype2::dg::unittests::BasisTableSet;
  using StubLayout = exahype2::dg::ader::unittests::StubLayout;
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

  constexpr double TimeStamp       = 0.75;
  constexpr double TimeStepSize    = 0.125;
  constexpr double PicardTolerance = 1.0e-8;
  constexpr double Slope           = 0.5;
  constexpr double StateSlope      = 3.0;

  constexpr double FluxSpeed               = 0.5;
  constexpr double DiffusionSpeed          = 0.25;
  constexpr double ProductSpeed            = 0.75;
  constexpr double SourceStrength          = 1.5;
  constexpr double DiffusiveSourceStrength = 0.5;

  static_assert(NumberOfData > NumberOfUnknowns, "The auxiliary variable follows the unknowns of a node.");

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

  /**
   * The Picard iteration writes the unknowns only. Every sweep evaluates the same flux.
   */
  struct AuxiliaryVariableFluxTerm {
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
        fluxValues[unknown] = (1.0 + unknown) * (1.0 + direction) * state[NumberOfUnknowns];
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

  /**
   * The Picard iteration writes the unknowns only. Every sweep adds the same right-hand side.
   */
  struct AuxiliaryVariableSourceTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         sourceValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        sourceValues[unknown] = (1.0 + unknown) * state[NumberOfUnknowns];
      }
    }
  };

  struct SinglePrecisionAuxiliaryVariableSourceTerm {
    static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
      const float* NOALIAS const                                    state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      float* NOALIAS const                                          sourceValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        sourceValues[unknown] = (1.0F + static_cast<float>(unknown)) * state[NumberOfUnknowns];
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
  struct AuxiliaryVariableFluxSolver final : StubLayout, AuxiliaryVariableFluxTerm {};
  struct SourceSolver final : StubLayout, SourceTerm {};
  struct AuxiliaryVariableSourceSolver final : StubLayout, AuxiliaryVariableSourceTerm {};
  struct SinglePrecisionAuxiliaryVariableSourceSolver final : StubLayout, SinglePrecisionAuxiliaryVariableSourceTerm {};
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
  using StubScratch    = Predictor<exahype2::dg::ader::unittests::StubSolver>::Scratch;

  /**
   * The time node whose flux and state gradient the scratch stores when predict() returns.
   */
  constexpr int LastTimeNode = NodesPerAxis - 1;

  /**
   * The square of this Picard tolerance exceeds every squared residual of the tests. predict() stops
   * after the first sweep. That sweep evaluates every term on the cell state.
   */
  constexpr double FirstSweepTolerance = 1.0e10;

  /**
   * The value of every scratch entry before a call. An entry that the call leaves unwritten keeps this value
   * and fails its check.
   */
  constexpr double StaleValue = 9.0;

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

  double getTimePositionOf(const Tables& tables, const int timeNode) {
    ASSERTION(timeNode >= 0 and timeNode < NodesPerAxis, timeNode);
    return TimeStamp + tables.quadraturePoints[timeNode] * TimeStepSize;
  }

  /**
   * Returns the factor with which the source of a space-time node enters the right-hand side.
   */
  double getUpdateSizeAt(const Tables& tables, const int timeNode, const int cellNode) {
    ASSERTION(timeNode >= 0 and timeNode < NodesPerAxis, timeNode);
    ASSERTION(cellNode >= 0 and cellNode < CellLayout::NodesPerCell, cellNode);
    return tables.quadratureWeights[timeNode] * tables.cellQuadratureWeights[cellNode] * TimeStepSize;
  }

  double getStateValueAt(const int cellNode, const int value) { return 1.0 + 0.5 * cellNode + value; }

  /**
   * Returns one value at a cell node of a cell state that grows quadratically along every axis.
   */
  double getQuadraticStateEntry(const Tables& tables, const int cellNode, const int value) {
    ASSERTION(cellNode >= 0 and cellNode < CellLayout::NodesPerCell, cellNode);
    const auto nodeIndex = peano4::dDelinearised(cellNode, NodesPerAxis);
    double     profile   = 1.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      const double reference = tables.quadraturePoints[nodeIndex(axis)];
      profile += (1.0 + axis) * reference * (1.0 + reference);
    }
    return (1.0 + value) * profile;
  }

  /**
   * Returns the physical derivative of getQuadraticStateEntry() along an axis at a cell node.
   */
  double getQuadraticStateDerivative(const Tables& tables, const int axis, const int cellNode, const int value) {
    ASSERTION(axis >= 0 and axis < DIMENSIONS, axis);
    ASSERTION(cellNode >= 0 and cellNode < CellLayout::NodesPerCell, cellNode);
    const double reference = tables.quadraturePoints[peano4::dDelinearised(cellNode, NodesPerAxis)(axis)];
    return (1.0 + value) * (1.0 + axis) * (1.0 + 2.0 * reference) * getInverseCellSize()(axis);
  }

  void fillQuadraticCellState(const Tables& tables, double (&cellState)[CellStateSize]) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = getQuadraticStateEntry(tables, cellNode, value);
      }
    }
  }

  /**
   * Fills a cell state that grows linearly along every axis with the reference slope StateSlope. The
   * nodal derivative of its interpolant is exact.
   */
  void fillSlopedCellState(const Tables& tables, double (&cellState)[CellStateSize]) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const auto nodeIndex = peano4::dDelinearised(cellNode, NodesPerAxis);
      double     reference = 0.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        reference += (1.0 + axis) * StateSlope * tables.quadraturePoints[nodeIndex(axis)];
      }
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = (1.0 + value) * (2.0 + reference);
      }
    }
  }

  /**
   * Returns the physical derivative of an unknown of fillSlopedCellState() along an axis.
   */
  double getSlopedStateDerivative(const int axis, const int unknown) {
    ASSERTION(axis >= 0 and axis < DIMENSIONS, axis);
    ASSERTION(unknown >= 0 and unknown < NumberOfUnknowns, unknown);
    return (1.0 + unknown) * (1.0 + axis) * StateSlope * getInverseCellSize()(axis);
  }

  /**
   * Returns the nonconservative product of ProductTerm on fillSlopedCellState(), summed over the axes.
   */
  double getProductOfSlopedState(const int unknown) {
    double product = 0.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      product += (1.0 + 0.25 * axis) * getSlopedStateDerivative(axis, unknown);
    }
    return product;
  }

  /**
   * Returns the update of the first Picard sweep of ProductSourceSolver on fillSlopedCellState() at a
   * space-time node. The source of that sweep grows linearly in time. The sweep integrates it exactly.
   */
  double getFirstUpdateOfSlopedState(
    const Tables& tables,
    const double (&cellState)[CellStateSize],
    const int timeNode,
    const int cellNode,
    const int unknown
  ) {
    ASSERTION(timeNode >= 0 and timeNode < NodesPerAxis, timeNode);
    ASSERTION(cellNode >= 0 and cellNode < CellLayout::NodesPerCell, cellNode);
    const double elapsedTime = tables.quadraturePoints[timeNode] * TimeStepSize;
    const double sourceAtTheTimeStamp =
      2.0 * cellState[cellNode * NumberOfData + unknown] + TimeStamp - getProductOfSlopedState(unknown);
    return elapsedTime * sourceAtTheTimeStamp + 0.5 * elapsedTime * elapsedTime;
  }

  /**
   * Returns the source of ProductSourceSolver in the second Picard sweep at a space-time node. That sweep
   * evaluates the terms on the first iterate of the time node. The first sweep scales the gradient of that
   * iterate by one plus twice the elapsed time.
   */
  double getSecondSourceOfSlopedState(
    const Tables& tables,
    const double (&cellState)[CellStateSize],
    const int timeNode,
    const int cellNode,
    const int unknown
  ) {
    const double elapsedTime = tables.quadraturePoints[timeNode] * TimeStepSize;
    const double iterate     = cellState[cellNode * NumberOfData + unknown]
      + getFirstUpdateOfSlopedState(tables, cellState, timeNode, cellNode, unknown);
    return 2.0 * iterate + getTimePositionOf(tables, timeNode)
      - (1.0 + 2.0 * elapsedTime) * getProductOfSlopedState(unknown);
  }

  /**
   * Returns the source of the quadratic cell state at a space-time node, summed over the terms the flags
   * name: The source of SourceTerm, the diffusive source of DiffusiveSourceTerm and the negated product of
   * ProductTerm.
   */
  double getSourceOfQuadraticState(
    const Tables& tables,
    const int     timeNode,
    const int     cellNode,
    const int     unknown,
    const bool    evaluateNonconservativeProduct,
    const bool    evaluateSource,
    const bool    evaluateDiffusiveSource
  ) {
    ASSERTION(unknown >= 0 and unknown < NumberOfUnknowns, unknown);
    double source = 0.0;
    if (evaluateSource) {
      source += 2.0 * getQuadraticStateEntry(tables, cellNode, unknown) + getTimePositionOf(tables, timeNode);
    }
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      const double derivative = getQuadraticStateDerivative(tables, axis, cellNode, unknown);
      if (evaluateDiffusiveSource) {
        source += (1.0 + axis) * derivative;
      }
      if (evaluateNonconservativeProduct) {
        source -= (1.0 + 0.25 * axis) * derivative;
      }
    }
    return source;
  }

  /**
   * Returns the basis tables with the identity as inverse Picard operator and unit inverse cell weights.
   * A Picard sweep on these tables adds the right-hand side to the cell state.
   */
  exahype2::dg::BasisTables<double> getTablesWithIdentityPicardOperator(
    const Tables& tables,
    double (&inversePicardOperator)[OperatorEntries],
    double (&inverseCellQuadratureWeights)[CellLayout::NodesPerCell]
  ) {
    fillIdentity(inversePicardOperator);
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      inverseCellQuadratureWeights[cellNode] = 1.0;
    }
    auto view                         = tables.getView();
    view.inversePicardOperator        = inversePicardOperator;
    view.inverseCellQuadratureWeights = inverseCellQuadratureWeights;
    return view;
  }

  /**
   * Returns one value of the iterate of AuxiliaryVariableSourceSolver at a space-time node on the tables of
   * getTablesWithIdentityPicardOperator(), in the operation order of predict(). A padding lane stores zero.
   */
  double getIterateOfAuxiliaryVariableSource(
    const Tables& tables,
    const double (&cellState)[CellStateSize],
    const int timeNode,
    const int cellNode,
    const int value
  ) {
    ASSERTION(value >= 0 and value < StateStride, value);
    if (value >= NumberOfData) {
      return 0.0;
    }
    const double stateValue = cellState[cellNode * NumberOfData + value];
    if (value >= NumberOfUnknowns) {
      return stateValue;
    }
    const double source        = (1.0 + value) * cellState[cellNode * NumberOfData + NumberOfUnknowns];
    const double rightHandSide = getUpdateSizeAt(tables, timeNode, cellNode) * source;
    return stateValue + rightHandSide;
  }

  template <typename Value, int ValuesPerFace>
  std::array<Value*, peano4::FacesPerCell> getRowsOf(Value (&rows)[peano4::FacesPerCell][ValuesPerFace]) {
    std::array<Value*, peano4::FacesPerCell> addresses{};
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      addresses[static_cast<std::size_t>(face)] = rows[face];
    }
    return addresses;
  }

  template <typename Precision>
  struct FaceBuffersOf final {
    FaceBuffersOf()                                      = default;
    FaceBuffersOf(const FaceBuffersOf& other)            = delete;
    FaceBuffersOf(FaceBuffersOf&& other)                 = delete;
    FaceBuffersOf& operator=(const FaceBuffersOf& other) = delete;
    FaceBuffersOf& operator=(FaceBuffersOf&& other)      = delete;
    ~FaceBuffersOf()                                     = default;

    Precision state[peano4::FacesPerCell][CellLayout::NodesPerFace * NumberOfData]{};
    Precision flux[peano4::FacesPerCell][CellLayout::NodesPerFace * NumberOfUnknowns]{};
    std::array<Precision*, peano4::FacesPerCell> statePointers{getRowsOf(state)};
    std::array<Precision*, peano4::FacesPerCell> fluxPointers{getRowsOf(flux)};
  };

  using FaceBuffers = FaceBuffersOf<double>;

  /**
   * Single-precision copies of the basis tables of the tests.
   */
  struct SinglePrecisionTables final {
    std::vector<float> quadraturePoints{};
    std::vector<float> quadratureWeights{};
    std::vector<float> inverseQuadratureWeights{};
    std::vector<float> cellQuadratureWeights{};
    std::vector<float> inverseCellQuadratureWeights{};
    std::vector<float> stiffnessOperator{};
    std::vector<float> stiffnessOperatorTransposed{};
    std::vector<float> derivativeOperator{};
    std::vector<float> inversePicardOperator{};
    std::vector<float> leftFaceCoefficients{};
    std::vector<float> rightFaceCoefficients{};

    [[nodiscard]] exahype2::dg::BasisTables<float> getView() const {
      return exahype2::dg::BasisTables<float>{
        quadraturePoints.data(),
        quadratureWeights.data(),
        inverseQuadratureWeights.data(),
        cellQuadratureWeights.data(),
        inverseCellQuadratureWeights.data(),
        stiffnessOperator.data(),
        stiffnessOperatorTransposed.data(),
        derivativeOperator.data(),
        inversePicardOperator.data(),
        leftFaceCoefficients.data(),
        rightFaceCoefficients.data()
      };
    }
  };

  std::vector<float> getSinglePrecisionCopyOf(const std::vector<double>& values) {
    std::vector<float> copy(values.size());
    for (std::size_t entry = 0; entry < values.size(); entry++) {
      copy[entry] = static_cast<float>(values[entry]);
    }
    return copy;
  }

  /**
   * Returns single-precision copies of the basis tables with the identity as inverse Picard operator and unit inverse
   * cell weights, like getTablesWithIdentityPicardOperator().
   */
  SinglePrecisionTables getSinglePrecisionTablesWithIdentityPicardOperator(const Tables& tables) {
    SinglePrecisionTables copies{
      getSinglePrecisionCopyOf(tables.quadraturePoints),
      getSinglePrecisionCopyOf(tables.quadratureWeights),
      getSinglePrecisionCopyOf(tables.inverseQuadratureWeights),
      getSinglePrecisionCopyOf(tables.cellQuadratureWeights),
      std::vector<float>(CellLayout::NodesPerCell, 1.0F),
      getSinglePrecisionCopyOf(tables.stiffnessOperator),
      getSinglePrecisionCopyOf(tables.stiffnessOperatorTransposed),
      getSinglePrecisionCopyOf(tables.derivativeOperator),
      std::vector<float>(OperatorEntries, 0.0F),
      getSinglePrecisionCopyOf(tables.leftFaceCoefficients),
      getSinglePrecisionCopyOf(tables.rightFaceCoefficients)
    };
    for (int node = 0; node < NodesPerAxis; node++) {
      copies.inversePicardOperator[static_cast<std::size_t>(node * NodesPerAxis + node)] = 1.0F;
    }
    return copies;
  }

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

  /**
   * Returns one value of the time-averaged state of ProfileSolver on the linear cell state: Every unknown
   * moves at the rate of the terms for half a time step.
   */
  double getAveragedProfileEntry(const Tables& tables, const int cellNode, const int value) {
    constexpr double Rate  = -FluxSpeed * Slope + SourceStrength + (DiffusiveSourceStrength - ProductSpeed) * Slope;
    const double     moved = value < NumberOfUnknowns ? 0.5 * TimeStepSize * Rate : 0.0;
    return getLinearCellStateEntry(tables, cellNode, value, getCellSize()(0)) + moved;
  }

  /**
   * Runs the predictor of Solver on one cell with the centre, the size and the time stamp of the tests.
   */
  template <typename Solver, typename StoragePrecision>
  int predictCell(
    FaceBuffers&                             faces,
    const StoragePrecision* const            cellState,
    std::vector<double>&                     scratch,
    const exahype2::dg::BasisTables<double>& tables,
    const double                             timeStepSize,
    const double                             picardTolerance
  ) {
    ASSERTION(scratch.size() == static_cast<std::size_t>(Predictor<Solver>::Scratch::Size), scratch.size());
    return Predictor<Solver>::predict(
      faces.statePointers.data(),
      faces.fluxPointers.data(),
      cellState,
      scratch.data(),
      tables,
      getCellCentre(),
      getCellSize(),
      TimeStamp,
      timeStepSize,
      picardTolerance
    );
  }

  /**
   * Returns the scratch of the first Picard sweep of Solver on the cell state, with StaleValue in every entry
   * before the sweep.
   */
  template <typename Solver>
  std::vector<double> getScratchOfFirstSweep(const Tables& tables, const double (&cellState)[CellStateSize]) {
    FaceBuffers                faces;
    std::vector<double>        scratch(Predictor<Solver>::Scratch::Size, StaleValue);
    [[maybe_unused]] const int numberOfSweeps =
      predictCell<Solver>(faces, cellState, scratch, tables.getView(), TimeStepSize, FirstSweepTolerance);
    ASSERTION(numberOfSweeps == 1, numberOfSweeps);
    return scratch;
  }

  /**
   * Compares the right-hand side, the first source and the source average in the scratch of the first Picard
   * sweep of Derived on the quadratic cell state against the sum of the terms the flags name.
   */
  template <typename Derived>
  void validateSourceOf(
    const Tables&              tables,
    const std::vector<double>& scratch,
    const bool                 evaluateNonconservativeProduct,
    const bool                 evaluateSource,
    const bool                 evaluateDiffusiveSource
  ) {
    using Scratch = typename Predictor<Derived>::Scratch;
    ASSERTION(scratch.size() == static_cast<std::size_t>(Scratch::Size), scratch.size());

    const auto getSourceAt = [&](const int timeNode, const int cellNode, const int unknown) {
      return getSourceOfQuadraticState(
        tables,
        timeNode,
        cellNode,
        unknown,
        evaluateNonconservativeProduct,
        evaluateSource,
        evaluateDiffusiveSource
      );
    };

    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int sourceEntry = cellNode * UnknownStride;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const double firstSource    = getSourceAt(0, cellNode, unknown);
        double       averagedSource = firstSource;
        for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
          const int    spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
          const double source        = getSourceAt(timeNode, cellNode, unknown);
          averagedSource += tables.quadratureWeights[timeNode] * (source - firstSource);
          VALIDATE_NUMERICAL_EQUALS(
            scratch[Scratch::RightHandSideOffset + spaceTimeNode * UnknownStride + unknown],
            getUpdateSizeAt(tables, timeNode, cellNode) * source,
            timeNode,
            cellNode,
            unknown
          );
        }
        VALIDATE_NUMERICAL_EQUALS(
          scratch[Scratch::TimeAveragedSourceOffset + sourceEntry + unknown],
          averagedSource,
          cellNode,
          unknown
        );
        VALIDATE_NUMERICAL_EQUALS(
          scratch[Scratch::FirstSourceOffset + sourceEntry + unknown],
          firstSource,
          cellNode,
          unknown
        );
      }
      for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
        for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
          const int spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
          VALIDATE_EQUALS(
            scratch[Scratch::RightHandSideOffset + spaceTimeNode * UnknownStride + unknown],
            0.0,
            timeNode,
            cellNode,
            unknown
          );
        }
        VALIDATE_EQUALS(scratch[Scratch::TimeAveragedSourceOffset + sourceEntry + unknown], 0.0, cellNode, unknown);
        VALIDATE_EQUALS(scratch[Scratch::FirstSourceOffset + sourceEntry + unknown], 0.0, cellNode, unknown);
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
      const int outcome = Predictor<exahype2::dg::ader::unittests::StubSolver>::predict(
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
    const Tables&                             tables,
    const exahype2::dg::unittests::CellChain& geometry,
    std::vector<double>&                      cellStates,
    std::vector<double>&                      faceStates,
    std::vector<double>&                      faceFluxes
  ) {
    const exahype2::dg::unittests::DeviceBasisTables deviceTables(tables);
    const std::vector<double> scratch(static_cast<std::size_t>(Setup::NumberOfCells) * StubScratch::Size, 0.0);
    std::vector<int>          sweeps(Setup::NumberOfCells, 0);

    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellStates(cellStates);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceFaceStates(faceStates);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceFaceFluxes(faceFluxes);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceScratch(scratch);
    const exahype2::dg::unittests::DeviceBuffer<int>    deviceSweeps(sweeps);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);

    const int threadsPerTeam = tarch::accelerator::getTeamThreads(CellLayout::NodesPerCell);
    const int threadsPerCell = std::min(CellLayout::NodesPerCell, threadsPerTeam);
    const int cellsPerTeam   = threadsPerTeam / threadsPerCell;
    tarch::accelerator::parallelFor(
      tarch::accelerator::TeamRange(
        (Setup::NumberOfCells + cellsPerTeam - 1) / cellsPerTeam,
        threadsPerTeam,
        Predictor<exahype2::dg::ader::unittests::StubSolver>::getTeamScratchBytes(threadsPerTeam)
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

  std::vector<int> predictOnHost(
    const Tables&                             tables,
    const exahype2::dg::unittests::CellChain& geometry,
    std::vector<double>&                      cellStates,
    std::vector<double>&                      faceStates,
    std::vector<double>&                      faceFluxes
  ) {
    ASSERTION(cellStates.size() == static_cast<std::size_t>(Setup::AllCellStatesCardinality), cellStates.size());
    ASSERTION(faceStates.size() == static_cast<std::size_t>(Setup::AllFaceStatesCardinality), faceStates.size());
    ASSERTION(faceFluxes.size() == static_cast<std::size_t>(Setup::AllFaceFluxesCardinality), faceFluxes.size());
    std::vector<double> scratch(static_cast<std::size_t>(Setup::NumberOfCells) * StubScratch::Size, 0.0);
    std::vector<int>    sweeps(Setup::NumberOfCells, 0);
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
      sweeps[cell] = Predictor<exahype2::dg::ader::unittests::StubSolver>::predict(
        faceState,
        faceFlux,
        cellStates.data() + stateOffset,
        scratch.data() + scratchOffset,
        hostTables,
        cellCentre,
        cellSize,
        Setup::TimeStamp,
        Setup::TimeStepSize,
        Setup::PicardTolerance
      );
    }
    return sweeps;
  }
} // namespace

UNIT_TEST("The space-time state of a solver without terms repeats the cell state in every time slice") {
  GIVEN("A solver without terms, a cell state in single precision and a scratch of stale values") {
    using Scratch = Predictor<InertSolver>::Scratch;
    const Tables tables;
    float        cellState[CellStateSize];
    for (int entry = 0; entry < CellStateSize; entry++) {
      cellState[entry] = 1.0F + static_cast<float>(entry);
    }
    std::vector<double> scratch(Scratch::Size, StaleValue);

    WHEN("The predictor runs") {
      FaceBuffers faces;
      predictCell<InertSolver>(faces, cellState, scratch, tables.getView(), TimeStepSize, PicardTolerance);

      THEN("Every time slice stores the cell state in double precision and every padding lane stores zero") {
        for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
          for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
            const int spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
            const int firstEntry    = Scratch::SpaceTimeStateOffset + spaceTimeNode * StateStride;
            for (int value = 0; value < NumberOfData; value++) {
              VALIDATE_NUMERICAL_EQUALS(
                scratch[firstEntry + value],
                static_cast<double>(cellState[cellNode * NumberOfData + value]),
                timeNode,
                cellNode,
                value
              );
            }
            for (int value = NumberOfData; value < StateStride; value++) {
              VALIDATE_NUMERICAL_EQUALS(scratch[firstEntry + value], 0.0, cellNode, value);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("Every time slice of the right-hand side starts from zero and keeps its own source") {
  GIVEN("A source that scales the auxiliary variable, a cell state and a scratch of stale values") {
    using Scratch = Predictor<AuxiliaryVariableSourceSolver>::Scratch;
    const Tables tables;
    double       cellState[CellStateSize] = {};
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      cellState[cellNode * NumberOfData + NumberOfUnknowns] = 1.0 + 0.5 * cellNode;
    }
    std::vector<double> scratch(Scratch::Size, StaleValue);

    WHEN("The predictor runs") {
      FaceBuffers faces;
      predictCell<AuxiliaryVariableSourceSolver>(
        faces,
        cellState,
        scratch,
        tables.getView(),
        TimeStepSize,
        PicardTolerance
      );

      THEN("Every time slice stores the quadrature of its own source and every padding lane stores zero") {
        for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
          for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
            const int    spaceTimeNode     = timeNode * CellLayout::NodesPerCell + cellNode;
            const int    firstEntry        = Scratch::RightHandSideOffset + spaceTimeNode * UnknownStride;
            const double auxiliaryVariable = cellState[cellNode * NumberOfData + NumberOfUnknowns];
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              VALIDATE_NUMERICAL_EQUALS(
                scratch[firstEntry + unknown],
                getUpdateSizeAt(tables, timeNode, cellNode) * (1.0 + unknown) * auxiliaryVariable,
                timeNode,
                cellNode,
                unknown
              );
            }
            for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
              VALIDATE_EQUALS(scratch[firstEntry + unknown], 0.0, timeNode, cellNode, unknown);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("A flux that is constant along every axis leaves the right-hand side at zero bitwise") {
  GIVEN("A flux that scales the auxiliary variable, five thirds at every node and a scratch of stale values") {
    using Scratch = Predictor<AuxiliaryVariableFluxSolver>::Scratch;
    const Tables tables;

    // The double nearest to 5/3 is inexact. Every product with it rounds.
    double cellState[CellStateSize] = {};
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      cellState[cellNode * NumberOfData + NumberOfUnknowns] = 5.0 / 3.0;
    }
    std::vector<double> scratch(Scratch::Size, StaleValue);

    WHEN("The predictor runs") {
      FaceBuffers faces;
      predictCell<AuxiliaryVariableFluxSolver>(
        faces,
        cellState,
        scratch,
        tables.getView(),
        TimeStepSize,
        PicardTolerance
      );

      THEN("Every entry of the right-hand side is zero bitwise") {
        for (int entry = 0; entry < CellLayout::SpaceTimeNodes * UnknownStride; entry++) {
          VALIDATE_EQUALS(scratch[Scratch::RightHandSideOffset + entry], 0.0, entry);
        }
      }
    }
  }
}

UNIT_TEST("A flux that grows linearly subtracts its exact divergence from every time slice") {
  GIVEN("A flux that scales the auxiliary variable, a linear function of the position and a scratch of stale values") {
    using Scratch              = Predictor<AuxiliaryVariableFluxSolver>::Scratch;
    constexpr double FluxSlope = 3.0;
    const Tables     tables;

    // Every flux block grows linearly along every axis. The nodal derivative of its interpolant is the slope.
    double cellState[CellStateSize] = {};
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const auto nodeIndex = peano4::dDelinearised(cellNode, NodesPerAxis);
      double     reference = 0.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        reference += tables.quadraturePoints[nodeIndex(axis)];
      }
      cellState[cellNode * NumberOfData + NumberOfUnknowns] = 2.0 + FluxSlope * reference;
    }
    std::vector<double> scratch(Scratch::Size, StaleValue);

    WHEN("The predictor runs") {
      FaceBuffers faces;
      predictCell<AuxiliaryVariableFluxSolver>(
        faces,
        cellState,
        scratch,
        tables.getView(),
        TimeStepSize,
        PicardTolerance
      );

      THEN("Every time slice stores the weighted negated divergence and every padding lane stores zero") {
        const auto inverseCellSize = getInverseCellSize();
        for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
          for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
            const int    spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
            const int    firstEntry    = Scratch::RightHandSideOffset + spaceTimeNode * UnknownStride;
            const double weight        = tables.quadratureWeights[timeNode] * tables.cellQuadratureWeights[cellNode];
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              double expected = 0.0;
              for (int axis = 0; axis < DIMENSIONS; axis++) {
                expected -= weight * TimeStepSize * inverseCellSize(axis) * (1.0 + unknown) * FluxSlope * (1.0 + axis);
              }
              VALIDATE_NUMERICAL_EQUALS(scratch[firstEntry + unknown], expected, timeNode, cellNode, unknown);
            }
            for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
              VALIDATE_NUMERICAL_EQUALS(scratch[firstEntry + unknown], 0.0, timeNode, cellNode, unknown);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("A Picard sweep without terms reproduces the cell state bitwise") {
  GIVEN("A solver without terms and a cell state without an exact binary representation") {
    using Scratch = Predictor<InertSolver>::Scratch;
    const Tables tables;

    // The double nearest to one third is inexact. Every product with these values rounds.
    double cellState[CellStateSize];
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = 1.0 / 3.0 + 0.5 * cellNode + value;
      }
    }

    WHEN("The predictor runs") {
      FaceBuffers         faces;
      std::vector<double> scratch(Scratch::Size, StaleValue);
      const int           numberOfSweeps =
        predictCell<InertSolver>(faces, cellState, scratch, tables.getView(), TimeStepSize, PicardTolerance);

      THEN("The iteration stops after one sweep and every time slice stores the cell state bitwise") {
        VALIDATE_EQUALS(numberOfSweeps, 1);
        for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
          for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
            const int spaceTimeNode = cellNode + CellLayout::NodesPerCell * timeNode;
            for (int value = 0; value < NumberOfData; value++) {
              VALIDATE_EQUALS(
                scratch[Scratch::SpaceTimeStateOffset + spaceTimeNode * StateStride + value],
                cellState[cellNode * NumberOfData + value],
                timeNode,
                cellNode,
                value
              );
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("A Picard sweep measures the squared distance of the new from the previous iterate and stores the new one") {
  GIVEN("A source that scales the auxiliary variable, the identity as inverse Picard operator and a cell state") {
    using Scratch = Predictor<AuxiliaryVariableSourceSolver>::Scratch;
    const Tables tables;
    double       inversePicardOperator[OperatorEntries];
    double       inverseCellQuadratureWeights[CellLayout::NodesPerCell];
    const auto   identityTables =
      getTablesWithIdentityPicardOperator(tables, inversePicardOperator, inverseCellQuadratureWeights);

    double cellState[CellStateSize] = {};
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      cellState[cellNode * NumberOfData + NumberOfUnknowns] = 1.0 + 0.5 * cellNode;
    }

    WHEN("The predictor runs with squared tolerances a numerical tolerance above and below the squared first update") {
      double squaredFirstUpdate = 0.0;
      for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            const double update = getIterateOfAuxiliaryVariableSource(tables, cellState, timeNode, cellNode, unknown)
              - cellState[cellNode * NumberOfData + unknown];
            squaredFirstUpdate += update * update;
          }
        }
      }
      FaceBuffers         faces;
      std::vector<double> scratch(Scratch::Size, 0.0);
      const int           sweepsAbove = predictCell<AuxiliaryVariableSourceSolver>(
        faces,
        cellState,
        scratch,
        identityTables,
        TimeStepSize,
        std::sqrt(squaredFirstUpdate + tarch::testing::NumericalTolerance)
      );
      const int sweepsBelow = predictCell<AuxiliaryVariableSourceSolver>(
        faces,
        cellState,
        scratch,
        identityTables,
        TimeStepSize,
        std::sqrt(squaredFirstUpdate - tarch::testing::NumericalTolerance)
      );

      THEN("The first sweep stops under the tolerance above and continues under the tolerance below") {
        VALIDATE_EQUALS(sweepsAbove, 1);
        VALIDATE_EQUALS(sweepsBelow, 2);
      }
    }

    WHEN("The predictor runs with the numerical tolerance as squared tolerance") {
      FaceBuffers         faces;
      std::vector<double> scratch(Scratch::Size, 0.0);
      const int           numberOfSweeps = predictCell<AuxiliaryVariableSourceSolver>(
        faces,
        cellState,
        scratch,
        identityTables,
        TimeStepSize,
        std::sqrt(tarch::testing::NumericalTolerance)
      );

      THEN("The second sweep reaches a zero distance and the later time slices store the new iterate") {
        VALIDATE_EQUALS(numberOfSweeps, 2);
        for (int timeNode = 1; timeNode < NodesPerAxis; timeNode++) {
          for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
            const int spaceTimeNode = cellNode + CellLayout::NodesPerCell * timeNode;
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              VALIDATE_NUMERICAL_EQUALS(
                scratch[Scratch::SpaceTimeStateOffset + spaceTimeNode * StateStride + unknown],
                getIterateOfAuxiliaryVariableSource(tables, cellState, timeNode, cellNode, unknown),
                timeNode,
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

UNIT_TEST("An iterate above the divergence bound reports a diverged Picard iteration") {
  GIVEN("A source that scales the auxiliary variable, the identity as inverse Picard operator and a zero cell state") {
    using Scratch = Predictor<AuxiliaryVariableSourceSolver>::Scratch;
    const Tables tables;
    double       inversePicardOperator[OperatorEntries];
    double       inverseCellQuadratureWeights[CellLayout::NodesPerCell];
    const auto   identityTables =
      getTablesWithIdentityPicardOperator(tables, inversePicardOperator, inverseCellQuadratureWeights);
    double cellState[CellStateSize] = {};

    WHEN("The auxiliary variable of the first node is 1e30 and the predictor runs") {
      cellState[NumberOfUnknowns] = 1e30;
      FaceBuffers         faces;
      std::vector<double> scratch(Scratch::Size, 0.0);
      const int           outcome = predictCell<AuxiliaryVariableSourceSolver>(
        faces,
        cellState,
        scratch,
        identityTables,
        TimeStepSize,
        PicardTolerance
      );

      THEN("The predictor reports a diverged Picard iteration") {
        VALIDATE_EQUALS(outcome, exahype2::dg::ader::DivergedPicardIteration);
      }
    }
  }
}

#if not defined(PEANO_WITH_FPE)
UNIT_TEST("An iterate that is not a number reports a diverged Picard iteration") {
  GIVEN("A source that scales the auxiliary variable, the identity as inverse Picard operator and a zero cell state") {
    using Scratch = Predictor<AuxiliaryVariableSourceSolver>::Scratch;
    const Tables tables;
    double       inversePicardOperator[OperatorEntries];
    double       inverseCellQuadratureWeights[CellLayout::NodesPerCell];
    const auto   identityTables =
      getTablesWithIdentityPicardOperator(tables, inversePicardOperator, inverseCellQuadratureWeights);
    double cellState[CellStateSize] = {};

    WHEN("The auxiliary variable of the first node is not a number and the predictor runs") {
      cellState[NumberOfUnknowns] = std::numeric_limits<double>::quiet_NaN();
      FaceBuffers         faces;
      std::vector<double> scratch(Scratch::Size, 0.0);
      const int           outcome = predictCell<AuxiliaryVariableSourceSolver>(
        faces,
        cellState,
        scratch,
        identityTables,
        TimeStepSize,
        PicardTolerance
      );

      THEN("The predictor reports a diverged Picard iteration") {
        VALIDATE_EQUALS(outcome, exahype2::dg::ader::DivergedPicardIteration);
      }
    }
  }
}
#endif

UNIT_TEST("An iterate whose squared change lies between the largest float and 1e40 diverges in single precision only") {
  GIVEN("A source that scales the auxiliary variable of the first node to a squared change of 1e39 per sweep") {
    using Scratch = Predictor<AuxiliaryVariableSourceSolver>::Scratch;
    using SinglePrecisionPredictor =
      exahype2::dg::ader::NonlinearPredictor<SinglePrecisionAuxiliaryVariableSourceSolver, float>;
    const Tables tables;
    double       inversePicardOperator[OperatorEntries];
    double       inverseCellQuadratureWeights[CellLayout::NodesPerCell];
    const auto   identityTables =
      getTablesWithIdentityPicardOperator(tables, inversePicardOperator, inverseCellQuadratureWeights);
    const SinglePrecisionTables singlePrecisionTables = getSinglePrecisionTablesWithIdentityPicardOperator(tables);

    double unitCellState[CellStateSize] = {};
    unitCellState[NumberOfUnknowns]     = 1.0;
    double largestChange                = 0.0;
    for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        largestChange = std::max(
          largestChange,
          std::abs(getIterateOfAuxiliaryVariableSource(tables, unitCellState, timeNode, 0, unknown))
        );
      }
    }
    const double auxiliary = std::sqrt(1.0e39) / largestChange;

    WHEN("A double-precision and a single-precision predictor run") {
      double cellState[CellStateSize] = {};
      cellState[NumberOfUnknowns]     = auxiliary;
      FaceBuffers         faces;
      std::vector<double> scratch(Scratch::Size, 0.0);
      const int           doubleOutcome = predictCell<AuxiliaryVariableSourceSolver>(
        faces,
        cellState,
        scratch,
        identityTables,
        TimeStepSize,
        PicardTolerance
      );

      float singlePrecisionCellState[CellStateSize] = {};
      singlePrecisionCellState[NumberOfUnknowns]    = static_cast<float>(auxiliary);
      FaceBuffersOf<float> singlePrecisionFaces;
      std::vector<float>   singlePrecisionScratch(SinglePrecisionPredictor::Scratch::Size, 0.0F);
      const int            singlePrecisionOutcome = SinglePrecisionPredictor::predict(
        singlePrecisionFaces.statePointers.data(),
        singlePrecisionFaces.fluxPointers.data(),
        singlePrecisionCellState,
        singlePrecisionScratch.data(),
        singlePrecisionTables.getView(),
        getCellCentre(),
        getCellSize(),
        TimeStamp,
        TimeStepSize,
        PicardTolerance
      );

      THEN("Only the single-precision predictor reports a diverged Picard iteration") {
        VALIDATE(doubleOutcome != exahype2::dg::ader::DivergedPicardIteration, doubleOutcome);
        VALIDATE_EQUALS(singlePrecisionOutcome, exahype2::dg::ader::DivergedPicardIteration);
      }
    }
  }
}

UNIT_TEST("The gradient of a constant state vanishes") {
  GIVEN("A nonconservative product, a cell state of threes and a scratch of stale values") {
    using Scratch = Predictor<ProductSolver>::Scratch;
    const Tables tables;
    double       cellState[CellStateSize];
    for (int entry = 0; entry < CellStateSize; entry++) {
      cellState[entry] = 3.0;
    }
    std::vector<double> scratch(Scratch::Size, StaleValue);

    WHEN("The predictor runs") {
      FaceBuffers faces;
      predictCell<ProductSolver>(faces, cellState, scratch, tables.getView(), TimeStepSize, PicardTolerance);

      THEN("The gradient of every axis, node and lane is zero") {
        for (int entry = 0; entry < DIMENSIONS * GradientAxisStride; entry++) {
          VALIDATE_NUMERICAL_EQUALS(scratch[Scratch::StateGradientOffset + entry], 0.0, entry);
        }
      }
    }
  }
}

UNIT_TEST("A state that grows linearly along every axis has the exact physical gradient") {
  GIVEN("A nonconservative product and a cell state that grows linearly along every axis") {
    using Scratch = Predictor<ProductSolver>::Scratch;
    const Tables tables;
    double       cellState[CellStateSize];
    fillSlopedCellState(tables, cellState);

    WHEN("The first Picard sweep runs on a scratch of stale values") {
      const std::vector<double> scratch = getScratchOfFirstSweep<ProductSolver>(tables, cellState);

      THEN("The gradient of the last time node equals the slope over the cell size and every padding lane is zero") {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
            const int firstEntry = Scratch::StateGradientOffset + axis * GradientAxisStride + cellNode * UnknownStride;
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              VALIDATE_NUMERICAL_EQUALS(
                scratch[firstEntry + unknown],
                getSlopedStateDerivative(axis, unknown),
                axis,
                cellNode,
                unknown
              );
            }
            for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
              VALIDATE_NUMERICAL_EQUALS(scratch[firstEntry + unknown], 0.0, axis, cellNode, unknown);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The flux fills one block per axis and the solver receives the node state, position and time") {
  GIVEN("A flux of the state, the position and the time and a cell state that differs at every node and value") {
    using Scratch = Predictor<PlainFluxSolver>::Scratch;
    const Tables tables;
    double       cellState[CellStateSize];
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = getStateValueAt(cellNode, value);
      }
    }

    WHEN("The first Picard sweep runs on a scratch of stale values") {
      const std::vector<double> scratch = getScratchOfFirstSweep<PlainFluxSolver>(tables, cellState);

      THEN("The flux of the first time node stores the flux of every node and axis and every padding lane is zero") {
        const double timePosition = getTimePositionOf(tables, 0);
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
            const int    firstEntry = Scratch::FirstFluxOffset + axis * FluxAxisStride + cellNode * UnknownStride;
            const double position   = getNodePositionAt(tables, cellNode)(axis);
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              const double expected = (1.0 + axis) * getStateValueAt(cellNode, unknown) + position + timePosition;
              VALIDATE_NUMERICAL_EQUALS(scratch[firstEntry + unknown], expected, axis, cellNode, unknown);
            }
            for (int unknown = NumberOfUnknowns; unknown < UnknownStride; unknown++) {
              VALIDATE_NUMERICAL_EQUALS(scratch[firstEntry + unknown], 0.0, axis, cellNode, unknown);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The diffusive flux of a node reads the gradient of every axis at that node") {
  GIVEN("A diffusive flux and a cell state that grows quadratically along every axis") {
    using Scratch = Predictor<DiffusiveFluxSolver>::Scratch;
    const Tables tables;
    double       cellState[CellStateSize];
    fillQuadraticCellState(tables, cellState);

    WHEN("The first Picard sweep runs on a scratch of stale values") {
      const std::vector<double> scratch = getScratchOfFirstSweep<DiffusiveFluxSolver>(tables, cellState);

      THEN("The flux of the first time node reads the state and the gradient of every axis at every node") {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
            const int firstEntry = Scratch::FirstFluxOffset + axis * FluxAxisStride + cellNode * UnknownStride;
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              double expected = (1.0 + axis) * getQuadraticStateEntry(tables, cellNode, unknown);
              for (int gradientAxis = 0; gradientAxis < DIMENSIONS; gradientAxis++) {
                expected += (0.5 + gradientAxis) * getQuadraticStateDerivative(tables, gradientAxis, cellNode, unknown);
              }
              VALIDATE_NUMERICAL_EQUALS(scratch[firstEntry + unknown], expected, axis, cellNode, unknown);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("Every combination of source, diffusive source and negated nonconservative product enters one buffer") {
  GIVEN("A cell state that grows quadratically along every axis") {
    const Tables tables;
    double       cellState[CellStateSize];
    fillQuadraticCellState(tables, cellState);

    WHEN("The first Picard sweep runs for the seven combinations of the three terms") {
      const auto sourceScratch           = getScratchOfFirstSweep<SourceSolver>(tables, cellState);
      const auto productScratch          = getScratchOfFirstSweep<ProductSolver>(tables, cellState);
      const auto diffusiveSourceScratch  = getScratchOfFirstSweep<DiffusiveSourceSolver>(tables, cellState);
      const auto productSourceScratch    = getScratchOfFirstSweep<ProductSourceSolver>(tables, cellState);
      const auto productDiffusiveScratch = getScratchOfFirstSweep<ProductDiffusiveSourceSolver>(tables, cellState);
      const auto sourceDiffusiveScratch  = getScratchOfFirstSweep<SourceDiffusiveSourceSolver>(tables, cellState);
      const auto allSourcesScratch       = getScratchOfFirstSweep<AllSourcesSolver>(tables, cellState);

      THEN("Every combination sums its terms into the right-hand side, the first source and the source average") {
        validateSourceOf<SourceSolver>(tables, sourceScratch, false, true, false);
        validateSourceOf<ProductSolver>(tables, productScratch, true, false, false);
        validateSourceOf<DiffusiveSourceSolver>(tables, diffusiveSourceScratch, false, false, true);
        validateSourceOf<ProductSourceSolver>(tables, productSourceScratch, true, true, false);
        validateSourceOf<ProductDiffusiveSourceSolver>(tables, productDiffusiveScratch, true, false, true);
        validateSourceOf<SourceDiffusiveSourceSolver>(tables, sourceDiffusiveScratch, false, true, true);
        validateSourceOf<AllSourcesSolver>(tables, allSourcesScratch, true, true, true);
      }
    }
  }
}

UNIT_TEST("The gradient and the source of every time node read the iterate of that time node") {
  GIVEN("A nonconservative product and a source of the state and a cell state that grows linearly along every axis") {
    using Scratch = Predictor<ProductSourceSolver>::Scratch;
    const Tables tables;
    double       cellState[CellStateSize];
    fillSlopedCellState(tables, cellState);

    WHEN("The predictor runs with a squared tolerance of a quarter of the squared first update") {
      double squaredFirstUpdate = 0.0;
      for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            const double update = getFirstUpdateOfSlopedState(tables, cellState, timeNode, cellNode, unknown);
            squaredFirstUpdate += update * update;
          }
        }
      }
      FaceBuffers         faces;
      std::vector<double> scratch(Scratch::Size, StaleValue);
      const int           numberOfSweeps = predictCell<ProductSourceSolver>(
        faces,
        cellState,
        scratch,
        tables.getView(),
        TimeStepSize,
        0.5 * std::sqrt(squaredFirstUpdate)
      );

      THEN("Two sweeps run and the gradient and sources of the second follow the first iterate of every time node") {
        VALIDATE_EQUALS(numberOfSweeps, 2);
        const double lastElapsedTime = tables.quadraturePoints[LastTimeNode] * TimeStepSize;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
            const int firstEntry = Scratch::StateGradientOffset + axis * GradientAxisStride + cellNode * UnknownStride;
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              VALIDATE_NUMERICAL_EQUALS(
                scratch[firstEntry + unknown],
                (1.0 + 2.0 * lastElapsedTime) * getSlopedStateDerivative(axis, unknown),
                axis,
                cellNode,
                unknown
              );
            }
          }
        }
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          const int sourceEntry = cellNode * UnknownStride;
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            const double firstSource    = getSecondSourceOfSlopedState(tables, cellState, 0, cellNode, unknown);
            double       averagedSource = firstSource;
            for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
              const int    spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
              const double source        = getSecondSourceOfSlopedState(tables, cellState, timeNode, cellNode, unknown);
              averagedSource += tables.quadratureWeights[timeNode] * (source - firstSource);
              VALIDATE_NUMERICAL_EQUALS(
                scratch[Scratch::RightHandSideOffset + spaceTimeNode * UnknownStride + unknown],
                getUpdateSizeAt(tables, timeNode, cellNode) * source,
                timeNode,
                cellNode,
                unknown
              );
            }
            VALIDATE_NUMERICAL_EQUALS(
              scratch[Scratch::TimeAveragedSourceOffset + sourceEntry + unknown],
              averagedSource,
              cellNode,
              unknown
            );
            VALIDATE_NUMERICAL_EQUALS(
              scratch[Scratch::FirstSourceOffset + sourceEntry + unknown],
              firstSource,
              cellNode,
              unknown
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("A state, a flux and a source of the same value at every time node average to that value bitwise") {
  GIVEN("A solver with every term, a cell state that is constant in space and a zero time step") {
    using Scratch                     = Predictor<FullSolver>::Scratch;
    constexpr double ZeroTimeStepSize = 0.0;
    const Tables     tables;
    double           cellState[CellStateSize];
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = 1.0 / 3.0 + value;
      }
    }

    WHEN("The predictor runs") {
      FaceBuffers         faces;
      std::vector<double> scratch(Scratch::Size, StaleValue);
      predictCell<FullSolver>(faces, cellState, scratch, tables.getView(), ZeroTimeStepSize, PicardTolerance);

      THEN("The time averages of the state, of the flux on the faces and of the source equal their values bitwise") {
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int value = 0; value < StateStride; value++) {
            const double expected = value < NumberOfData ? cellState[cellNode * NumberOfData + value] : 0.0;
            VALIDATE_EQUALS(
              scratch[Scratch::TimeAveragedStateOffset + cellNode * StateStride + value],
              expected,
              cellNode,
              value
            );
          }
        }
        for (int face = 0; face < peano4::FacesPerCell; face++) {
          for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfUnknowns; entry++) {
            const int unknown = entry % NumberOfUnknowns;
            VALIDATE_EQUALS(faces.flux[face][entry], (1.0 + face % DIMENSIONS) * cellState[unknown], face, entry);
          }
        }
        for (int entry = 0; entry < SourceSize; entry++) {
          const int    unknown  = entry % UnknownStride;
          const double expected = unknown < NumberOfUnknowns ? 2.0 * cellState[unknown] + TimeStamp : 0.0;
          VALIDATE_EQUALS(scratch[Scratch::TimeAveragedSourceOffset + entry], expected, entry);
        }
      }
    }
  }
}

UNIT_TEST("The first time node overwrites the running flux and source averages with its own flux and source") {
  GIVEN("A solver with every term, a state constant in space, a zero time step and a scratch of stale values") {
    using Scratch                     = Predictor<FullSolver>::Scratch;
    constexpr double ZeroTimeStepSize = 0.0;
    const Tables     tables;
    double           cellState[CellStateSize];
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = 1.0 / 3.0 + value;
      }
    }
    std::vector<double> scratch(Scratch::Size, StaleValue);

    WHEN("The predictor runs") {
      FaceBuffers faces;
      predictCell<FullSolver>(faces, cellState, scratch, tables.getView(), ZeroTimeStepSize, PicardTolerance);

      THEN("The faces store the flux of the first time node and both source buffers store its source") {
        for (int face = 0; face < peano4::FacesPerCell; face++) {
          for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfUnknowns; entry++) {
            const int firstFluxEntry = (face % DIMENSIONS) * FluxAxisStride + entry % NumberOfUnknowns;
            VALIDATE_EQUALS(faces.flux[face][entry], scratch[Scratch::FirstFluxOffset + firstFluxEntry], face, entry);
          }
        }
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < UnknownStride; unknown++) {
            const int    entry    = cellNode * UnknownStride + unknown;
            const double expected = unknown < NumberOfUnknowns ? 2.0 * cellState[unknown] + TimeStamp : 0.0;
            VALIDATE_NUMERICAL_EQUALS(scratch[Scratch::FirstSourceOffset + entry], expected, cellNode, unknown);
            VALIDATE_EQUALS(
              scratch[Scratch::TimeAveragedSourceOffset + entry],
              scratch[Scratch::FirstSourceOffset + entry],
              cellNode,
              unknown
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("The flux average stores one block per axis and every block runs in cell node order") {
  GIVEN("A flux of the state, the position and the time and a cell state that grows quadratically along every axis") {
    using Scratch = Predictor<PlainFluxSolver>::Scratch;
    const Tables tables;
    double       cellState[CellStateSize];
    fillQuadraticCellState(tables, cellState);

    WHEN("The first Picard sweep runs on a scratch of stale values") {
      const std::vector<double> scratch = getScratchOfFirstSweep<PlainFluxSolver>(tables, cellState);

      THEN("The time-averaged flux of every axis equals its derivative along that axis at every node") {
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
            const int firstEntry = Scratch::TimeAveragedFluxOffset + axis * FluxAxisStride + cellNode * UnknownStride;
            for (int unknown = 0; unknown < UnknownStride; unknown++) {
              const double expected = unknown < NumberOfUnknowns
                ? (1.0 + axis) * getQuadraticStateDerivative(tables, axis, cellNode, unknown) + 1.0
                : 0.0;
              VALIDATE_NUMERICAL_EQUALS(scratch[firstEntry + unknown], expected, axis, cellNode, unknown);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The state average overwrites the first time slice and leaves the later time slices bitwise") {
  GIVEN("A source that scales the auxiliary variable, the identity as inverse Picard operator and a cell state") {
    using Scratch = Predictor<AuxiliaryVariableSourceSolver>::Scratch;
    const Tables tables;
    double       inversePicardOperator[OperatorEntries];
    double       inverseCellQuadratureWeights[CellLayout::NodesPerCell];
    const auto   identityTables =
      getTablesWithIdentityPicardOperator(tables, inversePicardOperator, inverseCellQuadratureWeights);

    double cellState[CellStateSize];
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = 1.0 + 0.25 * cellNode + 0.5 * value;
      }
    }

    WHEN("The predictor runs") {
      FaceBuffers         faces;
      std::vector<double> scratch(Scratch::Size, StaleValue);
      predictCell<AuxiliaryVariableSourceSolver>(
        faces,
        cellState,
        scratch,
        identityTables,
        TimeStepSize,
        PicardTolerance
      );

      THEN("The first slice stores the time average of the iterate and the later slices store the iterate bitwise") {
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int value = 0; value < StateStride; value++) {
            const double firstValue = getIterateOfAuxiliaryVariableSource(tables, cellState, 0, cellNode, value);
            double       expected   = firstValue;
            for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
              const double iterate = getIterateOfAuxiliaryVariableSource(tables, cellState, timeNode, cellNode, value);
              expected += tables.quadratureWeights[timeNode] * (iterate - firstValue);
            }
            VALIDATE_NUMERICAL_EQUALS(
              scratch[Scratch::SpaceTimeStateOffset + cellNode * StateStride + value],
              expected,
              cellNode,
              value
            );
          }
        }
        for (int timeNode = 1; timeNode < NodesPerAxis; timeNode++) {
          for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
            const int spaceTimeNode = timeNode * CellLayout::NodesPerCell + cellNode;
            for (int value = 0; value < StateStride; value++) {
              VALIDATE_EQUALS(
                scratch[Scratch::SpaceTimeStateOffset + spaceTimeNode * StateStride + value],
                getIterateOfAuxiliaryVariableSource(tables, cellState, timeNode, cellNode, value),
                timeNode,
                cellNode,
                value
              );
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("The scratch places every buffer behind its predecessor without a gap") {
  GIVEN("The scratch of a solver with every term") {
    using Scratch = Predictor<FullSolver>::Scratch;

    THEN("Every buffer has the extent of its term and starts where its predecessor ends") {
      VALIDATE_EQUALS(Scratch::SpaceTimeStateSize, StateStride * CellLayout::SpaceTimeNodes);
      VALIDATE_EQUALS(Scratch::RightHandSideSize, UnknownStride * CellLayout::SpaceTimeNodes);
      VALIDATE_EQUALS(Scratch::FluxSize, DIMENSIONS * UnknownStride * CellLayout::NodesPerCell);
      VALIDATE_EQUALS(Scratch::FirstFluxSize, DIMENSIONS * UnknownStride * CellLayout::NodesPerCell);
      VALIDATE_EQUALS(Scratch::TimeAveragedFluxSize, DIMENSIONS * UnknownStride * CellLayout::NodesPerCell);
      VALIDATE_EQUALS(Scratch::FirstSourceSize, UnknownStride * CellLayout::NodesPerCell);
      VALIDATE_EQUALS(Scratch::TimeAveragedSourceSize, UnknownStride * CellLayout::NodesPerCell);
      VALIDATE_EQUALS(Scratch::StateGradientSize, DIMENSIONS * UnknownStride * CellLayout::NodesPerCell);
      VALIDATE(Scratch::NodeResidualSize >= CellLayout::NodesPerCell, Scratch::NodeResidualSize);
      VALIDATE(Scratch::NodeResidualSize < CellLayout::NodesPerCell + PEANO_WITH_SIMDLEN, Scratch::NodeResidualSize);
      VALIDATE_EQUALS(Scratch::NodeResidualSize % PEANO_WITH_SIMDLEN, 0);

      VALIDATE_EQUALS(Scratch::SpaceTimeStateOffset, 0);
      VALIDATE_EQUALS(Scratch::TimeAveragedStateOffset, Scratch::SpaceTimeStateOffset);
      VALIDATE_EQUALS(Scratch::RightHandSideOffset, Scratch::SpaceTimeStateOffset + Scratch::SpaceTimeStateSize);
      VALIDATE_EQUALS(Scratch::FluxOffset, Scratch::RightHandSideOffset + Scratch::RightHandSideSize);
      VALIDATE_EQUALS(Scratch::FirstFluxOffset, Scratch::FluxOffset + Scratch::FluxSize);
      VALIDATE_EQUALS(Scratch::TimeAveragedFluxOffset, Scratch::FirstFluxOffset + Scratch::FirstFluxSize);
      VALIDATE_EQUALS(Scratch::FirstSourceOffset, Scratch::TimeAveragedFluxOffset + Scratch::TimeAveragedFluxSize);
      VALIDATE_EQUALS(Scratch::TimeAveragedSourceOffset, Scratch::FirstSourceOffset + Scratch::FirstSourceSize);
      VALIDATE_EQUALS(
        Scratch::StateGradientOffset,
        Scratch::TimeAveragedSourceOffset + Scratch::TimeAveragedSourceSize
      );
      VALIDATE_EQUALS(Scratch::NodeResidualOffset, Scratch::StateGradientOffset + Scratch::StateGradientSize);
      VALIDATE_EQUALS(Scratch::Size, Scratch::NodeResidualOffset + Scratch::NodeResidualSize);
    }
  }
}

UNIT_TEST("The scratch of a predictor reserves buffers only for the terms of its solver") {
  GIVEN("The predictors of a solver with every term, without terms, with a product and with a diffusive flux") {
    using Full          = Predictor<FullSolver>;
    using Bare          = Predictor<InertSolver>;
    using Product       = Predictor<ProductSolver>;
    using DiffusiveFlux = Predictor<DiffusiveFluxSolver>;

    THEN("The predictor without terms skips every term, sizes every term buffer zero and reserves a shorter scratch") {
      VALIDATE(not Bare::EvaluateFlux);
      VALIDATE_EQUALS(Bare::Scratch::FluxSize, 0);
      VALIDATE_EQUALS(Bare::Scratch::FirstFluxSize, 0);
      VALIDATE_EQUALS(Bare::Scratch::TimeAveragedFluxSize, 0);
      VALIDATE_EQUALS(Bare::Scratch::FirstSourceSize, 0);
      VALIDATE_EQUALS(Bare::Scratch::TimeAveragedSourceSize, 0);
      VALIDATE_EQUALS(Bare::Scratch::StateGradientSize, 0);
      VALIDATE(not Bare::AccumulatesSource);
      VALIDATE(not Bare::NeedsStateGradient);
      VALIDATE(Bare::Scratch::Size < Full::Scratch::Size);
    }

    THEN("A nonconservative product enters the source and needs the source buffers without a source term") {
      VALIDATE(Product::AccumulatesSource);
      VALIDATE(Product::NeedsStateGradient);
      VALIDATE(Product::Scratch::FirstSourceSize > 0);
      VALIDATE(Product::Scratch::TimeAveragedSourceSize > 0);
      VALIDATE(Product::Scratch::StateGradientSize > 0);
      VALIDATE_EQUALS(Product::Scratch::FluxSize, 0);
    }

    THEN("A diffusive flux reserves the flux and gradient buffers and sizes the source buffer zero") {
      VALIDATE(DiffusiveFlux::EvaluateFlux);
      VALIDATE(DiffusiveFlux::NeedsStateGradient);
      VALIDATE(not DiffusiveFlux::AccumulatesSource);
      VALIDATE(DiffusiveFlux::Scratch::StateGradientSize > 0);
      VALIDATE_EQUALS(DiffusiveFlux::Scratch::TimeAveragedSourceSize, 0);
      VALIDATE(DiffusiveFlux::Scratch::FluxSize > 0);
    }
  }
}

UNIT_TEST("The predictor writes the time averages into the scratch and onto the faces and leaves the cell state") {
  GIVEN("A solver whose terms move a linear profile at a constant rate and the cell state of that profile") {
    const Tables                                tables;
    const auto                                  cellSize = getCellSize();
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

    double cellState[CellStateSize];
    fillLinearCellState(tables, cellState, cellSize(0));

    WHEN("The predictor runs") {
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

      THEN("The first sweep reaches the solution and the second sweep confirms it") {
        VALIDATE_EQUALS(numberOfSweeps, 2);
      }

      THEN("The cell state stays bitwise") {
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

      THEN("The scratch stores the averaged state and source and the derivative of the averaged flux along its axis") {
        constexpr double SourceAverage = SourceStrength + (DiffusiveSourceStrength - ProductSpeed) * Slope;
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int value = 0; value < NumberOfData; value++) {
            VALIDATE_NUMERICAL_EQUALS(
              scratch[ProfileScratch::TimeAveragedStateOffset + cellNode * StateStride + value],
              getAveragedProfileEntry(tables, cellNode, value),
              cellNode,
              value
            );
          }
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            VALIDATE_NUMERICAL_EQUALS(
              scratch[ProfileScratch::TimeAveragedSourceOffset + cellNode * UnknownStride + unknown],
              SourceAverage,
              cellNode,
              unknown
            );
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              const int    fluxEntry      = axis * FluxAxisStride + cellNode * UnknownStride + unknown;
              const double fluxDerivative = axis == 0 ? FluxSpeed * Slope : 0.0;
              VALIDATE_NUMERICAL_EQUALS(
                scratch[ProfileScratch::TimeAveragedFluxOffset + fluxEntry],
                fluxDerivative,
                axis,
                cellNode,
                unknown
              );
            }
          }
        }
      }

      THEN("Both faces of every axis store the extrapolated time averages of the state and the flux") {
        for (int direction = 0; direction < DIMENSIONS; direction++) {
          for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
            for (int value = 0; value < NumberOfData; value++) {
              double lower = 0.0;
              double upper = 0.0;
              for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
                const int    cellNode  = getCellNodeOnFace(direction, faceNode, normalIndex);
                const double nodeValue = getAveragedProfileEntry(tables, cellNode, value);
                lower += tables.leftFaceCoefficients[normalIndex] * nodeValue;
                upper += tables.rightFaceCoefficients[normalIndex] * nodeValue;
              }
              const int entry = faceNode * NumberOfData + value;
              VALIDATE_NUMERICAL_EQUALS(faces.state[direction][entry], lower, direction, faceNode, value);
              VALIDATE_NUMERICAL_EQUALS(faces.state[direction + DIMENSIONS][entry], upper, direction, faceNode, value);
            }
            for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
              double lower = 0.0;
              double upper = 0.0;
              for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
                const int    cellNode  = getCellNodeOnFace(direction, faceNode, normalIndex);
                const double diffusion = (direction == 0) ? DiffusionSpeed * Slope : 0.0;
                const double nodeValue =
                  FluxSpeed * (1.0 + direction) * getAveragedProfileEntry(tables, cellNode, unknown) + diffusion;
                lower += tables.leftFaceCoefficients[normalIndex] * nodeValue;
                upper += tables.rightFaceCoefficients[normalIndex] * nodeValue;
              }
              const int entry = faceNode * NumberOfUnknowns + unknown;
              VALIDATE_NUMERICAL_EQUALS(faces.flux[direction][entry], lower, direction, faceNode, unknown);
              VALIDATE_NUMERICAL_EQUALS(faces.flux[direction + DIMENSIONS][entry], upper, direction, faceNode, unknown);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("A zero state under a tolerance whose square rounds to zero returns UnconvergedPicardIteration") {
  GIVEN("A solver whose terms change a linear profile, a zero cell state and a tolerance whose square rounds to zero") {
    constexpr double UnreachableTolerance = 1.0e-200;
    const Tables     tables;
    const double     cellState[CellStateSize] = {};

    WHEN("The predictor runs") {
      FaceBuffers         faces;
      std::vector<double> scratch(ProfileScratch::Size, 0.0);
      const int           outcome =
        predictCell<ProfileSolver>(faces, cellState, scratch, tables.getView(), TimeStepSize, UnreachableTolerance);

      THEN("Every squared residual reaches the squared tolerance zero and the sweeps stop at their limit") {
        VALIDATE_EQUALS(outcome, exahype2::dg::ader::UnconvergedPicardIteration);
      }
    }
  }
}

UNIT_TEST("A tolerance below the rounding distance of the iterates writes the time averages of a converged iteration") {
  GIVEN("A solver whose terms change a linear profile, its cell state and a tolerance whose square rounds to zero") {
    constexpr double UnreachableTolerance = 1.0e-200;
    const Tables     tables;
    double           cellState[CellStateSize];
    fillLinearCellState(tables, cellState, getCellSize()(0));

    WHEN("The predictor runs with the tolerance of the tests and with the tolerance whose square rounds to zero") {
      FaceBuffers         facesOfTheTestTolerance;
      FaceBuffers         facesBelowTheRoundingDistance;
      std::vector<double> scratchOfTheTestTolerance(ProfileScratch::Size, 0.0);
      std::vector<double> scratchBelowTheRoundingDistance(ProfileScratch::Size, 0.0);
      const int           outcomeOfTheTestTolerance = predictCell<ProfileSolver>(
        facesOfTheTestTolerance,
        cellState,
        scratchOfTheTestTolerance,
        tables.getView(),
        TimeStepSize,
        PicardTolerance
      );
      const int outcomeBelowTheRoundingDistance = predictCell<ProfileSolver>(
        facesBelowTheRoundingDistance,
        cellState,
        scratchBelowTheRoundingDistance,
        tables.getView(),
        TimeStepSize,
        UnreachableTolerance
      );

      THEN("The second run stops at the rounding distance or at the sweep limit and both write the same averages") {
        VALIDATE(outcomeOfTheTestTolerance > 0, outcomeOfTheTestTolerance);
        VALIDATE(
          outcomeBelowTheRoundingDistance >= outcomeOfTheTestTolerance
            or outcomeBelowTheRoundingDistance == exahype2::dg::ader::UnconvergedPicardIteration,
          outcomeBelowTheRoundingDistance
        );
        for (int entry = 0; entry < CellLayout::NodesPerCell * StateStride; entry++) {
          VALIDATE_NUMERICAL_EQUALS(
            scratchBelowTheRoundingDistance[ProfileScratch::TimeAveragedStateOffset + entry],
            scratchOfTheTestTolerance[ProfileScratch::TimeAveragedStateOffset + entry],
            entry
          );
        }
        for (int face = 0; face < peano4::FacesPerCell; face++) {
          for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfData; entry++) {
            VALIDATE_NUMERICAL_EQUALS(
              facesBelowTheRoundingDistance.state[face][entry],
              facesOfTheTestTolerance.state[face][entry],
              face,
              entry
            );
          }
          for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfUnknowns; entry++) {
            VALIDATE_NUMERICAL_EQUALS(
              facesBelowTheRoundingDistance.flux[face][entry],
              facesOfTheTestTolerance.flux[face][entry],
              face,
              entry
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("A Picard iteration accepts updates below the squared tolerance or the squared rounding distance of a cell") {
  GIVEN("A source of the auxiliary variable, the identity as inverse Picard operator and unknowns of -8 and 3") {
    using Scratch                         = Predictor<AuxiliaryVariableSourceSolver>::Scratch;
    constexpr double UnreachableTolerance = 1.0e-200;
    constexpr int    LastNode             = CellLayout::NodesPerCell - 1;
    const Tables     tables;
    double           inversePicardOperator[OperatorEntries];
    double           inverseCellQuadratureWeights[CellLayout::NodesPerCell];
    const auto       identityTables =
      getTablesWithIdentityPicardOperator(tables, inversePicardOperator, inverseCellQuadratureWeights);

    // An auxiliary variable of 1e-20 updates the unknowns by less than the rounding distance of
    // eight. One of 1e-10 updates them by more than that distance and by less than 1e-7.
    const auto getCellState =
      [](const int node, const double firstValue, const double lastValue, const double auxiliaryVariable) {
        std::array<double, CellStateSize> cellState{};
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          cellState[static_cast<std::size_t>(cellNode * NumberOfData + NumberOfUnknowns)] = auxiliaryVariable;
        }
        cellState[static_cast<std::size_t>(node * NumberOfData)]                        = firstValue;
        cellState[static_cast<std::size_t>(node * NumberOfData + NumberOfUnknowns - 1)] = lastValue;
        return cellState;
      };
    const auto smallUpdateState                  = getCellState(0, -8.0, 3.0, 1.0e-20);
    const auto smallUpdateStateOnTheLastNode     = getCellState(LastNode, -8.0, 3.0, 1.0e-20);
    const auto smallUpdateStateWithZeroFirstNode = getCellState(0, 0.0, 0.0, 1.0e-20);
    const auto mediumUpdateState                 = getCellState(0, -8.0, 3.0, 1.0e-10);

    WHEN("The predictor runs on the four cell states with the tolerances 1e-200 and 1e-7") {
      FaceBuffers         faces;
      std::vector<double> scratch(Scratch::Size, 0.0);
      const auto predictWith = [&](const std::array<double, CellStateSize>& cellState, const double tolerance) {
        return predictCell<AuxiliaryVariableSourceSolver>(
          faces,
          cellState.data(),
          scratch,
          identityTables,
          TimeStepSize,
          tolerance
        );
      };
      const int smallUpdateOutcome   = predictWith(smallUpdateState, UnreachableTolerance);
      const int lastNodeOutcome      = predictWith(smallUpdateStateOnTheLastNode, UnreachableTolerance);
      const int zeroFirstNodeOutcome = predictWith(smallUpdateStateWithZeroFirstNode, UnreachableTolerance);
      const int mediumUpdateOutcome  = predictWith(mediumUpdateState, UnreachableTolerance);
      const int mediumUpdateOutcomeOfTheLooseTolerance = predictWith(mediumUpdateState, 1.0e-7);

      THEN("The rounding distance accepts the small update on either node and only 1e-7 accepts the medium one") {
        VALIDATE_EQUALS(smallUpdateOutcome, 1);
        VALIDATE_EQUALS(lastNodeOutcome, 1);
        VALIDATE_EQUALS(zeroFirstNodeOutcome, exahype2::dg::ader::UnconvergedPicardIteration);
        VALIDATE_EQUALS(mediumUpdateOutcome, 2);
        VALIDATE_EQUALS(mediumUpdateOutcomeOfTheLooseTolerance, 1);
      }
    }
  }
}

UNIT_TEST("A tiny first update compares against a squared tolerance of its own size") {
  GIVEN("A source of the auxiliary variable 2^-500, the identity as inverse Picard operator and zero unknowns") {
    using Scratch = Predictor<AuxiliaryVariableSourceSolver>::Scratch;
    const Tables tables;
    double       inversePicardOperator[OperatorEntries];
    double       inverseCellQuadratureWeights[CellLayout::NodesPerCell];
    const auto   identityTables =
      getTablesWithIdentityPicardOperator(tables, inversePicardOperator, inverseCellQuadratureWeights);
    double cellState[CellStateSize] = {};
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      cellState[cellNode * NumberOfData + NumberOfUnknowns] = 0x1p-500;
    }

    WHEN("The predictor runs with twice and with half the square root of two times the norm of the first update") {
      double squaredFirstUpdate = 0.0;
      for (int timeNode = 0; timeNode < NodesPerAxis; timeNode++) {
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            const double update = getIterateOfAuxiliaryVariableSource(tables, cellState, timeNode, cellNode, unknown)
              - cellState[cellNode * NumberOfData + unknown];
            squaredFirstUpdate += update * update;
          }
        }
      }
      FaceBuffers         faces;
      std::vector<double> scratch(Scratch::Size, 0.0);
      const int           sweepsOfTheLargerTolerance = predictCell<AuxiliaryVariableSourceSolver>(
        faces,
        cellState,
        scratch,
        identityTables,
        TimeStepSize,
        2.0 * std::sqrt(squaredFirstUpdate)
      );
      const int sweepsOfTheSmallerTolerance = predictCell<AuxiliaryVariableSourceSolver>(
        faces,
        cellState,
        scratch,
        identityTables,
        TimeStepSize,
        std::sqrt(0.5 * squaredFirstUpdate)
      );

      THEN("The larger tolerance accepts the first update and the smaller one needs a second sweep without update") {
        VALIDATE(squaredFirstUpdate > 0.0, squaredFirstUpdate);
        VALIDATE_EQUALS(sweepsOfTheLargerTolerance, 1);
        VALIDATE_EQUALS(sweepsOfTheSmallerTolerance, 2);
      }
    }
  }
}

UNIT_TEST("The predictor of every cell of a device range reproduces the serial host loop") {
  GIVEN("The stub solver on a chain of cells, a ramp of cell states and faces of zero") {
    const Tables                             tables;
    const exahype2::dg::unittests::CellChain geometry;
    const std::vector<double>                cellStates =
      exahype2::dg::unittests::getRamp(Setup::AllCellStatesCardinality, 1.0, 0.003);
    const std::vector<double> faceStates(Setup::AllFaceStatesCardinality, 0.0);
    const std::vector<double> faceFluxes(Setup::AllFaceFluxesCardinality, 0.0);

    WHEN("The predictor runs on the device and in a serial host loop") {
      std::vector<double>    deviceCellResult      = cellStates;
      std::vector<double>    deviceFaceStateResult = faceStates;
      std::vector<double>    deviceFaceFluxResult  = faceFluxes;
      const std::vector<int> deviceSweepResult =
        predictOnDevice(tables, geometry, deviceCellResult, deviceFaceStateResult, deviceFaceFluxResult);

      std::vector<double>    hostCellStates = cellStates;
      std::vector<double>    hostFaceStates = faceStates;
      std::vector<double>    hostFaceFluxes = faceFluxes;
      const std::vector<int> hostSweepResult =
        predictOnHost(tables, geometry, hostCellStates, hostFaceStates, hostFaceFluxes);

      THEN("Every cell converges after the same number of sweeps on both and the buffers agree within rounding") {
        for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
          VALIDATE(hostSweepResult[cell] > 0, cell, hostSweepResult[cell]);
          VALIDATE_EQUALS(deviceSweepResult[cell], hostSweepResult[cell], cell);
        }
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceCellResult, hostCellStates);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceFaceStateResult, hostFaceStates);
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceFaceFluxResult, hostFaceFluxes);
      }
    }
  }
}
