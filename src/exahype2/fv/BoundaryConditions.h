// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/PatchUtils.h"
#include "peano4/Globals.h"
#include "peano4/Loop.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv {
  /**
   * Apply boundary conditions. Works only for a halo size of 1.
   *
   * @param boundaryCondition Callback invoked per boundary volume. Its last
   *  argument receives faceNumber, i.e. the side-aware face number and not
   *  only the axis; faceNumber % DIMENSIONS yields the axis.
   * @param faceNumber Is usually taken from marker.getSelectedFaceNumber()
   *  and is thus a number between 0 and 2d-1.
   */
  template <typename T>
  void applyBoundaryConditions(
    std::function<void(
      const T* NOALIAS                             Qinside,
      T* NOALIAS                                   Qoutside,
      const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      const tarch::la::Vector<DIMENSIONS, double>& volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal
    )>                                           boundaryCondition,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& patchSize,
    double                                       t,
    double                                       dt,
    int                                          numberOfVolumesPerAxisInPatch,
    int                                          overlap,
    int                                          unknowns,
    int                                          faceNumber,
    T* NOALIAS                                   Q
  );
} // namespace exahype2::fv

#include "exahype2/fv/BoundaryConditions.cpph"
