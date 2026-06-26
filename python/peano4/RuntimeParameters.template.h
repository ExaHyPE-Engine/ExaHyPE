// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

#include <string>
#include <vector>

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
  static void load(const std::string& configFilePath, int& argc, char** argv);
  static void printUsage();

{%- for parameter in RUNTIME_PARAMETERS %}
{%- if parameter.is_array %}
  static INLINE_METHOD GPU_CALLABLE_METHOD {{ parameter.return_type }} {{ parameter.accessor_name }}() {
    tarch::la::Vector<{{ parameter.array_size }}, {{ parameter.scalar_cpp_type }}> result;
    for (int d = 0; d < {{ parameter.array_size }}; ++d) {
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
{%- for parameter in RUNTIME_PARAMETERS if parameter.is_array %}
  static {{ parameter.cpp_type }} to{{ parameter.name[0].upper() }}{{ parameter.name[1:] }}(
    const std::vector<{{ parameter.scalar_cpp_type }}>& values,
    const {{ parameter.cpp_type }}& defaultValue
  );
{%- endfor %}

  TARGET_DECLARE
  static Storage _storage;
  TARGET_DECLARE_END
};
