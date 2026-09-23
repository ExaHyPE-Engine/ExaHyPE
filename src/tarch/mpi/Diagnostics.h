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
 * Supervises the message exchange of one rank.
 *
 * A wait that exceeds the warning threshold reports itself, and one that
 * exceeds the deadlock threshold terminates the program. The pending-request
 * table records the origin, the partner and the tag of every non-blocking
 * request, so a timeout names the request it waits on, and the warnings and
 * the abort also work while the table is switched off.
 *
 * The table is a fixed array of seqlock-guarded slots: A writer claims a slot
 * by a compare-and-swap on the slot's sequence word and releases it with a
 * second increment, and a reader accepts a slot only when the sequence is even
 * and unchanged across its read.
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

  void setDeadlockWarningTimeStamp();

  void setDeadlockTimeoutTimeStamp();

  void writeTimeoutWarning(
    const std::string& className,
    const std::string& methodName,
    int                communicationPartnerRank,
    int                tag,
    int                numberOfExpectedMessages = 1
  );

  void triggerDeadlockTimeout(
    const std::string& className,
    const std::string& methodName,
    int                communicationPartnerRank,
    int                tag,
    int                numberOfExpectedMessages = 1,
    const std::string& comment                  = ""
  );

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

  static std::size_t slotIndex(const MPI_Request* request, std::size_t displacement);

  [[nodiscard]] bool exceededTimeoutWarningThreshold() const;

  [[nodiscard]] bool exceededDeadlockThreshold() const;

  std::chrono::duration<double> _timeoutWarning{0};
  std::chrono::duration<double> _deadlockTimeout{0};

  std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<double>> _globalTimeoutWarning{};
  std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<double>> _globalTimeoutDeadlock{};

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
