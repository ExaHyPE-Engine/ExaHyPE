// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/limiting/Admissibility.h"
#include "exahype2/dg/limiting/SubcellLayout.h"
#include "exahype2/fv/PatchUtils.h"
#include "peano4/Loop.h"
#include "peano4/dataview/CellMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2::dg::limiting {
  template <typename SolverType, typename Value>
  INLINE_METHOD void initialiseSubcells(const peano4::dataview::CellMarker& marker, Value* subcellSolution);

  template <typename SolverType, int NumberOfObservables, bool UsesRelaxationTerm, typename Value, typename CellLabel>
  INLINE_METHOD void initialiseSubcells(
    const peano4::dataview::CellMarker& marker,
    CellLabel&                          cellLabel,
    const Value*                        nodalSolution,
    Value*                              subcellSolution,
    const Value*                        dgToLobattoMatrix,
    const Value*                        dgToSubcellMatrix,
    Value                               relaxationParameter,
    Value                               differencesScaling
  );
} // namespace exahype2::dg::limiting

#include "exahype2/dg/limiting/InitialiseSubcells.cpph"
