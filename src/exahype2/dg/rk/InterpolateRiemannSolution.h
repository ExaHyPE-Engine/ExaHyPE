// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/amr/AMRRoutines.h"
#include "exahype2/dg/BoundaryExtrapolation.h"
#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/rk/UpdateStage.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceEnumerator.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2::dg::rk {
  template <
    typename SolverType,
    bool IsLinear,
    bool UseRiemannSolver,
    bool EvaluateFlux,
    bool EvaluateDiffusiveFlux,
    bool EvaluateNonconservativeProduct,
    typename Quadrature,
    typename DGMatrices,
    typename ButcherTableau,
    typename CellLabel,
    typename FaceLabel,
    typename FaceStateRecord,
    typename FaceFluxRecord,
    typename StoragePrecision>
  INLINE_METHOD void interpolateRiemannSolution(
    const peano4::dataview::CellMarker&                      marker,
    const CellLabel&                                         cellLabel,
    const peano4::dataview::FaceEnumerator<FaceLabel>&       faceLabels,
    const peano4::dataview::FaceEnumerator<FaceStateRecord>& fineGridFaceStates,
    const peano4::dataview::FaceEnumerator<FaceStateRecord>& coarseGridFaceStates,
    const peano4::dataview::FaceEnumerator<FaceFluxRecord>&  fineGridFaceFluxes,
    const StoragePrecision*                                  cellState,
    const StoragePrecision*                                  rightHandSideEstimates,
    int                                                      rungeKuttaStage,
    double                                                   relativeStageTimeStepSize
  );
} // namespace exahype2::dg::rk

#include "exahype2/dg/rk/InterpolateRiemannSolution.cpph"
