// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Enumeration.h"
#include "exahype2/fv/EigenvalueStatistics.h"
#include "exahype2/fv/PatchUtils.h"
#include "exahype2/fv/reconstruction/InterfaceFluxes.h"
#include "exahype2/fv/weno/NonlinearWeights.h"
#include "peano4/Constants.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::reconstruction {
  template <exahype2::fv::weno::NonlinearWeighting Weighting>
  struct WENO;
} // namespace exahype2::fv::reconstruction

TARGET_DECLARE
/**
 * Fifth-order weighted essentially non-oscillatory reconstruction. The state
 * at each face of a volume is a convex combination of the three quadratic
 * candidate reconstructions over the five volumes centred on it; the
 * @c Weighting policy turns the smoothness of each candidate into its share,
 * so the combination reaches fifth order where the solution is smooth and
 * falls back on the smooth candidates across a discontinuity. Both faces of a
 * volume are then evolved by half a time step through the same Hancock
 * predictor that MUSCL-Hancock uses, which keeps the single-step scheme
 * stable where a plain forward-Euler update of a fifth-order reconstruction
 * would not be. The two evolved states that meet at an interface feed the
 * Riemann solver. Each face state of the neighbours is reconstructed from the
 * neighbour's own five-volume window, so the stencil reaches three volumes
 * to either side and costs three halo layers.
 */
template <exahype2::fv::weno::NonlinearWeighting Weighting>
struct exahype2::fv::reconstruction::WENO final {
  static constexpr int HaloSize = 3;

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
    const StoragePrecision* NOALIAS const        inputPatch,
    const StoragePrecision* NOALIAS const        faceInputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, int>&    volumeIndex,
    const int                                    centreOffset,
    const StoragePrecision* NOALIAS const        centreState,
    const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    const double                                 timeStamp,
    const double                                 timeStepSize,
    StoragePrecision* NOALIAS const              outputState,
    double&                                      maxEigenvalueAfterTimeStep
  );

  template <int NumberOfData, typename StoragePrecision, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static void reconstructFaceState(
    const StoragePrecision* NOALIAS const states[2 * HaloSize + 1],
    const int                             volume,
    const int                             direction,
    ComputePrecision* NOALIAS const       faceState
  );

  template <
    typename SolverType,
    int  NumberOfUnknowns,
    int  NumberOfData,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static void evolveFaceStates(
    const ComputePrecision* NOALIAS const        centreState,
    const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    const int                                    normal,
    const double                                 timeStamp,
    const double                                 timeStepSize,
    ComputePrecision* NOALIAS const              minusState,
    ComputePrecision* NOALIAS const              plusState
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/reconstruction/WENO.cpph"
