// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Quadrature.h"

#include "tarch/accelerator/accelerator.h"

void {{FULL_QUALIFIED_NAMESPACE}}::Quadrature::initQuadratureNodesAndWeights() {
{%- for vector in QUADRATURE %}
  {{vector.name}} = tarch::allocateMemory<double>({{vector.entries|length}}, tarch::MemoryLocation::Heap);
{%- endfor %}
{% for vector in QUADRATURE %}
{%- for value in vector.entries %}
  {{vector.name}}[{{loop.index0}}] = {{value}};
{%- endfor %}
{% endfor -%}
}

void {{FULL_QUALIFIED_NAMESPACE}}::Quadrature::freeQuadratureNodesAndWeights() {
{%- for vector in QUADRATURE %}
  tarch::freeMemory({{vector.name}}, tarch::MemoryLocation::Heap);
{%- endfor %}
}
