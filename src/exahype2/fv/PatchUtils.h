// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Enumeration.h"
#include "peano4/Loop.h"
#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv {
  template <int Dimensions>
  struct VolumeSizeAndCentre;

  TARGET_DECLARE

  template <int Dimensions, int NumberOfVolumesPerAxisInPatch>
  static INLINE_METHOD GPU_CALLABLE_METHOD VolumeSizeAndCentre<Dimensions> getVolumeSizeAndCentre(
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    const tarch::la::Vector<Dimensions, int>&    index
  );

  template <int Dimensions, int NumberOfVolumesPerAxisInPatch>
  static INLINE_METHOD GPU_CALLABLE_METHOD
    std::pair<tarch::la::Vector<Dimensions, double>, tarch::la::Vector<Dimensions, double>>
    getFaceSizeAndCentre(
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      int                                          overlap,
      int                                          normal,
      const tarch::la::Vector<Dimensions, int>&    index
    );

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, double> getVolumeSize(
    const tarch::la::Vector<2, double>& h,
    int                                 numberOfVolumesPerAxisInPatch
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, double> getVolumeSize(
    const tarch::la::Vector<3, double>& h,
    int                                 numberOfVolumesPerAxisInPatch
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, double> getFaceSize(
    const tarch::la::Vector<2, double>& h,
    int                                 numberOfVolumesPerAxisInPatch
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, double> getFaceSize(
    const tarch::la::Vector<3, double>& h,
    int                                 numberOfVolumesPerAxisInPatch
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, double> getVolumeCentre(
    const tarch::la::Vector<2, double>& x,
    const tarch::la::Vector<2, double>& h,
    int                                 numberOfVolumesPerAxisInPatch,
    const tarch::la::Vector<2, int>&    index
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, double> getVolumeCentre(
    const tarch::la::Vector<3, double>& x,
    const tarch::la::Vector<3, double>& h,
    int                                 numberOfVolumesPerAxisInPatch,
    const tarch::la::Vector<3, int>&    index
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, double> getFaceCentre(
    const tarch::la::Vector<2, double>& x,
    const tarch::la::Vector<2, double>& h,
    int                                 numberOfVolumesPerAxisInPatch,
    int                                 overlap,
    int                                 normal,
    const tarch::la::Vector<2, int>&    index
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, double> getFaceCentre(
    const tarch::la::Vector<3, double>& x,
    const tarch::la::Vector<3, double>& h,
    int                                 numberOfVolumesPerAxisInPatch,
    int                                 overlap,
    int                                 normal,
    const tarch::la::Vector<3, int>&    index
  );

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
    const double*      minValues                   = nullptr,
    const double*      maxValues                   = nullptr
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
  void copyHalfOfHalo(
    int              unknownsPlusAuxiliaryVariables,
    int              numberOfVolumesPerAxisInPatch,
    int              haloSize, // The halo size equals the overlap of the face data.
    int              normal,
    bool             isRightLayer,
    const T* NOALIAS sourceQ,
    T* NOALIAS       destinationQ
  );
} // namespace exahype2::fv

template <int Dimensions>
struct exahype2::fv::VolumeSizeAndCentre final {
  const tarch::la::Vector<Dimensions, double> volumeSize;
  const tarch::la::Vector<Dimensions, double> volumeCentre;
};

#include "exahype2/fv/PatchUtils.cpph"
