// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Enumeration.h"
#include "exahype2/fv/EigenvalueStatistics.h"
#include "exahype2/fv/PatchUtils.h"
#include "exahype2/fv/limiter/SlopeLimiter.h"
#include "peano4/Constants.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::reconstruction {
  template <exahype2::fv::limiter::SlopeLimiter Limiter>
  struct LimitedLinear;
} // namespace exahype2::fv::reconstruction

TARGET_DECLARE
/**
 * Evolves limited boundary states by half a time step before solving face Riemann problems.
 * Each predictor uses one coordinate direction because face exchange omits diagonal halo values.
 * The Limiter policy computes each unknown's limited difference from neighbouring volumes.
 */
template <exahype2::fv::limiter::SlopeLimiter Limiter>
struct exahype2::fv::reconstruction::LimitedLinear final {
  static constexpr int HaloSize = 2;

  template <
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
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static void accumulateFluxDivergence(
    const StoragePrecision* NOALIAS              inputPatch,
    const StoragePrecision* NOALIAS const        faceInputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, int>&    volumeIndex,
    int                                          centreOffset,
    const StoragePrecision* NOALIAS              centreState,
    const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    StoragePrecision* NOALIAS                    outputState,
    double&                                      maxEigenvalueAfterTimeStep
  );

  template <int NumberOfVolumesPerAxis, int NumberOfData, typename StoragePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static const StoragePrecision* neighbourAlongNormal(
    const StoragePrecision* NOALIAS           inputPatch,
    const StoragePrecision* NOALIAS const     faceInputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, int>& volumeIndex,
    int                                       centreOffset,
    int                                       strideAlongNormal,
    int                                       normal,
    int                                       offset
  );

  template <
    typename SolverType,
    int  NumberOfUnknowns,
    int  NumberOfData,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static void evolveFaceStates(
    const StoragePrecision* NOALIAS              leftState,
    const StoragePrecision* NOALIAS              centreState,
    const StoragePrecision* NOALIAS              rightState,
    const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    int                                          normal,
    double                                       timeStamp,
    double                                       timeStepSize,
    ComputePrecision* NOALIAS                    evolvedMinusState,
    ComputePrecision* NOALIAS                    evolvedPlusState
  );

  template <typename SolverType, int NumberOfUnknowns, bool EvaluateFlux, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision rusanovFaceFlux(
    const ComputePrecision* NOALIAS              leftState,
    const ComputePrecision* NOALIAS              rightState,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    int                                          normal,
    double                                       timeStamp,
    double                                       timeStepSize,
    ComputePrecision* NOALIAS                    faceFlux
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/reconstruction/LimitedLinear.cpph"
