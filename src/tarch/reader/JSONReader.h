// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::reader {
  class JSONReader;
} // namespace tarch::reader

class tarch::reader::JSONReader {
private:
  struct Impl;
  std::unique_ptr<Impl> _impl;

  void markRead(const std::string& key) const;

public:
  explicit JSONReader(const std::string& filepath);
  // Out of line in the .cpp: the pimpl Impl is incomplete here.
  ~JSONReader();

  JSONReader(const JSONReader&)            = delete;
  JSONReader(JSONReader&&)                 = delete;
  JSONReader& operator=(const JSONReader&) = delete;
  JSONReader& operator=(JSONReader&&)      = delete;

  // Existence check only; does not mark the key read.
  bool hasKey(const std::string& key) const;

  std::string getString(const std::string& key) const;
  int         getInteger(const std::string& key) const;
  double      getDouble(const std::string& key) const;
  bool        getBoolean(const std::string& key) const;

  // Returns the value if present (marking it read), else the fallback.
  std::string getString(const std::string& key, const std::string& fallback) const;
  int         getInteger(const std::string& key, int fallback) const;
  double      getDouble(const std::string& key, double fallback) const;
  bool        getBoolean(const std::string& key, bool fallback) const;

  std::vector<std::string> getStringArray(const std::string& key) const;
  std::vector<int>         getIntegerArray(const std::string& key) const;
  std::vector<double>      getDoubleArray(const std::string& key) const;
  std::vector<bool>        getBooleanArray(const std::string& key) const;

  // Top-level keys never read through a getter.
  std::vector<std::string> getUnusedKeys() const;
};
