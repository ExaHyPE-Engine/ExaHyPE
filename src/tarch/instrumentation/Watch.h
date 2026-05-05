// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <chrono>
#include <ctime>
#include <string>

namespace tarch::instrumentation {
  class Watch;
} // namespace tarch::instrumentation

/**
 * Measure CPU and wall clock time for an operation. Create an instance at the
 * beginning of the operation block you want to measure:
 *
 * <pre>
 *   void anyOperation() {
 *     tarch::instrumentation::Watch watch("MyClass", "anyOperation()", true);
 *     ...
 *   }
 * </pre>
 *
 * If requested, the result is written through logInfo() in the destructor. You can
 * also stop the watch explicitly and retrieve the elapsed CPU and wall clock
 * time through the getters.
 *
 * @version $Revision: 1.9 $
 * @author  Tobias Weinzierl, Tobias Neckel
 */
class tarch::instrumentation::Watch {
private:
  /**
   * Flag to distinguish the (original) standard watch. Is used in the
   * destructor.
   */
  bool _plotResultInDestructor;

  /**
   * Stores the name of the operation the watch is used within.
   */
  std::string _operationName;

  /**
   * Holds the clock ticks at the beginning of the time measurement.
   */
  std::clock_t _startClockTicks;

  /**
   * Holds the wall clock time at the beginning of the measurement.
   */
  std::chrono::steady_clock::time_point _startTime;

  /**
   * Holds the elapsed processor time.
   */
  std::clock_t _elapsedClockTicks;

  /**
   * Holds the elapsed wall clock time.
   */
  std::chrono::steady_clock::duration _elapsedTime;

  /**
   * Is the watch currently running?
   */
  bool _isRunning;

public:
  /**
   * Construct a watch
   *
   * @param className     Retained for source compatibility; ignored.
   * @param operationName Name of the operation the watch is used within.
   * @param plotResultInDestructor Whether the destructor should log the result.
   */
  Watch(
    [[maybe_unused]] const std::string& className,
    const std::string&                  operationName,
    bool                                plotResultInDestructor,
    bool                                startToTickImmediately = true
  );

  /**
   * Stops the watch if necessary and logs the elapsed time if requested.
   */
  virtual ~Watch();

  /**
   * (Re)Start the Timer
   *
   * This method starts the timer. Actually, it restarts the time as the
   * constructor also invokes this operation. You don't have to call start()
   * after stop(). If you omit another call to start(), you just won't reset
   * the baseline, i.e., all getXXXTime() calls will still refer to the same
   * time zero when the Watch has been constructed.
   *
   * @see stopTimer()
   */
  void start();

  /**
   * Stop timer
   *
   * This method stops the timer. You may stop a timer multiple time because
   * the stop does not reset the start time. Please note that all the getters
   * return meaningful results if and only if you call stopTimer().
   */
  void stop();

  /**
   * Return CPU Time in Seconds
   *
   * This method returns the elapsed CPU time between the start and stop
   * command of the timer, i.e., the clock ticks actually spent by the process.
   * Take care: This counter might overflow, especially on a 32 bit
   * architecture.
   *
   * !!! Multithreading
   *
   * If you use multithreading, CPU time sums up the clock ticks invested by
   * the individual cores, i.e., if you switch from a one core to a dual core
   * machine, the CPU time should roughly double.
   *
   * @return CPU time in seconds
   */
  double getCPUTime() const;

  /**
   * Equals getCPUTime() but returns the clock ticks instead of the time in
   * seconds.
   */
  std::clock_t getCPUTicks() const;

  /**
   * This method returns the elapsed calendar time between the start and stop
   * command of the timer, i.e., the real world time. The result is given in
   * seconds.
   */
  double getCalendarTime() const;

  /**
   * This operation returns whether the watch is currently on. If the routine
   * returns false, it is currently stopped. The concept of on/off is a
   * logical concept. Internally, a watch simply administers system time
   * snapshots, while the get routines return differences of these snapshots.
   */
  bool isOn() const;
};
