// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Watch.h"

#include "tarch/instrumentation/Info.h"

tarch::instrumentation::Watch::Watch(
  [[maybe_unused]] const std::string& className,
  const std::string&                  operationName,
  const bool                          plotResultInDestructor,
  bool                                startToTickImmediately
) :
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
        "Total number of clock ticks within block (CPU time, calendar time): " << "(" << getCPUTime() << "s"
                                                                               << "," << getCalendarTime() << "s"
                                                                               << ")"
      );
    } else {
      logInfo(
        "Operation " << _operationName << ": Total number of clock ticks within block (CPU time, calendar time): "
                     << "(" << getCPUTime() << "s"
                     << "," << getCalendarTime() << "s"
                     << ")"
      );
    }
  }
}

/**
 * Restarts the timer, so the getters report the time between this call and the
 * next stop().
 */
void
tarch::instrumentation::Watch::start() {
  _startClockTicks = std::clock();
  _startTime       = std::chrono::steady_clock::now();
  _isRunning       = true;
}

/**
 * Stops the timer without resetting the start time, so a repeated call
 * measures from the same start. The getters return the elapsed time only after
 * this call.
 */
void
tarch::instrumentation::Watch::stop() {
  _elapsedClockTicks = std::clock() - _startClockTicks;
  _elapsedTime       = std::chrono::steady_clock::now() - _startTime;
  _isRunning         = false;
}

/**
 * Returns the CPU time in seconds between start() and stop(), summed over the
 * cores the process ran on. The clock tick counter can overflow.
 */
double
tarch::instrumentation::Watch::getCPUTime() const {
  const double elapsedTicks   = static_cast<double>(_elapsedClockTicks);
  const double ticksPerSecond = static_cast<double>(CLOCKS_PER_SEC);
  return elapsedTicks / ticksPerSecond;
}

/**
 * Returns the elapsed calendar time between the start and the stop command of
 * the timer, i.e. the wall clock time, in seconds.
 */
double
tarch::instrumentation::Watch::getCalendarTime() const {
  return std::chrono::duration<double>(_elapsedTime).count();
}
