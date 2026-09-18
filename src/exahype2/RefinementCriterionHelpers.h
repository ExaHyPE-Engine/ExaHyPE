// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/RefinementControl.h"
#include "exahype2/amr/MultiresolutionDetail.h"
#include "peano4/Constants.h"
#include "peano4/sfc/ActiveCurve.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2 {
  /**
   * Hysteresis ratio between the refine and the erase threshold of the detail
   * criterion: The detail of an affine fit shrinks by this factor from one
   * level to the next.
   */
  constexpr double DefaultCoarsenSafetyFactor{peano4::power(peano4::sfc::ActiveCurve::SubdivisionFactor, 2)};

  INLINE_METHOD RefinementCommand refinementCommandFromDetail(
    double detail,
    double eps                 = 1.0e-2,
    double coarsenSafetyFactor = exahype2::DefaultCoarsenSafetyFactor
  );

  template <typename Value, typename... Indicators>
  INLINE_METHOD RefinementCommand refineOnDetail(
    const Value*  Q,
    int           numberOfDofsPerAxis,
    int           numberOfUnknownsPlusAuxiliaryVariables,
    double        eps,
    double        coarsenSafetyFactor,
    const double* nodesInUnitInterval,
    const Indicators&... indicators
  );
} // namespace exahype2

#include "exahype2/RefinementCriterionHelpers.cpph"
