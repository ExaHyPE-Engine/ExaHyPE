// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

template <typename solutionType, typename cCompType>
void faceIntegral(
  solutionType*           lduh,
  const cCompType* const  lFhbnd,
  const int               direction,
  const int               orientation,
  const double            inverseDxDirection,
  const double            timeStepSize
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
