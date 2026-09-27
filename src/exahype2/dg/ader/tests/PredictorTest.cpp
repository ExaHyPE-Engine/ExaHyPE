// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PredictorTest.h"

#include "exahype2/dg/ader/Predictor.h"
#include "exahype2/dg/ader/tests/KernelTestData.h"
#include "exahype2/dg/tests/KernelTestData.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace {
  using Tables     = exahype2::dg::tests::BasisTableSet;
  using CellLayout = exahype2::dg::ader::tests::KernelTestSetup::CellLayout;

  constexpr int    NodesPerAxis       = CellLayout::NodesPerAxis;
  constexpr int    NumberOfUnknowns   = CellLayout::NumberOfUnknowns;
  constexpr int    NumberOfData       = CellLayout::NumberOfData;
  constexpr int    CellStateSize      = CellLayout::NodesPerCell * NumberOfData;
  constexpr int    FluxAxisSize       = CellLayout::NodesPerCell * NumberOfUnknowns;
  constexpr int    AveragedAxisStride = CellLayout::NodesPerCell * CellLayout::PaddedNumberOfUnknowns;
  constexpr double TimeStamp          = 0.5;
  constexpr double TimeStepSize       = 0.25;
  constexpr double Slope              = 0.5;

  constexpr double FluxSpeed               = 0.5;
  constexpr double DiffusionSpeed          = 0.25;
  constexpr double ProductSpeed            = 0.75;
  constexpr double SourceStrength          = 1.5;
  constexpr double DiffusiveSourceStrength = 0.5;
  constexpr double DivergingSpeed          = 1.0e30;
  constexpr double LinearFluxSpeed         = 0.5;
  constexpr double LinearProductSpeed      = 0.25;
  constexpr double LinearSourceRate        = 0.75;

  /** The scheme constants of a nonlinear generated solver. */
  struct NonlinearStub : exahype2::dg::ader::tests::StubLayout {
    static constexpr bool   IsLinear                = false;
    static constexpr double PicardResidualTolerance = 1.0e-8;
  };

  /** The scheme constants of a linear generated solver without point sources. */
  struct LinearStub : exahype2::dg::ader::tests::StubLayout {
    static constexpr bool IsLinear = true;
  };

  struct InertNonlinearSolver final : NonlinearStub {};

  /** Every term moves a profile that is linear along the first axis at a constant rate. */
  struct TermSolver final : NonlinearStub {
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

  struct DivergingSolver final : NonlinearStub {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    direction,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] += DivergingSpeed * state[unknown];
      }
    }
  };

  struct InertLinearSolver final : LinearStub {};

  /** A flux and a product that transport every unknown along its axis. */
  struct LinearTransportTerms {
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
  };

  struct TransportSolver final : LinearStub, LinearTransportTerms {};

  struct SourceTransportSolver final : LinearStub, LinearTransportTerms {
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

  using InertScratch           = exahype2::dg::ader::Predictor<InertNonlinearSolver, double>::Nonlinear::Scratch;
  using TermScratch            = exahype2::dg::ader::Predictor<TermSolver, double>::Nonlinear::Scratch;
  using DivergingScratch       = exahype2::dg::ader::Predictor<DivergingSolver, double>::Nonlinear::Scratch;
  using InertLinearScratch     = exahype2::dg::ader::Predictor<InertLinearSolver, double>::Linear::Scratch;
  using TransportScratch       = exahype2::dg::ader::Predictor<TransportSolver, double>::Linear::Scratch;
  using SourceTransportScratch = exahype2::dg::ader::Predictor<SourceTransportSolver, double>::Linear::Scratch;

  /** The face states and face fluxes of one cell, with one pointer per face for the kernels. */
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

  /** Returns the cell node that carries the face node of the face along direction in layer normalIndex. */
  int getCellNodeOnFace(const int direction, const int faceNode, const int normalIndex) {
    auto nodeIndex       = peano4::dDelinearisedExclusive(faceNode, NodesPerAxis, direction);
    nodeIndex(direction) = normalIndex;
    return peano4::dLinearised(nodeIndex, NodesPerAxis);
  }

  tarch::la::Vector<DIMENSIONS, double> getCellSize() {
    tarch::la::Vector<DIMENSIONS, double> cellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSize(axis) = 0.5 + 0.25 * axis;
    }
    return cellSize;
  }

  double getBaseValue(const int value) { return 1.0 + 0.25 * value; }

  double getVaryingCellStateEntry(const int cellNode, const int value) { return 1.0 + 0.125 * (cellNode + value); }

  void fillVaryingCellState(double (&cellState)[CellStateSize]) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = getVaryingCellStateEntry(cellNode, value);
      }
    }
  }

  double getLinearCellStateEntry(const Tables& tables, const int cellNode, const int value, const double cellExtent) {
    return getBaseValue(value)
      + Slope * cellExtent * tables.quadraturePoints[peano4::dDelinearised(cellNode, NodesPerAxis)(0)];
  }

  void fillLinearCellState(const Tables& tables, double (&cellState)[CellStateSize], const double cellExtent) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = getLinearCellStateEntry(tables, cellNode, value, cellExtent);
      }
    }
  }

  /** Returns the rate at which the terms of TermSolver move a profile of the test's slope. */
  double getNonlinearStateRate() {
    return -FluxSpeed * Slope + SourceStrength + (DiffusiveSourceStrength - ProductSpeed) * Slope;
  }

  double getNonlinearSourceAverage() { return SourceStrength + (DiffusiveSourceStrength - ProductSpeed) * Slope; }

  /**
   * Taylor coefficients of the linear system, one offset and one physical slope per order and
   * unknown, together with the factor that coefficient enters the time average with.
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
      coefficients.slope[0][unknown]  = Slope;
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

  /** Runs the prediction of one cell of Derived on a cell at the unit position. */
  template <typename Derived>
  int predict(
    const Tables& tables,
    FaceBuffers&  faces,
    double (&cellState)[CellStateSize],
    double* const scratch,
    double* const picardScratch
  ) {
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    return exahype2::dg::ader::Predictor<Derived, double>::predict(
      cellCentre,
      getCellSize(),
      faces.statePointers,
      faces.fluxPointers,
      cellState,
      nullptr,
      scratch,
      picardScratch,
      tables.getView(),
      TimeStamp,
      TimeStepSize
    );
  }
} // namespace

exahype2::dg::ader::tests::PredictorTest::PredictorTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::PredictorTest") {}

void
exahype2::dg::ader::tests::PredictorTest::run() {
  testMethod(testAnInertNonlinearSystemLeavesTheCellAtRest);
  testMethod(testTheNonlinearPredictorMovesALinearProfileByTheExactAmount);
  testMethod(testADivergingNonlinearPredictorReportsMinusOne);
  testMethod(testAnInertLinearSystemLeavesTheCellAtRest);
  testMethod(testTheLinearPredictorTranslatesALinearProfileByTheExactAmount);
  testMethod(testTheLinearPredictorFollowsTheTaylorSeriesOfTheSystem);
  testMethod(testTheVolumeIntegralSubtractsTheDivergenceAndAddsTheSource);
}

/** Compares both faces of every axis against the extrapolation of the expected nodal state. */
void
exahype2::dg::ader::tests::PredictorTest::validateFaceStates(
  const double* const* const faceStates,
  const double* const        expectedState
) {
  const Tables tables;
  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        double lower = 0.0;
        double upper = 0.0;
        for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
          const double nodeValue =
            expectedState[getCellNodeOnFace(direction, faceNode, normalIndex) * NumberOfData + value];
          lower += tables.leftFaceCoefficients[normalIndex] * nodeValue;
          upper += tables.rightFaceCoefficients[normalIndex] * nodeValue;
        }
        const int entry = faceNode * NumberOfData + value;
        validateNumericalEqualsWithParams3(faceStates[direction][entry], lower, direction, faceNode, value);
        validateNumericalEqualsWithParams3(
          faceStates[direction + DIMENSIONS][entry],
          upper,
          direction,
          faceNode,
          value
        );
      }
    }
  }
}

/** Compares both faces of every axis against the extrapolation of the expected nodal flux of that axis. */
void
exahype2::dg::ader::tests::PredictorTest::validateFaceFluxes(
  const double* const* const faceFluxes,
  const double* const        expectedFlux
) {
  const Tables tables;
  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        double lower = 0.0;
        double upper = 0.0;
        for (int normalIndex = 0; normalIndex < NodesPerAxis; normalIndex++) {
          const double nodeValue = expectedFlux
            [direction * FluxAxisSize + getCellNodeOnFace(direction, faceNode, normalIndex) * NumberOfUnknowns
             + unknown];
          lower += tables.leftFaceCoefficients[normalIndex] * nodeValue;
          upper += tables.rightFaceCoefficients[normalIndex] * nodeValue;
        }
        const int entry = faceNode * NumberOfUnknowns + unknown;
        validateNumericalEqualsWithParams3(faceFluxes[direction][entry], lower, direction, faceNode, unknown);
        validateNumericalEqualsWithParams3(
          faceFluxes[direction + DIMENSIONS][entry],
          upper,
          direction,
          faceNode,
          unknown
        );
      }
    }
  }
}

/** A nonlinear system without terms keeps the cell state, writes it onto every face and converges in one sweep. */
void
exahype2::dg::ader::tests::PredictorTest::testAnInertNonlinearSystemLeavesTheCellAtRest() {
  const Tables tables;

  double cellState[CellStateSize];
  fillVaryingCellState(cellState);

  FaceBuffers faces;
  double      scratch[InertScratch::TimeAveragedSize] = {};
  double      picardScratch[InertScratch::PicardSize] = {};

  const int numberOfSweeps = predict<InertNonlinearSolver>(tables, faces, cellState, scratch, picardScratch);

  validateEquals(numberOfSweeps, 1);

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      validateEqualsWithParams2(
        cellState[cellNode * NumberOfData + value],
        getVaryingCellStateEntry(cellNode, value),
        cellNode,
        value
      );
      validateEqualsWithParams2(
        scratch[InertScratch::TimeAveragedStateOffset + cellNode * CellLayout::PaddedNumberOfData + value],
        getVaryingCellStateEntry(cellNode, value),
        cellNode,
        value
      );
    }
  }

  validateFaceStates(faces.statePointers, cellState);

  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfUnknowns; entry++) {
      validateNumericalEqualsWithParams2(faces.flux[face][entry], 0.0, face, entry);
    }
  }
}

/**
 * The nonlinear predictor moves a profile that is linear along the first axis by the amount its
 * flux, product, source and diffusive terms prescribe.
 */
void
exahype2::dg::ader::tests::PredictorTest::testTheNonlinearPredictorMovesALinearProfileByTheExactAmount() {
  const Tables tables;
  const auto   cellSize = getCellSize();

  double cellState[CellStateSize];
  fillLinearCellState(tables, cellState, cellSize(0));

  FaceBuffers faces;
  double      scratch[TermScratch::TimeAveragedSize] = {};
  double      picardScratch[TermScratch::PicardSize] = {};

  const int numberOfSweeps = predict<TermSolver>(tables, faces, cellState, scratch, picardScratch);

  // The first sweep reaches the solution and the second one confirms it.
  validateEquals(numberOfSweeps, 2);

  const double rate = getNonlinearStateRate();

  double averagedState[CellStateSize];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      const double moved = (value < NumberOfUnknowns) ? 0.5 * TimeStepSize * rate : 0.0;
      averagedState[cellNode * NumberOfData + value] =
        getLinearCellStateEntry(tables, cellNode, value, cellSize(0)) + moved;
    }
  }

  double averagedFlux[DIMENSIONS * FluxAxisSize];
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const double diffusion = (axis == 0) ? DiffusionSpeed * Slope : 0.0;
        averagedFlux[axis * FluxAxisSize + cellNode * NumberOfUnknowns + unknown] =
          FluxSpeed * (1.0 + axis) * averagedState[cellNode * NumberOfData + unknown] + diffusion;
      }
    }
  }

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams2(
        scratch[TermScratch::TimeAveragedSourceOffset + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown],
        getNonlinearSourceAverage(),
        cellNode,
        unknown
      );
    }
  }

  validateFaceStates(faces.statePointers, averagedState);
  validateFaceFluxes(faces.fluxPointers, averagedFlux);

  // The volume integral moves every unknown by the time step times the rate of the profile.
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams2(
        cellState[cellNode * NumberOfData + unknown],
        getLinearCellStateEntry(tables, cellNode, unknown, cellSize(0)) + TimeStepSize * rate,
        cellNode,
        unknown
      );
    }
    for (int value = NumberOfUnknowns; value < NumberOfData; value++) {
      validateNumericalEqualsWithParams2(
        cellState[cellNode * NumberOfData + value],
        getLinearCellStateEntry(tables, cellNode, value, cellSize(0)),
        cellNode,
        value
      );
    }
  }
}

/** A flux that drives the iterate past the bound of the solve reports a diverged cell. */
void
exahype2::dg::ader::tests::PredictorTest::testADivergingNonlinearPredictorReportsMinusOne() {
  const Tables tables;
  const auto   cellSize = getCellSize();

  double cellState[CellStateSize];
  fillLinearCellState(tables, cellState, cellSize(0));

  FaceBuffers faces;
  double      scratch[DivergingScratch::TimeAveragedSize] = {};
  double      picardScratch[DivergingScratch::PicardSize] = {};

  const int numberOfSweeps = predict<DivergingSolver>(tables, faces, cellState, scratch, picardScratch);

  validateEquals(numberOfSweeps, -1);
}

/** A linear system without terms keeps the cell state, writes it onto every face and leaves every face flux at zero. */
void
exahype2::dg::ader::tests::PredictorTest::testAnInertLinearSystemLeavesTheCellAtRest() {
  const Tables tables;

  double cellState[CellStateSize];
  fillVaryingCellState(cellState);

  FaceBuffers faces;
  double      scratch[InertLinearScratch::Size] = {};

  const int numberOfSweeps = predict<InertLinearSolver>(tables, faces, cellState, scratch, nullptr);

  validateEquals(numberOfSweeps, 0);

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      validateNumericalEqualsWithParams2(
        cellState[cellNode * NumberOfData + value],
        getVaryingCellStateEntry(cellNode, value),
        cellNode,
        value
      );
    }
  }

  validateFaceStates(faces.statePointers, cellState);

  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfUnknowns; entry++) {
      validateNumericalEqualsWithParams2(faces.flux[face][entry], 0.0, face, entry);
    }
  }
}

/**
 * The linear predictor translates a profile that is linear along the first axis by the flux and
 * the product times the time step size.
 */
void
exahype2::dg::ader::tests::PredictorTest::testTheLinearPredictorTranslatesALinearProfileByTheExactAmount() {
  const Tables tables;
  const auto   cellSize = getCellSize();

  double cellState[CellStateSize];
  fillLinearCellState(tables, cellState, cellSize(0));

  FaceBuffers faces;
  double      scratch[TransportScratch::Size] = {};

  predict<TransportSolver>(tables, faces, cellState, scratch, nullptr);

  const double transportSpeed = LinearFluxSpeed + LinearProductSpeed;

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const double expected =
        getLinearCellStateEntry(tables, cellNode, unknown, cellSize(0)) - TimeStepSize * transportSpeed * Slope;
      validateNumericalEqualsWithParams2(cellState[cellNode * NumberOfData + unknown], expected, cellNode, unknown);
    }
    for (int value = NumberOfUnknowns; value < NumberOfData; value++) {
      validateNumericalEqualsWithParams2(
        cellState[cellNode * NumberOfData + value],
        getLinearCellStateEntry(tables, cellNode, value, cellSize(0)),
        cellNode,
        value
      );
    }
  }

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        const double expected = (axis == 0) ? transportSpeed * Slope : 0.0;
        validateNumericalEqualsWithParams3(
          scratch
            [TransportScratch::TimeAveragedFluxOffset + axis * AveragedAxisStride
             + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown],
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
 * The Taylor recursion of a linear system with a flux, a product and a source reproduces the time
 * derivatives of that system.
 */
void
exahype2::dg::ader::tests::PredictorTest::testTheLinearPredictorFollowsTheTaylorSeriesOfTheSystem() {
  const Tables tables;
  const auto   cellSize = getCellSize();

  double cellState[CellStateSize];
  fillLinearCellState(tables, cellState, cellSize(0));

  FaceBuffers faces;
  double      scratch[SourceTransportScratch::Size] = {};

  predict<SourceTransportSolver>(tables, faces, cellState, scratch, nullptr);

  const double             transportSpeed = LinearFluxSpeed + LinearProductSpeed;
  const TaylorCoefficients coefficients   = getTaylorCoefficients(transportSpeed, LinearSourceRate);

  double averagedState[CellLayout::NodesPerCell * NumberOfUnknowns];
  double averagedFlux[NumberOfUnknowns] = {};
  for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
    for (int order = 0; order < NodesPerAxis; order++) {
      averagedFlux[unknown] += coefficients.factor[order] * transportSpeed * coefficients.slope[order][unknown];
    }
  }
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    const double reference = tables.quadraturePoints[peano4::dDelinearised(cellNode, NodesPerAxis)(0)];
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      double average = 0.0;
      for (int order = 0; order < NodesPerAxis; order++) {
        average += coefficients.factor[order]
          * (coefficients.offset[order][unknown] + coefficients.slope[order][unknown] * cellSize(0) * reference);
      }
      averagedState[cellNode * NumberOfUnknowns + unknown] = average;
    }
  }

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const double average = averagedState[cellNode * NumberOfUnknowns + unknown];
      validateNumericalEqualsWithParams2(
        scratch[SourceTransportScratch::TimeAveragedStateOffset + cellNode * CellLayout::PaddedNumberOfData + unknown],
        average,
        cellNode,
        unknown
      );
      validateNumericalEqualsWithParams2(
        scratch
          [SourceTransportScratch::TimeAveragedSourceOffset + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown],
        LinearSourceRate * average,
        cellNode,
        unknown
      );
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        const double expectedFlux = (axis == 0) ? averagedFlux[unknown] : 0.0;
        validateNumericalEqualsWithParams3(
          scratch
            [SourceTransportScratch::TimeAveragedFluxOffset + axis * AveragedAxisStride
             + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown],
          expectedFlux,
          axis,
          cellNode,
          unknown
        );
      }
      const double expectedState = getLinearCellStateEntry(tables, cellNode, unknown, cellSize(0))
        + TimeStepSize * (LinearSourceRate * average - averagedFlux[unknown]);
      validateNumericalEqualsWithParams2(
        cellState[cellNode * NumberOfData + unknown],
        expectedState,
        cellNode,
        unknown
      );
    }
  }
}

/**
 * The volume integral subtracts the sum of the flux derivatives over the axes and adds the source,
 * both times the time step size.
 */
void
exahype2::dg::ader::tests::PredictorTest::testTheVolumeIntegralSubtractsTheDivergenceAndAddsTheSource() {
  double timeAveragedFlux[DIMENSIONS * AveragedAxisStride] = {};
  double timeAveragedSource[AveragedAxisStride]            = {};
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        timeAveragedFlux[axis * AveragedAxisStride + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown] =
          1.0 + axis;
      }
      timeAveragedSource[cellNode * CellLayout::PaddedNumberOfUnknowns + unknown] = 4.0;
    }
  }

  double withBothTerms[CellStateSize];
  double withoutFlux[CellStateSize];
  double withoutSource[CellStateSize];
  fillVaryingCellState(withBothTerms);
  fillVaryingCellState(withoutFlux);
  fillVaryingCellState(withoutSource);

  exahype2::dg::ader::Predictor<NonlinearStub, double>::subtractTimeAveragedVolumeIntegral<true, true>(
    withBothTerms,
    timeAveragedFlux,
    timeAveragedSource,
    TimeStepSize
  );
  exahype2::dg::ader::Predictor<NonlinearStub, double>::subtractTimeAveragedVolumeIntegral<false, true>(
    withoutFlux,
    timeAveragedFlux,
    timeAveragedSource,
    TimeStepSize
  );
  exahype2::dg::ader::Predictor<NonlinearStub, double>::subtractTimeAveragedVolumeIntegral<true, false>(
    withoutSource,
    timeAveragedFlux,
    timeAveragedSource,
    TimeStepSize
  );

  double sumOverAxes = 0.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    sumOverAxes += 1.0 + axis;
  }

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const int    entry   = cellNode * NumberOfData + unknown;
      const double initial = getVaryingCellStateEntry(cellNode, unknown);
      validateNumericalEqualsWithParams2(
        withBothTerms[entry],
        initial + TimeStepSize * (4.0 - sumOverAxes),
        cellNode,
        unknown
      );
      validateNumericalEqualsWithParams2(withoutFlux[entry], initial + TimeStepSize * 4.0, cellNode, unknown);
      validateNumericalEqualsWithParams2(withoutSource[entry], initial - TimeStepSize * sumOverAxes, cellNode, unknown);
    }
    for (int value = NumberOfUnknowns; value < NumberOfData; value++) {
      const int entry = cellNode * NumberOfData + value;
      validateNumericalEqualsWithParams2(
        withBothTerms[entry],
        getVaryingCellStateEntry(cellNode, value),
        cellNode,
        value
      );
    }
  }
}
