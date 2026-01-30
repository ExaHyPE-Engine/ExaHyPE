// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Abstract{{CLASSNAME}}.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}
  class {{CLASSNAME}};
{% for item in NAMESPACE -%}
}
{%- endfor %}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}}: public {{NAMESPACE | join("::")}}::Abstract{{CLASSNAME}} {
private:
  static tarch::logging::Log _log;

public:
  TARGET_DECLARE
  {{SOLVER_USER_DECLARATIONS}}
  TARGET_DECLARE_END
};
