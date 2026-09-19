// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParameterTable.h"

#include "runner/RuntimeParameters.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/reader/JSONReader.h"

namespace {
  template <typename Scalar>
  std::string typeName() {
    if constexpr (std::is_same_v<Scalar, double>) {
      return "double";
    } else if constexpr (std::is_same_v<Scalar, int>) {
      return "int";
    } else if constexpr (std::is_same_v<Scalar, bool>) {
      return "bool";
    } else {
      return "string";
    }
  }

  template <typename Scalar>
  std::string format(const Scalar& value) {
    if constexpr (std::is_same_v<Scalar, bool>) {
      return value ? "true" : "false";
    } else {
      return runner::RuntimeParameters::formatDefault(value);
    }
  }

  template <typename Scalar>
  std::string format(const Scalar* const values, const int size) {
    std::string result;
    for (int index = 0; index < size; index++) {
      result += (index > 0 ? "," : "") + format(values[index]);
    }
    return result;
  }

  bool parseBoolean(std::string value, const std::string& key) {
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char character) {
      return static_cast<char>(std::tolower(character));
    });
    if (value == "true" or value == "1" or value == "yes" or value == "on") {
      return true;
    }
    if (value == "false" or value == "0" or value == "no" or value == "off") {
      return false;
    }
    tarch::terminateWithError("Value \"" + value + "\" of runtime parameter \"" + key + "\" is not a boolean.");
  }

  template <typename Scalar>
  Scalar parse(const std::string& value, const std::string& key) {
    if constexpr (std::is_same_v<Scalar, double>) {
      return runner::RuntimeParameters::parseDouble(value, key);
    } else if constexpr (std::is_same_v<Scalar, int>) {
      return runner::RuntimeParameters::parseInteger(value, key);
    } else if constexpr (std::is_same_v<Scalar, bool>) {
      return parseBoolean(value, key);
    } else {
      return value;
    }
  }

  template <typename Scalar>
  Scalar readScalar(const tarch::reader::JSONReader& reader, const std::string& name) {
    if constexpr (std::is_same_v<Scalar, double>) {
      return reader.getDouble(name);
    } else if constexpr (std::is_same_v<Scalar, int>) {
      return reader.getInteger(name);
    } else if constexpr (std::is_same_v<Scalar, bool>) {
      return reader.getBoolean(name);
    } else {
      return reader.getString(name);
    }
  }

  template <typename Scalar>
  std::vector<Scalar> readArray(const tarch::reader::JSONReader& reader, const std::string& name) {
    if constexpr (std::is_same_v<Scalar, double>) {
      return reader.getDoubleArray(name);
    } else if constexpr (std::is_same_v<Scalar, int>) {
      return reader.getIntegerArray(name);
    } else if constexpr (std::is_same_v<Scalar, bool>) {
      return reader.getBooleanArray(name);
    } else {
      return reader.getStringArray(name);
    }
  }

  template <typename Scalar>
  void assign(const std::vector<Scalar>& source, Scalar* const values, const int size, const std::string& name) {
    if (source.size() < static_cast<std::size_t>(size)) {
      tarch::terminateWithError(
        "Runtime parameter \"" + name + "\" needs " + std::to_string(size) + " entries and carries "
        + std::to_string(source.size()) + "."
      );
    }
    std::copy_n(source.begin(), size, values);
  }

  template <typename Scalar>
  void readFromConfiguration(
    const tarch::reader::JSONReader& reader,
    const std::string&               name,
    Scalar* const                    values,
    const int                        size,
    const bool                       isArray
  ) {
    if (isArray) {
      assign(readArray<Scalar>(reader, name), values, size, name);
    } else {
      *values = readScalar<Scalar>(reader, name);
    }
  }

  template <typename Scalar>
  void readFromCommandLine(
    const std::string& value,
    const std::string& argument,
    Scalar* const      values,
    const int          size,
    const bool         isArray
  ) {
    if (not isArray) {
      *values = parse<Scalar>(value, argument);
      return;
    }
    std::vector<Scalar> parsed;
    std::string         entry;
    std::stringstream   stream(value);
    while (std::getline(stream, entry, ',')) {
      parsed.push_back(parse<Scalar>(entry, argument));
    }
    assign(parsed, values, size, argument);
  }
} // namespace

/**
 * The value the storage holds at registration is the default the usage text reports.
 */
void runner::ParameterTable::add(const std::string& name, const Values values) {
  assertion(not name.empty());
  _entries.push_back(
    {name, values, 1, false, std::visit([](const auto* const pointer) { return format(*pointer); }, values)}
  );
}

void runner::ParameterTable::addArray(const std::string& name, const Values values, const int size) {
  assertion(not name.empty());
  assertion(size > 0, name, size);
  _entries.push_back(
    {name, values, size, true, std::visit([size](const auto* const pointer) { return format(pointer, size); }, values)}
  );
}

void runner::ParameterTable::load(const tarch::reader::JSONReader* const reader) const {
  if (reader == nullptr) {
    return;
  }
  for (const auto& entry : _entries) {
    if (reader->hasKey(entry.name)) {
      std::visit(
        [&](auto* const values) { readFromConfiguration(*reader, entry.name, values, entry.size, entry.isArray); },
        entry.values
      );
    }
  }
}

/**
 * The arguments that name no entry keep their order, so the caller's own parser reads them
 * afterwards.
 */
void runner::ParameterTable::load(const tarch::reader::JSONReader* const reader, int& argc, char** const argv) const {
  load(reader);

  int writeIndex = 1;
  for (int readIndex = 1; readIndex < argc; readIndex++) {
    const std::string argument = argv[readIndex];
    const auto        entry    = std::find_if(_entries.begin(), _entries.end(), [&argument](const Entry& candidate) {
      return argument == "--" + candidate.name;
    });
    if (entry == _entries.end()) {
      argv[writeIndex] = argv[readIndex];
      writeIndex++;
      continue;
    }
    const std::string value = RuntimeParameters::consumeValue(readIndex, argc, argv, argument);
    std::visit(
      [&](auto* const values) { readFromCommandLine(value, argument, values, entry->size, entry->isArray); },
      entry->values
    );
  }
  argc       = writeIndex;
  argv[argc] = nullptr;
}

std::string runner::ParameterTable::usage() const {
  std::string result = "\nApplication-specific runtime configuration:\n";
  if (_entries.empty()) {
    return result + "  <none>\n";
  }
  bool hasArrays = false;
  for (const auto& entry : _entries) {
    const auto type = std::visit(
      [](const auto* const pointer) { return typeName<std::remove_cvref_t<decltype(*pointer)>>(); },
      entry.values
    );
    result += "  --" + entry.name + " <" + type + (entry.isArray ? "[" + std::to_string(entry.size) + "]" : "") + ">"
      + " (default: " + entry.defaultText + ").\n";
    hasArrays = hasArrays or entry.isArray;
  }
  if (hasArrays) {
    result += "  Array values are comma-separated.\n";
  }
  return result;
}

void runner::ParameterTable::printUsage() const { std::cout << usage(); }
