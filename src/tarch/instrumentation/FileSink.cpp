// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileSink.h"

tarch::instrumentation::FileSink::~FileSink() { close(); }

tarch::instrumentation::FileSink&
tarch::instrumentation::FileSink::getInstance() {
  // The sink is never destroyed, so logging works during static destruction.
  static FileSink& singleton = *new FileSink();
  return singleton;
}

void
tarch::instrumentation::FileSink::reopenOutputStream() {
  if (_outputStream.is_open()) {
    _outputStream.flush();
    _outputStream.close();
  }

  if (not _outputFileName.empty()) {
    _outputStream.open(_outputFileName, std::ios::out | std::ios::trunc);
  }
}

void
tarch::instrumentation::FileSink::setOutputFile(const std::string& outputLogFileName) {
  const std::scoped_lock lock(_mutex);
  _outputFileName = outputLogFileName;

  reopenOutputStream();
}

/**
 * Writes the message and flushes the stream at most once per FlushInterval, so
 * a process that dies loses at most one interval of lines.
 */
void
tarch::instrumentation::FileSink::writeAndFlushPeriodically(const std::string_view message) {
  if (not _outputStream.is_open()) {
    return;
  }
  _outputStream.write(message.data(), static_cast<std::streamsize>(message.size()));
  const auto now = std::chrono::steady_clock::now();
  if (now - _lastFlush >= FlushInterval) {
    _outputStream.flush();
    _lastFlush = now;
  }
}

void
tarch::instrumentation::FileSink::write(const std::string_view message) {
  const std::scoped_lock lock(_mutex);
  writeAndFlushPeriodically(message);
}

void
tarch::instrumentation::FileSink::write(const std::string_view message, const std::string_view outputLogFileName) {
  const std::scoped_lock lock(_mutex);

  if (_outputFileName != outputLogFileName) {
    _outputFileName.assign(outputLogFileName.data(), outputLogFileName.size());
    reopenOutputStream();
  }
  writeAndFlushPeriodically(message);
}

void
tarch::instrumentation::FileSink::close() {
  const std::scoped_lock lock(_mutex);

  if (_outputStream.is_open()) {
    _outputStream.flush();
    _outputStream.close();
  }
}
