// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HandleBoundary.h"

{% from "ApplyBoundaryCondition.macro.h" import ApplyBoundaryCondition with context %}
namespace {
  const auto serialisePatchIndex = [](const tarch::la::Vector<DIMENSIONS, int>& overlapCell, int normal) {
    int base   = 1;
    int result = 0;
    for (int d = 0; d < DIMENSIONS; d++) {
      result += overlapCell(d) * base;
      if (d==normal) {
        base *= {{HALO_SIZE}} * 2;
      } else {
        base *= {{NUMBER_OF_VOLUMES_PER_AXIS}};
      }
    }
    return result;
  };
}

void {{FULL_QUALIFIED_NAMESPACE}}::handleBoundary(
  {{FULL_QUALIFIED_SOLVER_NAME}}&             solver,
  const tarch::la::Vector<DIMENSIONS,double>  patchCentre,
  const tarch::la::Vector<DIMENSIONS,double>  patchSize,
  double                                      t,
  double                                      dt,
  int                                         faceNumber,
  {{STORAGE_PRECISION}}*                      Q
) {
  {{ ApplyBoundaryCondition() }}
}
