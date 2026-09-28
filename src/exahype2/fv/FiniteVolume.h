// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/TimeStepping.h"
#include "peano4/Constants.h"
#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

/**
 * @file
 * The finite-volume scheme on the patch of one cell and the pieces the reconstruction schemes
 * build on. A patch holds PatchSize^DIMENSIONS volumes with one value per unknown and auxiliary
 * variable, the first axis running fastest. A face holds 2 * HaloSize layers of
 * PatchSize^(DIMENSIONS-1) volumes along its normal, the lower side's layers first and the first
 * tangential axis running fastest; the cell on a side writes the layers on that side, so the
 * lower cell writes the lower half of its upper face and the upper cell the upper half of its
 * lower face.
 */
namespace exahype2::fv {
  template <typename Derived, typename ComputePrecision>
  class FiniteVolume;

  /**
   * A reconstruction scheme forms the states that meet at a volume face and adds their flux
   * divergence to the update; HaloSize is the number of layers its stencil reads.
   */
  template <typename ReconstructionPolicy>
  concept ReconstructionScheme = std::is_empty_v<ReconstructionPolicy> and requires {
    { ReconstructionPolicy::HaloSize } -> std::convertible_to<int>;
  };

  /**
   * A time-stepping scheme advances a patch by one time step in NumberOfStages evaluations.
   */
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
  class FiniteVolumeTest;
} // namespace exahype2::fv::tests

TARGET_DECLARE
/**
 * The finite-volume scheme on the patch of one cell of Derived, the generated solver class, whose
 * Reconstruction and TimeIntegration form the scheme and whose TimeSteppingRule decides whether
 * update() reports wave speeds.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::fv::FiniteVolume final {
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
  static constexpr bool ReportEigenvalues = Derived::TimeSteppingRule == exahype2::TimeStepping::Rule::Adaptive;

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void initialise(
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
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double reduceMaxEigenvalue(
    const StoragePrecision* NOALIAS              patch,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp
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
  friend class exahype2::fv::tests::FiniteVolumeTest;

  template <typename Value, typename Functor>
  static INLINE_METHOD constexpr void forEachVolume(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    Value*                                       values,
    Functor&&                                    functor
  );

  template <typename StoragePrecision>
  static INLINE_METHOD GPU_CALLABLE_METHOD void raiseMaxEigenvalue(
    double&                                      maximum,
    const StoragePrecision* NOALIAS              patch,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/FiniteVolume.cpph"
