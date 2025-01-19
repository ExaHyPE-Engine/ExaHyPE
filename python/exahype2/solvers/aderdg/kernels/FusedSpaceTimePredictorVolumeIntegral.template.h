// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

{% if IS_LINEAR %}

template <typename cStoreType, typename pCompType, typename pStoreType>
int fusedSpaceTimePredictorVolumeIntegral{{NAME_SUFFIX}}(
  {{FULL_QUALIFIED_SOLVER_NAME}}&               solver,
  pStoreType* __restrict__                      lQhbnd[2*Dimensions],
  pStoreType* __restrict__                      lFhbnd[2*Dimensions],
  cStoreType* const __restrict__                luh,
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize,
  const double                                  timeStamp,
  const double                                  timeStepSize,
  const std::vector<int>&                       pointSources // Empty vector if no point sources are set
);

{% else %}{# IS_LINEAR #}

template <typename cStoreType, typename pCompType, typename pStoreType>
int fusedSpaceTimePredictorVolumeIntegral(
  {{FULL_QUALIFIED_SOLVER_NAME}}&               solver,
  pStoreType* __restrict__                      lQhbnd[2*Dimensions],
  pStoreType* __restrict__                      lFhbnd[2*Dimensions],
  cStoreType* const __restrict__                luh,
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize,
  const double                                  timeStamp,
  const double                                  timeStepSize
);

{% endif %}{# IS_LINEAR #}

{% for item in NAMESPACE -%}
}
{%- endfor %}
