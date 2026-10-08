// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <cstddef>

#include "exahype2/fv/Patch.h"
#include "exahype2/fv/reconstruction/ReconstructionScheme.h"
#include "exahype2/terms/DiffusiveSourceTerm.h"
#include "exahype2/terms/Flux.h"
#include "exahype2/terms/NonconservativeProduct.h"
#include "exahype2/terms/RiemannSolver.h"
#include "exahype2/terms/SourceTerm.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::singlestep {
  template <typename Derived, typename ComputePrecision>
  class PatchUpdate;

  template <typename TimeSteppingPolicy>
  concept TimeSteppingScheme = std::is_empty_v<TimeSteppingPolicy> and requires {
    { TimeSteppingPolicy::NumberOfStages } -> std::convertible_to<int>;
  };

  TARGET_DECLARE
  // SweepStackBytes bounds the ring and scratch of a sweep that stay on the stack of the host
  // thread.
  constexpr std::size_t SweepStackBytes = std::size_t{512} * 1024;
  TARGET_DECLARE_END
} // namespace exahype2::fv::singlestep

TARGET_DECLARE
/**
 * PatchUpdate advances the patch of one cell of the generated solver class Derived by one time
 * step of a scheme that reads the patch and its halo only. The types Derived::Reconstruction and
 * Derived::TimeIntegration form the scheme. The constant Derived::TimeSteppingRule selects whether
 * update() returns wave speeds. It adds the update to the operations of exahype2::fv::Patch.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::fv::singlestep::PatchUpdate final : public exahype2::fv::Patch<Derived, ComputePrecision> {
public:
  using Base = exahype2::fv::Patch<Derived, ComputePrecision>;
  using Base::getMaxEigenvalueOfVolume;
  using Base::NumberOfAuxiliaryVariables;
  using Base::NumberOfData;
  using Base::NumberOfUnknowns;
  using Base::PatchSize;
  using Base::postprocessUpdatedPatch;
  using Base::ReportEigenvalues;
  using Base::ValuesPerPatch;
  using Base::VolumesPerPatch;

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double update(
    const StoragePrecision*                      patch,
    StoragePrecision*                            updated,
    const StoragePrecision* NOALIAS const        faces[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD double sweep(
    const StoragePrecision*                      patch,
    StoragePrecision*                            updated,
    const StoragePrecision* NOALIAS const        faces[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double updateEveryVolume(
    const StoragePrecision* NOALIAS              patch,
    StoragePrecision* NOALIAS                    updated,
    const StoragePrecision* NOALIAS const        faces[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double computeVolumeUpdate(
    int                                          volume,
    const StoragePrecision* NOALIAS              patch,
    const StoragePrecision* NOALIAS const        faces[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces,
    StoragePrecision* NOALIAS                    updatedState
  );

  template <typename StoragePrecision, typename Terms>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double computeVolumeUpdate(
    int                                          volume,
    const StoragePrecision* NOALIAS              patch,
    const StoragePrecision* NOALIAS const        faces[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces,
    Terms&                                       terms,
    StoragePrecision* NOALIAS                    updatedState
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void computePatchFaceFlux(
    int                                          volume,
    const StoragePrecision* NOALIAS              patch,
    const StoragePrecision* NOALIAS const        faces[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces,
    int                                          patchFace,
    ComputePrecision* NOALIAS                    flux
  );

private:
  /**
   * Names the ring of terms a host sweep with the storage precision keeps. A solver without a
   * reconstruction leaves it uninstantiated.
   */
  template <typename StoragePrecision>
  struct SweepStorage final {
    using Terms = typename Derived::Reconstruction::template SweepTerms<
      Derived,
      PatchSize,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      exahype2::terms::HasFlux<Derived, ComputePrecision>,
      exahype2::terms::HasNonconservativeProduct<Derived, ComputePrecision>,
      exahype2::terms::HasSourceTerm<Derived, ComputePrecision>,
      exahype2::terms::HasDiffusiveSourceTerm<Derived, ComputePrecision>,
      exahype2::terms::HasRiemannSolver<Derived, ComputePrecision>,
      ReportEigenvalues,
      StoragePrecision,
      ComputePrecision
    >;
  };

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD constexpr std::size_t getSweepStorageBytes();

  template <typename Terms, typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD double sweepWithStorage(
    Terms&                                       terms,
    StoragePrecision* NOALIAS                    scratch,
    const StoragePrecision*                      patch,
    StoragePrecision*                            updated,
    const StoragePrecision* NOALIAS const        faces[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces
  );

  template <typename StoragePrecision, typename Terms>
  static INLINE_METHOD void evaluateTermsOfState(
    Terms&                                       terms,
    int                                          state,
    const StoragePrecision* NOALIAS              patch,
    const StoragePrecision* NOALIAS const        faces[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/singlestep/PatchUpdate.cpph"
