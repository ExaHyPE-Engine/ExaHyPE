// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/amr/ChildDataStage.h"
#include "exahype2/amr/LevelJumpEvents.h"
#include "exahype2/dg/LimiterStatus.h"
#include "exahype2/dg/limiting/SubcellLayout.h"
#include "peano4/dataview/CellMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2::dg::limiting {
  template <typename CellLabel>
  INLINE_METHOD void resetCoarseningParentLabel(const peano4::dataview::CellMarker& marker, CellLabel& cellLabel);

  template <typename SolverType, typename Value, typename CellLabel>
  INLINE_METHOD void createCell(
    const peano4::dataview::CellMarker& marker,
    const double* NOALIAS               dgToSubcellMatrix,
    const Value* NOALIAS                coarseSubcellSolution,
    Value* NOALIAS                      fineSubcellSolution,
    const Value* NOALIAS                fineNodalSolution,
    const CellLabel&                    coarseCellLabel,
    CellLabel&                          fineCellLabel
  );

  template <typename SolverType, typename Value, typename CellLabel>
  INLINE_METHOD void destroyCell(
    const peano4::dataview::CellMarker&   marker,
    exahype2::amr::ChildDataStage<Value>& subcellStage,
    const double* NOALIAS                 dgToSubcellMatrix,
    Value* NOALIAS                        coarseSubcellSolution,
    Value* NOALIAS                        fineSubcellSolution,
    const Value* NOALIAS                  fineNodalSolution,
    CellLabel&                            coarseCellLabel,
    const CellLabel&                      fineCellLabel
  );
} // namespace exahype2::dg::limiting

#include "exahype2/dg/limiting/DynamicAMR.cpph"
