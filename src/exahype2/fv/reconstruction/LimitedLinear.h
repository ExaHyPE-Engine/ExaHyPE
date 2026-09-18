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
 * Piecewise-linear reconstruction with a slope limiter, which yields the
 * second-order MUSCL-Hancock scheme: Each volume carries a limited slope in
 * every coordinate direction, the boundary-extrapolated values are evolved by
 * half a time step with the flux balance along that direction alone, and the
 * two evolved states that meet at an interface feed the Riemann solver. The
 * half-step uses only the direction it belongs to, so the stencil stays on the
 * axis-aligned five-point line and never needs the diagonal halo, which the
 * face exchange does not provide. The @c Limiter policy maps the neighbourhood
 * of a volume onto the limited difference of each unknown.
 */
template <exahype2::fv::limiter::SlopeLimiter Limiter>
struct exahype2::fv::reconstruction::LimitedLinear final {
  static constexpr int HaloSize = 2;

  template <
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
    typename Solver,
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

  template <typename Solver, int NumberOfUnknowns, bool EvaluateFlux, typename ComputePrecision>
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
