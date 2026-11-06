// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::instrumentation {
  class Watch;
} // namespace tarch::instrumentation

/**
 * Measures the CPU time and the wall clock time between start() and stop(). A watch created with
 * plotResultInDestructor logs both times when it leaves its scope.
 */
class tarch::instrumentation::Watch final {
public:
  Watch(
    [[maybe_unused]] const std::string& className,
    const std::string&                  operationName,
    bool                                plotResultInDestructor,
    bool                                startToTickImmediately = true
  );

  Watch(const Watch&)            = delete;
  Watch(Watch&&)                 = delete;
  Watch& operator=(const Watch&) = delete;
  Watch& operator=(Watch&&)      = delete;
  ~Watch();

  void start();
  void stop();
  [[nodiscard]] double getCPUTime() const;
  [[nodiscard]] double getCalendarTime() const;

private:
  bool                                  _plotResultInDestructor{false};
  std::string                           _operationName{};
  std::clock_t                          _startClockTicks{0};
  std::chrono::steady_clock::time_point _startTime{};
  std::clock_t                          _elapsedClockTicks{0};
  std::chrono::steady_clock::duration   _elapsedTime{};
  bool                                  _isRunning{false};
};
