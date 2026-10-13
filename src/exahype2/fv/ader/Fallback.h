// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <type_traits>

#include "exahype2/TimeStepping.h"
#include "exahype2/fv/Patch.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "exahype2/fv/ader/FallbackGranularity.h"
#include "exahype2/fv/ader/Scheme.h"
#include "exahype2/fv/singlestep/PatchUpdate.h"
#include "exahype2/fv/singlestep/SingleStep.h"
#include "exahype2/terms/DmpObservables.h"
#include "exahype2/terms/DmpRelaxationParameter.h"
#include "exahype2/terms/Flux.h"
#include "exahype2/terms/MaxEigenvalue.h"
#include "exahype2/terms/NonconservativeProduct.h"
#include "exahype2/terms/PhysicalAdmissibility.h"
#include "exahype2/terms/RiemannSolver.h"
#include "exahype2/terms/SourceTerm.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::ader {
  /**
   * The status of a patch of the ADER-WENO scheme after its candidate update, also written into
   * the own half of each of its faces. Between the prediction and the correction, a patch with a
   * volume whose Picard iteration diverged or stopped at its sweep limit is FailedPrediction.
   */
  enum class FallbackStatus : int { Accepted = 0, FallsBack = 1, FailedPrediction = 2 };

  /**
   * Why a volume repeated its step with the fallback scheme in the last time step, or Accepted: Its
   * candidate state is not finite, fails the physical admissibility term on its own or the discrete
   * maximum principle, it lies beside such a volume, or its patch repeated the step as a whole.
   */
  enum class VolumeStatus : int {
    Accepted         = 0,
    NotFinite        = 1,
    Inadmissible     = 2,
    MaximumPrinciple = 3,
    Neighbour        = 4,
    Patch            = 5
  };

  template <typename Derived, typename Reconstruct>
  struct FallbackSolver;

  template <typename Derived, typename FallbackDerived, typename ComputePrecision>
  struct Fallback;

  /**
   * Returns whether the generated solver class Derived names a fallback reconstruction.
   */
  template <typename Derived>
  constexpr bool fallsBack() {
    if constexpr (requires { typename Derived::FallbackReconstruction; }) {
      return not std::is_void_v<typename Derived::FallbackReconstruction>;
    } else {
      return false;
    }
  }

  /**
   * Returns whether Derived falls back volume by volume. Only a solver with a fallback names a
   * granularity.
   */
  template <typename Derived>
  constexpr bool fallsBackPerVolume() {
    return getFallbackGranularity<Derived>() != FallbackGranularity::Patch;
  }

  /**
   * Returns the statuses of one half of a face of Derived: one per volume on the face when it falls
   * back volume by volume, otherwise one.
   */
  template <typename Derived>
  constexpr int getFallbackStatusesPerHalf() {
    return fallsBackPerVolume<Derived>() ? peano4::power(Derived::PatchSize, DIMENSIONS - 1) : 1;
  }

  /**
   * Names the fallback reconstruction of Derived, or void.
   */
  template <typename Derived>
  struct FallbackReconstructionOf {
    using Type = void;
  };

  template <typename Derived>
    requires requires { typename Derived::FallbackReconstruction; }
  struct FallbackReconstructionOf<Derived> {
    using Type = typename Derived::FallbackReconstruction;
  };
} // namespace exahype2::fv::ader

TARGET_DECLARE
/**
 * The finite-volume solver Derived in the form the single-step schemes read with the
 * reconstruction Reconstruct. The update computes no wave speeds, and every term calls the term of
 * Derived with the same name.
 */
template <typename Derived, typename Reconstruct>
struct exahype2::fv::ader::FallbackSolver final {
  static constexpr int PatchSize                  = Derived::PatchSize;
  static constexpr int NumberOfUnknowns           = Derived::NumberOfUnknowns;
  static constexpr int NumberOfAuxiliaryVariables = Derived::NumberOfAuxiliaryVariables;

  static constexpr exahype2::TimeStepping::Rule TimeSteppingRule = exahype2::TimeStepping::Rule::Fixed;

  using Reconstruction  = Reconstruct;
  using TimeIntegration = exahype2::fv::singlestep::SingleStep;

  template <typename Precision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void flux(
    const Precision*                             Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    Precision*                                   F
  )
    requires exahype2::terms::HasFlux<Derived, Precision>
  {
    Derived::flux(Q, x, h, t, dt, normal, F);
  }

  template <typename Precision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void nonconservativeProduct(
    const Precision*                             Q,
    const Precision*                             deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    Precision*                                   BTimesDeltaQ
  )
    requires exahype2::terms::HasNonconservativeProduct<Derived, Precision>
  {
    Derived::nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);
  }

  template <typename Precision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void sourceTerm(
    const Precision*                             Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    Precision*                                   S
  )
    requires exahype2::terms::HasSourceTerm<Derived, Precision>
  {
    Derived::sourceTerm(Q, x, h, t, dt, S);
  }

  template <typename Precision>
  static INLINE_METHOD GPU_CALLABLE_METHOD double maxEigenvalue(
    const Precision*                             Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal
  )
    requires exahype2::terms::HasMaxEigenvalue<Derived, Precision>
  {
    return Derived::maxEigenvalue(Q, x, h, t, dt, normal);
  }

  template <typename Precision>
  static INLINE_METHOD GPU_CALLABLE_METHOD double solveRiemannProblem(
    const Precision*                             QL,
    const Precision*                             QR,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    const bool                                   isBoundaryFace,
    const int                                    faceIndex,
    Precision*                                   FL,
    Precision*                                   FR
  )
    requires exahype2::terms::HasRiemannSolver<Derived, Precision>
  {
    return Derived::solveRiemannProblem(QL, QR, x, h, t, dt, normal, isBoundaryFace, faceIndex, FL, FR);
  }
};

/**
 * The a-posteriori fallback of the ADER-WENO scheme on the patch of one cell of Derived after the
 * MOOD paradigm. FallbackDerived is Derived with the fallback reconstruction, which reads one layer
 * of the halo of Derived. A patch is troubled when its candidate states together fail the physical
 * admissibility term of Derived, as the nodes of a cell of the limited ADER-DG solver do, or when
 * one of its volumes leaves the finite states or the extrema of the observables of Derived over its
 * previous state and the previous states around it, widened by the larger of
 * Derived::DmpRelaxationParameter and Derived::DmpDifferencesScaling times their range. A troubled
 * patch repeats its step with FallbackDerived from its previous states. With a fallback granularity
 * other than FallbackGranularity::Patch, each volume passes the physical admissibility term on its
 * own instead, and only the troubled volumes repeat their step, while a kept volume beside one takes
 * the fallback flux on the face between them.
 *
 * Both sides of a face evaluate the fallback flux from the same previous states, the patch from its
 * previous patch and the halo, so they agree on it bit by bit.
 */
template <typename Derived, typename FallbackDerived, typename ComputePrecision>
struct exahype2::fv::ader::Fallback final {
  using Scheme        = exahype2::fv::ader::Scheme<Derived, ComputePrecision>;
  using Patch         = exahype2::fv::Patch<Derived, ComputePrecision>;
  using FallbackPatch = exahype2::fv::singlestep::PatchUpdate<FallbackDerived, ComputePrecision>;

  static constexpr int PatchSize        = Patch::PatchSize;
  static constexpr int NumberOfUnknowns = Patch::NumberOfUnknowns;
  static constexpr int NumberOfData     = Patch::NumberOfData;
  static constexpr int VolumesPerPatch  = Patch::VolumesPerPatch;
  static constexpr int HaloSize         = Patch::HaloSize;
  static constexpr int FallbackHaloSize = FallbackDerived::Reconstruction::HaloSize;
  static constexpr int HaloOffset       = (HaloSize - FallbackHaloSize) * Patch::VolumesPerLayer * NumberOfData;

  static constexpr bool PerVolume       = fallsBackPerVolume<Derived>();
  static constexpr int  StatusesPerHalf = getFallbackStatusesPerHalf<Derived>();

  static_assert(FallbackHaloSize <= HaloSize, "The fallback reads no more layers than the halo holds.");

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void getFallbackHalo(
    const StoragePrecision* const halo[peano4::FacesPerCell],
    const StoragePrecision*       fallbackHalo[peano4::FacesPerCell]
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool isPhysicallyAdmissible(
    const StoragePrecision* NOALIAS              candidate,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool isPhysicallyAdmissibleVolume(
    int                                          volume,
    const StoragePrecision* NOALIAS              candidate,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool isAdmissible(
    int                             volume,
    const StoragePrecision* NOALIAS candidate,
    const StoragePrecision* NOALIAS previous,
    const StoragePrecision* const   halo[peano4::FacesPerCell]
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void recomputeVolume(
    int                                          volume,
    const StoragePrecision* NOALIAS              previous,
    const StoragePrecision* const                halo[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces,
    StoragePrecision* NOALIAS                    patch
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void swapFaceFlux(
    int                                          volume,
    int                                          face,
    bool                                         toFallback,
    const StoragePrecision* NOALIAS              previous,
    const StoragePrecision* const                halo[peano4::FacesPerCell],
    const ComputePrecision* NOALIAS              fluxHalf,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces,
    StoragePrecision* NOALIAS                    patch
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void swapInteriorFlux(
    int                                          volume,
    int                                          face,
    const StoragePrecision* NOALIAS              previous,
    const ComputePrecision* NOALIAS              interiorUpdates,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces,
    StoragePrecision* NOALIAS                    patch
  );

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool isOnPatchFace(int volume, int face);

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD int getNeighbour(int volume, int face);

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
    int                             volume,
    const StoragePrecision* NOALIAS candidate,
    const StoragePrecision* NOALIAS previous,
    const StoragePrecision* const   halo[peano4::FacesPerCell]
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/ader/Fallback.cpph"
