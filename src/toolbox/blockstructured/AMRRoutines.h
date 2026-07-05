// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Loop.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceMarker.h"

#include <type_traits>

namespace toolbox::blockstructured {
  /*
   * Piecewise-constant interpolation and averaging restriction for block-structured
   * (e.g. finite-volume) patch data. A cell holds numberOfDofsPerAxis^DIMENSIONS volumes of
   * unknowns elements each; a face holds 2 * numberOfDofsPerAxis^(DIMENSIONS-1) * unknowns
   * values (both sides), in lexicographic order.
   *
   * Peano subdivides every cell into 3^DIMENSIONS children, so interpolation copies each
   * coarse volume onto its 3^DIMENSIONS fine volumes, and restriction averages the
   * 3^DIMENSIONS fine volumes back onto their coarse volume.
   *
   * The routines assume a halo of size 1; a larger halo would need extrapolation because a
   * fine face is smaller than the coarse face it restricts onto.
   */

  template <typename T>
  INLINE_METHOD void interpolateHaloLayer(
    const peano4::datamanagement::FaceMarker& marker,
    const T* NOALIAS                          coarseGridFaceValues,
    T* NOALIAS                                fineGridFaceValues,
    const int                                 side,
    const int                                 numberOfDofsPerAxis,
    const int                                 unknowns
  );

  template <typename T>
  INLINE_METHOD void interpolateHaloLayerFromCoarseCell(
    const peano4::datamanagement::FaceMarker& marker,
    const T* NOALIAS                          coarseGridCellValues,
    T* NOALIAS                                fineGridFaceValues,
    const int                                 side,
    const int                                 numberOfDofsPerAxis,
    const int                                 unknowns
  );

  /*
   * The coarse face must contain all zeroes before the restriction
   * as the restriction function cannot know which order the fine
   * faces will be restricted in.
   */
  template <typename T>
  INLINE_METHOD void restrictHaloLayer(
    const peano4::datamanagement::FaceMarker& marker,
    T* NOALIAS                                coarseGridFaceValues,
    const T* NOALIAS                          fineGridFaceValues,
    const int                                 side,
    const int                                 numberOfDofsPerAxis,
    const int                                 unknowns
  );

  template <typename T>
  INLINE_METHOD void interpolateCell(
    const peano4::datamanagement::CellMarker& marker,
    const T* NOALIAS                          coarseGridValues,
    T* NOALIAS                                fineGridValues,
    const int                                 numberOfDofsPerAxis,
    const int                                 unknowns
  );

  /*
   * The coarse cell must contain all zeroes before the restriction
   * as the restriction function cannot know which order the fine
   * cells will be restricted in.
   */
  template <typename T>
  INLINE_METHOD void restrictCell(
    const peano4::datamanagement::CellMarker& marker,
    T* NOALIAS                                coarseGridValues,
    const T* NOALIAS                          fineGridValues,
    const int                                 numberOfDofsPerAxis,
    const int                                 unknowns
  );
} // namespace toolbox::blockstructured

#include "AMRRoutines.cpph"
