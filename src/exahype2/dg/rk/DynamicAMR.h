// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/amr/AMRRoutines.h"
#include "exahype2/amr/ChildDataStage.h"
#include "exahype2/amr/LevelJumpEvents.h"
#include "exahype2/dg/BoundaryExtrapolation.h"
#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/Interpolation.h"
#include "exahype2/dg/Restriction.h"
#include "exahype2/dg/rk/UpdateStage.h"
#include "exahype2/dg/rk/VolumeIntegral.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceEnumerator.h"
#include "peano4/dataview/FaceMarker.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

/**
 * @file
 * The mesh transfer of the Runge-Kutta discontinuous Galerkin family.
 *
 * A cell carries its state and the right-hand side estimate of every Runge-Kutta stage; a face
 * carries the extrapolated state of both adjacent cells and the Riemann solution of both
 * sides. One routine per traversal
 * event calls the routines of exahype2/amr/LevelJumpEvents.h with the kernels of
 * exahype2/dg/Interpolation.h and exahype2/dg/Restriction.h.
 */
namespace exahype2::dg::rk {
  template <typename T>
  class StageEstimatesRestriction;

  template <
    typename SolverType,
    bool EvaluateFlux,
    bool EvaluateDiffusiveFlux,
    typename Quadrature,
    typename DGMatrices,
    typename FaceLabel,
    typename T>
  INLINE_METHOD void restrictHangingFaceTrace(
    const peano4::dataview::FaceMarker& marker,
    exahype2::amr::ChildDataStage<T>&   stage,
    T* NOALIAS                          coarseFaceState,
    const T* NOALIAS                    fineFaceState,
    T* NOALIAS                          coarseFaceRiemannSolution,
    FaceLabel&                          coarseFaceLabel,
    const FaceLabel&                    fineFaceLabel,
    double                              timeStepSize
  );

  template <typename CellLabel, typename FaceLabel, typename FaceStateRecord, typename FaceFluxRecord>
  INLINE_METHOD void createFace(
    FaceStateRecord& fineFaceState,
    FaceFluxRecord&  fineFaceRiemannSolution,
    const CellLabel& coarseCellLabel,
    FaceLabel&       fineFaceLabel
  );

  template <
    typename SolverType,
    typename DGMatrices,
    typename CellLabel,
    typename CellStateRecord,
    typename CellStageRecord>
  INLINE_METHOD void createCell(
    const peano4::dataview::CellMarker& marker,
    const SolverType&                   solver,
    const CellStateRecord&              coarseCellState,
    CellStateRecord&                    fineCellState,
    const CellStageRecord&              coarseStageEstimates,
    CellStageRecord&                    fineStageEstimates,
    const CellLabel&                    coarseCellLabel,
    CellLabel&                          fineCellLabel
  );

  template <typename SolverType, typename DGMatrices, typename CellLabel, typename FaceLabelEnumerator, typename T>
  INLINE_METHOD void destroyCell(
    const peano4::dataview::CellMarker& marker,
    exahype2::amr::ChildDataStage<T>&   cellStateStage,
    exahype2::amr::ChildDataStage<T>&   stageEstimatesStage,
    T* NOALIAS                          coarseCellState,
    const T* NOALIAS                    fineCellState,
    T* NOALIAS                          coarseStageEstimates,
    const T* NOALIAS                    fineStageEstimates,
    const FaceLabelEnumerator&          coarseFaceLabels,
    CellLabel&                          coarseCellLabel,
    const CellLabel&                    fineCellLabel
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
    typename T>
  INLINE_METHOD void projectStageAfterTransfer(
    const peano4::dataview::CellMarker&                      marker,
    const SolverType&                                        solver,
    CellLabel&                                               cellLabel,
    const peano4::dataview::FaceEnumerator<FaceLabel>&       faceLabels,
    const peano4::dataview::FaceEnumerator<FaceStateRecord>& faceStates,
    const peano4::dataview::FaceEnumerator<FaceFluxRecord>&  faceFluxes,
    const T* NOALIAS                                         cellState,
    T* NOALIAS                                               stageEstimates
  );
} // namespace exahype2::dg::rk

/**
 * Restricts the right-hand side estimate of every Runge-Kutta stage of a sibling set.
 *
 * The estimates of all stages sit in one buffer, so the frame stages that whole buffer and
 * the reduction reads it stage by stage.
 */
template <typename T>
class exahype2::dg::rk::StageEstimatesRestriction final {
public:
  INLINE_METHOD StageEstimatesRestriction(
    const double* restrictionMatrix1D,
    int           numberOfDofsPerAxis,
    int           numberOfUnknowns,
    int           numberOfStages,
    int           valuesPerStage
  );

  INLINE_METHOD void operator()(exahype2::amr::ChildDataStage<T>& stage, T* NOALIAS coarseGridValues) const;

private:
  const double* const _restrictionMatrix1D;
  const int           _numberOfDofsPerAxis;
  const int           _numberOfUnknowns;
  const int           _numberOfStages;
  const int           _valuesPerStage;
};

#include "exahype2/dg/rk/DynamicAMR.cpph"
