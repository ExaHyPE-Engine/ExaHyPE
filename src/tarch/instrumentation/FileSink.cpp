// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FileSink.h"

#include "tarch/multicore/Lock.h"

tarch::instrumentation::FileSink::~FileSink() { close(); }

tarch::instrumentation::FileSink& tarch::instrumentation::FileSink::getInstance() {
  static FileSink singleton;
  return singleton;
}

void tarch::instrumentation::FileSink::reopenOutputStream() {
  if (_outputStream.is_open()) {
    _outputStream.flush();
    _outputStream.close();
  }

  if (not _outputFileName.empty()) {
    _outputStream.open(_outputFileName, std::ios::out | std::ios::trunc);
  }
}

void tarch::instrumentation::FileSink::setOutputFile(const std::string& outputLogFileName) {
  tarch::multicore::Lock lock(_semaphore);
  _outputFileName = outputLogFileName;

  reopenOutputStream();
}

void tarch::instrumentation::FileSink::write(std::string_view message) {
  tarch::multicore::Lock lock(_semaphore);

  if (_outputStream.is_open()) {
    _outputStream.write(message.data(), static_cast<std::streamsize>(message.size()));
    _outputStream.flush();
  }
}

void tarch::instrumentation::FileSink::write(std::string_view message, std::string_view outputLogFileName) {
  tarch::multicore::Lock lock(_semaphore);

  if (_outputFileName != outputLogFileName) {
    _outputFileName.assign(outputLogFileName.data(), outputLogFileName.size());
    reopenOutputStream();
  }

  if (_outputStream.is_open()) {
    _outputStream.write(message.data(), static_cast<std::streamsize>(message.size()));
    _outputStream.flush();
  }
}

void tarch::instrumentation::FileSink::close() {
  tarch::multicore::Lock lock(_semaphore);

  if (_outputStream.is_open()) {
    _outputStream.flush();
    _outputStream.close();
  }
}
