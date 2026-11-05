// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/TimeStepping.h"
#include "exahype2/fv/Patch.h"
#include "exahype2/fv/reconstruction/Constant.h"
#include "exahype2/fv/ader/Scheme.h"
#include "exahype2/fv/singlestep/PatchUpdate.h"
#include "exahype2/fv/singlestep/SingleStep.h"
#include "exahype2/terms/DiffusiveSourceTerm.h"
#include "exahype2/terms/Flux.h"
#include "exahype2/terms/MaxEigenvalue.h"
#include "exahype2/terms/NonconservativeProduct.h"
#include "exahype2/terms/RiemannSolver.h"
#include "exahype2/terms/SourceTerm.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::ader {
  template <typename Derived, typename ComputePrecision>
  class FallbackScheme;

  /**
   * Returns whether the generated solver class Derived names a fallback reconstruction.
   */
  template <typename Derived>
  constexpr bool hasLimiter() {
    if constexpr (requires { typename Derived::FallbackReconstruction; }) {
      return not std::is_void_v<typename Derived::FallbackReconstruction>;
    } else {
      return false;
    }
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
 * FallbackScheme repeats the step of the volumes of the patch of one cell of Derived with the
 * fallback reconstruction of Derived, which reads one layer of the halo of Derived. Both sides of a
 * face evaluate the fallback flux from the same previous states, the patch from its previous patch
 * and the halo, so they agree on it bit by bit.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::fv::ader::FallbackScheme final {
public:
  template <typename Reconstruct>
  struct FallbackSolver;

  using Scheme = exahype2::fv::ader::Scheme<Derived, ComputePrecision>;
  using Patch  = exahype2::fv::Patch<Derived, ComputePrecision>;
  // Without a fallback, the scheme names the Godunov fallback, which no kernel runs.
  using Solver = FallbackSolver<std::conditional_t<
    exahype2::fv::ader::hasLimiter<Derived>(),
    typename exahype2::fv::ader::FallbackReconstructionOf<Derived>::Type,
    exahype2::fv::reconstruction::Constant>>;
  using FallbackPatch = exahype2::fv::singlestep::PatchUpdate<Solver, ComputePrecision>;

  static constexpr int PatchSize        = Patch::PatchSize;
  static constexpr int NumberOfUnknowns = Patch::NumberOfUnknowns;
  static constexpr int NumberOfData     = Patch::NumberOfData;
  static constexpr int HaloSize         = Patch::HaloSize;
  static constexpr int FallbackHaloSize = Solver::Reconstruction::HaloSize;
  static constexpr int HaloOffset       = (HaloSize - FallbackHaloSize) * Patch::VolumesPerLayer * NumberOfData;

  static_assert(FallbackHaloSize <= HaloSize, "The fallback reads no more layers than the halo holds.");

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void getFallbackHalo(
    const StoragePrecision* const halo[peano4::FacesPerCell],
    const StoragePrecision*       fallbackHalo[peano4::FacesPerCell]
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
};

/**
 * Derived in the form the single-step schemes read with the reconstruction Reconstruct. The update
 * computes no wave speeds, and every term calls the term of Derived with the same name.
 */
template <typename Derived, typename ComputePrecision>
template <typename Reconstruct>
struct exahype2::fv::ader::FallbackScheme<Derived, ComputePrecision>::FallbackSolver final {
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
  static INLINE_METHOD GPU_CALLABLE_METHOD void diffusiveSourceTerm(
    const Precision*                             Q,
    const Precision*                             deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    Precision*                                   S
  )
    requires exahype2::terms::HasDiffusiveSourceTerm<Derived, Precision>
  {
    Derived::diffusiveSourceTerm(Q, deltaQ, x, h, t, dt, S);
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
TARGET_DECLARE_END

#include "exahype2/fv/ader/FallbackScheme.cpph"
