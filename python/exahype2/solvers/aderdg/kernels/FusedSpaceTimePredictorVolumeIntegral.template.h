// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

template <typename cStoreType, typename pCompType, typename pStoreType>
int fusedSpaceTimePredictorVolumeIntegral{{NAME_SUFFIX}}(
  {{FULL_QUALIFIED_SOLVER_NAME}}&               solver,
  pStoreType* noalias                      lQhbnd[2*Dimensions],
  pStoreType* noalias                      lFhbnd[2*Dimensions],
  cStoreType* const noalias                luh,
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize,
  const double                                  timeStamp,
  const double                                  timeStepSize
{% if USE_POINT_SOURCE %}
  ,const std::vector<int>&                      pointSources
{% endif %}
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
