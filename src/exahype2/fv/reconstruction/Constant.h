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

namespace exahype2::fv::reconstruction {
  struct Constant;
} // namespace exahype2::fv::reconstruction

TARGET_DECLARE
/**
 * Piecewise-constant reconstruction, which yields the first-order Godunov
 * scheme. The two states meeting at an interface are the cell-centred values of
 * the two neighbouring volumes, so a single halo layer suffices.
 */
struct exahype2::fv::reconstruction::Constant final {
  static constexpr int HaloSize = 1;

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
};
TARGET_DECLARE_END

#include "exahype2/fv/reconstruction/Constant.cpph"
