// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/PatchUtils.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2::fv {
  template <
    int NumberOfValuesPerVolume,
    int NumberOfVolumesPerAxis,
    int HaloSize,
    typename FaceLabel,
    typename StoragePrecision>
  INLINE_METHOD void rollOverFaceHalo(
    const peano4::dataview::FaceMarker& marker,
    FaceLabel&                          faceLabel,
    const StoragePrecision*             newHalo,
    StoragePrecision*                   oldHalo,
    bool                                hasRealisedTopologyChanges
  );

  template <typename CellLabel, typename OldPatch, typename NewPatch>
  INLINE_METHOD void rollOverCellSolution(CellLabel& cellLabel, OldPatch& oldPatch, NewPatch& newPatch);
} // namespace exahype2::fv

#include "exahype2/fv/RollOverUpdates.cpph"
