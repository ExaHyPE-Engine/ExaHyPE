// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::instrumentation {
  class FileSink;
} // namespace tarch::instrumentation

class tarch::instrumentation::FileSink final {
public:
  FileSink(const FileSink&)            = delete;
  FileSink(FileSink&&)                 = delete;
  FileSink& operator=(const FileSink&) = delete;
  FileSink& operator=(FileSink&&)      = delete;
  ~FileSink();

  static FileSink& getInstance();

  void setOutputFile(const std::string& outputLogFileName);
  void write(const std::string_view message, const std::string_view outputLogFileName);
  void write(const std::string_view message);
  void close();

private:
  static constexpr std::chrono::seconds FlushInterval{1};

  std::mutex    _mutex{};
  std::ofstream _outputStream{};
  std::string   _outputFileName{};

  std::chrono::steady_clock::time_point _lastFlush{};

  FileSink() = default;

  void reopenOutputStream();
  void writeAndFlushPeriodically(const std::string_view message);
};
