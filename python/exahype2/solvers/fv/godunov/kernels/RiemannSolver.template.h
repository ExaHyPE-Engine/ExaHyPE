// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "peano4/Loop.h"     // for TWO_TIMES_D
#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

double riemannSolver(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                       solver,
  const {{STORAGE_PRECISION}}* NOALIAS const            QIn,
  {{STORAGE_PRECISION}}* NOALIAS const                  QOut,
  const {{STORAGE_PRECISION}}* NOALIAS const            faceQIn[TWO_TIMES_D],
  {{STORAGE_PRECISION}}* NOALIAS const                  faceQOut[TWO_TIMES_D],
  const double                                          timeStamp,
  const double                                          timeStepSize,
  const tarch::la::Vector<DIMENSIONS, double>&          patchCentre,
  const tarch::la::Vector<DIMENSIONS, double>&          patchSize
);

{% for item in NAMESPACE -%}
}
{%- endfor %}