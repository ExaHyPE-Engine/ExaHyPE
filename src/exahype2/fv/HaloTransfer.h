// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/amr/ChildDataStage.h"
#include "exahype2/amr/LevelJumpEvents.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/CompilerSpecificSettings.h"

/**
 * @file
 * The halo transfer kernels of the finite-volume family, which the grid events of a level jump
 * hand to the routines of exahype2/amr/LevelJumpEvents.h.
 */
namespace exahype2::fv {
  template <typename T>
  class FaceHaloInterpolation;

  template <typename T>
  class FaceHaloRestriction;
} // namespace exahype2::fv

/**
 * Interpolates one side of a hanging face from the coarse face with a tangential slope.
 */
template <typename T>
class exahype2::fv::FaceHaloInterpolation final {
public:
  INLINE_METHOD FaceHaloInterpolation(int numberOfDofsPerAxis, int numberOfUnknowns);

  INLINE_METHOD void operator()(
    const peano4::dataview::FaceMarker& marker,
    const T* NOALIAS                    coarseFaceValues,
    T* NOALIAS                          fineFaceValues,
    int                                 side
  ) const;

private:
  const int _numberOfDofsPerAxis{0};
  const int _numberOfUnknowns{0};
};

/**
 * Averages the staged subfaces of one side of a face onto that side. The key may alias the face
 * values.
 */
template <typename T>
class exahype2::fv::FaceHaloRestriction final {
public:
  INLINE_METHOD FaceHaloRestriction(int normal, int numberOfDofsPerAxis, int numberOfUnknowns);

  INLINE_METHOD void operator()(
    exahype2::amr::ChildDataStage<T>& stage,
    const void*                       coarseFaceKey,
    T*                                coarseFaceValues
  ) const;

private:
  const int _normal{0};
  const int _numberOfDofsPerAxis{0};
  const int _numberOfUnknowns{0};
};

#include "exahype2/fv/HaloTransfer.cpph"
