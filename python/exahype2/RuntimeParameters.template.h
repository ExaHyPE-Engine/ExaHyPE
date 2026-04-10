// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

#include <string>

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}
  struct RuntimeParameters;
{% for item in NAMESPACE -%}
}
{%- endfor %}

struct {{NAMESPACE | join("::")}}::RuntimeParameters {
  struct Storage {
{%- for parameter in RUNTIME_PARAMETERS %}
    {{ parameter.cpp_type }} {{ parameter.name }} = {{ parameter.default_value }};
{%- endfor %}
  };

  static void load(const std::string& configFilePath);

{%- for parameter in RUNTIME_PARAMETERS %}
{%- if parameter.is_array %}
  static INLINE_METHOD GPU_CALLABLE_METHOD {{ parameter.return_type }} {{ parameter.accessor_name }}() {
    tarch::la::Vector<DIMENSIONS, {{ parameter.scalar_cpp_type }}> result;
    for (int d = 0; d < DIMENSIONS; ++d) {
      result[d] = _storage.{{ parameter.name }}[d];
    }
    return result;
  }
{%- else %}
  static INLINE_METHOD GPU_CALLABLE_METHOD {{ parameter.return_type }} {{ parameter.accessor_name }}() {
    return _storage.{{ parameter.name }};
  }
{%- endif %}
{%- endfor %}

private:
  TARGET_DECLARE
  static Storage _storage;
  TARGET_DECLARE_END
};
