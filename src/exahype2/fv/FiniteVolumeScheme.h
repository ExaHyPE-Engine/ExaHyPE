// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/TimeStepping.h"
#include "exahype2/terms/DiffusiveSourceTerm.h"
#include "exahype2/terms/Flux.h"
#include "exahype2/terms/NonconservativeProduct.h"
#include "exahype2/terms/RiemannSolver.h"
#include "exahype2/terms/SourceTerm.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

/**
 * @file
 * Declares the finite-volume scheme on the patch of one cell and the pieces that the
 * reconstruction schemes build on. A patch holds PatchSize^DIMENSIONS volumes with one value per
 * unknown and auxiliary variable, the first axis fastest. A face holds 2 * HaloSize layers of
 * PatchSize^(DIMENSIONS-1) volumes along its normal and the cell on each side writes the layers
 * on its side.
 */
namespace exahype2::fv {
  template <typename Derived, typename ComputePrecision>
  class FiniteVolumeScheme;

  template <typename Entry, int LookaheadOfRing>
  class TermRing;

  struct NoTermRing;

  /**
   * A reconstruction scheme forms the states that meet at a volume face and adds their flux
   * divergence to the update. The constant HaloSize is the number of layers its stencil reads.
   */
  template <typename ReconstructionPolicy>
  concept ReconstructionScheme = std::is_empty_v<ReconstructionPolicy> and requires {
    { ReconstructionPolicy::HaloSize } -> std::convertible_to<int>;
  };

  template <typename TimeSteppingPolicy>
  concept TimeSteppingScheme = std::is_empty_v<TimeSteppingPolicy> and requires {
    { TimeSteppingPolicy::NumberOfStages } -> std::convertible_to<int>;
  };

  TARGET_DECLARE
  template <int Dimensions>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Dimensions, double> getVolumeSize(
    const tarch::la::Vector<Dimensions, double>& cellSize,
    int                                          volumesPerAxis
  );

  template <int Dimensions>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<Dimensions, double> getVolumeCentre(
    const tarch::la::Vector<Dimensions, double>& cellCentre,
    const tarch::la::Vector<Dimensions, double>& cellSize,
    int                                          volumesPerAxis,
    const tarch::la::Vector<Dimensions, int>&    volume
  );

  template <int VolumesPerAxis, int HaloSize>
  INLINE_METHOD GPU_CALLABLE_METHOD int getFaceVolumeIndex(
    int normal,
    int layer,
    int firstTangential,
    int secondTangential
  );

  INLINE_METHOD GPU_CALLABLE_METHOD int getFaceVolumeIndex(
    const tarch::la::Vector<DIMENSIONS, int>& volume,
    int                                       volumesPerAxis,
    int                                       haloSize,
    int                                       normal
  );

  template <typename SolverType, int VolumesPerAxis, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD double solveRiemannProblemOnVolumeFace(
    const ComputePrecision*                      QL,
    const ComputePrecision*                      QR,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          normal,
    const tarch::la::Vector<DIMENSIONS, int>&    volume,
    int                                          side,
    int                                          boundaryFaces,
    ComputePrecision*                            FL,
    ComputePrecision*                            FR
  );

  template <typename Maximum, typename Eigenvalue>
  INLINE_METHOD GPU_CALLABLE_METHOD void accumulateMaxEigenvalue(Maximum& maximum, Eigenvalue eigenvalue);
  TARGET_DECLARE_END
} // namespace exahype2::fv

namespace exahype2::fv::tests {
  class FiniteVolumeSchemeTest;
} // namespace exahype2::fv::tests

/**
 * A sweep over a patch updates the volumes in linearised order and reads the terms of the states
 * at most Lookahead states before and behind the current volume. The ring holds one Entry for
 * each of these 2 * Lookahead + 1 states on the host only. The function at(state) masks the state
 * with the power of two RingSize above the window and returns its entry.
 */
template <typename Entry, int LookaheadOfRing>
class exahype2::fv::TermRing final {
public:
  static constexpr int Lookahead = LookaheadOfRing;
  static constexpr int RingSize  = std::bit_ceil(static_cast<unsigned>(2 * Lookahead + 1));
  static_assert(Lookahead > 0, "The finite-volume update must read at least one volume ahead of its sweep.");
  static_assert(RingSize > 2 * Lookahead, "The ring buffer of the finite-volume update must cover its window.");

  [[nodiscard]] INLINE_METHOD Entry& at(int state);
  [[nodiscard]] INLINE_METHOD const Entry& at(int state) const;

private:
  Entry _entries[RingSize]{};
};

/**
 * Takes the place of a ring for a reconstruction that evaluates every term at the volume.
 */
struct exahype2::fv::NoTermRing final {
  static constexpr int Lookahead = 0;
};

namespace exahype2::fv {
  TARGET_DECLARE
  // SweepStackBytes bounds the ring and scratch of a sweep that stay on the stack of the host
  // thread.
  constexpr std::size_t SweepStackBytes = std::size_t{512} * 1024;
  TARGET_DECLARE_END
} // namespace exahype2::fv

TARGET_DECLARE
/**
 * FiniteVolumeScheme runs the scheme of the generated solver class Derived on the patch of one cell.
 * The types Derived::Reconstruction and Derived::TimeIntegration form the scheme. The constant
 * Derived::TimeSteppingRule selects whether update() returns wave speeds.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::fv::FiniteVolumeScheme final {
public:
  static constexpr int  PatchSize                  = Derived::PatchSize;
  static constexpr int  NumberOfUnknowns           = Derived::NumberOfUnknowns;
  static constexpr int  NumberOfAuxiliaryVariables = Derived::NumberOfAuxiliaryVariables;
  static constexpr int  NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  static constexpr int  HaloSize                   = Derived::Reconstruction::HaloSize;
  static constexpr int  VolumesPerPatch            = peano4::power(PatchSize, DIMENSIONS);
  static constexpr int  VolumesPerLayer            = peano4::power(PatchSize, DIMENSIONS - 1);
  static constexpr int  ValuesPerPatch             = VolumesPerPatch * NumberOfData;
  static constexpr int  ValuesPerFace              = 2 * HaloSize * VolumesPerLayer * NumberOfData;
  static constexpr int  LayerVolumesPerPatch       = peano4::FacesPerCell * HaloSize * VolumesPerLayer;
  static constexpr bool ReportEigenvalues = Derived::TimeSteppingRule == exahype2::TimeStepping::Rule::Adaptive;

  template <typename StoragePrecision>
  static INLINE_METHOD void initialise(
    StoragePrecision* NOALIAS                    patch,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void applyBoundaryConditions(
    StoragePrecision* NOALIAS                    face,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    int                                          faceNumber
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void projectOntoFaces(
    const StoragePrecision* NOALIAS patch,
    StoragePrecision* NOALIAS const faces[peano4::FacesPerCell]
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void projectVolumeOntoFace(
    int                             layerVolume,
    const StoragePrecision* NOALIAS patch,
    StoragePrecision* NOALIAS const faces[peano4::FacesPerCell]
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void interpolateHalo(
    const StoragePrecision* NOALIAS coarseFace,
    StoragePrecision* NOALIAS       fineFace,
    int                             side,
    int                             normal,
    int                             subface
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void injectHalo(
    const StoragePrecision* NOALIAS coarseFace,
    StoragePrecision* NOALIAS       fineFace,
    int                             side,
    int                             normal,
    int                             subface
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void restrictHalo(
    const StoragePrecision* const NOALIAS fineFaces[peano4::SubfacesPerFace],
    StoragePrecision* NOALIAS             coarseFace,
    int                                   side,
    int                                   normal
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void interpolatePatch(
    const StoragePrecision* NOALIAS parent,
    StoragePrecision* NOALIAS       child,
    int                             childPosition
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void restrictPatch(
    const StoragePrecision* const NOALIAS children[peano4::ChildrenPerCell],
    StoragePrecision* NOALIAS             parent
  );

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
  static INLINE_METHOD GPU_CALLABLE_METHOD void postprocessUpdatedPatch(
    StoragePrecision* NOALIAS                    updated,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double reduceMaxEigenvalue(
    const StoragePrecision* NOALIAS              patch,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp
  );

  template <typename StoragePrecision>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double getMaxEigenvalueOfVolume(
    int                                          volume,
    const StoragePrecision* NOALIAS              patch,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void measureErrors(
    const StoragePrecision* NOALIAS              patch,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    double* NOALIAS                              errors
  );

private:
  friend class exahype2::fv::tests::FiniteVolumeSchemeTest;

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

  template <typename Value, typename Functor>
  static INLINE_METHOD constexpr void forEachVolume(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    Value*                                       values,
    Functor&&                                    functor
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/FiniteVolumeScheme.cpph"
