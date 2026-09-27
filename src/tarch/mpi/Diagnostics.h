// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <mpi.h>

namespace tarch::mpi {
  class Diagnostics;

  std::string mpiReturnValueToString(int result);

  void checkReturnValue(int result, const char* operation, const std::source_location& location);
} // namespace tarch::mpi

/**
 * Warns about a wait on a request that exceeds the warning threshold and terminates the program
 * when a wait exceeds the deadlock threshold. A table records the origin, the partner and the tag
 * of every tracked non-blocking request, so a timeout report names the request. Every access
 * claims a table slot with a compare-and-swap on its sequence word and releases it with a second
 * increment.
 */
class tarch::mpi::Diagnostics final {
public:
  static constexpr int DeadlockExitCode = -2;

  static Diagnostics& getInstance();

  Diagnostics(const Diagnostics&)            = delete;
  Diagnostics(Diagnostics&&)                 = delete;
  Diagnostics& operator=(const Diagnostics&) = delete;
  Diagnostics& operator=(Diagnostics&&)      = delete;

  void setTimeoutWarning(double valueInSeconds);

  void setDeadlockTimeout(double valueInSeconds);

  [[nodiscard]] std::chrono::duration<double> getTimeoutWarning() const;

  [[nodiscard]] std::chrono::duration<double> getDeadlockTimeout() const;

  void setRequestTracking(bool enabled);

  [[nodiscard]] bool isRequestTrackingEnabled() const;

  [[nodiscard]] bool useTimeoutChecks() const;

  void plotMessageQueues();

  void registerRequest(
    const MPI_Request*          request,
    const char*                 operation,
    int                         communicationPartner,
    int                         tag,
    const std::source_location& location
  );

  void clearRequest(const MPI_Request* request);

  bool reportRequest(const MPI_Request* request, const char* waitOperation, const std::source_location& waitLocation);

private:
  using TimeoutPoint = std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>>;

  /**
   * An odd sequence marks a slot a writer holds, an even one a stable slot,
   * and a stable slot with a null key is free. The payload fields are not
   * atomic; the sequence orders every access to them.
   */
  struct Slot {
    std::atomic<std::uint32_t>      sequence{0};
    std::atomic<const MPI_Request*> key{nullptr};
    const char*                     originOperation{nullptr};
    std::source_location            originLocation{};
    int                             communicationPartner{-1};
    int                             tag{-1};
    bool                            waitingStarted{false};
    TimeoutPoint                    waitStartedAt{};
    TimeoutPoint                    nextWarningAt{};
  };

  /** Power of two, so the probe sequence wraps by masking. */
  static constexpr std::size_t TableSize = 1024;

  static Diagnostics _singleton;

  Diagnostics()  = default;
  ~Diagnostics() = default;

  static std::size_t getSlotIndex(const MPI_Request* request, std::size_t displacement);

  std::chrono::duration<double> _timeoutWarning{0};
  std::chrono::duration<double> _deadlockTimeout{0};

  bool _requestTrackingEnabled{false};

  std::array<Slot, TableSize> _slots{};

  /** Lets a completion skip the table walk while nothing is tracked. */
  std::atomic<int> _pendingCount{0};

  /**
   * Longest displacement any registration needed. A lookup stops after this
   * many probes, so a deletion may leave a hole behind.
   */
  std::atomic<std::size_t> _maxProbeDisplacement{0};
};
