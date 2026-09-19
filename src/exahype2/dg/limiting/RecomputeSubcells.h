// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Enumeration.h"
#include "exahype2/dg/EigenvalueStatistics.h"
#include "exahype2/dg/LimiterStatus.h"
#include "exahype2/dg/SubcellProjection.h"
#include "exahype2/dg/limiting/SubcellLayout.h"
#include "exahype2/fv/FiniteVolume.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/dataview/CellMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

TARGET_DECLARE
namespace exahype2::dg::limiting {
  template <typename SolverType, int HaloWidth, typename Value, typename EstimatePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void rebuildTransitionFaceHalo(
    int                      faceNumber,
    bool                     isCoarseTransitionSide,
    bool                     isFineTransitionSide,
    const EstimatePrecision* estimateFace,
    const Value*             subcellSolution,
    Value*                   transitionFace,
    const Value*             dgToFvMatrix
  );
} // namespace exahype2::dg::limiting
TARGET_DECLARE_END

namespace exahype2::dg::limiting {
  template <
    typename SolverType,
    typename Reconstruction,
    typename FallbackReconstruction,
    typename TimeStepping,
    int  HaloWidth,
    bool UsesAdmissibilityCriterion,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateDiffusiveSource,
    bool EvaluateCustomRiemann,
    bool EvaluateMaxEigenvalue,
    bool EvaluateMaxEigenvalueOfFinalState,
    typename Value,
    typename CellLabel,
    typename FaceLabels,
    typename EstimateFaces,
    typename OldSubcellFaces,
    typename SubcellPatch,
    typename NodalPatch>
  INLINE_METHOD void recomputeSubcells(
    const peano4::dataview::CellMarker& marker,
    SolverType&                         solver,
    const CellLabel&                    cellLabel,
    const FaceLabels&                   faceLabels,
    const EstimateFaces&                estimateFaces,
    const OldSubcellFaces&              oldSubcellFaces,
    SubcellPatch&                       subcellPatch,
    NodalPatch&                         updatedState,
    const Value*                        dgToFvMatrix,
    const Value*                        fvToDgMatrix
  );
} // namespace exahype2::dg::limiting

#include "exahype2/dg/limiting/RecomputeSubcells.cpph"
