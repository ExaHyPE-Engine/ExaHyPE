// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Loop.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceMarker.h"

/**
 * Interpolation and restriction for block-structured patch data
 *
 * A cell holds numberOfDofsPerAxis^DIMENSIONS volumes of numberOfUnknownsPlusAuxiliaryVariables elements each; a face
 * holds 2 * numberOfDofsPerAxis^(DIMENSIONS-1) * numberOfUnknownsPlusAuxiliaryVariables values (both sides), in
 * lexicographic order.
 *
 * Peano subdivides every cell into 3^DIMENSIONS children. Cell interpolation therefore
 * copies each coarse volume onto its 3^DIMENSIONS fine volumes and cell restriction
 * averages them back, while a face carries one dimension less and works on 3^(DIMENSIONS-1)
 * fine values per coarse one.
 *
 * The routines assume a halo of size 1; a larger halo would need extrapolation because a
 * fine face is smaller than the coarse face it restricts onto.
 */
namespace toolbox::blockstructured {
  template <typename T>
  INLINE_METHOD void interpolateHaloLayer(
    const peano4::datamanagement::FaceMarker& marker,
    const T* NOALIAS                          coarseGridFaceValues,
    T* NOALIAS                                fineGridFaceValues,
    const int                                 side,
    const int                                 numberOfDofsPerAxis,
    const int                                 numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void interpolateHaloLayerWithTangentialSlope(
    const peano4::datamanagement::FaceMarker& marker,
    const T* NOALIAS                          coarseGridFaceValues,
    T* NOALIAS                                fineGridFaceValues,
    const int                                 side,
    const int                                 numberOfDofsPerAxis,
    const int                                 numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void interpolateHaloLayerFromCoarseCell(
    const peano4::datamanagement::FaceMarker& marker,
    const T* NOALIAS                          coarseGridCellValues,
    T* NOALIAS                                fineGridFaceValues,
    const int                                 side,
    const int                                 numberOfDofsPerAxis,
    const int                                 numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void restrictHaloLayer(
    const peano4::datamanagement::FaceMarker& marker,
    T* NOALIAS                                coarseGridFaceValues,
    const T* NOALIAS                          fineGridFaceValues,
    const int                                 side,
    const int                                 numberOfDofsPerAxis,
    const int                                 numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void interpolateCell(
    const peano4::datamanagement::CellMarker& marker,
    const T* NOALIAS                          coarseGridValues,
    T* NOALIAS                                fineGridValues,
    const int                                 numberOfDofsPerAxis,
    const int                                 numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void interpolateCell(
    const peano4::datamanagement::CellMarker& marker,
    const T* NOALIAS                          coarseGridValues,
    T* NOALIAS                                fineGridValues,
    const double* NOALIAS                     interpolationMatrix1d,
    const int                                 numberOfDofsPerAxis,
    const int                                 numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void restrictCell(
    const peano4::datamanagement::CellMarker& marker,
    T* NOALIAS                                coarseGridValues,
    const T* NOALIAS                          fineGridValues,
    const int                                 numberOfDofsPerAxis,
    const int                                 numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void restrictCell(
    const peano4::datamanagement::CellMarker& marker,
    T* NOALIAS                                coarseGridValues,
    const T* NOALIAS                          fineGridValues,
    const double* NOALIAS                     restrictionMatrix1d,
    const double* NOALIAS                     quadratureWeights1d,
    const int                                 numberOfDofsPerAxis,
    const int                                 numberOfUnknownsPlusAuxiliaryVariables
  );
} // namespace toolbox::blockstructured

#include "AMRRoutines.cpph"
