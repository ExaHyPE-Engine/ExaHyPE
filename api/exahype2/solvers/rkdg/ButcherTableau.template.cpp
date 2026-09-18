// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ButcherTableau.h"

#include "tarch/accelerator/accelerator.h"

void {{FULL_QUALIFIED_NAMESPACE}}::ButcherTableau::initButcherTableau() {
{%- for vector in BUTCHER_TABLEAU %}
  {{vector.name}} = tarch::allocateMemory<double>({{vector.entries|length}}, tarch::MemoryLocation::Heap);
{%- endfor %}
{% for vector in BUTCHER_TABLEAU %}
{%- for value in vector.entries %}
  {{vector.name}}[{{loop.index0}}] = {{value}};
{%- endfor %}
{% endfor -%}
}

void {{FULL_QUALIFIED_NAMESPACE}}::ButcherTableau::freeButcherTableau() {
{%- for vector in BUTCHER_TABLEAU %}
  tarch::freeMemory({{vector.name}}, tarch::MemoryLocation::Heap);
{%- endfor %}
}
