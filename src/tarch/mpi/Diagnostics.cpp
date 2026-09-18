// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Diagnostics.h"

#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Error.h"
#include "tarch/instrumentation/Warning.h"
#include "tarch/mpi/Rank.h"

namespace {
  std::string locationToString(const std::source_location& location) {
    std::ostringstream out;
    out << location.file_name() << ":" << location.line() << " (" << location.function_name() << ")";
    return out.str();
  }
} // namespace

tarch::mpi::Diagnostics tarch::mpi::Diagnostics::_singleton;

tarch::mpi::Diagnostics& tarch::mpi::Diagnostics::getInstance() { return _singleton; }

std::string tarch::mpi::mpiReturnValueToString(const int result) {
  std::ostringstream out;

  int                                    resultLength{0};
  std::array<char, MPI_MAX_ERROR_STRING> errorString{};
  MPI_Error_string(result, errorString.data(), &resultLength);

  int errorClass{MPI_ERR_UNKNOWN};
  MPI_Error_class(result, &errorClass);

  out << "MPI error class: " << errorClass << ", MPI error name: ";

  switch (errorClass) {
  case MPI_SUCCESS:
    out << "MPI_SUCCESS [no error]";
    break;
  case MPI_ERR_BUFFER:
    out << "MPI_ERR_BUFFER [invalid buffer pointer]";
    break;
  case MPI_ERR_COUNT:
    out << "MPI_ERR_COUNT [invalid count argument]";
    break;
  case MPI_ERR_TYPE:
    out << "MPI_ERR_TYPE [invalid datatype]";
    break;
  case MPI_ERR_TAG:
    out << "MPI_ERR_TAG [invalid tag]";
    break;
  case MPI_ERR_COMM:
    out << "MPI_ERR_COMM [invalid communicator]";
    break;
  case MPI_ERR_RANK:
    out << "MPI_ERR_RANK [invalid rank]";
    break;
  case MPI_ERR_REQUEST:
    out << "MPI_ERR_REQUEST [invalid request handle]";
    break;
  case MPI_ERR_ROOT:
    out << "MPI_ERR_ROOT [invalid root argument]";
    break;
  case MPI_ERR_GROUP:
    out << "MPI_ERR_GROUP [invalid group]";
    break;
  case MPI_ERR_OP:
    out << "MPI_ERR_OP [invalid operation]";
    break;
  case MPI_ERR_TOPOLOGY:
    out << "MPI_ERR_TOPOLOGY [invalid topology]";
    break;
  case MPI_ERR_DIMS:
    out << "MPI_ERR_DIMS [invalid dimensions]";
    break;
  case MPI_ERR_ARG:
    out << "MPI_ERR_ARG [invalid argument]";
    break;
  case MPI_ERR_UNKNOWN:
    out << "MPI_ERR_UNKNOWN [unknown error]";
    break;
  case MPI_ERR_TRUNCATE:
    out << "MPI_ERR_TRUNCATE [message has been truncated by receiver]";
    break;
  case MPI_ERR_OTHER:
    out << "MPI_ERR_OTHER [other unknown error]";
    break;
  case MPI_ERR_INTERN:
    out << "MPI_ERR_INTERN [internal MPI error]";
    break;
  default:
    out << "Unknown MPI error";
    break;
  }
  out << ", MPI error text: " << errorString.data() << ".";

  return out.str();
}

void tarch::mpi::checkReturnValue(const int result, const char* operation, const std::source_location& location) {
  if (result == MPI_SUCCESS) {
    return;
  }

  std::ostringstream message;
  message << operation << " failed at " << locationToString(location) << ": "
          << tarch::mpi::mpiReturnValueToString(result);
  tarch::terminateWithError(message.str(), result, location);
}

void tarch::mpi::Diagnostics::setTimeoutWarning(const double valueInSeconds) {
  assertion(valueInSeconds >= 0.0, valueInSeconds);
  _timeoutWarning = std::chrono::duration<double>(valueInSeconds);
}

void tarch::mpi::Diagnostics::setDeadlockTimeout(const double valueInSeconds) {
  assertion(valueInSeconds >= 0.0, valueInSeconds);
  _deadlockTimeout = std::chrono::duration<double>(valueInSeconds);
}

std::chrono::duration<double> tarch::mpi::Diagnostics::getTimeoutWarning() const { return _timeoutWarning; }

std::chrono::duration<double> tarch::mpi::Diagnostics::getDeadlockTimeout() const { return _deadlockTimeout; }

void tarch::mpi::Diagnostics::setRequestTracking(const bool enabled) { _requestTrackingEnabled = enabled; }

bool tarch::mpi::Diagnostics::isRequestTrackingEnabled() const { return _requestTrackingEnabled; }

bool tarch::mpi::Diagnostics::useTimeoutChecks() const {
  return tarch::mpi::Rank::getInstance().isInitialised()
    and (_timeoutWarning > std::chrono::seconds(0) or _deadlockTimeout > std::chrono::seconds(0));
}

void tarch::mpi::Diagnostics::setDeadlockWarningTimeStamp() {
  _globalTimeoutWarning = std::chrono::system_clock::now() + _timeoutWarning;
}

void tarch::mpi::Diagnostics::setDeadlockTimeoutTimeStamp() {
  _globalTimeoutDeadlock = std::chrono::system_clock::now() + _deadlockTimeout;
}

bool tarch::mpi::Diagnostics::exceededTimeoutWarningThreshold() const {
  return _timeoutWarning > std::chrono::seconds(0) and std::chrono::system_clock::now() > _globalTimeoutWarning;
}

bool tarch::mpi::Diagnostics::exceededDeadlockThreshold() const {
  return _deadlockTimeout > std::chrono::seconds(0) and std::chrono::system_clock::now() > _globalTimeoutDeadlock;
}

/**
 * Writes a warning once the warning threshold has passed. Every warning moves
 * the next warning timestamp forward and widens the warning interval, so a
 * long wait does not flood the terminal; the growth stops at half the
 * deadlock timeout so the abort still announces itself.
 */
void tarch::mpi::Diagnostics::writeTimeoutWarning(
  const std::string& className,
  const std::string& methodName,
  const int          communicationPartnerRank,
  const int          tag,
  const int          numberOfExpectedMessages
) {
  if (exceededTimeoutWarningThreshold()) {
    logWarning(
      "Operation " << className << "::" << methodName << " on rank " << tarch::mpi::Rank::getInstance().getRank()
                   << " had to wait more than " << _timeoutWarning.count() << " seconds for "
                   << numberOfExpectedMessages << " message(s) from rank " << communicationPartnerRank << " with tag "
                   << tag << "."
    );

    if (_deadlockTimeout.count() > 0.0) {
      logWarning("The application terminates after " << _deadlockTimeout.count() << " seconds because of a deadlock.");
    } else {
      logWarning(
        "Deadlock detection is switched off as the deadlock timeout is set to "
        << _deadlockTimeout.count() << ", so warnings continue but the application does not shut down."
      );
    }

    if (_timeoutWarning < _deadlockTimeout / 2 or _deadlockTimeout.count() <= 0.0) {
      const std::chrono::duration<double> newTimeoutWarning = _timeoutWarning * 2;
      logWarning(
        "The timeout warning threshold grows from " << _timeoutWarning.count() << " seconds to "
                                                    << newTimeoutWarning.count()
                                                    << " seconds to avoid a flood of warnings."
      );
      _timeoutWarning       = newTimeoutWarning;
      _globalTimeoutWarning = _globalTimeoutWarning + _timeoutWarning;
    }
  }
}

/**
 * Terminates the program once the deadlock threshold has passed. The
 * termination calls terminateWithError(), so every rank stops, and
 * plotMessageQueues() runs first so the report names the messages that are
 * still in the queue.
 */
void tarch::mpi::Diagnostics::triggerDeadlockTimeout(
  const std::string& className,
  const std::string& methodName,
  const int          communicationPartnerRank,
  const int          tag,
  const int          numberOfExpectedMessages,
  const std::string& comment
) {
  if (exceededDeadlockThreshold()) {
    std::ostringstream out;
    out << "Operation " << className << "::" << methodName << " on node " << tarch::mpi::Rank::getInstance().getRank()
        << " had to wait more than " << _deadlockTimeout.count() << " seconds for " << numberOfExpectedMessages
        << " message(s) from node " << communicationPartnerRank << " with tag " << tag << ". Timeout. " << comment;
    plotMessageQueues();
    tarch::terminateWithError(out.str(), DeadlockExitCode);
  }
}

/**
 * Reports what is left in the incoming queue, so a timeout names the message
 * that arrives while a rank waits for a different one.
 */
void tarch::mpi::Diagnostics::plotMessageQueues() {
  int        flag = 0;
  MPI_Status status;
  MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, tarch::mpi::Rank::getInstance().getCommunicator(), &flag, &status);
  if (flag == 0) {
    logError("There are no messages from any sender in the MPI queue.");
  } else {
    logError(
      "There is still a message in the queue from rank " << status.MPI_SOURCE << " with tag " << status.MPI_TAG << "."
    );
  }
}

std::size_t tarch::mpi::Diagnostics::slotIndex(const MPI_Request* request, const std::size_t displacement) {
  // Fibonacci hashing over the pointer bits above the allocation granularity.
  const auto address = reinterpret_cast<std::uintptr_t>(request);
  return (static_cast<std::size_t>(address >> 4) * 2654435761U + displacement) & (TableSize - 1U);
}

void tarch::mpi::Diagnostics::registerRequest(
  const MPI_Request*          request,
  const char*                 operation,
  const int                   communicationPartner,
  const int                   tag,
  const std::source_location& location
) {
  if (not _requestTrackingEnabled or not useTimeoutChecks() or request == nullptr or *request == MPI_REQUEST_NULL) {
    return;
  }

  for (std::size_t displacement = 0; displacement < TableSize; displacement++) {
    Slot& slot = _slots[slotIndex(request, displacement)];

    std::uint32_t sequence = slot.sequence.load(std::memory_order_acquire);
    if ((sequence % 2U) != 0U or slot.key.load(std::memory_order_acquire) != nullptr) {
      continue;
    }
    if (not slot.sequence.compare_exchange_strong(sequence, sequence + 1U, std::memory_order_acq_rel)) {
      continue;
    }
    if (slot.key.load(std::memory_order_relaxed) != nullptr) {
      slot.sequence.store(sequence + 2U, std::memory_order_release);
      continue;
    }

    slot.originOperation      = operation;
    slot.originLocation       = location;
    slot.communicationPartner = communicationPartner;
    slot.tag                  = tag;
    slot.waitingStarted       = false;
    slot.key.store(request, std::memory_order_release);
    slot.sequence.store(sequence + 2U, std::memory_order_release);
    _pendingCount.fetch_add(1, std::memory_order_release);

    std::size_t knownMaximum = _maxProbeDisplacement.load(std::memory_order_relaxed);
    while (displacement > knownMaximum
           and not _maxProbeDisplacement.compare_exchange_weak(knownMaximum, displacement, std::memory_order_relaxed)) {
    }
    return;
  }
  // A full table drops the entry; the request then stays unnamed in a timeout report.
}

void tarch::mpi::Diagnostics::clearRequest(const MPI_Request* request) {
  if (request == nullptr or _pendingCount.load(std::memory_order_acquire) == 0) {
    return;
  }

  const std::size_t maxDisplacement = _maxProbeDisplacement.load(std::memory_order_acquire);
  for (std::size_t displacement = 0; displacement <= maxDisplacement; displacement++) {
    Slot& slot = _slots[slotIndex(request, displacement)];
    if (slot.key.load(std::memory_order_acquire) != request) {
      continue;
    }

    std::uint32_t sequence = slot.sequence.load(std::memory_order_acquire);
    if ((sequence % 2U) != 0U
        or not slot.sequence.compare_exchange_strong(sequence, sequence + 1U, std::memory_order_acq_rel)) {
      continue;
    }
    if (slot.key.load(std::memory_order_relaxed) == request) {
      slot.key.store(nullptr, std::memory_order_release);
      slot.sequence.store(sequence + 2U, std::memory_order_release);
      _pendingCount.fetch_sub(1, std::memory_order_release);
      return;
    }
    slot.sequence.store(sequence + 2U, std::memory_order_release);
  }
}

/**
 * Names the pending request a wait hangs on, warns once the warning threshold
 * passes and terminates once the deadlock threshold passes. Returns whether
 * the request sits in the table, so a caller can fall back to a generic report
 * for an untracked request. Only the thread that waits on a request updates
 * its deadlines, so the slot's sequence claim only ever contends with the
 * completion that removes the entry.
 */
bool tarch::mpi::Diagnostics::reportRequest(
  const MPI_Request*          request,
  const char*                 waitOperation,
  const std::source_location& waitLocation
) {
  if (request == nullptr or not _requestTrackingEnabled or not useTimeoutChecks()
      or _pendingCount.load(std::memory_order_acquire) == 0) {
    return false;
  }

  const std::size_t maxDisplacement = _maxProbeDisplacement.load(std::memory_order_acquire);
  for (std::size_t displacement = 0; displacement <= maxDisplacement; displacement++) {
    Slot& slot = _slots[slotIndex(request, displacement)];
    if (slot.key.load(std::memory_order_acquire) != request) {
      continue;
    }

    std::uint32_t sequence = slot.sequence.load(std::memory_order_acquire);
    if ((sequence % 2U) != 0U
        or not slot.sequence.compare_exchange_strong(sequence, sequence + 1U, std::memory_order_acq_rel)) {
      // The completion is removing the entry, so there is nothing left to report.
      return true;
    }
    if (slot.key.load(std::memory_order_relaxed) != request) {
      slot.sequence.store(sequence + 2U, std::memory_order_release);
      continue;
    }

    const auto now = std::chrono::steady_clock::now();
    if (not slot.waitingStarted) {
      slot.waitingStarted = true;
      slot.waitStartedAt  = now;
      slot.nextWarningAt  = now + _timeoutWarning;
    }

    const bool warnNow = _timeoutWarning > std::chrono::seconds(0) and now >= slot.nextWarningAt;
    if (warnNow) {
      slot.nextWarningAt = now + _timeoutWarning;
    }
    const bool abortNow = _deadlockTimeout > std::chrono::seconds(0) and now - slot.waitStartedAt >= _deadlockTimeout;

    const char* const          originOperation = slot.originOperation;
    const std::source_location originLocation  = slot.originLocation;
    const int                  partner         = slot.communicationPartner;
    const int                  tag             = slot.tag;

    slot.sequence.store(sequence + 2U, std::memory_order_release);

    if (warnNow) {
      logWarning(
        waitOperation << " at " << locationToString(waitLocation) << " still waits for request from " << originOperation
                      << " posted at " << locationToString(originLocation) << " (partner=" << partner << ", tag=" << tag
                      << ")"
      );
    }
    if (abortNow) {
      std::ostringstream message;
      message << waitOperation << " at " << locationToString(waitLocation) << " deadlocked on request from "
              << originOperation << " posted at " << locationToString(originLocation) << " (partner=" << partner
              << ", tag=" << tag << ") after " << _deadlockTimeout.count() << " second(s)";
      plotMessageQueues();
      tarch::terminateWithError(message.str(), DeadlockExitCode, waitLocation);
    }
    return true;
  }

  return false;
}
