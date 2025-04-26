// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

template <typename cCompType>
void riemannSolver(
  {{FULL_QUALIFIED_SOLVER_NAME}}&               solver,
  cCompType* noalias                       FL,
  cCompType* noalias                       FR,
  const cCompType* noalias const           QL,
  const cCompType* noalias const           QR,
  const double                                  timeStamp,
  const double                                  timeStepSize,
  const tarch::la::Vector<Dimensions, double>&  faceCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize,
  const int                                     direction
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
