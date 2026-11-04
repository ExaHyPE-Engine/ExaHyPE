// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Watch.h"

#include "tarch/instrumentation/Info.h"

tarch::instrumentation::Watch::Watch(
  [[maybe_unused]] const std::string& className,
  const std::string&                  operationName,
  const bool                          plotResultInDestructor,
  const bool                          startToTickImmediately
) :
  _plotResultInDestructor(plotResultInDestructor),
  _operationName(operationName) {
  if (startToTickImmediately) {
    start();
  }
}

tarch::instrumentation::Watch::~Watch() {
  if (_isRunning) {
    stop();
  }

  if (_plotResultInDestructor) {
    logInfo(
      (_operationName.empty() ? std::string("Block") : "Operation " + _operationName)
      << ": CPU time " << getCPUTime() << " s, calendar time " << getCalendarTime() << " s."
    );
  }
}

/**
 * Restarts the timer. The getters report the time between this call and the next stop().
 */
void
tarch::instrumentation::Watch::start() {
  _startClockTicks = std::clock();
  _startTime       = std::chrono::steady_clock::now();
  _isRunning       = true;
}

/**
 * Stops the timer and keeps the start time. A repeated call measures from the same start. The
 * getters return the elapsed time only after this call.
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
  const auto elapsedTicks   = static_cast<double>(_elapsedClockTicks);
  const auto ticksPerSecond = static_cast<double>(CLOCKS_PER_SEC);
  return elapsedTicks / ticksPerSecond;
}

/**
 * Returns the wall clock time in seconds between start() and stop().
 */
double
tarch::instrumentation::Watch::getCalendarTime() const {
  return std::chrono::duration<double>(_elapsedTime).count();
}
