// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/DGUtils.h"
#include "peano4/Loop.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg::ader {
  template <
    int NumberOfDofsPerAxis,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    int NumberOfMeasuredUnknowns,
    typename StoragePrecision,
    typename ExactSolution>
  INLINE_METHOD void accumulateCellError(
    ExactSolution&&                 evaluateExactSolution,
    const StoragePrecision* NOALIAS cellState,
    const StoragePrecision* NOALIAS quadraturePoints,
    const StoragePrecision* NOALIAS quadratureWeights,
    const int (&measuredUnknowns)[NumberOfMeasuredUnknowns],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    double (&errors)[3]
  );
} // namespace exahype2::dg::ader

#include "exahype2/dg/ader/CellErrorIntegral.cpph"
