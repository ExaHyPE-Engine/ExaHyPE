// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::instrumentation {
  class CommandLineSink;
} // namespace tarch::instrumentation

class tarch::instrumentation::CommandLineSink final {
public:
  enum class TerminalStyle { Plain, Warning, Error };

  CommandLineSink(const CommandLineSink&)            = delete;
  CommandLineSink(CommandLineSink&&)                 = delete;
  CommandLineSink& operator=(const CommandLineSink&) = delete;
  CommandLineSink& operator=(CommandLineSink&&)      = delete;
  ~CommandLineSink();

  static CommandLineSink& getInstance();

  void writeToStdOut(std::string_view message, bool flush, TerminalStyle terminalStyle = TerminalStyle::Plain);
  void writeToStdErr(std::string_view message, bool flush, TerminalStyle terminalStyle = TerminalStyle::Plain);
  void close();

private:
  std::mutex _mutex{};
  bool       _stdoutSupportsANSI{false};
  bool       _stderrSupportsANSI{false};

  static bool streamSupportsANSI(int fileDescriptor);
  static const char* getANSIPrefix(TerminalStyle terminalStyle);

  void configureOutputStreams();

  CommandLineSink();
};
