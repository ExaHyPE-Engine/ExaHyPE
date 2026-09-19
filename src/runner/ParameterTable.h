// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::reader {
  class JSONReader;
} // namespace tarch::reader

namespace runner {
  class ParameterTable;
} // namespace runner

/**
 * Every entry names a scalar or an array in a storage the caller owns. load() reads the
 * configuration file first and the command line second, and it removes the consumed arguments
 * from the command line.
 */
class runner::ParameterTable final {
public:
  using Values = std::variant<double*, int*, bool*, std::string*>;

  ParameterTable()                                 = default;
  ParameterTable(const ParameterTable&)            = default;
  ParameterTable(ParameterTable&&)                 = default;
  ParameterTable& operator=(const ParameterTable&) = default;
  ParameterTable& operator=(ParameterTable&&)      = default;
  ~ParameterTable()                                = default;

  void add(const std::string& name, Values values);
  void addArray(const std::string& name, Values values, int size);

  void load(const tarch::reader::JSONReader* reader) const;
  void load(const tarch::reader::JSONReader* reader, int& argc, char** argv) const;

  [[nodiscard]] std::string usage() const;
  void                      printUsage() const;

private:
  struct Entry final {
    std::string name{};
    Values      values{};
    int         size{1};
    bool        isArray{false};
    std::string defaultText{};
  };

  std::vector<Entry> _entries{};
};
