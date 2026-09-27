// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Basis.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::ader::tests {
  struct StubLayout;
  struct KernelTestSetup;
  struct StubSolver;
} // namespace exahype2::dg::ader::tests

/**
 * The layout constants of a generated solver of order two with two unknowns and one auxiliary
 * variable; the stub solvers of the ADER-DG tests derive from it.
 */
struct exahype2::dg::ader::tests::StubLayout {
  static constexpr int DGOrder                    = 2;
  static constexpr int NumberOfUnknowns           = 2;
  static constexpr int NumberOfAuxiliaryVariables = 1;
};

TARGET_DECLARE
/**
 * Sizes and coefficients of the predictor device tests.
 */
struct exahype2::dg::ader::tests::KernelTestSetup final {
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
 * Every callback depends on state, position and direction, so a wrong offset changes the result.
 * The stub declares every term the two predictors read: The nonlinear predictor takes the
 * diffusive flux, the source, the diffusive source and the product, the linear predictor the
 * flux, the product, the source, the material parameters and the point source.
 */
struct exahype2::dg::ader::tests::StubSolver final : exahype2::dg::ader::tests::StubLayout {
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

#include "exahype2/dg/ader/tests/KernelTestData.cpph"
