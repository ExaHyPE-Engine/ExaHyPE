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
 * This file declares the operations on the patch of one cell that every finite-volume solver
 * family shares. A patch holds PatchSize^DIMENSIONS volumes with one value per unknown and
 * auxiliary variable. The first axis of a patch runs fastest. A face holds 2 * HaloSize layers of
 * PatchSize^(DIMENSIONS-1) volumes along its normal with the layers of the lower side first. The
 * first tangential axis of a face runs fastest. The cell on each side of a face writes the layers
 * on its side.
 */
namespace exahype2::fv {
  template <typename Derived, typename ComputePrecision>
  class Patch;

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

  template <typename Maximum, typename Eigenvalue>
  INLINE_METHOD GPU_CALLABLE_METHOD void accumulateMaxEigenvalue(Maximum& maximum, Eigenvalue eigenvalue);
  TARGET_DECLARE_END
} // namespace exahype2::fv

namespace exahype2::fv::tests {
  class PatchSchemeTest;
} // namespace exahype2::fv::tests

TARGET_DECLARE
/**
 * Patch holds the operations of the generated solver class Derived on the patch of one cell that
 * do not depend on the solver family: the initial condition, the boundary conditions, the halo
 * of the faces, the transfer between the levels and the wave speeds and errors of a patch. The
 * constant Derived::Reconstruction::HaloSize fixes the layers of a face.
 */
template <typename Derived, typename ComputePrecision>
class exahype2::fv::Patch {
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
  friend class exahype2::fv::tests::PatchSchemeTest;

  template <typename Value, typename Functor>
  static INLINE_METHOD constexpr void forEachVolume(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    Value*                                       values,
    Functor&&                                    functor
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/Patch.cpph"
