// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <memory>

#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::fv {
  /**
   * A reconstruction scheme forms the states that meet at a volume interface
   * and adds their flux divergence to the update. @c HaloSize fixes how many
   * neighbour layers the stencil reads.
   */
  template <typename ReconstructionPolicy>
  concept ReconstructionScheme = std::is_empty_v<ReconstructionPolicy> and requires {
    { ReconstructionPolicy::HaloSize } -> std::convertible_to<int>;
  };

  /**
   * A time-stepping scheme advances a finite-volume patch by one time step.
   * @c NumberOfStages reports how many spatial evaluations one step performs.
   * The variable template @c ScratchSize gives the number of compute-precision
   * entries the step needs as scratch per patch, which the caller provides, as
   * a kernel on a device cannot allocate memory itself.
   */
  template <typename TimeSteppingPolicy>
  concept TimeSteppingScheme = std::is_empty_v<TimeSteppingPolicy> and requires {
    { TimeSteppingPolicy::NumberOfStages } -> std::convertible_to<int>;
  };

  template <
    ReconstructionScheme Reconstruction,
    TimeSteppingScheme   TimeStepping,
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
    bool EvaluateMaxEigenvalueOfFinalState,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD double applyFiniteVolume(
    const StoragePrecision* NOALIAS              inputPatch,
    StoragePrecision* NOALIAS                    outputPatch,
    const StoragePrecision* NOALIAS const        faceInputPatches[peano4::FacesPerCell],
    StoragePrecision* NOALIAS const              faceOutputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& patchCentre,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    ComputePrecision* NOALIAS                    scratch
  );
} // namespace exahype2::fv
TARGET_DECLARE_END

namespace exahype2::fv {
  /**
   * Returns the scratch of @p Size entries that the calling thread hands to applyFiniteVolume on
   * the host, or nullptr if the time stepping needs none.
   */
  template <typename ComputePrecision, int Size>
  ComputePrecision* hostScratch();
} // namespace exahype2::fv

#include "exahype2/fv/FiniteVolume.cpph"
