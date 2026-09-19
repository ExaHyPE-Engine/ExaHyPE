// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/ProjectPatch.h"
#include "exahype2/dg/LimiterStatus.h"
#include "exahype2/dg/SubcellProjection.h"
#include "exahype2/dg/limiting/FaceStatus.h"
#include "exahype2/dg/limiting/SubcellLayout.h"
#include "exahype2/fv/ApplyBoundaryConditionsOnFace.h"
#include "peano4/dataview/CellMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2::dg::limiting {
  template <
    typename SolverType,
    int HaloWidth,
    typename Value,
    typename CellLabel,
    typename FaceLabels,
    typename SubcellFaces,
    typename AcceptedPatch,
    typename SubcellPatch>
  INLINE_METHOD void spreadDomain(
    const peano4::dataview::CellMarker& marker,
    SolverType&                         solver,
    CellLabel&                          cellLabel,
    const FaceLabels&                   faceLabels,
    const SubcellFaces&                 subcellFaces,
    AcceptedPatch&                      acceptedSolution,
    SubcellPatch&                       subcellPatch,
    const Value*                        dgToFvMatrix
  );
} // namespace exahype2::dg::limiting

#include "exahype2/dg/limiting/SpreadDomain.cpph"
