// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::reader {
  class JSONReader;
} // namespace tarch::reader

class tarch::reader::JSONReader final {
public:
  explicit JSONReader(const std::string& filepath);
  JSONReader(const JSONReader&)            = delete;
  JSONReader(JSONReader&&)                 = delete;
  JSONReader& operator=(const JSONReader&) = delete;
  JSONReader& operator=(JSONReader&&)      = delete;
  ~JSONReader();

  [[nodiscard]] bool hasKey(const std::string& key) const;

  [[nodiscard]] std::string getString(const std::string& key) const;
  [[nodiscard]] int getInteger(const std::string& key) const;
  [[nodiscard]] double getDouble(const std::string& key) const;
  [[nodiscard]] bool getBoolean(const std::string& key) const;

  [[nodiscard]] std::string getString(const std::string& key, const std::string& fallback) const;
  [[nodiscard]] int getInteger(const std::string& key, int fallback) const;
  [[nodiscard]] double getDouble(const std::string& key, double fallback) const;
  [[nodiscard]] bool getBoolean(const std::string& key, bool fallback) const;

  [[nodiscard]] std::vector<std::string> getStringArray(const std::string& key) const;
  [[nodiscard]] std::vector<int> getIntegerArray(const std::string& key) const;
  [[nodiscard]] std::vector<double> getDoubleArray(const std::string& key) const;
  [[nodiscard]] std::vector<bool> getBooleanArray(const std::string& key) const;

  [[nodiscard]] std::vector<std::string> getUnusedKeys() const;

private:
  struct Impl;
  std::unique_ptr<Impl> _impl;

  void markRead(const std::string& key) const;
};
