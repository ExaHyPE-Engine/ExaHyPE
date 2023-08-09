// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "Watch.h"

#include <sstream>
#include <string>
#include <sys/time.h>

tarch::timing::Watch::Watch(
  const std::string& className,
  const std::string& operationName,
  const bool         plotResultInDestructor,
  bool               startToTickImmediately // = true
):
  _log(className),
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


tarch::timing::Watch::~Watch() {
  if (_isRunning) {
    stop();
  }

  if (_plotResultInDestructor) {
    logInfo(
      _operationName,
      "total number of clock ticks within block (cpu-time,calendar-time): "
        << "(" << getCPUTime() << "s"
        << "," << getCalendarTime() << "s"
        << ")"
    );
  }
}


void tarch::timing::Watch::start() {
  _startClockTicks = std::clock();
  _startTime       = std::chrono::steady_clock::now();
  _isRunning       = true;
}


void tarch::timing::Watch::stop() {
  _elapsedClockTicks = std::clock() - _startClockTicks;
  _elapsedTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - _startTime)
                   .count();
  _elapsedTime = _elapsedTime / 1000000000.0; // Convert to seconds
  _isRunning   = false;
}


double tarch::timing::Watch::getCPUTime() {
  double lhs = static_cast<double>(_elapsedClockTicks);
  double rhs = static_cast<double>(CLOCKS_PER_SEC);
  return lhs / rhs;
}


std::clock_t tarch::timing::Watch::getCPUTicks() { return _elapsedClockTicks; }


double tarch::timing::Watch::getCalendarTime() { return _elapsedTime; }


bool tarch::timing::Watch::isOn() const { return _isRunning; }
