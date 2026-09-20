// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/ApplyBoundaryConditionsOnFace.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2::fv {
  template <
    int HaloSize,
    typename StoragePrecision,
    typename SolverType,
    typename FaceLabelEnumerator,
    typename FaceDataEnumerator>
  INLINE_METHOD void applyBoundaryConditions(
    const peano4::dataview::CellMarker& marker,
    SolverType&                         solver,
    const FaceLabelEnumerator&          faceLabels,
    const FaceDataEnumerator&           faceData
  );
} // namespace exahype2::fv

#include "exahype2/fv/ApplyBoundaryConditions.cpph"
