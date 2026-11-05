// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/dg/ader/Predictor.h"

#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/exahype2/dg/KernelTestData.h"
#include "unittests/exahype2/dg/ader/KernelTestData.h"

namespace {
  using Tables            = exahype2::dg::unittests::BasisTableSet;
  using CellLayout        = exahype2::dg::ader::unittests::KernelTestSetup::CellLayout;
  using RuntimeParameters = exahype2::dg::ader::unittests::RuntimeParameters;

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
  constexpr double ConstantSourceValue     = 4.0;

  struct NonlinearStub : exahype2::dg::ader::unittests::StubLayout {
    static constexpr bool   IsLinear                = false;
    static constexpr double PicardResidualTolerance = 1.0e-8;
  };

  struct LinearStub : exahype2::dg::ader::unittests::StubLayout {
    static constexpr bool IsLinear = true;
  };

  struct InertNonlinearSolver final : NonlinearStub {};

  /**
   * Every term changes a profile that is linear along the first axis at a constant rate.
   */
  struct TermSolver final : NonlinearStub {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const double* NOALIAS const                                   state,
      const double* const NOALIAS                                   stateGradients[DIMENSIONS],
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         values,
      [[maybe_unused]] const RuntimeParameters&                     parameters
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
      double* NOALIAS const                                         values,
      [[maybe_unused]] const RuntimeParameters&                     parameters
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
      double* NOALIAS const                                         values,
      [[maybe_unused]] const RuntimeParameters&                     parameters
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
      double* NOALIAS const                                         values,
      [[maybe_unused]] const RuntimeParameters&                     parameters
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
      double* NOALIAS const                                         values,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] = DivergingSpeed * state[unknown];
      }
    }
  };

  /**
   * A flux of one plus the axis number times the coordinate along that axis. Its derivative along
   * the axis is one plus the axis number.
   */
  struct CoordinateFlux {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const tarch::la::Vector<DIMENSIONS, double>&                  position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         values,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] += (1.0 + direction) * position(direction);
      }
    }
  };

  /**
   * A source that adds ConstantSourceValue to every unknown.
   */
  struct ConstantSource {
    static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
      [[maybe_unused]] const double* NOALIAS const                  state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         values,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] += ConstantSourceValue;
      }
    }
  };

  struct CoordinateFluxSourceSolver final : NonlinearStub, CoordinateFlux, ConstantSource {};
  struct ConstantSourceSolver final : NonlinearStub, ConstantSource {};
  struct CoordinateFluxSolver final : NonlinearStub, CoordinateFlux {};

  struct InertLinearSolver final : LinearStub {};

  /**
   * A flux and a product that transport every unknown along its axis.
   */
  struct LinearTransportTerms {
    static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         values,
      [[maybe_unused]] const RuntimeParameters&                     parameters
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
      double* NOALIAS const                                         values,
      [[maybe_unused]] const RuntimeParameters&                     parameters
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
      double* NOALIAS const                                         values,
      [[maybe_unused]] const RuntimeParameters&                     parameters
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

  /**
   * Returns the rate at which the terms of TermSolver change a profile with slope Slope.
   */
  double getNonlinearStateRate() {
    return -FluxSpeed * Slope + SourceStrength + (DiffusiveSourceStrength - ProductSpeed) * Slope;
  }

  double getNonlinearSourceAverage() { return SourceStrength + (DiffusiveSourceStrength - ProductSpeed) * Slope; }

  /**
   * The Taylor coefficients of the linear system and the factors they enter the time average with.
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

  template <typename Derived>
  int predict(const Tables& tables, FaceBuffers& faces, double (&cellState)[CellStateSize], double* const scratch) {
    const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
    return exahype2::dg::ader::Predictor<Derived, double>::predict(
      cellCentre,
      getCellSize(),
      faces.statePointers,
      faces.fluxPointers,
      cellState,
      nullptr,
      scratch,
      tables.getView(),
      TimeStamp,
      TimeStepSize,
      RuntimeParameters{}
    );
  }

  /**
   * Runs the prediction of one cell of Derived on cellState with face buffers and a scratch block of its own.
   */
  template <typename Derived>
  void predictCellState(const Tables& tables, double (&cellState)[CellStateSize]) {
    FaceBuffers         faces;
    std::vector<double> scratch(exahype2::dg::ader::Predictor<Derived, double>::ScratchSize, 0.0);
    predict<Derived>(tables, faces, cellState, scratch.data());
  }

  /**
   * Compares both faces of every axis against the extrapolation of the expected nodal state.
   */
  void validateFaceStates(const double* const* const faceStates, const double* const expectedState) {
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
          VALIDATE_NUMERICAL_EQUALS(faceStates[direction][entry], lower, direction, faceNode, value);
          VALIDATE_NUMERICAL_EQUALS(faceStates[direction + DIMENSIONS][entry], upper, direction, faceNode, value);
        }
      }
    }
  }

  /**
   * Compares both faces of every axis against the extrapolation of the expected nodal flux of that axis.
   */
  void validateFaceFluxes(const double* const* const faceFluxes, const double* const expectedFlux) {
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
          VALIDATE_NUMERICAL_EQUALS(faceFluxes[direction][entry], lower, direction, faceNode, unknown);
          VALIDATE_NUMERICAL_EQUALS(faceFluxes[direction + DIMENSIONS][entry], upper, direction, faceNode, unknown);
        }
      }
    }
  }
} // namespace

UNIT_TEST("A nonlinear system without terms keeps the cell state, copies it to the faces and converges in one sweep") {
  GIVEN("Basis tables and a varying cell state") {
    const Tables tables;
    double       cellState[CellStateSize];
    fillVaryingCellState(cellState);

    WHEN("The predictor of a nonlinear system without terms runs") {
      FaceBuffers faces;
      double      scratch[InertScratch::Size] = {};
      const int   numberOfSweeps              = predict<InertNonlinearSolver>(tables, faces, cellState, scratch);

      THEN("One sweep converges and the cell state and its time average keep every value bitwise") {
        VALIDATE_EQUALS(numberOfSweeps, 1);
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int value = 0; value < NumberOfData; value++) {
            VALIDATE_EQUALS(
              cellState[cellNode * NumberOfData + value],
              getVaryingCellStateEntry(cellNode, value),
              cellNode,
              value
            );
            VALIDATE_EQUALS(
              scratch[InertScratch::TimeAveragedStateOffset + cellNode * CellLayout::PaddedNumberOfData + value],
              getVaryingCellStateEntry(cellNode, value),
              cellNode,
              value
            );
          }
        }
      }

      THEN("Every face stores the extrapolated cell state and a zero flux") {
        validateFaceStates(faces.statePointers, cellState);
        for (int face = 0; face < peano4::FacesPerCell; face++) {
          for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfUnknowns; entry++) {
            VALIDATE_NUMERICAL_EQUALS(faces.flux[face][entry], 0.0, face, entry);
          }
        }
      }
    }
  }
}

UNIT_TEST("The nonlinear predictor changes a linear profile by the exact amount of its terms") {
  GIVEN("Basis tables and a cell whose state is linear along the first axis") {
    const Tables tables;
    const auto   cellSize = getCellSize();
    double       cellState[CellStateSize];
    fillLinearCellState(tables, cellState, cellSize(0));

    WHEN("The predictor of a flux, a product, a source and a diffusive source runs") {
      FaceBuffers faces;
      double      scratch[TermScratch::Size] = {};
      const int   numberOfSweeps             = predict<TermSolver>(tables, faces, cellState, scratch);

      THEN("The first sweep reaches the solution and the second one confirms it") {
        VALIDATE_EQUALS(numberOfSweeps, 2);
      }

      THEN("The time-averaged source and the faces store the time averages of the moved profile") {
        const double rate = getNonlinearStateRate();
        double       averagedState[CellStateSize];
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
            VALIDATE_NUMERICAL_EQUALS(
              scratch[TermScratch::TimeAveragedSourceOffset + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown],
              getNonlinearSourceAverage(),
              cellNode,
              unknown
            );
          }
        }
        validateFaceStates(faces.statePointers, averagedState);
        validateFaceFluxes(faces.fluxPointers, averagedFlux);
      }

      THEN("The volume integral moves the unknowns alone by the time step times the rate of the profile") {
        const double rate = getNonlinearStateRate();
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            VALIDATE_NUMERICAL_EQUALS(
              cellState[cellNode * NumberOfData + unknown],
              getLinearCellStateEntry(tables, cellNode, unknown, cellSize(0)) + TimeStepSize * rate,
              cellNode,
              unknown
            );
          }
          for (int value = NumberOfUnknowns; value < NumberOfData; value++) {
            VALIDATE_NUMERICAL_EQUALS(
              cellState[cellNode * NumberOfData + value],
              getLinearCellStateEntry(tables, cellNode, value, cellSize(0)),
              cellNode,
              value
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("A flux that drives the iterate past the divergence bound reports a diverged cell") {
  GIVEN("Basis tables and a cell whose state is linear along the first axis") {
    const Tables tables;
    const auto   cellSize = getCellSize();
    double       cellState[CellStateSize];
    fillLinearCellState(tables, cellState, cellSize(0));

    WHEN("The predictor of a flux that scales the state by 1e30 runs") {
      FaceBuffers faces;
      double      scratch[DivergingScratch::Size] = {};
      const int   outcome                         = predict<DivergingSolver>(tables, faces, cellState, scratch);

      THEN("The outcome marks the Picard iteration as diverged") {
        VALIDATE_EQUALS(outcome, exahype2::dg::ader::DivergedPicardIteration);
      }
    }
  }
}

UNIT_TEST(
  "The identity of the prediction counts counts zero cells and combining two counts adds both counts and keeps the "
  "smaller slot of a diverged cell"
) {
  using Counts = exahype2::dg::ader::PredictionCounts;

  WHEN("The test takes the identity of the prediction counts") {
    const Counts identity = Counts::identity();

    THEN("The identity counts zero diverged and zero unconverged cells and stores the slot -1") {
      VALIDATE_EQUALS(identity.divergedCells, 0);
      VALIDATE_EQUALS(identity.unconvergedCells, 0);
      VALIDATE_EQUALS(identity.firstDivergedCell, -1);
    }
  }

  GIVEN("The counts of one diverged cell in slot 7 and two unconverged cells and of three diverged cells in slot 5") {
    const Counts first{1, 2, 7};
    const Counts second{3, 4, 5};

    WHEN("The two counts combine") {
      const Counts combined = Counts::combine(first, second);

      THEN("The combination counts four diverged and six unconverged cells and the slot 5") {
        VALIDATE_EQUALS(combined.divergedCells, 4);
        VALIDATE_EQUALS(combined.unconvergedCells, 6);
        VALIDATE_EQUALS(combined.firstDivergedCell, 5);
      }
    }
  }

  GIVEN("The counts of one diverged cell in slot 9 and the counts of cells that did not diverge") {
    const Counts diverged{1, 0, 9};
    const Counts withoutDivergence{0, 0, -1};
    const Counts unconverged{0, 1, -1};

    WHEN("The counts combine in both orders and the counts without a diverged cell combine with each other") {
      const int divergedFirst   = Counts::combine(diverged, withoutDivergence).firstDivergedCell;
      const int divergedSecond  = Counts::combine(withoutDivergence, diverged).firstDivergedCell;
      const int neitherDiverged = Counts::combine(unconverged, unconverged).firstDivergedCell;

      THEN("A combination with the diverged cell returns the slot 9 and a combination without it the slot -1") {
        VALIDATE_EQUALS(divergedFirst, 9);
        VALIDATE_EQUALS(divergedSecond, 9);
        VALIDATE_EQUALS(neitherDiverged, -1);
      }
    }
  }
}

UNIT_TEST("A predictor launch runs at least one team and at most one team per group of cells that a team advances") {
  using Kernel = exahype2::dg::ader::Predictor<InertNonlinearSolver, double>;

  GIVEN("The threads of a predictor team, the threads per cell and the groups of cells of one team") {
    const int threads        = Kernel::getTeamThreads();
    const int threadsPerCell = Kernel::getThreadsPerCell();
    const int groups         = threads / threadsPerCell;

    WHEN("The predictor counts its teams and its scratch chunks for 1, 7 and 4096 cells") {
      const int cellCounts[3] = {1, 7, 4096};
      int       teams[3]      = {};
      int       chunks[3]     = {};
      for (int count = 0; count < 3; count++) {
        teams[count]  = Kernel::getNumberOfTeams(cellCounts[count]);
        chunks[count] = Kernel::getNumberOfScratchChunks(cellCounts[count]);
      }

      THEN("A cell runs on one to all threads of a team and every launch stays within its team and chunk bounds") {
        VALIDATE(threadsPerCell >= 1 and threadsPerCell <= threads, threadsPerCell, threads);
        VALIDATE(threadsPerCell <= Kernel::CellLayout::NodesPerCell, threadsPerCell);
        for (int count = 0; count < 3; count++) {
          const int cells = cellCounts[count];
          VALIDATE(teams[count] >= 1 and teams[count] <= cells, teams[count], cells);
          VALIDATE(teams[count] <= (cells + groups - 1) / groups, teams[count], cells, groups);
          VALIDATE(chunks[count] >= groups and chunks[count] % groups == 0, chunks[count], groups, cells);
        }
      }
    }
  }
}

UNIT_TEST("A linear system without terms keeps the cell state, copies it to the faces and writes zero face fluxes") {
  GIVEN("Basis tables and a varying cell state") {
    const Tables tables;
    double       cellState[CellStateSize];
    fillVaryingCellState(cellState);

    WHEN("The predictor of a linear system without terms runs") {
      FaceBuffers faces;
      double      scratch[InertLinearScratch::Size] = {};
      const int   numberOfSweeps                    = predict<InertLinearSolver>(tables, faces, cellState, scratch);

      THEN("The outcome is zero and the cell state keeps every value") {
        VALIDATE_EQUALS(numberOfSweeps, 0);
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int value = 0; value < NumberOfData; value++) {
            VALIDATE_NUMERICAL_EQUALS(
              cellState[cellNode * NumberOfData + value],
              getVaryingCellStateEntry(cellNode, value),
              cellNode,
              value
            );
          }
        }
      }

      THEN("Every face stores the extrapolated cell state and a zero flux") {
        validateFaceStates(faces.statePointers, cellState);
        for (int face = 0; face < peano4::FacesPerCell; face++) {
          for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfUnknowns; entry++) {
            VALIDATE_NUMERICAL_EQUALS(faces.flux[face][entry], 0.0, face, entry);
          }
        }
      }
    }
  }
}

UNIT_TEST("The linear predictor translates a linear profile by the flux and the product times the time step size") {
  GIVEN("Basis tables and a cell whose state is linear along the first axis") {
    const Tables tables;
    const auto   cellSize = getCellSize();
    double       cellState[CellStateSize];
    fillLinearCellState(tables, cellState, cellSize(0));

    WHEN("The predictor of a flux and a product that transport every unknown runs") {
      FaceBuffers faces;
      double      scratch[TransportScratch::Size] = {};
      predict<TransportSolver>(tables, faces, cellState, scratch);

      THEN("The cell state drops by the time step times the transport speed times the slope in the unknowns alone") {
        const double transportSpeed = LinearFluxSpeed + LinearProductSpeed;
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            const double expected =
              getLinearCellStateEntry(tables, cellNode, unknown, cellSize(0)) - TimeStepSize * transportSpeed * Slope;
            VALIDATE_NUMERICAL_EQUALS(cellState[cellNode * NumberOfData + unknown], expected, cellNode, unknown);
          }
          for (int value = NumberOfUnknowns; value < NumberOfData; value++) {
            VALIDATE_NUMERICAL_EQUALS(
              cellState[cellNode * NumberOfData + value],
              getLinearCellStateEntry(tables, cellNode, value, cellSize(0)),
              cellNode,
              value
            );
          }
        }
      }

      THEN("The flux of the time-averaged state is the transport speed times the slope along the first axis alone") {
        const double transportSpeed = LinearFluxSpeed + LinearProductSpeed;
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              const double expected = (axis == 0) ? transportSpeed * Slope : 0.0;
              VALIDATE_NUMERICAL_EQUALS(
                scratch
                  [TransportScratch::FluxOffset + axis * AveragedAxisStride
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
    }
  }
}

UNIT_TEST("The Taylor recursion of a linear flux, product and source reproduces the time derivatives of the system") {
  GIVEN("Basis tables and a cell whose state is linear along the first axis") {
    const Tables tables;
    const auto   cellSize = getCellSize();
    double       cellState[CellStateSize];
    fillLinearCellState(tables, cellState, cellSize(0));

    WHEN("The predictor of a transporting flux and product and of a linear source runs") {
      FaceBuffers faces;
      double      scratch[SourceTransportScratch::Size] = {};
      predict<SourceTransportSolver>(tables, faces, cellState, scratch);

      THEN("The time averages and the updated cell state follow the Taylor series of the system") {
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
            VALIDATE_NUMERICAL_EQUALS(
              scratch
                [SourceTransportScratch::TimeAveragedStateOffset + cellNode * CellLayout::PaddedNumberOfData + unknown],
              average,
              cellNode,
              unknown
            );
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              const double expectedFlux = (axis == 0) ? averagedFlux[unknown] : 0.0;
              VALIDATE_NUMERICAL_EQUALS(
                scratch
                  [SourceTransportScratch::FluxOffset + axis * AveragedAxisStride
                   + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown],
                expectedFlux,
                axis,
                cellNode,
                unknown
              );
            }
            const double expectedState = getLinearCellStateEntry(tables, cellNode, unknown, cellSize(0))
              + TimeStepSize * (LinearSourceRate * average - averagedFlux[unknown]);
            VALIDATE_NUMERICAL_EQUALS(cellState[cellNode * NumberOfData + unknown], expectedState, cellNode, unknown);
          }
        }
      }
    }
  }
}

UNIT_TEST("The volume integral subtracts the flux divergence and adds the source, both times the time step size") {
  GIVEN("Basis tables and three copies of a varying cell state") {
    const Tables tables;
    double       withBothTerms[CellStateSize];
    double       withoutFlux[CellStateSize];
    double       withoutSource[CellStateSize];
    fillVaryingCellState(withBothTerms);
    fillVaryingCellState(withoutFlux);
    fillVaryingCellState(withoutSource);

    WHEN("The predictors run for a flux growing with the coordinates and a constant source and for each term alone") {
      predictCellState<CoordinateFluxSourceSolver>(tables, withBothTerms);
      predictCellState<ConstantSourceSolver>(tables, withoutFlux);
      predictCellState<CoordinateFluxSolver>(tables, withoutSource);

      THEN("The unknowns alone gain the time step times the source minus the divergence of the flux") {
        double fluxDivergence = 0.0;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          fluxDivergence += 1.0 + axis;
        }
        for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
          for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
            const int    entry   = cellNode * NumberOfData + unknown;
            const double initial = getVaryingCellStateEntry(cellNode, unknown);
            VALIDATE_NUMERICAL_EQUALS(
              withBothTerms[entry],
              initial + TimeStepSize * (ConstantSourceValue - fluxDivergence),
              cellNode,
              unknown
            );
            VALIDATE_NUMERICAL_EQUALS(
              withoutFlux[entry],
              initial + TimeStepSize * ConstantSourceValue,
              cellNode,
              unknown
            );
            VALIDATE_NUMERICAL_EQUALS(withoutSource[entry], initial - TimeStepSize * fluxDivergence, cellNode, unknown);
          }
          for (int value = NumberOfUnknowns; value < NumberOfData; value++) {
            const int entry = cellNode * NumberOfData + value;
            VALIDATE_NUMERICAL_EQUALS(withBothTerms[entry], getVaryingCellStateEntry(cellNode, value), cellNode, value);
          }
        }
      }
    }
  }
}
