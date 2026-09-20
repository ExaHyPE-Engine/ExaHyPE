// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/ProjectPatch.h"
#include "exahype2/amr/AMRRoutines.h"
#include "exahype2/amr/ChildDataStage.h"
#include "exahype2/amr/LevelJumpEvents.h"
#include "exahype2/fv/HaloTransfer.h"
#include "exahype2/fv/PatchUtils.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
/**
 * A face patch holds Overlap halo layers of NumberOfDofsPerAxis^(DIMENSIONS-1)
 * volumes on each of its two sides, and a cell patch holds
 * NumberOfDofsPerAxis^DIMENSIONS volumes; every volume carries NumberOfUnknowns
 * values.
 */
namespace exahype2::fv {
  template <int NumberOfDofsPerAxis, int NumberOfUnknowns>
  constexpr int ValuesPerFaceLayer = NumberOfUnknowns * peano4::power(NumberOfDofsPerAxis, DIMENSIONS - 1);

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns>
  constexpr int ValuesPerCellPatch = NumberOfUnknowns * peano4::power(NumberOfDofsPerAxis, DIMENSIONS);

  template <int NumberOfDofsPerAxis, int Overlap, int NumberOfUnknowns, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void clearFacePatch(T* NOALIAS faceValues);

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, typename T>
  INLINE_METHOD GPU_CALLABLE_METHOD void clearCellPatch(T* NOALIAS cellValues);
} // namespace exahype2::fv
TARGET_DECLARE_END

namespace exahype2::fv {
  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, typename T, typename FaceLabel>
  INLINE_METHOD void interpolateHangingFace(
    const peano4::dataview::FaceMarker& marker,
    const T* NOALIAS                    coarseFaceValues,
    T* NOALIAS                          fineFaceValues,
    const FaceLabel&                    coarseFaceLabel,
    FaceLabel&                          fineFaceLabel
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, typename T, typename FaceLabel>
  INLINE_METHOD void restrictHangingFace(
    const peano4::dataview::FaceMarker& marker,
    exahype2::amr::ChildDataStage<T>&   stage,
    const T* NOALIAS                    coarseFaceValues,
    T* NOALIAS                          fineFaceValues,
    FaceLabel&                          coarseFaceLabel,
    const FaceLabel&                    fineFaceLabel
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, typename T>
  INLINE_METHOD void restrictFaceHalves(
    const peano4::dataview::FaceMarker& marker,
    exahype2::amr::ChildDataStage<T>&   stage,
    T* NOALIAS                          fineFaceValues
  );

  template <int NumberOfDofsPerAxis, int Overlap, int NumberOfUnknowns, typename T, typename FaceLabel>
  INLINE_METHOD void clearCoarseningFace(
    const peano4::dataview::FaceMarker& marker,
    FaceLabel&                          fineFaceLabel,
    T* NOALIAS                          fineFaceValuesNew,
    T* NOALIAS                          fineFaceValuesOld
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, typename T>
  INLINE_METHOD void clearCoarseningCell(
    const peano4::dataview::CellMarker& marker,
    T* NOALIAS                          fineCellValuesNew,
    T* NOALIAS                          fineCellValuesOld
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, typename T, typename FaceLabel>
  INLINE_METHOD void restrictCoarseningFaceHalves(
    const peano4::dataview::FaceMarker& marker,
    exahype2::amr::ChildDataStage<T>&   stageNew,
    exahype2::amr::ChildDataStage<T>&   stageOld,
    T* NOALIAS                          fineFaceValuesNew,
    T* NOALIAS                          fineFaceValuesOld,
    FaceLabel&                          fineFaceLabel
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, typename T, typename CellLabel, typename FaceLabel>
  INLINE_METHOD void createFace(
    const peano4::dataview::FaceMarker& marker,
    const T* NOALIAS                    coarseCellValuesOld,
    const T* NOALIAS                    coarseCellValuesNew,
    const T* NOALIAS                    coarseFaceValuesOld,
    const T* NOALIAS                    coarseFaceValuesNew,
    T* NOALIAS                          fineFaceValuesOld,
    T* NOALIAS                          fineFaceValuesNew,
    const CellLabel&                    coarseCellLabel,
    FaceLabel&                          fineFaceLabel
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, typename T, typename FaceLabel>
  INLINE_METHOD void destroyFace(
    const peano4::dataview::FaceMarker&          marker,
    exahype2::amr::ChildDataStage<T>&            stageNew,
    exahype2::amr::ChildDataStage<T>&            stageOld,
    const T* NOALIAS                             coarseFaceValuesNew,
    const T* NOALIAS                             coarseFaceValuesOld,
    const T* NOALIAS                             fineFaceValuesNew,
    const T* NOALIAS                             fineFaceValuesOld,
    FaceLabel&                                   coarseFaceLabel,
    const FaceLabel&                             fineFaceLabel,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainSize
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, typename T, typename CellLabel>
  INLINE_METHOD void createCell(
    const peano4::dataview::CellMarker& marker,
    const T* NOALIAS                    coarseCellValuesNew,
    T* NOALIAS                          fineCellValuesNew,
    const T* NOALIAS                    coarseCellValuesOld,
    T* NOALIAS                          fineCellValuesOld,
    const CellLabel&                    coarseCellLabel,
    CellLabel&                          fineCellLabel
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, typename T, typename CellLabel>
  INLINE_METHOD void destroyCell(
    const peano4::dataview::CellMarker& marker,
    exahype2::amr::ChildDataStage<T>&   stageNew,
    exahype2::amr::ChildDataStage<T>&   stageOld,
    T* NOALIAS                          coarseCellValuesNew,
    const T* NOALIAS                    fineCellValuesNew,
    T* NOALIAS                          coarseCellValuesOld,
    const T* NOALIAS                    fineCellValuesOld,
    CellLabel&                          coarseCellLabel,
    const CellLabel&                    fineCellLabel
  );

  template <
    int NumberOfDofsPerAxis,
    int Overlap,
    int NumberOfUnknowns,
    typename T,
    typename CellLabel,
    typename FaceDataEnumerator,
    typename FaceLabelEnumerator>
  INLINE_METHOD void projectPatchAfterTransfer(
    const peano4::dataview::CellMarker& marker,
    const T* NOALIAS                    cellValuesNew,
    const FaceDataEnumerator&           fineFaceValuesNew,
    const FaceLabelEnumerator&          fineFaceLabels,
    CellLabel&                          fineCellLabel
  );
} // namespace exahype2::fv

#include "exahype2/fv/DynamicAMR.cpph"
