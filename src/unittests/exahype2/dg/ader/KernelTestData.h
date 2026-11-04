// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Basis.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::ader::unittests {
  struct StubLayout;
  struct RiemannSolverTerm;
  struct KernelTestSetup;
  struct StubSolver;
} // namespace exahype2::dg::ader::unittests

TARGET_DECLARE
/**
 * The layout constants the stub solvers of the ADER-DG tests derive from.
 */
struct exahype2::dg::ader::unittests::StubLayout {
  static constexpr int DGOrder                    = 2;
  static constexpr int NumberOfUnknowns           = 2;
  static constexpr int NumberOfAuxiliaryVariables = 1;
};
TARGET_DECLARE_END

TARGET_DECLARE
/**
 * A Riemann solver that leaves both flux halves as they are. The linear predictor of a solver with
 * it writes the face fluxes.
 */
struct exahype2::dg::ader::unittests::RiemannSolverTerm {
  static INLINE_METHOD GPU_CALLABLE_METHOD double solveRiemannProblem(
    [[maybe_unused]] const double* const                          QL,
    [[maybe_unused]] const double* const                          QR,
    [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
    [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
    [[maybe_unused]] const double                                 t,
    [[maybe_unused]] const double                                 dt,
    [[maybe_unused]] const int                                    normal,
    [[maybe_unused]] const bool                                   isBoundaryFace,
    [[maybe_unused]] const int                                    faceIndex,
    [[maybe_unused]] double* const                                FL,
    [[maybe_unused]] double* const                                FR
  ) {
    return 0.0;
  }
};
TARGET_DECLARE_END

TARGET_DECLARE
/**
 * Sizes and coefficients of the predictor device tests.
 */
struct exahype2::dg::ader::unittests::KernelTestSetup final {
  static constexpr int    NumberOfUnknowns           = StubLayout::NumberOfUnknowns;
  static constexpr int    NumberOfAuxiliaryVariables = StubLayout::NumberOfAuxiliaryVariables;
  static constexpr int    NumberOfCells              = 8;
  static constexpr int    NumberOfPointSources       = 2;
  static constexpr double TimeStamp                  = 0.75;
  static constexpr double TimeStepSize               = 0.01;
  static constexpr double PicardTolerance            = 1.0e-10;
  static constexpr double FluxSpeed                  = 0.5;
  static constexpr double DiffusionSpeed             = 0.05;
  static constexpr double ProductSpeed               = 0.25;
  static constexpr double SourceStrength             = 0.5;
  static constexpr double DiffusiveSourceStrength    = 0.1;

  using CellLayout = exahype2::dg::PaddedLayout<StubLayout>;

  static constexpr int CellStateCardinality     = CellLayout::NodesPerCell * CellLayout::NumberOfData;
  static constexpr int FaceStateCardinality     = CellLayout::NodesPerFace * CellLayout::NumberOfData;
  static constexpr int FaceFluxCardinality      = CellLayout::NodesPerFace * NumberOfUnknowns;
  static constexpr int AllCellStatesCardinality = NumberOfCells * CellStateCardinality;
  static constexpr int AllFaceStatesCardinality = NumberOfCells * peano4::FacesPerCell * FaceStateCardinality;
  static constexpr int AllFaceFluxesCardinality = NumberOfCells * peano4::FacesPerCell * FaceFluxCardinality;
  static constexpr int LocationCardinality      = NumberOfPointSources * DIMENSIONS;
  static constexpr int AllIndicesCardinality    = NumberOfCells * NumberOfPointSources;
};
TARGET_DECLARE_END

TARGET_DECLARE
/**
 * A solver with every term of both predictors. The terms depend on the state, the position and the
 * direction. A wrong offset changes the result.
 */
struct exahype2::dg::ader::unittests::StubSolver final : exahype2::dg::ader::unittests::StubLayout {
  static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const double* NOALIAS                        state,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction,
    double* NOALIAS                              values
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const double* NOALIAS                        state,
    const double* const NOALIAS                  stateGradients[DIMENSIONS],
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction,
    double* NOALIAS                              values
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
    const double* NOALIAS                        state,
    const double* NOALIAS                        stateGradient,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction,
    double* NOALIAS                              values
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
    const double* NOALIAS                        state,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    double* NOALIAS                              values
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void diffusiveSourceTerm(
    const double* NOALIAS                        state,
    const double* NOALIAS                        stateGradient,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    double* NOALIAS                              values
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void multiplyMaterialParameterMatrix(
    const double* NOALIAS                        state,
    const tarch::la::Vector<DIMENSIONS, double>& position,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction,
    double* NOALIAS                              values
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void pointSource(
    const double* NOALIAS state,
    const double* NOALIAS nodePosition,
    double                timeStamp,
    double                timeStepSize,
    double* NOALIAS       forceVector,
    int                   pointSource
  );
};
TARGET_DECLARE_END

#include "unittests/exahype2/dg/ader/KernelTestData.cpph"
