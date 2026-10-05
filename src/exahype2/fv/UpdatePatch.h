// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/EigenvalueStatistics.h"
#include "exahype2/fv/FiniteVolume.h"
#include "exahype2/fv/PatchUtils.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "peano4/dataview/CellMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Termination.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/la/VectorOperations.h"

namespace exahype2::fv {
  template <
    ReconstructionScheme Reconstruction,
    TimeSteppingScheme   TimeStepping,
    bool                 EvaluateFlux,
    bool                 EvaluateNonconservativeProduct,
    bool                 EvaluateSource,
    bool                 EvaluateDiffusiveSource,
    bool                 EvaluateCustomRiemann,
    bool                 EvaluateMaxEigenvalue,
    bool                 EvaluateMaxEigenvalueOfFinalState,
    typename ComputePrecision,
    typename SolverType,
    typename CellLabel,
    typename FaceLabelEnumerator,
    typename StoragePrecision,
    typename FaceInputEnumerator,
    typename FaceOutputEnumerator,
    typename Postprocess>
  INLINE_METHOD void updatePatch(
    const peano4::dataview::CellMarker& marker,
    SolverType&                         solver,
    CellLabel&                          cellLabel,
    const FaceLabelEnumerator&          faceLabels,
    StoragePrecision*                   inputPatch,
    StoragePrecision*                   outputPatch,
    const FaceInputEnumerator&          faceInput,
    const FaceOutputEnumerator&         faceOutput,
    Postprocess                         postprocess
  );
} // namespace exahype2::fv

#include "exahype2/fv/UpdatePatch.cpph"
