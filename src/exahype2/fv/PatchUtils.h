// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/Loop.h"
#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"
#include "toolbox/blockstructured/Enumeration.h"

namespace exahype2::fv {
  TARGET_DECLARE

  template <int Dimensions>
  struct VolumeSizeAndCentre {
    const tarch::la::Vector<Dimensions, double> volumeSize;
    const tarch::la::Vector<Dimensions, double> volumeCentre;
  };

  template <int Dimensions, int NumberOfVolumesPerAxisInPatch>
  static INLINE_METHOD GPU_CALLABLE_METHOD VolumeSizeAndCentre<Dimensions> getVolumeSizeAndCentre(
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    const tarch::la::Vector<Dimensions, int>&    index
  ) {
    static_assert(NumberOfVolumesPerAxisInPatch >= 1, "NumberOfVolumesPerAxisInPatch must be >= 1");

    tarch::la::Vector<Dimensions, double> size;
    tarch::la::Vector<Dimensions, double> centre;

    const auto* cellCentreData  = x.data();
    const auto* cellSizeData    = h.data();
    const auto* volumeIndexData = index.data();

    auto* volumeSizeData   = size.data();
    auto* volumeCentreData = centre.data();

    constexpr double InverseVolumesPerAxis = 1.0 / NumberOfVolumesPerAxisInPatch;

    UNROLL(DIMENSIONS)
    for (int axis = 0; axis < Dimensions; axis++) {
      const double volumeSize = cellSizeData[axis] * InverseVolumesPerAxis;
      volumeSizeData[axis]    = volumeSize;
      volumeCentreData[axis] =
        cellCentreData[axis] - 0.5 * cellSizeData[axis] + (0.5 + volumeIndexData[axis]) * volumeSize;
    }

    return {size, centre};
  }

  template <int Dimensions, int NumberOfVolumesPerAxisInPatch>
  static INLINE_METHOD GPU_CALLABLE_METHOD
    std::pair<tarch::la::Vector<Dimensions, double>, tarch::la::Vector<Dimensions, double>>
    getFaceSizeAndCentre(
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      const int                                    overlap,
      const int                                    normal,
      const tarch::la::Vector<Dimensions, int>&    index
    ) {
    static_assert(NumberOfVolumesPerAxisInPatch >= 1, "NumberOfVolumesPerAxisInPatch must be >= 1");

    tarch::la::Vector<Dimensions, double> faceSize;
    tarch::la::Vector<Dimensions, double> faceCentre;

    const auto* cellCentreData = x.data();
    const auto* cellSizeData   = h.data();
    const auto* faceIndexData  = index.data();

    auto* faceSizeData   = faceSize.data();
    auto* faceCentreData = faceCentre.data();

    constexpr double InverseVolumesPerAxis = 1.0 / NumberOfVolumesPerAxisInPatch;

    UNROLL(DIMENSIONS)
    for (int axis = 0; axis < Dimensions; axis++) {
      const double volumeSize = cellSizeData[axis] * InverseVolumesPerAxis;
      faceSizeData[axis]      = volumeSize;

      const double faceCentreOnAxis = (axis == normal)
        ? cellCentreData[axis] - overlap * volumeSize + 0.5 * volumeSize
        : cellCentreData[axis] - 0.5 * cellSizeData[axis] + 0.5 * volumeSize;

      faceCentreData[axis] = faceCentreOnAxis + faceIndexData[axis] * volumeSize;
    }

    return {faceSize, faceCentre};
  }

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, double> getVolumeSize(
    const tarch::la::Vector<2, double>& h,
    const int                           numberOfVolumesPerAxisInPatch
  ) {
    assertion(numberOfVolumesPerAxisInPatch >= 1, h, numberOfVolumesPerAxisInPatch);
    tarch::la::Vector<2, double> volumeSize;
    for (int axis = 0; axis < 2; axis++) {
      volumeSize(axis) = h(axis) / numberOfVolumesPerAxisInPatch;
    }
    return volumeSize;
  }

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, double> getVolumeSize(
    const tarch::la::Vector<3, double>& h,
    const int                           numberOfVolumesPerAxisInPatch
  ) {
    assertion(numberOfVolumesPerAxisInPatch >= 1, h, numberOfVolumesPerAxisInPatch);
    tarch::la::Vector<3, double> volumeSize;
    for (int axis = 0; axis < 3; axis++) {
      volumeSize(axis) = h(axis) / numberOfVolumesPerAxisInPatch;
    }
    return volumeSize;
  }

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, double> getFaceSize(
    const tarch::la::Vector<2, double>& h,
    const int                           numberOfVolumesPerAxisInPatch
  ) {
    return getVolumeSize(h, numberOfVolumesPerAxisInPatch);
  }

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, double> getFaceSize(
    const tarch::la::Vector<3, double>& h,
    const int                           numberOfVolumesPerAxisInPatch
  ) {
    return getVolumeSize(h, numberOfVolumesPerAxisInPatch);
  }

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, double> getVolumeCentre(
    const tarch::la::Vector<2, double>& x,
    const tarch::la::Vector<2, double>& h,
    const int                           numberOfVolumesPerAxisInPatch,
    const tarch::la::Vector<2, int>&    index
  ) {
    tarch::la::Vector<2, double> volumeSize   = getVolumeSize(h, numberOfVolumesPerAxisInPatch);
    tarch::la::Vector<2, double> volumeCentre = x - 0.5 * h + 0.5 * volumeSize;
    for (int axis = 0; axis < 2; axis++) {
      volumeCentre(axis) += index(axis) * volumeSize(axis);
    }
    return volumeCentre;
  }

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, double> getVolumeCentre(
    const tarch::la::Vector<3, double>& x,
    const tarch::la::Vector<3, double>& h,
    const int                           numberOfVolumesPerAxisInPatch,
    const tarch::la::Vector<3, int>&    index
  ) {
    tarch::la::Vector<3, double> volumeSize   = getVolumeSize(h, numberOfVolumesPerAxisInPatch);
    tarch::la::Vector<3, double> volumeCentre = x - 0.5 * h + 0.5 * volumeSize;
    for (int axis = 0; axis < 3; axis++) {
      volumeCentre(axis) += index(axis) * volumeSize(axis);
    }
    return volumeCentre;
  }

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, double> getFaceCentre(
    const tarch::la::Vector<2, double>& x,
    const tarch::la::Vector<2, double>& h,
    const int                           numberOfVolumesPerAxisInPatch,
    const int                           overlap,
    const int                           normal,
    const tarch::la::Vector<2, int>&    index
  ) {
    tarch::la::Vector<2, double> volumeSize = getFaceSize(h, numberOfVolumesPerAxisInPatch);
    tarch::la::Vector<2, double> faceCentre;
    for (int axis = 0; axis < 2; axis++) {
      if (axis == normal) {
        faceCentre(axis) = x(axis) - overlap * volumeSize(axis) + 0.5 * volumeSize(axis);
      } else {
        faceCentre(axis) = x(axis) - 0.5 * h(axis) + 0.5 * volumeSize(axis);
      }
      faceCentre(axis) += index(axis) * volumeSize(axis);
    }
    return faceCentre;
  }

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, double> getFaceCentre(
    const tarch::la::Vector<3, double>& x,
    const tarch::la::Vector<3, double>& h,
    const int                           numberOfVolumesPerAxisInPatch,
    const int                           overlap,
    const int                           normal,
    const tarch::la::Vector<3, int>&    index
  ) {
    tarch::la::Vector<3, double> volumeSize = getVolumeSize(h, numberOfVolumesPerAxisInPatch);
    tarch::la::Vector<3, double> faceCentre = x - 0.5 * h + 0.5 * volumeSize;
    for (int axis = 0; axis < 3; axis++) {
      if (axis == normal) {
        faceCentre(axis) = x(axis) - overlap * volumeSize(axis) + 0.5 * volumeSize(axis);
      } else {
        faceCentre(axis) = x(axis) - 0.5 * h(axis) + 0.5 * volumeSize(axis);
      }
      faceCentre(axis) += index(axis) * volumeSize(axis);
    }
    return faceCentre;
  }

  TARGET_DECLARE_END

  double getVolumeLength(const tarch::la::Vector<2, double>& h, int numberOfVolumesPerAxisInPatch);

  double getVolumeLength(const tarch::la::Vector<3, double>& h, int numberOfVolumesPerAxisInPatch);

  std::string plotVolume(const double* NOALIAS Q, int unknowns);

  template <typename T>
  void validatePatch(
    const T* NOALIAS   Q,
    int                unknowns,
    int                auxiliaryVariables,
    int                numberOfVolumesPerAxisInPatch,
    int                haloSize,
    const std::string& location                    = "",
    bool               triggerNonCriticalAssertion = true,
    double*            minValues                   = nullptr,
    double*            maxValues                   = nullptr
  );

  template <typename T>
  std::string plotPatch(
    const T* NOALIAS Q,
    int              unknowns,
    int              auxiliaryVariables,
    int              numberOfVolumesPerAxisInPatch,
    int              haloSize,
    bool             prettyPrint = false
  );

  template <typename T>
  std::string plotPatchOverlap(
    const T* NOALIAS Q,
    int              unknowns,
    int              auxiliaryVariables,
    int              numberOfVolumesPerAxisInPatch,
    int              haloSize,
    int              normal,
    bool             prettyPrint = false
  );

  template <typename T>
  void copyHalfOfHalo(
    int              unknownsPlusAuxiliaryVariables,
    int              numberOfVolumesPerAxisInPatch,
    int              haloSize, // same as overlap
    int              normal,
    bool             isRightLayer,
    const T* NOALIAS srcQ,
    T* NOALIAS       destQ
  );
} // namespace exahype2::fv

#include "exahype2/fv/PatchUtils.cpph"
