// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "tarch/la/Vector.h"

#include "exahype2/CellData.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

{% if STATELESS_PDE_TERMS==False -%}

double updateSolutionWithRiemann(
  {{FULL_QUALIFIED_SOLVER_NAME}}&               solver,
  const double* __restrict__ const              QIn,
  double* __restrict__ const                    QOut,
  const double                                  timeStamp,
  const double                                  timeStepSize,
  const tarch::la::Vector<Dimensions, double>&  patchCentre,
  const tarch::la::Vector<Dimensions, double>&  patchSize
);

{% else %}

double updateSolutionWithRiemann(
  const double* __restrict__ const              QIn,
  double* __restrict__ const                    QOut,
  const double                                  timeStamp,
  const double                                  timeStepSize,
  const tarch::la::Vector<Dimensions, double>&  patchCentre,
  const tarch::la::Vector<Dimensions, double>&  patchSize
);

double updateSolutionWithRiemann(
  ::exahype2::CellData<double, double>&         patchData,
  const int                                     targetDevice
);

{%- endif %}

{% for item in NAMESPACE -%}
}
{%- endfor %}
