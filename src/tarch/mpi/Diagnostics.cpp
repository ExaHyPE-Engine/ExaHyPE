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

tarch::mpi::Diagnostics&
tarch::mpi::Diagnostics::getInstance() {
  return _singleton;
}

std::string
tarch::mpi::mpiReturnValueToString(const int result) {
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

void
tarch::mpi::checkReturnValue(const int result, const char* operation, const std::source_location& location) {
  if (result == MPI_SUCCESS) {
    return;
  }

  std::ostringstream message;
  message << operation << " failed at " << locationToString(location) << ": "
          << tarch::mpi::mpiReturnValueToString(result);
  tarch::terminateWithError(message.str(), result, location);
}

void
tarch::mpi::Diagnostics::setTimeoutWarning(const double valueInSeconds) {
  assertion(valueInSeconds >= 0.0, valueInSeconds);
  _timeoutWarning = std::chrono::duration<double>(valueInSeconds);
}

void
tarch::mpi::Diagnostics::setDeadlockTimeout(const double valueInSeconds) {
  assertion(valueInSeconds >= 0.0, valueInSeconds);
  _deadlockTimeout = std::chrono::duration<double>(valueInSeconds);
}

std::chrono::duration<double>
tarch::mpi::Diagnostics::getTimeoutWarning() const {
  return _timeoutWarning;
}

std::chrono::duration<double>
tarch::mpi::Diagnostics::getDeadlockTimeout() const {
  return _deadlockTimeout;
}

void
tarch::mpi::Diagnostics::setRequestTracking(const bool enabled) {
  _requestTrackingEnabled = enabled;
}

bool
tarch::mpi::Diagnostics::isRequestTrackingEnabled() const {
  return _requestTrackingEnabled;
}

bool
tarch::mpi::Diagnostics::useTimeoutChecks() const {
  return tarch::mpi::Rank::getInstance().isInitialised()
    and (_timeoutWarning > std::chrono::seconds(0) or _deadlockTimeout > std::chrono::seconds(0));
}

/**
 * Logs whether a message waits in the incoming queue, so a timeout report shows a message that
 * arrived while the rank waits for a different one.
 */
void
tarch::mpi::Diagnostics::plotMessageQueues() {
  int        flag = 0;
  MPI_Status status;
  MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, tarch::mpi::Rank::getInstance().getCommunicator(), &flag, &status);
  if (flag == 0) {
    logError("The MPI queue is empty.");
  } else {
    logError(
      "The MPI queue still holds a message from rank " << status.MPI_SOURCE << " with tag " << status.MPI_TAG << "."
    );
  }
}

std::size_t
tarch::mpi::Diagnostics::getSlotIndex(const MPI_Request* request, const std::size_t displacement) {
  // Fibonacci hashing over the pointer bits above the allocation granularity.
  const auto address = reinterpret_cast<std::uintptr_t>(request);
  return (static_cast<std::size_t>(address >> 4) * 2654435761U + displacement) & (TableSize - 1U);
}

void
tarch::mpi::Diagnostics::registerRequest(
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
    Slot& slot = _slots[getSlotIndex(request, displacement)];

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
  // A full table drops the entry, so a timeout report cannot identify the request.
}

void
tarch::mpi::Diagnostics::clearRequest(const MPI_Request* request) {
  if (request == nullptr or _pendingCount.load(std::memory_order_acquire) == 0) {
    return;
  }

  const std::size_t maxDisplacement = _maxProbeDisplacement.load(std::memory_order_acquire);
  for (std::size_t displacement = 0; displacement <= maxDisplacement; displacement++) {
    Slot& slot = _slots[getSlotIndex(request, displacement)];
    if (slot.key.load(std::memory_order_acquire) != request) {
      continue;
    }

    std::uint32_t sequence = slot.sequence.load(std::memory_order_acquire);
    if (
      (sequence % 2U) != 0U
      or not slot.sequence.compare_exchange_strong(sequence, sequence + 1U, std::memory_order_acq_rel)
    ) {
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
 * Reports the pending request of a wait, warns after the warning threshold and terminates after
 * the deadlock threshold. Returns whether the table tracks the request. Only the thread that waits
 * on a request updates its deadlines, so a sequence claim contends only with the completion that
 * removes the entry.
 */
bool
tarch::mpi::Diagnostics::reportRequest(
  const MPI_Request*          request,
  const char*                 waitOperation,
  const std::source_location& waitLocation
) {
  if (
    request == nullptr or not _requestTrackingEnabled or not useTimeoutChecks()
    or _pendingCount.load(std::memory_order_acquire) == 0
  ) {
    return false;
  }

  const std::size_t maxDisplacement = _maxProbeDisplacement.load(std::memory_order_acquire);
  for (std::size_t displacement = 0; displacement <= maxDisplacement; displacement++) {
    Slot& slot = _slots[getSlotIndex(request, displacement)];
    if (slot.key.load(std::memory_order_acquire) != request) {
      continue;
    }

    std::uint32_t sequence = slot.sequence.load(std::memory_order_acquire);
    if (
      (sequence % 2U) != 0U
      or not slot.sequence.compare_exchange_strong(sequence, sequence + 1U, std::memory_order_acq_rel)
    ) {
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
