// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::timestepping {
  struct SingleStep;
} // namespace exahype2::fv::timestepping

TARGET_DECLARE
/**
 * One-stage explicit time-stepping scheme.
 */
struct exahype2::fv::timestepping::SingleStep final {
  static constexpr int NumberOfStages = 1;

  template <
    typename Reconstruction,
    typename SolverType,
    int  NumberOfVolumesPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateDiffusiveSource,
    bool EvaluateCustomRiemann,
    bool EvaluateMaxEigenvalue,
    typename StoragePrecision,
    typename ComputePrecision
  >
  INLINE_METHOD GPU_CALLABLE_METHOD static double apply(
    const StoragePrecision* NOALIAS              inputPatch,
    StoragePrecision* NOALIAS                    outputPatch,
    const StoragePrecision* NOALIAS const        faceInputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/timestepping/SingleStep.cpph"
