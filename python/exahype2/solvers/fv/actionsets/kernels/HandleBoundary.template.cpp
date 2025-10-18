// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HandleBoundary.h"

{% import 'Loop.macro.h' as patch with context %}{# get template macros #}

void {{FULL_QUALIFIED_NAMESPACE}}::handleBoundary(
    {{FULL_QUALIFIED_SOLVER_NAME}}&             solver,
    const tarch::la::Vector<DIMENSIONS,double>  patchCentre,
    const tarch::la::Vector<DIMENSIONS,double>  patchSize,
    double                                      t,
    double                                      dt,
    int                                         faceNumber,
    {{STORAGE_PRECISION}}*                      Q
  ) {
  {{ patch.inner_loop_macro() }}
}
