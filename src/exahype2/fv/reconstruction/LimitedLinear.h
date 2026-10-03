// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/limiter/SlopeLimiter.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::reconstruction {
  template <exahype2::fv::limiter::SlopeLimiter Limiter>
  struct LimitedLinear;
} // namespace exahype2::fv::reconstruction

TARGET_DECLARE
/**
 * MUSCL-Hancock reconstruction with the slope limiter Limiter. The half-step predictor reads
 * neighbours along one axis only, since the faces store no diagonal halo volumes.
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
    typename ComputePrecision
  >
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
    int                                          boundaryFaces,
    StoragePrecision* NOALIAS                    outputState,
    double&                                      maxEigenvalueAfterTimeStep
  );

  template <int NumberOfVolumesPerAxis, int NumberOfData, typename StoragePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static const StoragePrecision* getNeighbourAlongNormal(
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
    typename ComputePrecision
  >
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
