// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace tarch::reader {
  class JSONReader;
} // namespace tarch::reader

class tarch::reader::JSONReader {
private:
  struct Impl;
  std::unique_ptr<Impl> _impl;

public:
  explicit JSONReader(const std::string& filepath);
  ~JSONReader();

  bool hasKey(const std::string& key) const;

  std::string getString(const std::string& key) const;
  int         getInteger(const std::string& key) const;
  double      getDouble(const std::string& key) const;
  bool        getBoolean(const std::string& key) const;

  std::vector<std::string> getStringArray(const std::string& key) const;
  std::vector<int>         getIntegerArray(const std::string& key) const;
  std::vector<double>      getDoubleArray(const std::string& key) const;
  std::vector<bool>        getBooleanArray(const std::string& key) const;
};
