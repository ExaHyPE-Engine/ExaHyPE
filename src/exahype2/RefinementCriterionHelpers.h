// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementControl.h"
#include "toolbox/blockstructured/MultiresolutionDetail.h"

namespace exahype2 {
  INLINE_METHOD RefinementCommand
  refinementCommandFromDetail(const double detail, const double eps = 1.0e-2, const double coarsenSafetyFactor = 9.0);

  template <typename Value, typename... Indicators>
  INLINE_METHOD RefinementCommand refineOnDetail(
    const Value* const  Q,
    const int           numberOfDofsPerAxis,
    const int           numberOfUnknownsPlusAuxiliaryVariables,
    const double        eps,
    const double        coarsenSafetyFactor,
    const double* const nodesInUnitInterval,
    const Indicators&... indicators
  );
} // namespace exahype2

#include "RefinementCriterionHelpers.cpph"
