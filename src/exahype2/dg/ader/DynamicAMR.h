// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementControlService.h"
#include "exahype2/amr/ChildDataStage.h"
#include "exahype2/amr/LevelJumpEvents.h"
#include "exahype2/dg/Interpolation.h"
#include "exahype2/dg/Restriction.h"
#include "exahype2/dg/ader/PredictorScratch.h"
#include "exahype2/dg/ader/SpaceTimePredictorRun.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Termination.h"

/**
 * The mesh transfer of the ADER-DG family.
 *
 * A face carries the state and flux estimates of the two cells that meet on it.
 */
namespace exahype2::dg::ader {
  template <typename SolverType, typename T>
  INLINE_METHOD void interpolateHangingFace(
    const peano4::dataview::FaceMarker& marker,
    const double* NOALIAS               prolongationMatrix1D,
    const T* NOALIAS                    coarseFaceState,
    T* NOALIAS                          fineFaceState,
    const T* NOALIAS                    coarseFaceFlux,
    T* NOALIAS                          fineFaceFlux
  );

  template <typename SolverType, typename T>
  INLINE_METHOD void stageHangingFaceFlux(
    const peano4::dataview::FaceMarker& marker,
    exahype2::amr::ChildDataStage<T>&   stage,
    const T* NOALIAS                    coarseFaceFlux,
    const T* NOALIAS                    fineFaceFlux
  );

  template <typename SolverType, typename T>
  INLINE_METHOD void restrictFaceFluxHalves(
    exahype2::amr::ChildDataStage<T>& stage,
    const double* NOALIAS             restrictionMatrix1D,
    T* NOALIAS                        fineFaceFlux
  );

  template <
    typename SolverType,
    bool IsLinear,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateDiffusiveFlux,
    bool EvaluateDiffusiveSource,
    bool EvaluateMaterialParameters,
    bool EvaluatePointSources,
    typename ComputePrecision,
    typename PicardPrecision,
    typename StoragePrecision,
    typename CellLabel,
    typename FaceLabelEnumerator,
    typename FaceStateEnumerator,
    typename FaceFluxEnumerator,
    typename RiemannSolve>
  INLINE_METHOD void solveRiemannProblemsOnHangingFaces(
    const peano4::dataview::CellMarker&  marker,
    CellLabel&                           cellLabel,
    const FaceLabelEnumerator&           faceLabels,
    const FaceStateEnumerator&           faceStates,
    const FaceFluxEnumerator&            faceFluxes,
    const StoragePrecision* NOALIAS      previousCellState,
    const BasisTables<ComputePrecision>& tables,
    const BasisTables<PicardPrecision>&  picardTables,
    double                               timeStepSize,
    RiemannSolve&&                       solveRiemannProblem
  );

  template <typename SolverType, typename T, typename CellLabel>
  INLINE_METHOD void createCell(
    const peano4::dataview::CellMarker& marker,
    const double* NOALIAS               prolongationMatrix1D,
    const T* NOALIAS                    coarseCellState,
    T* NOALIAS                          fineCellState,
    const T* NOALIAS                    coarsePreviousCellState,
    T* NOALIAS                          finePreviousCellState,
    const CellLabel&                    coarseCellLabel,
    CellLabel&                          fineCellLabel
  );

  template <typename SolverType, typename T, typename CellLabel, typename FaceLabelEnumerator>
  INLINE_METHOD void destroyCell(
    const peano4::dataview::CellMarker& marker,
    exahype2::amr::ChildDataStage<T>&   cellStateStage,
    exahype2::amr::ChildDataStage<T>&   previousCellStateStage,
    const double* NOALIAS               restrictionMatrix1D,
    T* NOALIAS                          coarseCellState,
    const T* NOALIAS                    fineCellState,
    T* NOALIAS                          coarsePreviousCellState,
    const T* NOALIAS                    finePreviousCellState,
    const FaceLabelEnumerator&          coarseFaceLabels,
    CellLabel&                          coarseCellLabel,
    const CellLabel&                    fineCellLabel
  );

  template <typename CellLabel, typename FaceLabelEnumerator>
  INLINE_METHOD void releaseFacesAfterSubcellTransfer(const FaceLabelEnumerator& faceLabels, CellLabel& cellLabel);

  template <
    typename SolverType,
    bool IsLinear,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateDiffusiveFlux,
    bool EvaluateDiffusiveSource,
    bool EvaluateMaterialParameters,
    bool EvaluatePointSources,
    typename ComputePrecision,
    typename PicardPrecision,
    typename StoragePrecision,
    typename CellLabel,
    typename FaceLabelEnumerator,
    typename FaceStateEnumerator,
    typename FaceFluxEnumerator>
  INLINE_METHOD void rederivePrediction(
    const peano4::dataview::CellMarker&  marker,
    CellLabel&                           cellLabel,
    const FaceLabelEnumerator&           faceLabels,
    const FaceStateEnumerator&           faceStates,
    const FaceFluxEnumerator&            faceFluxes,
    StoragePrecision* NOALIAS            cellState,
    const StoragePrecision* NOALIAS      previousCellState,
    const BasisTables<ComputePrecision>& tables,
    const BasisTables<PicardPrecision>&  picardTables
  );
} // namespace exahype2::dg::ader

#include "exahype2/dg/ader/DynamicAMR.cpph"
