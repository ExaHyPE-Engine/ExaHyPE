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

/**
 * (Re)start the timer
 *
 * The constructor invokes this operation as well, so a call here restarts the
 * measurement. There is no need to call start() after stop(): omitting it
 * simply keeps the baseline, i.e. all getters continue to refer to the time
 * zero from the watch's construction.
 *
 * @see stop()
 */
void tarch::instrumentation::Watch::start() {
  _startClockTicks = std::clock();
  _startTime       = std::chrono::steady_clock::now();
  _isRunning       = true;
}

/**
 * Stop the timer
 *
 * A timer may be stopped multiple times, as stopping does not reset the start
 * time. The getters return meaningful results only once stop() has been
 * called.
 *
 * @see start()
 */
void tarch::instrumentation::Watch::stop() {
  _elapsedClockTicks = std::clock() - _startClockTicks;
  _elapsedTime       = std::chrono::steady_clock::now() - _startTime;
  _isRunning         = false;
}

/**
 * Return CPU time in seconds
 *
 * Returns the elapsed CPU time between the start and the stop command of the
 * timer, i.e. the clock ticks spent by the process. Take care: this counter
 * might overflow, especially on a 32 bit architecture.
 *
 * ## Multithreading
 *
 * Under multithreading, CPU time sums up the clock ticks invested by the
 * individual cores, i.e. moving from a one core to a dual core machine
 * roughly doubles the CPU time.
 *
 * @return CPU time in seconds
 */
double tarch::instrumentation::Watch::getCPUTime() const {
  double lhs = static_cast<double>(_elapsedClockTicks);
  double rhs = static_cast<double>(CLOCKS_PER_SEC);
  return lhs / rhs;
}

std::clock_t tarch::instrumentation::Watch::getCPUTicks() const { return _elapsedClockTicks; }

/**
 * Returns the elapsed calendar time between the start and the stop command of
 * the timer, i.e. the wall clock time, in seconds.
 */
double tarch::instrumentation::Watch::getCalendarTime() const {
  return std::chrono::duration<double>(_elapsedTime).count();
}

/**
 * Returns whether the watch is currently on, and false once it is stopped.
 * On and off is a logical concept: internally a watch administers system time
 * snapshots, and the getters return differences of these snapshots.
 */
bool tarch::instrumentation::Watch::isOn() const { return _isRunning; }
