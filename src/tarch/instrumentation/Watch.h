// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

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
 * @author Tobias Weinzierl, Tobias Neckel
 */
class tarch::instrumentation::Watch {
private:
  /**
   * Whether the destructor logs the measured times.
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
   * Is the watch running?
   */
  bool _isRunning;

public:
  /**
   * Construct a watch
   *
   * @param className     Unused.
   * @param operationName Name of the operation the watch is used within.
   * @param plotResultInDestructor Whether the destructor should log the result.
   * @param startToTickImmediately Whether the constructor already starts the
   *        measurement.
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
   * (Re)start the timer
   *
   * @see stop()
   */
  void start();

  /**
   * Stop the timer
   *
   * @see start()
   */
  void stop();

  /**
   * @return CPU time in seconds
   */
  double getCPUTime() const;

  /**
   * @return Elapsed calendar time in seconds
   */
  double getCalendarTime() const;
};
