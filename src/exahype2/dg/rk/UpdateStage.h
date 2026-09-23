// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/BoundaryConditions.h"
#include "exahype2/dg/BoundaryExtrapolation.h"
#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/EigenvalueStatistics.h"
#include "exahype2/dg/FaceIntegral.h"
#include "exahype2/dg/Riemann.h"
#include "exahype2/dg/rk/LinearCombination.h"
#include "exahype2/dg/rk/StageScratch.h"
#include "exahype2/dg/rk/VolumeIntegral.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceEnumerator.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg::rk {
  template <
    typename SolverType,
    bool IsLinear,
    bool UseRiemannSolver,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void solveFaceRiemannProblem(
    StoragePrecision*                            leftFlux,
    StoragePrecision*                            rightFlux,
    const StoragePrecision*                      leftState,
    const StoragePrecision*                      rightState,
    const ComputePrecision*                      faceQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction,
    bool                                         faceIsBoundary,
    int                                          faceNumber
  );
} // namespace exahype2::dg::rk
TARGET_DECLARE_END

namespace exahype2::dg::rk {
  template <
    typename SolverType,
    bool IsLinear,
    bool UseRiemannSolver,
    bool EvaluateFlux,
    bool EvaluateDiffusiveFlux,
    bool EvaluateNonconservativeProduct,
    typename Quadrature,
    typename FaceLabel,
    typename StoragePrecision>
  INLINE_METHOD void applyBoundaryConditionsAndSolveRiemannProblem(
    const peano4::dataview::FaceMarker& marker,
    const SolverType&                   solver,
    FaceLabel&                          faceLabel,
    StoragePrecision*                   faceState,
    StoragePrecision*                   faceFlux
  );

  template <typename SolverType, typename FaceLabel>
  INLINE_METHOD void rollOverFaceTimeStamps(const SolverType& solver, FaceLabel& faceLabel);

  template <
    typename SolverType,
    bool EvaluateFlux,
    bool EvaluateDiffusiveFlux,
    typename Quadrature,
    typename DGMatrices,
    typename CellLabel,
    typename FaceLabel,
    typename FaceStateRecord,
    typename FaceFluxRecord,
    typename StoragePrecision>
  INLINE_METHOD void projectStageOntoFaces(
    const peano4::dataview::CellMarker&                      marker,
    const CellLabel&                                         cellLabel,
    const peano4::dataview::FaceEnumerator<FaceLabel>&       faceLabels,
    const peano4::dataview::FaceEnumerator<FaceStateRecord>& faceStates,
    const peano4::dataview::FaceEnumerator<FaceFluxRecord>&  faceFluxes,
    const StoragePrecision*                                  stageState,
    double                                                   stageTimeStamp
  );

  template <
    typename SolverType,
    bool IsLinear,
    bool EvaluateFlux,
    bool EvaluateDiffusiveFlux,
    bool EvaluateSource,
    bool EvaluateNonconservativeProduct,
    typename Quadrature,
    typename DGMatrices,
    typename ButcherTableau,
    typename CellLabel,
    typename FaceLabel,
    typename FaceStateRecord,
    typename FaceFluxRecord,
    typename StoragePrecision,
    typename PostprocessStage,
    typename PostprocessTimeStep>
  INLINE_METHOD void updateStage(
    const peano4::dataview::CellMarker&                      marker,
    SolverType&                                              solver,
    CellLabel&                                               cellLabel,
    const peano4::dataview::FaceEnumerator<FaceLabel>&       faceLabels,
    const peano4::dataview::FaceEnumerator<FaceStateRecord>& faceStates,
    const peano4::dataview::FaceEnumerator<FaceFluxRecord>&  faceFluxes,
    StoragePrecision*                                        cellState,
    StoragePrecision*                                        rightHandSideEstimates,
    PostprocessStage&&                                       postprocessStage,
    PostprocessTimeStep&&                                    postprocessTimeStep
  );
} // namespace exahype2::dg::rk

#include "exahype2/dg/rk/UpdateStage.cpph"
