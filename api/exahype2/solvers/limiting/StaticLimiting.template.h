// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Abstract{{CLASSNAME}}.h"
#include "tarch/instrumentation/Log.h"

{% for item in NAMESPACE -%}
  namespace {{ item }} {
{%- endfor %}
  class {{CLASSNAME}};
{% for item in NAMESPACE -%}
  }
{%- endfor %}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}}: public Abstract{{CLASSNAME}} {
  public:
  {% if ADMISSIBILITY_IMPLEMENTATION=="<user-defined>" %}
    bool isPhysicallyAdmissible(
      const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const Q,
      const tarch::la::Vector<DIMENSIONS, double>&      x,
      const tarch::la::Vector<DIMENSIONS, double>&      h,
      const double                                      timeStamp
    ) override;
  {% endif %}
};
