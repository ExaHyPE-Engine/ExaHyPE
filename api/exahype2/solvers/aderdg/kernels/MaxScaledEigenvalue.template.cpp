// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MaxScaledEigenvalue.h"

#include "Quadrature.h"

#include "exahype2/dg/DGUtils.h"
#include "peano4/Loop.h"

{{FULL_QUALIFIED_NAMESPACE}}::MaxEigenvalueStatistics {{FULL_QUALIFIED_NAMESPACE}}::reduceMaxEigenvalues(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&  solver,
  const {{SOLUTION_STORAGE_PRECISION}}* const       luh,
  const tarch::la::Vector<DIMENSIONS, double>&      cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>&      cellSize,
  const double                                      timeStamp,
  const double                                      timeStepSize
) {
  // Inverse of the smallest cell size
  const double invDx = 1.0 / tarch::la::min(cellSize);

  MaxEigenvalueStatistics result{0.0, 0.0};
  for (int i = 0; i < {{NUMBER_OF_DOF}}; i++) {
    for (int j = 0; j < {{NUMBER_OF_DOF}}; j++) {
{% if DIMENSIONS == 3 %}
      for (int k = 0; k < {{NUMBER_OF_DOF}}; k++) {
        const int                                linearisedIndex = i + {{NUMBER_OF_DOF}} * j + {{NUMBER_OF_DOF}} * {{NUMBER_OF_DOF}} * k;
        const tarch::la::Vector<DIMENSIONS, int> index           = {i, j, k};
{% else %}
        const int                                linearisedIndex = i + {{NUMBER_OF_DOF}} * j;
        const tarch::la::Vector<DIMENSIONS, int> index           = {i, j};
{% endif %}

        double scaledEigenvalue = 0.0;
        for (int d = 0; d < DIMENSIONS; d++) {
{% if STATELESS_MAX_EIGENVALUE %}
          const double physicalEigenvalue = {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(
{% else %}
          const double physicalEigenvalue = solver.maxEigenvalue(
{% endif %}
            &luh[linearisedIndex * {{NUMBER_OF_DATA}}],
            ::exahype2::dg::getQuadraturePoint(
              cellCentre,
              cellSize,
              index,
              {{NUMBER_OF_DOF}} - 1,
              Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes
            ), // faceCentre
            cellSize,
            timeStamp,
            timeStepSize,
            d
          );

          result.maxPhysicalEigenvalue = std::fmax(result.maxPhysicalEigenvalue, physicalEigenvalue);
          scaledEigenvalue += physicalEigenvalue * invDx;
        }

        result.maxScaledEigenvalue = std::fmax(result.maxScaledEigenvalue, scaledEigenvalue);
{% if DIMENSIONS == 3 %}
      }
{% endif %}
    }
  }

  return result;
}
