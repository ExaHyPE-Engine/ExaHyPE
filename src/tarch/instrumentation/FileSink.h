// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/multicore/BooleanSemaphore.h"

namespace tarch::instrumentation {
  class FileSink;
} // namespace tarch::instrumentation

/**
 * File sink used by Log.
 *
 * Log configures and orchestrates this sink. The file sink only owns the file
 * stream and writes already formatted messages.
 */
class tarch::instrumentation::FileSink {
private:
  tarch::multicore::BooleanSemaphore _semaphore;

  std::ofstream _outputStream;
  std::string   _outputFileName;

  FileSink() = default;

  FileSink(const FileSink&)            = delete;
  FileSink& operator=(const FileSink&) = delete;

  void reopenOutputStream();

public:
  ~FileSink();

  static FileSink& getInstance();

  void setOutputFile(const std::string& outputLogFileName);
  void write(std::string_view message, std::string_view outputLogFileName);
  void write(std::string_view message);
  void close();
};
