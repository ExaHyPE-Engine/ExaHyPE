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
 * Warns when a wait on a request exceeds the warning threshold and terminates the program when it
 * exceeds the deadlock threshold. A table records the origin, the partner and the tag of every
 * tracked request for the timeout report.
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

  static constexpr std::size_t TableSize = 1024;

  static Diagnostics _singleton;

  Diagnostics()  = default;
  ~Diagnostics() = default;

  static std::size_t getSlotIndex(const MPI_Request* request, std::size_t displacement);

  std::chrono::duration<double> _timeoutWarning{0};
  std::chrono::duration<double> _deadlockTimeout{0};

  bool _requestTrackingEnabled{false};

  std::array<Slot, TableSize> _slots{};

  std::atomic<int> _pendingCount{0};

  std::atomic<std::size_t> _maxProbeDisplacement{0};
};
