// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PointSources.h"

#include "Quadrature.h"
#include "DGMatrices.h"

std::vector<int> {{FULL_QUALIFIED_NAMESPACE}}::pointSources(
  {{FULL_QUALIFIED_SOLVER_NAME}}&              solver,
  const tarch::la::Vector<Dimensions, double>& cellCentre,
  const tarch::la::Vector<Dimensions, double>& cellSize
) {
  std::vector<int> pointSources; // Empty vector

  for (int m = 0; m < {{NUMBER_OF_POINT_SOURCES}}; m++) {
    const double* const pointSourceLocation = solver.pointSourceLocation[m];
    if (cellCentre[0] - 0.5 * cellSize[0] <= pointSourceLocation[0] && pointSourceLocation[0] <= cellCentre[0] + 0.5 * cellSize[0]
     && cellCentre[1] - 0.5 * cellSize[1] <= pointSourceLocation[1] && pointSourceLocation[1] <= cellCentre[1] + 0.5 * cellSize[1]
{% if DIMENSIONS == 3 %}
     && cellCentre[2] - 0.5 * cellSize[2] <= pointSourceLocation[2] && pointSourceLocation[2] <= cellCentre[2] + 0.5 * cellSize[2]
{% endif %}
      ) {
      pointSources.push_back(m);
    }
  }

  return pointSources;
}
