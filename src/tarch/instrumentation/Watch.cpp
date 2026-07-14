// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Watch.h"

#include "Log.h"

tarch::instrumentation::Watch::Watch(
  [[maybe_unused]] const std::string& className,
  const std::string&                  operationName,
  const bool                          plotResultInDestructor,
  bool                                startToTickImmediately // = true
):
  _plotResultInDestructor(plotResultInDestructor),
  _operationName(operationName),
  _startClockTicks(0),
  _startTime(),
  _elapsedClockTicks(0),
  _elapsedTime(),
  _isRunning(startToTickImmediately) {
  if (startToTickImmediately) {
    start();
  }
}

tarch::instrumentation::Watch::~Watch() {
  if (_isRunning) {
    stop();
  }

  if (_plotResultInDestructor) {
    if (_operationName.empty()) {
      logInfo(
        "Total number of clock ticks within block (CPU time, calendar time): "
        << "(" << getCPUTime() << "s"
        << "," << getCalendarTime() << "s"
        << ")"
      );
    } else {
      logInfo(
        "Operation "
        << _operationName << ": Total number of clock ticks within block (CPU time, calendar time): "
        << "(" << getCPUTime() << "s"
        << "," << getCalendarTime() << "s"
        << ")"
      );
    }
  }
}

void tarch::instrumentation::Watch::start() {
  _startClockTicks = std::clock();
  _startTime       = std::chrono::steady_clock::now();
  _isRunning       = true;
}

void tarch::instrumentation::Watch::stop() {
  _elapsedClockTicks = std::clock() - _startClockTicks;
  _elapsedTime       = std::chrono::steady_clock::now() - _startTime;
  _isRunning         = false;
}

double tarch::instrumentation::Watch::getCPUTime() const {
  double lhs = static_cast<double>(_elapsedClockTicks);
  double rhs = static_cast<double>(CLOCKS_PER_SEC);
  return lhs / rhs;
}

std::clock_t tarch::instrumentation::Watch::getCPUTicks() const { return _elapsedClockTicks; }

double tarch::instrumentation::Watch::getCalendarTime() const {
  return std::chrono::duration<double>(_elapsedTime).count();
}

bool tarch::instrumentation::Watch::isOn() const { return _isRunning; }
