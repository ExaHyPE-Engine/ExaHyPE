// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/Patch.h"
#include "exahype2/fv/ader/FallbackGranularity.h"
#include "exahype2/fv/ader/LimiterStatus.h"
#include "exahype2/terms/DmpObservables.h"
#include "exahype2/terms/DmpRelaxationParameter.h"
#include "exahype2/terms/PhysicalAdmissibility.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::ader {
  template <typename Derived, typename ComputePrecision>
  class FallbackLimiter;

  /**
   * Returns whether Derived falls back volume by volume. Only a solver with a limiter names a
   * granularity.
   */
  template <typename Derived>
  constexpr bool fallsBackPerVolume() {
    return getFallbackGranularity<Derived>() != FallbackGranularity::Patch;
  }

  /**
   * Returns the limiter statuses of one half of a face of Derived: one per volume on the face when
   * it falls back volume by volume, otherwise one.
   */
  template <typename Derived>
  constexpr int getLimiterStatusesPerHalf() {
    return fallsBackPerVolume<Derived>() ? peano4::power(Derived::PatchSize, DIMENSIONS - 1) : 1;
  }
} // namespace exahype2::fv::ader

TARGET_DECLARE
/**
 * The a-posteriori fallback limiter of the ADER-WENO scheme on the patch of one cell of Derived
 * after the MOOD paradigm. A patch is troubled when its candidate states together fail the physical
 * admissibility term of Derived, as the nodes of a cell of the limited ADER-DG solver do, or when
 * one of its volumes leaves the finite states or the extrema of the observables of Derived over its
 * previous state and the previous states around it, widened by the larger of
 * Derived::DmpRelaxationParameter and Derived::DmpDifferencesScaling times their range. A troubled
 * patch repeats its step with exahype2::fv::ader::FallbackScheme from its previous states. With a
 * fallback granularity other than FallbackGranularity::Patch, each volume passes the physical
 * admissibility term on its own instead, and only the troubled volumes repeat their step, while a
 * kept volume beside one takes the fallback flux on the face between them.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::fv::ader::FallbackLimiter final {
public:
  using Patch = exahype2::fv::Patch<Derived, ComputePrecision>;

  static constexpr int PatchSize        = Patch::PatchSize;
  static constexpr int NumberOfUnknowns = Patch::NumberOfUnknowns;
  static constexpr int NumberOfData     = Patch::NumberOfData;
  static constexpr int VolumesPerPatch  = Patch::VolumesPerPatch;
  static constexpr int HaloSize         = Patch::HaloSize;

  static constexpr bool PerVolume       = fallsBackPerVolume<Derived>();
  static constexpr int  StatusesPerHalf = getLimiterStatusesPerHalf<Derived>();

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool isPhysicallyAdmissible(
    const StoragePrecision* NOALIAS              candidate,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const typename Derived::RuntimeParameters&   parameters
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool isPhysicallyAdmissibleVolume(
    int                                          volume,
    const StoragePrecision* NOALIAS              candidate,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const typename Derived::RuntimeParameters&   parameters
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool isAdmissible(
    int                                        volume,
    const StoragePrecision* NOALIAS            candidate,
    const StoragePrecision* NOALIAS            previous,
    const StoragePrecision* const              halo[peano4::FacesPerCell],
    const typename Derived::RuntimeParameters& parameters
  );

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD int getStatusOnFace(int volume, int face);

private:
  static constexpr int NumberOfObservables = [] {
    if constexpr (requires { Derived::NumberOfObservables; }) {
      return Derived::NumberOfObservables;
    } else {
      return 0;
    }
  }();

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD const StoragePrecision* getPreviousState(
    const tarch::la::Vector<DIMENSIONS, int>& index,
    const StoragePrecision* NOALIAS           previous,
    const StoragePrecision* const             halo[peano4::FacesPerCell]
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool satisfiesMaximumPrinciple(
    int                                        volume,
    const StoragePrecision* NOALIAS            candidate,
    const StoragePrecision* NOALIAS            previous,
    const StoragePrecision* const              halo[peano4::FacesPerCell],
    const typename Derived::RuntimeParameters& parameters
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/ader/FallbackLimiter.cpph"
