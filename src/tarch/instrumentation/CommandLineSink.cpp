// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CommandLineSink.h"

#include "tarch/multicore/Lock.h"

#if defined(_WIN32)
#include <io.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <unistd.h>
#endif

tarch::instrumentation::CommandLineSink::CommandLineSink():
  _stdoutSupportsAnsi(streamSupportsAnsi(1)),
  _stderrSupportsAnsi(streamSupportsAnsi(2)) {
  configureOutputStreams();
}

tarch::instrumentation::CommandLineSink::~CommandLineSink() { close(); }

tarch::instrumentation::CommandLineSink& tarch::instrumentation::CommandLineSink::getInstance() {
  // Deliberately leaked: the log sink must outlive every other static object,
  // since some (e.g. solver globals dumping a database) log from their destructor.
  static CommandLineSink& singleton = *new CommandLineSink();
  return singleton;
}

void tarch::instrumentation::CommandLineSink::configureOutputStreams() {
  std::cout.setf(std::ios_base::scientific, std::ios_base::floatfield);
  std::cout.precision(20);
  std::cerr.setf(std::ios_base::scientific, std::ios_base::floatfield);
  std::cerr.precision(20);
}

bool tarch::instrumentation::CommandLineSink::streamSupportsAnsi(int fileDescriptor) {
#if defined(_WIN32)
  if (_isatty(fileDescriptor) == 0) {
    return false;
  }

  const DWORD standardHandle = fileDescriptor == 1 ? STD_OUTPUT_HANDLE : fileDescriptor == 2 ? STD_ERROR_HANDLE : 0;
  if (standardHandle == 0) {
    return false;
  }

  const HANDLE console = GetStdHandle(standardHandle);
  if (console == INVALID_HANDLE_VALUE or console == nullptr) {
    return false;
  }

  DWORD mode = 0;
  if (not GetConsoleMode(console, &mode)) {
    return false;
  }

  if ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0) {
    return true;
  }

  return SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
  return isatty(fileDescriptor) == 1;
#endif
}

const char* tarch::instrumentation::CommandLineSink::getAnsiPrefix(TerminalStyle terminalStyle) {
  switch (terminalStyle) {
  case TerminalStyle::Warning:
    return "\033[33m";
  case TerminalStyle::Error:
    return "\033[1;31m";
  case TerminalStyle::Plain:
    return "";
  }

  return "";
}

void tarch::instrumentation::CommandLineSink::
  writeToStdOut(std::string_view message, bool flush, TerminalStyle terminalStyle) {
  tarch::multicore::Lock lock(_semaphore);

  const bool useAnsi = _stdoutSupportsAnsi and terminalStyle != TerminalStyle::Plain;
  if (useAnsi) {
    std::cout << getAnsiPrefix(terminalStyle);
  }
  std::cout << message;
  if (useAnsi) {
    std::cout << "\033[0m";
  }
  if (flush) {
    std::cout.flush();
  }
}

void tarch::instrumentation::CommandLineSink::
  writeToStdErr(std::string_view message, bool flush, TerminalStyle terminalStyle) {
  tarch::multicore::Lock lock(_semaphore);

  const bool useAnsi = _stderrSupportsAnsi and terminalStyle != TerminalStyle::Plain;
  if (useAnsi) {
    std::cerr << getAnsiPrefix(terminalStyle);
  }
  std::cerr << message;
  if (useAnsi) {
    std::cerr << "\033[0m";
  }
  if (flush) {
    std::cerr.flush();
  }
}

void tarch::instrumentation::CommandLineSink::close() {
  tarch::multicore::Lock lock(_semaphore);

  std::cout.flush();
  std::cerr.flush();
}
