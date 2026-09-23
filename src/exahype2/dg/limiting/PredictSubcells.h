// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/LimiterStatus.h"
#include "exahype2/dg/limiting/Admissibility.h"
#include "exahype2/dg/limiting/SubcellLayout.h"
#include "exahype2/dg/limiting/VertexExtrema.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2::dg::limiting {
  template <typename CellLabel>
  INLINE_METHOD void predictSubcells(CellLabel& cellLabel);

  template <typename SolverType, int NumberOfObservables, typename Value, typename CellLabel, typename VertexLabels>
  INLINE_METHOD void predictSubcells(
    CellLabel&          cellLabel,
    const Value*        acceptedSolution,
    const Value*        dgToLobattoMatrix,
    const Value*        dgToSubcellMatrix,
    const VertexLabels& vertexLabels
  );
} // namespace exahype2::dg::limiting

#include "exahype2/dg/limiting/PredictSubcells.cpph"
