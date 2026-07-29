// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <mpi.h>

namespace tarch::mpi {
  class Diagnostics;

  std::string MPIReturnValueToString(int result);

  void checkReturnValue(int result, const char* operation, const std::source_location& location);
} // namespace tarch::mpi

/**
 * Supervises the message exchange of one rank.
 *
 * The class owns the timeout policy: a wait that exceeds the warning threshold
 * reports itself, and one that exceeds the deadlock threshold terminates the
 * whole application, so a wedged job becomes a named failure instead of
 * burning its allocation. It also holds the pending-request table, which
 * records origin, partner and tag of every non-blocking request so that a
 * timeout can name the request it hangs on. The warnings and the abort work
 * without the table; it only adds the naming and is therefore reserved for a
 * run that hunts a hang.
 *
 * The table is a fixed array of seqlock-guarded slots. A writer claims a slot
 * by a compare-and-swap on the slot's sequence word and publishes it back with
 * a second increment, and a reader accepts a slot only when the sequence is
 * even and unchanged across its read. No thread ever blocks another, which
 * keeps the request path free of locks: every send, receive and test of every
 * thread passes through this table when tracking is on.
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

  std::chrono::duration<double> getTimeoutWarning() const;

  std::chrono::duration<double> getDeadlockTimeout() const;

  void setRequestTracking(bool enabled);

  bool isRequestTrackingEnabled() const;

  bool useTimeoutChecks() const;

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
   * and a stable slot with a null key is free. The payload fields carry no
   * atomicity of their own; the sequence protocol is what makes them safe.
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

  Diagnostics() = default;
  ~Diagnostics() = default;

  static std::size_t slotIndex(const MPI_Request* request, std::size_t displacement);

  bool exceededTimeoutWarningThreshold() const;

  bool exceededDeadlockThreshold() const;

  std::chrono::duration<double> _timeoutWarning{0};
  std::chrono::duration<double> _deadlockTimeout{0};

  std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<double>> _globalTimeoutWarning{};
  std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<double>> _globalTimeoutDeadlock{};

  bool _requestTrackingEnabled{false};

  std::array<Slot, TableSize> _slots{};

  /** Lets a completion skip the table walk while nothing is tracked. */
  std::atomic<int> _pendingCount{0};

  /**
   * Longest displacement any registration ever needed. A lookup stops after
   * this many probes, so deletions can leave holes without a lookup having to
   * scan the whole table past them.
   */
  std::atomic<std::size_t> _maxProbeDisplacement{0};
};
