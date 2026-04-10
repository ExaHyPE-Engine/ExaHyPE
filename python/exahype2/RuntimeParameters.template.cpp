// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RuntimeParameters.h"

#include "tarch/reader/JSONReader.h"

#include <sstream>
#include <stdexcept>
#include <vector>

{% for parameter in RUNTIME_PARAMETERS if parameter.is_array %}
namespace {
  {{ parameter.cpp_type }} to{{ parameter.name[0].upper() }}{{ parameter.name[1:] }}(
    const std::vector<{{ parameter.scalar_cpp_type }}>& values,
    const {{ parameter.cpp_type }}& defaultValue
  ) {
    if (values.size() < DIMENSIONS) {
      std::ostringstream message;
      message << "JSON key \"{{ parameter.name }}\" must contain at least " << DIMENSIONS << " entries, but got " << values.size() << ".";
      throw std::runtime_error(message.str());
    }

    auto result = defaultValue;
    for (int d = 0; d < DIMENSIONS; ++d) {
      result[d] = values[d];
    }
    return result;
  }
} // namespace
{% endfor %}

TARGET_DECLARE
{{NAMESPACE | join("::")}}::RuntimeParameters::Storage {{NAMESPACE | join("::")}}::RuntimeParameters::_storage;
TARGET_DECLARE_END

void {{NAMESPACE | join("::")}}::RuntimeParameters::load(const std::string& configFilePath) {
  _storage = Storage{};

  if (configFilePath.empty()) {
    return;
  }

  tarch::reader::JSONReader reader(configFilePath);
{% for parameter in RUNTIME_PARAMETERS %}
{% if parameter.is_array %}
  _storage.{{ parameter.name }} = to{{ parameter.name[0].upper() }}{{ parameter.name[1:] }}(
    reader.{{ parameter.getter }}("{{ parameter.name }}"),
    _storage.{{ parameter.name }}
  );
{% else %}
  _storage.{{ parameter.name }} = reader.{{ parameter.getter }}("{{ parameter.name }}");
{% endif %}
{% endfor %}
}
