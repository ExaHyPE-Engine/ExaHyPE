// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <functional>

#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

void cellErrorIntegral(
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
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
