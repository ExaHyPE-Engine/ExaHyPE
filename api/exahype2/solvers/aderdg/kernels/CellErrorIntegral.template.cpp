// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellErrorIntegral.h"

#include "Quadrature.h"

#include "exahype2/dg/DGUtils.h"

#include "peano4/Loop.h"

void {{FULL_QUALIFIED_NAMESPACE}}::cellErrorIntegral(
  std::function<void(
    const tarch::la::Vector<DIMENSIONS, double>&  x,
    const double                                  t,
    const double                                  dt,
    const {{SOLUTION_STORAGE_PRECISION}}*         Q,
    double*                                       solution
  )>                                              exactSolutionFunctor,
  const tarch::la::Vector<DIMENSIONS, double>&    cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>&    cellSize,
  double                                          timeStamp,
  double                                          timeStepSize,
  {{SOLUTION_STORAGE_PRECISION}}* NOALIAS         Q,
  double                                          errors[3]
) {
  errors[0] = 0.0;
  errors[1] = 0.0;
  errors[2] = 0.0;

{% if DIMENSIONS==2 %}
  const double cellVolume = cellSize[0] * cellSize[1];
{% else %}
  const double cellVolume = cellSize[0] * cellSize[1] * cellSize[2];
{% endif %}

  double solution[{{NUMBER_OF_UNKNOWNS}}];
  int linearisedIndex = 0;

  // Iterate over all quadrature points in the cell
  peano4::dForEach({{NUMBER_OF_DOF}}, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
{% if DIMENSIONS==2 %}
    const double weight = cellVolume * Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::weights1[index[0]] * Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::weights1[index[1]];
{% else %}
    const double weight = cellVolume * Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::weights1[index[0]] * Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::weights1[index[1]] * Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::weights1[index[2]];
{% endif %}

    // Compute the exact solution
    exactSolutionFunctor(
      ::exahype2::dg::getQuadraturePoint(
        cellCentre,
        cellSize,
        index,
        {{NUMBER_OF_DOF}},
        Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes
      ),
      timeStamp,
      timeStepSize,
      &Q[linearisedIndex],
      solution
    );

    // Increment the respective errors
    constexpr int MeasuredUnknowns[] = {{MEASURED_UNKNOWNS}};
    for (const int unknown : MeasuredUnknowns) {
      const double error = std::abs(Q[linearisedIndex + unknown] - solution[unknown]);
      errors[0] += weight * error; // L1 norm
      errors[1] += weight * error * error; // L2 norm
      errors[2] = std::max(errors[2], error); // Linf norm
    }

    linearisedIndex += {{NUMBER_OF_DATA}};
  });
}
