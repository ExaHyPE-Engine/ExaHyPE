// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/multicore/BooleanSemaphore.h"

namespace tarch::instrumentation {
  class CommandLineSink;
} // namespace tarch::instrumentation

class tarch::instrumentation::CommandLineSink {
public:
  enum class TerminalStyle { Plain, Warning, Error };

private:
  tarch::multicore::BooleanSemaphore _semaphore;
  bool                               _stdoutSupportsAnsi;
  bool                               _stderrSupportsAnsi;

  CommandLineSink& operator=(const CommandLineSink&) = delete;
  CommandLineSink(const CommandLineSink&)            = delete;

  static bool        streamSupportsAnsi(int fileDescriptor);
  static const char* getAnsiPrefix(TerminalStyle terminalStyle);

  void configureOutputStreams();

  CommandLineSink();

public:
  ~CommandLineSink();

  static CommandLineSink& getInstance();

  void writeToStdOut(std::string_view message, bool flush, TerminalStyle terminalStyle = TerminalStyle::Plain);
  void writeToStdErr(std::string_view message, bool flush, TerminalStyle terminalStyle = TerminalStyle::Plain);
  void close();
};
