// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/amr/ChildDataStage.h"
#include "peano4/Loop.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/la/ExactAccumulator.h"

/**
 * Interpolation and restriction for block-structured patch data.
 *
 * A cell holds numberOfDofsPerAxis^DIMENSIONS volumes of numberOfUnknownsPlusAuxiliaryVariables elements each; a face
 * holds 2 * numberOfDofsPerAxis^(DIMENSIONS-1) * numberOfUnknownsPlusAuxiliaryVariables values (both sides), in
 * lexicographic order.
 *
 * The routines assume a halo of size 1; a larger halo would need extrapolation because a
 * fine face is smaller than the coarse face it restricts onto.
 */
namespace exahype2::amr {
  template <typename T>
  INLINE_METHOD void interpolateHaloLayer(
    const peano4::dataview::FaceMarker& marker,
    const T* NOALIAS                    coarseGridFaceValues,
    T* NOALIAS                          fineGridFaceValues,
    const int                           side,
    const int                           numberOfDofsPerAxis,
    const int                           numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void interpolateHaloLayerWithTangentialSlope(
    const peano4::dataview::FaceMarker& marker,
    const T* NOALIAS                    coarseGridFaceValues,
    T* NOALIAS                          fineGridFaceValues,
    const int                           side,
    const int                           numberOfDofsPerAxis,
    const int                           numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void interpolateHaloLayerFromCoarseCell(
    const peano4::dataview::FaceMarker& marker,
    const T* NOALIAS                    coarseGridCellValues,
    T* NOALIAS                          fineGridFaceValues,
    const int                           side,
    const int                           numberOfDofsPerAxis,
    const int                           numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void restrictHaloLayer(
    const peano4::dataview::FaceMarker& marker,
    T* NOALIAS                          coarseGridFaceValues,
    const T* NOALIAS                    fineGridFaceValues,
    const int                           side,
    const int                           numberOfDofsPerAxis,
    const int                           numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void stageHaloLayerForGatheredRestriction(
    const peano4::dataview::FaceMarker& marker,
    exahype2::amr::ChildDataStage<T>&   stage,
    const void*                         coarseFaceKey,
    const T* NOALIAS                    fineGridFaceValues,
    const int                           side,
    const int                           numberOfDofsPerAxis,
    const int                           numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void restrictHaloLayerFromStagedSubfaces(
    exahype2::amr::ChildDataStage<T>& stage,
    const void*                       coarseFaceKey,
    T* NOALIAS                        coarseGridFaceValues,
    const int                         normal,
    const int                         numberOfDofsPerAxis,
    const int                         numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void restrictHaloLayerFromStagedSubfaces(
    exahype2::amr::ChildDataStage<T>& stage,
    const void*                       coarseFaceKey,
    T* NOALIAS                        coarseGridFaceValues,
    const double* NOALIAS             restrictionMatrix1D,
    const double* NOALIAS             quadratureWeights1D,
    const int                         normal,
    const int                         numberOfDofsPerAxis,
    const int                         numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void interpolateCell(
    const peano4::dataview::CellMarker& marker,
    const T* NOALIAS                    coarseGridValues,
    T* NOALIAS                          fineGridValues,
    const int                           numberOfDofsPerAxis,
    const int                           numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void interpolateCell(
    const peano4::dataview::CellMarker& marker,
    const T* NOALIAS                    coarseGridValues,
    T* NOALIAS                          fineGridValues,
    const double* NOALIAS               interpolationMatrix1D,
    const int                           numberOfDofsPerAxis,
    const int                           numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void restrictCell(
    const peano4::dataview::CellMarker& marker,
    T* NOALIAS                          coarseGridValues,
    const T* NOALIAS                    fineGridValues,
    const int                           numberOfDofsPerAxis,
    const int                           numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void restrictCell(
    const peano4::dataview::CellMarker& marker,
    T* NOALIAS                          coarseGridValues,
    const T* NOALIAS                    fineGridValues,
    const double* NOALIAS               restrictionMatrix1D,
    const double* NOALIAS               quadratureWeights1D,
    const int                           numberOfDofsPerAxis,
    const int                           numberOfUnknownsPlusAuxiliaryVariables
  );

  INLINE_METHOD double orderInvariantProduct(double firstFactor, double secondFactor, double thirdFactor);

  template <typename T>
  INLINE_METHOD void applyTensorProductOrderInvariant(
    T* NOALIAS                   targetValues,
    const T* NOALIAS             sourceValues,
    const double* const* NOALIAS matrixPerAxis,
    const int                    numberOfSourceDofsPerAxis,
    const int                    numberOfTargetDofsPerAxis,
    const int                    numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void restrictCellFromChildren(
    T* NOALIAS                   coarseGridValues,
    const T* const* NOALIAS      childGridValues,
    const double* const* NOALIAS restrictionMatrixPerInterval,
    const int                    numberOfDofsPerAxis,
    const int                    numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void restrictCellFromChildren(
    T* NOALIAS              coarseGridValues,
    const T* const* NOALIAS childGridValues,
    const int               numberOfDofsPerAxis,
    const int               numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void interpolateFaceFromCoarse(
    T* NOALIAS                   fineFaceValues,
    const T* NOALIAS             coarseFaceValues,
    const double* const* NOALIAS prolongationMatrixPerAxis,
    const int                    numberOfDofsPerAxis,
    const int                    numberOfUnknownsPlusAuxiliaryVariables
  );

  template <typename T>
  INLINE_METHOD void restrictFaceFromChildren(
    T* NOALIAS                   coarseFaceValues,
    const T* const* NOALIAS      childFaceValues,
    const double* const* NOALIAS restrictionMatrixPerInterval,
    const int                    numberOfDofsPerAxis,
    const int                    numberOfUnknownsPlusAuxiliaryVariables
  );
} // namespace exahype2::amr

#include "exahype2/amr/AMRRoutines.cpph"
