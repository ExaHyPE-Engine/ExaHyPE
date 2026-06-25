// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/Loop.h"
#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"
#include "toolbox/blockstructured/Enumeration.h"

#include <sstream>
#include <vector>

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

    const auto* xd = x.data();
    const auto* hd = h.data();
    const auto* id = index.data();

    auto* sd = size.data();
    auto* cd = centre.data();

    constexpr double InvN = 1.0 / NumberOfVolumesPerAxisInPatch;

    UNROLL(DIMENSIONS)
    for (int d = 0; d < Dimensions; d++) {
      const double volumeSize = hd[d] * InvN;
      sd[d]                   = volumeSize;
      cd[d]                   = xd[d] - 0.5 * hd[d] + (0.5 + id[d]) * volumeSize;
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

    const auto* xd = x.data();
    const auto* hd = h.data();
    const auto* id = index.data();

    auto* fsd = faceSize.data();
    auto* fcd = faceCentre.data();

    constexpr double InvN = 1.0 / NumberOfVolumesPerAxisInPatch;

    UNROLL(DIMENSIONS)
    for (int d = 0; d < Dimensions; d++) {
      const double volumeSize = hd[d] * InvN;
      fsd[d]                  = volumeSize;

      // Base centre
      double centre = (d == normal) ? xd[d] - overlap * volumeSize + 0.5 * volumeSize
                                    : xd[d] - 0.5 * hd[d] + 0.5 * volumeSize;

      // Shift by index
      fcd[d] = centre + id[d] * volumeSize;
    }

    return {faceSize, faceCentre};
  }

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<2, double> getVolumeSize(
    const tarch::la::Vector<2, double>& h,
    const int                           numberOfVolumesPerAxisInPatch
  ) {
    assertion(numberOfVolumesPerAxisInPatch >= 1, h, numberOfVolumesPerAxisInPatch);
    tarch::la::Vector<2, double> volumeSize;
    for (int d = 0; d < 2; d++) {
      volumeSize(d) = h(d) / numberOfVolumesPerAxisInPatch;
    }
    return volumeSize;
  }

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, double> getVolumeSize(
    const tarch::la::Vector<3, double>& h,
    const int                           numberOfVolumesPerAxisInPatch
  ) {
    assertion(numberOfVolumesPerAxisInPatch >= 1, h, numberOfVolumesPerAxisInPatch);
    tarch::la::Vector<3, double> volumeSize;
    for (int d = 0; d < 3; d++) {
      volumeSize(d) = h(d) / numberOfVolumesPerAxisInPatch;
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
    tarch::la::Vector<2, double> volumeSize = getVolumeSize(h, numberOfVolumesPerAxisInPatch);
    tarch::la::Vector<2, double> result     = x - 0.5 * h + 0.5 * volumeSize;
    for (int d = 0; d < 2; d++) {
      result(d) += index(d) * volumeSize(d);
    }
    return result;
  }

  static INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<3, double> getVolumeCentre(
    const tarch::la::Vector<3, double>& x,
    const tarch::la::Vector<3, double>& h,
    const int                           numberOfVolumesPerAxisInPatch,
    const tarch::la::Vector<3, int>&    index
  ) {
    tarch::la::Vector<3, double> volumeSize = getVolumeSize(h, numberOfVolumesPerAxisInPatch);
    tarch::la::Vector<3, double> result     = x - 0.5 * h + 0.5 * volumeSize;
    for (int d = 0; d < 3; d++) {
      result(d) += index(d) * volumeSize(d);
    }
    return result;
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
    tarch::la::Vector<2, double> result;
    for (int d = 0; d < 2; d++) {
      if (d == normal) {
        result(d) = x(d) - overlap * volumeSize(d) + 0.5 * volumeSize(d);
      } else {
        result(d) = x(d) - 0.5 * h(d) + 0.5 * volumeSize(d);
      }
      result(d) += index(d) * volumeSize(d);
    }
    return result;
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
    tarch::la::Vector<3, double> result     = x - 0.5 * h + 0.5 * volumeSize;
    for (int d = 0; d < 3; d++) {
      if (d == normal) {
        result(d) = x(d) - overlap * volumeSize(d) + 0.5 * volumeSize(d);
      } else {
        result(d) = x(d) - 0.5 * h(d) + 0.5 * volumeSize(d);
      }
      result(d) += index(d) * volumeSize(d);
    }
    return result;
  }

  TARGET_DECLARE_END

  double getVolumeLength(const tarch::la::Vector<2, double>& h, int numberOfVolumesPerAxisInPatch);

  double getVolumeLength(const tarch::la::Vector<3, double>& h, int numberOfVolumesPerAxisInPatch);

  std::string plotVolume(const double* NOALIAS Q, int unknowns);

  /**
   * Just runs over the patch and ensures that no entry is non or infinite. In
   * ExaHyPE, we primarily work with split approaches. That is, diagonal halo
   * entries are never initialised properly: We can copy over the face-connected
   * data, but we lack information through the diagonals. This routine takes
   * this into account when it validates the entries.
   *
   * Assumes all data are held as AoS.
   *
   * @param location String that tells system from where this routine got called
   * @param minValues Is either a nullptr or it points to a double array with
   *   exactly unknowns+auxiliaryVariables entries
   */
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

  /**
   * Plot patch.
   *
   * Usually used for debugging.
   *
   * Assumes all data are held as AoS.
   */
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
    int              numberOfGridCellsPerPatchPerAxis,
    int              haloSize,
    int              normal,
    bool             prettyPrint = false
  );

  /**
   * A face always holds a left and a right overlap
   *
   * This routine copies over half of the overlap
   *
   * @param isRightLayer
   */
  template <typename T>
  void copyHalfOfHalo(
    int              unknownsPlusAuxiliaryVariables,
    int              numberOfGridCellsPerPatchPerAxis,
    int              haloSize, // same as overlap
    int              normal,
    bool             isRightLayer,
    const T* NOALIAS srcQ,
    T* NOALIAS       destQ
  );
} // namespace exahype2::fv

#include "PatchUtils.cpph"
