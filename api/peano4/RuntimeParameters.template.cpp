// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RuntimeParameters.h"

#include "tarch/reader/JSONReader.h"

{% if HAS_RUNTIME_PARAMETERS %}
namespace {
  std::string consumeValue(
    int&               index,
    int                argc,
    char**             argv,
    const std::string& argument
  ) {
    if (index + 1 >= argc) {
      throw std::runtime_error("Missing value for " + argument + ".");
    }
    ++index;
    return argv[index];
  }

{% if HAS_STRING_RUNTIME_PARAMETERS %}
  std::string formatDefault(const std::string& value) {
    return value.empty() ? "<empty>" : value;
  }
{% endif %}

{% if HAS_BOOLEAN_RUNTIME_PARAMETERS %}
  std::string formatDefault(bool value) {
    return value ? "true" : "false";
  }
{% endif %}

  template <typename T>
  std::string formatDefault(T value) {
    std::ostringstream result;
    result << value;
    return result.str();
  }

  template <int Size, typename T>
  std::string formatDefault(const tarch::la::Vector<Size, T>& values) {
    std::ostringstream result;
    for (int index = 0; index < Size; ++index) {
      if (index > 0) {
        result << ",";
      }
      result << formatDefault(values[index]);
    }
    return result.str();
  }

{% if HAS_DOUBLE_RUNTIME_PARAMETERS %}
  double parseDouble(const std::string& value, const std::string& key) {
    try {
      size_t       processed = 0;
      const double result    = std::stod(value, &processed);
      if (processed != value.size()) {
        throw std::invalid_argument("trailing characters");
      }
      return result;
    } catch (const std::exception&) {
      throw std::runtime_error("Invalid numeric value for " + key + ": " + value);
    }
  }
{% endif %}

{% if HAS_INTEGER_RUNTIME_PARAMETERS %}
  int parseInteger(const std::string& value, const std::string& key) {
    try {
      size_t    processed = 0;
      const int result    = std::stoi(value, &processed);
      if (processed != value.size()) {
        throw std::invalid_argument("trailing characters");
      }
      return result;
    } catch (const std::exception&) {
      throw std::runtime_error("Invalid integer value for " + key + ": " + value);
    }
  }
{% endif %}

{% if HAS_BOOLEAN_RUNTIME_PARAMETERS %}
  bool parseBoolean(std::string value, const std::string& key) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });

    if (value == "true" or value == "1" or value == "yes" or value == "on") {
      return true;
    }
    if (value == "false" or value == "0" or value == "no" or value == "off") {
      return false;
    }

    throw std::runtime_error("Invalid boolean value for " + key + ": " + value);
  }
{% endif %}

{% if HAS_ARRAY_RUNTIME_PARAMETERS %}
  std::vector<std::string> splitCommaSeparated(const std::string& value) {
    std::vector<std::string> result;
    std::string              current;
    std::stringstream        stream(value);
    while (std::getline(stream, current, ',')) {
      result.push_back(current);
    }
    return result;
  }
{% endif %}

{% if RUNTIME_PARAMETERS | selectattr("is_array") | selectattr("base_type", "equalto", "double") | list %}
  std::vector<double> parseDoubleArray(const std::string& value, const std::string& key) {
    std::vector<double> result;
    for (const auto& entry: splitCommaSeparated(value)) {
      result.push_back(parseDouble(entry, key));
    }
    return result;
  }
{% endif %}

{% if RUNTIME_PARAMETERS | selectattr("is_array") | selectattr("base_type", "equalto", "int") | list %}
  std::vector<int> parseIntegerArray(const std::string& value, const std::string& key) {
    std::vector<int> result;
    for (const auto& entry: splitCommaSeparated(value)) {
      result.push_back(parseInteger(entry, key));
    }
    return result;
  }
{% endif %}

{% if RUNTIME_PARAMETERS | selectattr("is_array") | selectattr("base_type", "equalto", "bool") | list %}
  std::vector<bool> parseBooleanArray(const std::string& value, const std::string& key) {
    std::vector<bool> result;
    for (const auto& entry: splitCommaSeparated(value)) {
      result.push_back(parseBoolean(entry, key));
    }
    return result;
  }
{% endif %}
} // namespace
{% endif %}

{% for parameter in RUNTIME_PARAMETERS if parameter.is_array %}
{{ parameter.cpp_type }} {{NAMESPACE | join("::")}}::RuntimeParameters::to{{ parameter.name[0].upper() }}{{ parameter.name[1:] }}(
  const std::vector<{{ parameter.scalar_cpp_type }}>& values,
  const {{ parameter.cpp_type }}& defaultValue
) {
  if (values.size() < {{ parameter.array_size }}) {
    std::ostringstream message;
    message << "Runtime parameter \"{{ parameter.name }}\" must contain at least " << {{ parameter.array_size }} << " entries, but got " << values.size() << ".";
    throw std::runtime_error(message.str());
  }

  auto result = defaultValue;
  for (int d = 0; d < {{ parameter.array_size }}; ++d) {
    result[d] = values[d];
  }
  return result;
}
{% endfor %}

TARGET_DECLARE
{{NAMESPACE | join("::")}}::RuntimeParameters::Storage {{NAMESPACE | join("::")}}::RuntimeParameters::_storage;
TARGET_DECLARE_END

void {{NAMESPACE | join("::")}}::RuntimeParameters::printUsage() {
  std::cout << "\nApplication-specific runtime configuration:\n";
{% if HAS_RUNTIME_PARAMETERS %}
{% for parameter in RUNTIME_PARAMETERS %}
  std::cout
    << "  --{{ parameter.name }} <{% if parameter.is_array %}{{ parameter.base_type }}[{{ parameter.array_size }}]{% else %}{{ parameter.base_type }}{% endif %}>"
    << " (default: " << formatDefault(Storage{}.{{ parameter.name }}) << ").\n";
{% endfor %}
{% if HAS_ARRAY_RUNTIME_PARAMETERS %}
  std::cout << "  Array values are comma-separated.\n";
{% endif %}
{% else %}
  std::cout << "  <none>\n";
{% endif %}
}

void {{NAMESPACE | join("::")}}::RuntimeParameters::load(tarch::reader::JSONReader* reader) {
  _storage = Storage{};

  if (reader == nullptr) {
    return;
  }

{% for parameter in RUNTIME_PARAMETERS %}
  if (reader->hasKey("{{ parameter.name }}")) {
{% if parameter.is_array %}
    _storage.{{ parameter.name }} = to{{ parameter.name[0].upper() }}{{ parameter.name[1:] }}(
      reader->{{ parameter.getter }}("{{ parameter.name }}"),
      _storage.{{ parameter.name }}
    );
{% else %}
    _storage.{{ parameter.name }} = reader->{{ parameter.getter }}("{{ parameter.name }}");
{% endif %}
  }
{% endfor %}
}

void {{NAMESPACE | join("::")}}::RuntimeParameters::load(tarch::reader::JSONReader* reader, int& argc, char** argv) {
  load(reader);

{% if RUNTIME_PARAMETERS %}
  int writeIndex = 1;
  for (int readIndex = 1; readIndex < argc; ++readIndex) {
    const std::string argument = argv[readIndex];
{% for parameter in RUNTIME_PARAMETERS %}
    {% if loop.first %}if{% else %}else if{% endif %} (argument == "--{{ parameter.name }}") {
      const std::string value = consumeValue(readIndex, argc, argv, argument);
{% if parameter.is_array %}
{% if parameter.base_type == "double" %}
      _storage.{{ parameter.name }} = to{{ parameter.name[0].upper() }}{{ parameter.name[1:] }}(parseDoubleArray(value, argument), _storage.{{ parameter.name }});
{% elif parameter.base_type == "int" %}
      _storage.{{ parameter.name }} = to{{ parameter.name[0].upper() }}{{ parameter.name[1:] }}(parseIntegerArray(value, argument), _storage.{{ parameter.name }});
{% elif parameter.base_type == "bool" %}
      _storage.{{ parameter.name }} = to{{ parameter.name[0].upper() }}{{ parameter.name[1:] }}(parseBooleanArray(value, argument), _storage.{{ parameter.name }});
{% endif %}
{% else %}
{% if parameter.base_type == "double" %}
      _storage.{{ parameter.name }} = parseDouble(value, argument);
{% elif parameter.base_type == "int" %}
      _storage.{{ parameter.name }} = parseInteger(value, argument);
{% elif parameter.base_type == "bool" %}
      _storage.{{ parameter.name }} = parseBoolean(value, argument);
{% elif parameter.base_type == "string" %}
      _storage.{{ parameter.name }} = value;
{% endif %}
{% endif %}
    }
{% endfor %}
    else {
      argv[writeIndex] = argv[readIndex];
      ++writeIndex;
    }
  }

  argc       = writeIndex;
  argv[argc] = nullptr;
{% else %}
  (void)argc;
  (void)argv;
{% endif %}
}
