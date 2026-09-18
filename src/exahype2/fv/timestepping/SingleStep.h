// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Enumeration.h"
#include "exahype2/fv/EigenvalueStatistics.h"
#include "exahype2/fv/PatchUtils.h"
#include "peano4/Constants.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::timestepping {
  struct SingleStep;
} // namespace exahype2::fv::timestepping

TARGET_DECLARE
/**
 * Implements a one-stage explicit finite-volume time-stepping policy.
 */
struct exahype2::fv::timestepping::SingleStep final {
  static constexpr int NumberOfStages = 1;

  template <
    typename Reconstruction,
    typename Solver,
    int  NumberOfVolumesPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateDiffusiveSource,
    bool EvaluateCustomRiemann,
    bool EvaluateMaxEigenvalue,
    bool EvaluateMaxEigenvalueOfFinalState,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static double apply(
    const StoragePrecision* NOALIAS              inputPatch,
    StoragePrecision* NOALIAS                    outputPatch,
    const StoragePrecision* NOALIAS const        faceInputPatches[peano4::FacesPerCell],
    StoragePrecision* NOALIAS const              faceOutputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/timestepping/SingleStep.cpph"
