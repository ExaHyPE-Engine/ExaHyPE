// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MaxScaledEigenvalue.h"

#include "Quadrature.h"

#include "exahype2/dg/DGUtils.h"
#include "peano4/Loop.h"

::exahype2::dg::MaxEigenvalueStatistics
{{FULL_QUALIFIED_NAMESPACE}}::reduceMaxEigenvalues(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}& solver,
  const {{SOLUTION_STORAGE_PRECISION}}* const Q,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double timeStamp,
  const double timeStepSize
) {
  auto eigenvalueStatistics = ::exahype2::dg::MaxEigenvalueStatistics{};
  constexpr int NumberOfDofsPerAxis = {{NUMBER_OF_DOF}};
  peano4::dForEach(
    NumberOfDofsPerAxis,
    [&](const tarch::la::Vector<DIMENSIONS, int>& quadraturePointIndex) {
      const int linearisedQuadraturePoint =
        peano4::dLinearised(quadraturePointIndex, NumberOfDofsPerAxis);
      const auto x = ::exahype2::dg::getQuadraturePoint(
        cellCentre,
        cellSize,
        quadraturePointIndex,
        {{FULL_QUALIFIED_NAMESPACE}}::Quadrature<
          {{SOLUTION_STORAGE_PRECISION}}
        >::nodes
      );
      eigenvalueStatistics.accumulate(
        cellSize,
        [&](const int dimension) {
{% if STATELESS_MAX_EIGENVALUE %}
          return {{FULL_QUALIFIED_SOLVER_NAME}}::maxEigenvalue(
{% else %}
          return solver.maxEigenvalue(
{% endif %}
            &Q[linearisedQuadraturePoint * {{NUMBER_OF_DATA}}],
            x,
            cellSize,
            timeStamp,
            timeStepSize,
            dimension
          );
        }
      );
    }
  );

  return eigenvalueStatistics;
}
