// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

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
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::singlestep {
  template <typename Derived, typename ComputePrecision>
  class PatchUpdate;

  TARGET_DECLARE
  // SweepStackBytes bounds the ring and scratch of a sweep that stay on the stack of the host
  // thread.
  constexpr std::size_t SweepStackBytes = std::size_t{512} * 1024;
  TARGET_DECLARE_END
} // namespace exahype2::fv::singlestep

TARGET_DECLARE
/**
 * PatchUpdate advances the patch of one cell of the generated solver class Derived by one time
 * step of a scheme that reads the patch and its halo only. Every volume adds its source term and
 * the flux divergence of the reconstruction Derived::Reconstruction to its state. The constant
 * Derived::TimeSteppingRule selects whether update() returns wave speeds. It adds the update to
 * the operations of exahype2::fv::Patch.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::fv::singlestep::PatchUpdate final : public exahype2::fv::Patch<Derived, ComputePrecision> {
public:
  using Base = exahype2::fv::Patch<Derived, ComputePrecision>;
  using Base::getMaxEigenvalueOfVolume;
  using Base::getWaveSpeedsOfVolume;
  using Base::HaloSize;
  using Base::NumberOfAuxiliaryVariables;
  using Base::NumberOfData;
  using Base::NumberOfUnknowns;
  using Base::PatchSize;
  using Base::postprocessUpdatedPatch;
  using Base::ReportEigenvalues;
  using Base::ValuesPerPatch;
  using Base::VolumesPerLayer;
  using Base::VolumesPerPatch;
  using typename Base::RuntimeParameters;

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double update(
    const StoragePrecision*                      patch,
    StoragePrecision*                            updated,
    const StoragePrecision* NOALIAS const        faces[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces,
    const RuntimeParameters&                     parameters
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD int getTeamScratchBytes();

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double updateInTeam(
    tarch::accelerator::TeamHandle               team,
    StoragePrecision*                            patch,
    const StoragePrecision* const                faces[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces,
    const RuntimeParameters&                     parameters
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
    int                                          boundaryFaces,
    const RuntimeParameters&                     parameters
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
    int                                          boundaryFaces,
    const RuntimeParameters&                     parameters
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
    StoragePrecision* NOALIAS                    updatedState,
    const RuntimeParameters&                     parameters
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
    StoragePrecision* NOALIAS                    updatedState,
    const RuntimeParameters&                     parameters
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
    ComputePrecision* NOALIAS                    flux,
    const RuntimeParameters&                     parameters
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

  // A team copies its patch into the team scratch when the patch fits into it. The team writes the
  // wave speeds of all volumes behind the copy if the reconstruction reads them and the scratch
  // fits both. Every volume then reads the wave speeds of its neighbours inside the patch from one
  // evaluation. A compiler may round a wave speed that it inlines at two places differently.
  template <typename StoragePrecision>
  struct TeamStorage final {
    using WaveSpeedTerms = typename Derived::Reconstruction::template TeamTerms<
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
    static constexpr int  PatchBytes       = ValuesPerPatch * static_cast<int>(sizeof(StoragePrecision));
    static constexpr int  WaveSpeedBytes   = static_cast<int>(sizeof(ComputePrecision));
    static constexpr int  WaveSpeedsOffset = (PatchBytes + WaveSpeedBytes - 1) / WaveSpeedBytes * WaveSpeedBytes;
    static constexpr int  WaveSpeedsEnd    = WaveSpeedsOffset + VolumesPerPatch * DIMENSIONS * WaveSpeedBytes;
    static constexpr bool StagesPatch      = PatchBytes <= tarch::accelerator::TeamScratchBytes;
    static constexpr bool ReadsWaveSpeeds  = StagesPatch and WaveSpeedsEnd <= tarch::accelerator::TeamScratchBytes
      and not std::is_same_v<WaveSpeedTerms, exahype2::fv::NoTermRing>;
    using Terms                       = std::conditional_t<ReadsWaveSpeeds, WaveSpeedTerms, exahype2::fv::NoTermRing>;
    static constexpr int ScratchBytes = ReadsWaveSpeeds ? WaveSpeedsEnd : (StagesPatch ? PatchBytes : 0);
  };

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double updateWithThreadsOfTeam(
    tarch::accelerator::TeamHandle               team,
    StoragePrecision*                            patch,
    const StoragePrecision* const                faces[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces,
    const RuntimeParameters&                     parameters
  );

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
    int                                          boundaryFaces,
    const RuntimeParameters&                     parameters
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
    double                                       timeStepSize,
    const RuntimeParameters&                     parameters
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/singlestep/PatchUpdate.cpph"
