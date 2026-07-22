// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "mpi.h"

#include "tarch/Termination.h"
#include "tarch/instrumentation/Warning.h"
#include "tarch/mpi/Rank.h"
#include "tarch/services/ServiceRepository.h"

#ifdef PEANO_WITH_MPI
namespace {
  // A steady-clock time point that can hold a fractional-seconds deadline, so the
  // timeouts can be set to sub-second values such as 0.1s.
  using TimeoutPoint = std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>>;

  struct PendingRequest {
    const char*          originOperation = nullptr;
    std::source_location originLocation;
    int                  communicationPartner = -1;
    int                  tag                  = -1;
    bool                 waitingStarted       = false;
    TimeoutPoint         waitStartedAt;
    TimeoutPoint         nextWarningAt;
  };

  bool useTimeoutChecks() {
    auto& rank = tarch::mpi::Rank::getInstance();
    return rank.isInitialised() and rank.areTimeoutsEnabled()
      and (rank.getTimeoutWarning() > std::chrono::seconds(0) or rank.getDeadlockTimeout() > std::chrono::seconds(0));
  }

  bool useDetailedDeadlockDetection() {
    return useTimeoutChecks() and tarch::mpi::Rank::getInstance().isDeadlockDetectionEnabled();
  }

  std::mutex                                             PendingRequestSemaphore;
  std::unordered_map<const MPI_Request*, PendingRequest> PendingRequests;
  std::atomic<bool>                                      HaveTrackedRequests(false);

  std::string toString(const std::source_location& location) {
    std::ostringstream out;
    out << location.file_name() << ":" << location.line() << " (" << location.function_name() << ")";
    return out.str();
  }

  void checkReturnValue(int result, const char* operation, const std::source_location& location) {
    if (result == MPI_SUCCESS) {
      return;
    }

    std::ostringstream message;
    message << operation << " failed at " << toString(location) << ": " << tarch::mpi::MPIReturnValueToString(result);
    tarch::terminateWithError(message.str(), result, location);
  }

  void registerPendingRequest(
    MPI_Request*                request,
    const char*                 operation,
    int                         communicationPartner,
    int                         tag,
    const std::source_location& location
  ) {
    if (not useDetailedDeadlockDetection() or request == nullptr or *request == MPI_REQUEST_NULL) {
      return;
    }

    std::lock_guard<std::mutex> lock(PendingRequestSemaphore);
    PendingRequest              entry;
    entry.originOperation      = operation;
    entry.originLocation       = location;
    entry.communicationPartner = communicationPartner;
    entry.tag                  = tag;
    entry.waitingStarted       = false;
    PendingRequests[request]   = entry;
    HaveTrackedRequests.store(true, std::memory_order_release);
  }

  void clearPendingRequest(MPI_Request* request) {
    if (request == nullptr or not HaveTrackedRequests.load(std::memory_order_acquire)) {
      return;
    }

    std::lock_guard<std::mutex> lock(PendingRequestSemaphore);
    PendingRequests.erase(request);
    if (PendingRequests.empty()) {
      HaveTrackedRequests.store(false, std::memory_order_release);
    }
  }

  bool reportPendingRequest(MPI_Request* request, const char* waitOperation, const std::source_location& waitLocation) {
    if (request == nullptr) {
      return false;
    }

    auto& rank = tarch::mpi::Rank::getInstance();
    if (not useDetailedDeadlockDetection()) {
      return false;
    }

    std::lock_guard<std::mutex> lock(PendingRequestSemaphore);
    auto                        pending = PendingRequests.find(request);
    if (pending == PendingRequests.end()) {
      return false;
    }

    auto& entry = pending->second;
    auto  now   = std::chrono::steady_clock::now();

    if (not entry.waitingStarted) {
      entry.waitingStarted = true;
      entry.waitStartedAt  = now;
      entry.nextWarningAt  = now + rank.getTimeoutWarning();
    }

    if (rank.getTimeoutWarning() > std::chrono::seconds(0) and now >= entry.nextWarningAt) {
      logWarning(
        waitOperation << " at " << toString(waitLocation) << " still waits for request from " << entry.originOperation
                      << " posted at " << toString(entry.originLocation) << " (partner=" << entry.communicationPartner
                      << ", tag=" << entry.tag << ")"
      );
      entry.nextWarningAt = now + rank.getTimeoutWarning();
    }

    if (rank.getDeadlockTimeout() > std::chrono::seconds(0)
        and now - entry.waitStartedAt >= rank.getDeadlockTimeout()) {
      std::ostringstream message;
      message << waitOperation << " at " << toString(waitLocation) << " deadlocked on request from "
              << entry.originOperation << " posted at " << toString(entry.originLocation)
              << " (partner=" << entry.communicationPartner << ", tag=" << entry.tag << ") after "
              << rank.getDeadlockTimeout().count() << " second(s)";
      rank.plotMessageQueues();
      tarch::terminateWithError(message.str(), tarch::mpi::Rank::DeadlockExitCode, waitLocation);
    }

    return true;
  }

  void reportRequestWait(
    const char*                 operation,
    const std::source_location& location,
    TimeoutPoint&               startedWaiting,
    TimeoutPoint&               nextWarning,
    bool&                       waitingStarted
  ) {
    auto& rank = tarch::mpi::Rank::getInstance();
    if (not useTimeoutChecks()) {
      return;
    }

    auto now = std::chrono::steady_clock::now();
    if (not waitingStarted) {
      waitingStarted = true;
      startedWaiting = now;
      nextWarning    = now + rank.getTimeoutWarning();
    }

    if (rank.getTimeoutWarning() > std::chrono::seconds(0) and now >= nextWarning) {
      logWarning(operation << " at " << toString(location) << " still waits for MPI request");
      nextWarning = now + rank.getTimeoutWarning();
    }

    if (rank.getDeadlockTimeout() > std::chrono::seconds(0) and now - startedWaiting >= rank.getDeadlockTimeout()) {
      std::ostringstream message;
      message << operation << " at " << toString(location) << " deadlocked while waiting for MPI request after "
              << rank.getDeadlockTimeout().count() << " second(s)";
      rank.plotMessageQueues();
      tarch::terminateWithError(message.str(), tarch::mpi::Rank::DeadlockExitCode, location);
    }
  }

  void reportBlockingOperation(
    const char*                 operation,
    const std::source_location& location,
    int                         communicationPartner,
    int                         tag,
    TimeoutPoint&               startedWaiting,
    TimeoutPoint&               nextWarning,
    bool&                       waitingStarted
  ) {
    auto& rank = tarch::mpi::Rank::getInstance();
    if (not useTimeoutChecks()) {
      return;
    }

    auto now = std::chrono::steady_clock::now();
    if (not waitingStarted) {
      waitingStarted = true;
      startedWaiting = now;
      nextWarning    = now + rank.getTimeoutWarning();
    }

    if (rank.getTimeoutWarning() > std::chrono::seconds(0) and now >= nextWarning) {
      logWarning(
        operation << " at " << toString(location) << " still waits for partner=" << communicationPartner
                  << " and tag=" << tag
      );
      nextWarning = now + rank.getTimeoutWarning();
    }

    if (rank.getDeadlockTimeout() > std::chrono::seconds(0) and now - startedWaiting >= rank.getDeadlockTimeout()) {
      std::ostringstream message;
      message << operation << " at " << toString(location)
              << " deadlocked while waiting for partner=" << communicationPartner << " and tag=" << tag << " after "
              << rank.getDeadlockTimeout().count() << " second(s)";
      rank.plotMessageQueues();
      tarch::terminateWithError(message.str(), tarch::mpi::Rank::DeadlockExitCode, location);
    }
  }

  int waitForRequest(
    MPI_Request*                 request,
    MPI_Status*                  status,
    const char*                  waitOperation,
    const std::function<void()>& progressFunctor,
    bool                         useDefaultProgress,
    const std::source_location&  location
  ) {
    int          flag           = 0;
    bool         waitingStarted = false;
    TimeoutPoint startedWaiting;
    TimeoutPoint nextWarning;
    while (not flag) {
      tarch::mpi::test(request, &flag, status, tarch::mpi::RequestTestMode::TestOnly, location);
      if (not flag) {
        if (not reportPendingRequest(request, waitOperation, location)) {
          reportRequestWait(waitOperation, location, startedWaiting, nextWarning, waitingStarted);
        }

        if (progressFunctor) {
          progressFunctor();
        } else if (useDefaultProgress) {
          tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
        }
      }
    }

    clearPendingRequest(request);
    return MPI_SUCCESS;
  }
} // namespace

std::string tarch::mpi::MPIReturnValueToString(int result) {
  std::ostringstream out;

  int   resultlen;
  char* string = new char[MPI_MAX_ERROR_STRING];
  PMPI_Error_string(result, string, &resultlen);

  int errorclass;
  PMPI_Error_class(result, &errorclass);

  out << "mpi error class: " << errorclass << "="
      << ", mpi error text: " << string;

  switch (errorclass) {
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
    out << "MPI_ERR_INTERN [internal mpi error]";
    break;
  default:
    out << "unknown";
    break;
  }

  delete[] string;

  return out.str();
}

std::string tarch::mpi::MPIStatusToString(const MPI_Status& status) {
  std::ostringstream out;
  out << "status flag:"
      << " MPI_ERROR=" << status.MPI_ERROR << " (" << MPIReturnValueToString(status.MPI_ERROR)
      << ") ,MPI_SOURCE=" << status.MPI_SOURCE << ",MPI_TAG=" << status.MPI_TAG;
  return out.str();
}

int tarch::mpi::isend(
  const void*          buffer,
  int                  count,
  MPI_Datatype         datatype,
  int                  destination,
  int                  tag,
  MPI_Comm             communicator,
  MPI_Request*         request,
  std::source_location location
) {
  int result = PMPI_Isend(buffer, count, datatype, destination, tag, communicator, request);
  checkReturnValue(result, "MPI_Isend", location);
  registerPendingRequest(request, "MPI_Isend", destination, tag, location);
  return result;
}

int tarch::mpi::irecv(
  void*                buffer,
  int                  count,
  MPI_Datatype         datatype,
  int                  source,
  int                  tag,
  MPI_Comm             communicator,
  MPI_Request*         request,
  std::source_location location
) {
  int result = PMPI_Irecv(buffer, count, datatype, source, tag, communicator, request);
  checkReturnValue(result, "MPI_Irecv", location);
  registerPendingRequest(request, "MPI_Irecv", source, tag, location);
  return result;
}

int tarch::mpi::test(
  MPI_Request*         request,
  int*                 flag,
  MPI_Status*          status,
  RequestTestMode      mode,
  std::source_location location
) {
  int result = PMPI_Test(request, flag, status);
  checkReturnValue(result, "MPI_Test", location);

  if (flag != nullptr and *flag) {
    clearPendingRequest(request);
  } else if (mode == RequestTestMode::WaitForCompletion and useDetailedDeadlockDetection()) {
    reportPendingRequest(request, "MPI_Test", location);
  }

  return result;
}

int tarch::mpi::wait(MPI_Request* request, MPI_Status* status, std::source_location location) {
  // Always pump the default progress functor on the wait loop under MPI; only the warn/abort
  // bookkeeping is gated on the timeouts (inside waitForRequest). This keeps MPI progress and the
  // tree-management handshake alive even with timeouts disabled (a plain PMPI_Wait would not pump).
  return waitForRequest(request, status, "MPI_Wait", std::function<void()>(), true, location);
}

int tarch::mpi::wait(
  MPI_Request*          request,
  std::function<void()> progressFunctor,
  MPI_Status*           status,
  std::source_location  location
) {
  if (not progressFunctor) {
    return wait(request, status, location);
  }

  return waitForRequest(request, status, "MPI_Wait", progressFunctor, false, location);
}

int tarch::mpi::send(
  const void*          buffer,
  int                  count,
  MPI_Datatype         datatype,
  int                  destination,
  int                  tag,
  MPI_Comm             communicator,
  std::source_location location
) {
  MPI_Request request = MPI_REQUEST_NULL;
  isend(buffer, count, datatype, destination, tag, communicator, &request, location);
  return wait(&request, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::receive(
  void*                buffer,
  int                  count,
  MPI_Datatype         datatype,
  int                  source,
  int                  tag,
  MPI_Comm             communicator,
  MPI_Status*          status,
  std::source_location location
) {
  MPI_Request request = MPI_REQUEST_NULL;
  irecv(buffer, count, datatype, source, tag, communicator, &request, location);
  return wait(&request, status, location);
}

int tarch::mpi::sendrecv(
  const void*          sendBuffer,
  int                  sendCount,
  MPI_Datatype         sendDatatype,
  int                  destination,
  int                  sendTag,
  void*                receiveBuffer,
  int                  receiveCount,
  MPI_Datatype         receiveDatatype,
  int                  source,
  int                  receiveTag,
  MPI_Comm             communicator,
  MPI_Status*          status,
  std::source_location location
) {
  MPI_Request receiveRequest = MPI_REQUEST_NULL;
  MPI_Request sendRequest    = MPI_REQUEST_NULL;

  irecv(receiveBuffer, receiveCount, receiveDatatype, source, receiveTag, communicator, &receiveRequest, location);
  isend(sendBuffer, sendCount, sendDatatype, destination, sendTag, communicator, &sendRequest, location);

  wait(&sendRequest, MPI_STATUS_IGNORE, location);
  return wait(&receiveRequest, status, location);
}

int tarch::mpi::cancel(MPI_Request* request, std::source_location location) {
  int result = PMPI_Cancel(request);
  checkReturnValue(result, "MPI_Cancel", location);
  return result;
}

int tarch::mpi::ibarrier(MPI_Comm communicator, MPI_Request* request, std::source_location location) {
  int result = PMPI_Ibarrier(communicator, request);
  checkReturnValue(result, "MPI_Ibarrier", location);
  registerPendingRequest(request, "MPI_Ibarrier", -1, -1, location);
  return result;
}

int tarch::mpi::barrier(MPI_Comm communicator, std::source_location location) {
  MPI_Request request = MPI_REQUEST_NULL;
  ibarrier(communicator, &request, location);
  return wait(&request, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::barrier(MPI_Comm communicator, std::function<void()> progressFunctor, std::source_location location) {
  if (not progressFunctor) {
    return barrier(communicator, location);
  }

  MPI_Request request = MPI_REQUEST_NULL;
  ibarrier(communicator, &request, location);
  return wait(&request, progressFunctor, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::iallreduce(
  const void*          sendBuffer,
  void*                receiveBuffer,
  int                  count,
  MPI_Datatype         datatype,
  MPI_Op               operation,
  MPI_Comm             communicator,
  MPI_Request*         request,
  std::source_location location
) {
  int result = PMPI_Iallreduce(sendBuffer, receiveBuffer, count, datatype, operation, communicator, request);
  checkReturnValue(result, "MPI_Iallreduce", location);
  registerPendingRequest(request, "MPI_Iallreduce", -1, -1, location);
  return result;
}

int tarch::mpi::allreduce(
  const void*          sendBuffer,
  void*                receiveBuffer,
  int                  count,
  MPI_Datatype         datatype,
  MPI_Op               operation,
  MPI_Comm             communicator,
  std::source_location location
) {
  MPI_Request request = MPI_REQUEST_NULL;
  iallreduce(sendBuffer, receiveBuffer, count, datatype, operation, communicator, &request, location);
  return wait(&request, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::allreduce(
  const void*           sendBuffer,
  void*                 receiveBuffer,
  int                   count,
  MPI_Datatype          datatype,
  MPI_Op                operation,
  MPI_Comm              communicator,
  std::function<void()> progressFunctor,
  std::source_location  location
) {
  if (not progressFunctor) {
    return allreduce(sendBuffer, receiveBuffer, count, datatype, operation, communicator, location);
  }

  MPI_Request request = MPI_REQUEST_NULL;
  iallreduce(sendBuffer, receiveBuffer, count, datatype, operation, communicator, &request, location);
  return wait(&request, progressFunctor, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::iallgather(
  const void*          sendBuffer,
  int                  sendCount,
  MPI_Datatype         sendDatatype,
  void*                receiveBuffer,
  int                  receiveCount,
  MPI_Datatype         receiveDatatype,
  MPI_Comm             communicator,
  MPI_Request*         request,
  std::source_location location
) {
  int result = PMPI_Iallgather(
    sendBuffer,
    sendCount,
    sendDatatype,
    receiveBuffer,
    receiveCount,
    receiveDatatype,
    communicator,
    request
  );
  checkReturnValue(result, "MPI_Iallgather", location);
  registerPendingRequest(request, "MPI_Iallgather", -1, -1, location);
  return result;
}

int tarch::mpi::allgather(
  const void*          sendBuffer,
  int                  sendCount,
  MPI_Datatype         sendDatatype,
  void*                receiveBuffer,
  int                  receiveCount,
  MPI_Datatype         receiveDatatype,
  MPI_Comm             communicator,
  std::source_location location
) {
  MPI_Request request = MPI_REQUEST_NULL;
  iallgather(
    sendBuffer,
    sendCount,
    sendDatatype,
    receiveBuffer,
    receiveCount,
    receiveDatatype,
    communicator,
    &request,
    location
  );
  return wait(&request, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::allgather(
  const void*           sendBuffer,
  int                   sendCount,
  MPI_Datatype          sendDatatype,
  void*                 receiveBuffer,
  int                   receiveCount,
  MPI_Datatype          receiveDatatype,
  MPI_Comm              communicator,
  std::function<void()> progressFunctor,
  std::source_location  location
) {
  if (not progressFunctor) {
    return allgather(
      sendBuffer,
      sendCount,
      sendDatatype,
      receiveBuffer,
      receiveCount,
      receiveDatatype,
      communicator,
      location
    );
  }

  MPI_Request request = MPI_REQUEST_NULL;
  iallgather(
    sendBuffer,
    sendCount,
    sendDatatype,
    receiveBuffer,
    receiveCount,
    receiveDatatype,
    communicator,
    &request,
    location
  );
  return wait(&request, progressFunctor, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::iallgatherv(
  const void*          sendBuffer,
  int                  sendCount,
  MPI_Datatype         sendDatatype,
  void*                receiveBuffer,
  const int*           receiveCounts,
  const int*           displacements,
  MPI_Datatype         receiveDatatype,
  MPI_Comm             communicator,
  MPI_Request*         request,
  std::source_location location
) {
  int result = PMPI_Iallgatherv(
    sendBuffer,
    sendCount,
    sendDatatype,
    receiveBuffer,
    receiveCounts,
    displacements,
    receiveDatatype,
    communicator,
    request
  );
  checkReturnValue(result, "MPI_Iallgatherv", location);
  registerPendingRequest(request, "MPI_Iallgatherv", -1, -1, location);
  return result;
}

int tarch::mpi::ialltoall(
  const void*          sendBuffer,
  int                  sendCount,
  MPI_Datatype         sendDatatype,
  void*                receiveBuffer,
  int                  receiveCount,
  MPI_Datatype         receiveDatatype,
  MPI_Comm             communicator,
  MPI_Request*         request,
  std::source_location location
) {
  int result = PMPI_Ialltoall(
    sendBuffer,
    sendCount,
    sendDatatype,
    receiveBuffer,
    receiveCount,
    receiveDatatype,
    communicator,
    request
  );
  checkReturnValue(result, "MPI_Ialltoall", location);
  registerPendingRequest(request, "MPI_Ialltoall", -1, -1, location);
  return result;
}

int tarch::mpi::alltoall(
  const void*          sendBuffer,
  int                  sendCount,
  MPI_Datatype         sendDatatype,
  void*                receiveBuffer,
  int                  receiveCount,
  MPI_Datatype         receiveDatatype,
  MPI_Comm             communicator,
  std::source_location location
) {
  MPI_Request request = MPI_REQUEST_NULL;
  ialltoall(
    sendBuffer,
    sendCount,
    sendDatatype,
    receiveBuffer,
    receiveCount,
    receiveDatatype,
    communicator,
    &request,
    location
  );
  return wait(&request, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::ialltoallv(
  const void*          sendBuffer,
  const int*           sendCounts,
  const int*           sendDisplacements,
  MPI_Datatype         sendDatatype,
  void*                receiveBuffer,
  const int*           receiveCounts,
  const int*           receiveDisplacements,
  MPI_Datatype         receiveDatatype,
  MPI_Comm             communicator,
  MPI_Request*         request,
  std::source_location location
) {
  int result = PMPI_Ialltoallv(
    sendBuffer,
    sendCounts,
    sendDisplacements,
    sendDatatype,
    receiveBuffer,
    receiveCounts,
    receiveDisplacements,
    receiveDatatype,
    communicator,
    request
  );
  checkReturnValue(result, "MPI_Ialltoallv", location);
  registerPendingRequest(request, "MPI_Ialltoallv", -1, -1, location);
  return result;
}

int tarch::mpi::alltoallv(
  const void*          sendBuffer,
  const int*           sendCounts,
  const int*           sendDisplacements,
  MPI_Datatype         sendDatatype,
  void*                receiveBuffer,
  const int*           receiveCounts,
  const int*           receiveDisplacements,
  MPI_Datatype         receiveDatatype,
  MPI_Comm             communicator,
  std::source_location location
) {
  MPI_Request request = MPI_REQUEST_NULL;
  ialltoallv(
    sendBuffer,
    sendCounts,
    sendDisplacements,
    sendDatatype,
    receiveBuffer,
    receiveCounts,
    receiveDisplacements,
    receiveDatatype,
    communicator,
    &request,
    location
  );
  return wait(&request, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::allgatherv(
  const void*          sendBuffer,
  int                  sendCount,
  MPI_Datatype         sendDatatype,
  void*                receiveBuffer,
  const int*           receiveCounts,
  const int*           displacements,
  MPI_Datatype         receiveDatatype,
  MPI_Comm             communicator,
  std::source_location location
) {
  MPI_Request request = MPI_REQUEST_NULL;
  iallgatherv(
    sendBuffer,
    sendCount,
    sendDatatype,
    receiveBuffer,
    receiveCounts,
    displacements,
    receiveDatatype,
    communicator,
    &request,
    location
  );
  return wait(&request, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::allgatherv(
  const void*           sendBuffer,
  int                   sendCount,
  MPI_Datatype          sendDatatype,
  void*                 receiveBuffer,
  const int*            receiveCounts,
  const int*            displacements,
  MPI_Datatype          receiveDatatype,
  MPI_Comm              communicator,
  std::function<void()> progressFunctor,
  std::source_location  location
) {
  if (not progressFunctor) {
    return allgatherv(
      sendBuffer,
      sendCount,
      sendDatatype,
      receiveBuffer,
      receiveCounts,
      displacements,
      receiveDatatype,
      communicator,
      location
    );
  }

  MPI_Request request = MPI_REQUEST_NULL;
  iallgatherv(
    sendBuffer,
    sendCount,
    sendDatatype,
    receiveBuffer,
    receiveCounts,
    displacements,
    receiveDatatype,
    communicator,
    &request,
    location
  );
  return wait(&request, progressFunctor, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::ireduce(
  const void*          sendBuffer,
  void*                receiveBuffer,
  int                  count,
  MPI_Datatype         datatype,
  MPI_Op               operation,
  int                  root,
  MPI_Comm             communicator,
  MPI_Request*         request,
  std::source_location location
) {
  int result = PMPI_Ireduce(sendBuffer, receiveBuffer, count, datatype, operation, root, communicator, request);
  checkReturnValue(result, "MPI_Ireduce", location);
  registerPendingRequest(request, "MPI_Ireduce", root, -1, location);
  return result;
}

int tarch::mpi::reduce(
  const void*          sendBuffer,
  void*                receiveBuffer,
  int                  count,
  MPI_Datatype         datatype,
  MPI_Op               operation,
  int                  root,
  MPI_Comm             communicator,
  std::source_location location
) {
  MPI_Request request = MPI_REQUEST_NULL;
  ireduce(sendBuffer, receiveBuffer, count, datatype, operation, root, communicator, &request, location);
  return wait(&request, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::reduce(
  const void*           sendBuffer,
  void*                 receiveBuffer,
  int                   count,
  MPI_Datatype          datatype,
  MPI_Op                operation,
  int                   root,
  MPI_Comm              communicator,
  std::function<void()> progressFunctor,
  std::source_location  location
) {
  if (not progressFunctor) {
    return reduce(sendBuffer, receiveBuffer, count, datatype, operation, root, communicator, location);
  }

  MPI_Request request = MPI_REQUEST_NULL;
  ireduce(sendBuffer, receiveBuffer, count, datatype, operation, root, communicator, &request, location);
  return wait(&request, progressFunctor, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::scan(
  const void*          sendBuffer,
  void*                receiveBuffer,
  int                  count,
  MPI_Datatype         datatype,
  MPI_Op               operation,
  MPI_Comm             communicator,
  std::source_location location
) {
  MPI_Request request = MPI_REQUEST_NULL;
  int         result  = PMPI_Iscan(sendBuffer, receiveBuffer, count, datatype, operation, communicator, &request);
  checkReturnValue(result, "MPI_Iscan", location);
  registerPendingRequest(&request, "MPI_Iscan", -1, -1, location);
  return wait(&request, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::iprobe(
  int                  source,
  int                  tag,
  MPI_Comm             communicator,
  int*                 flag,
  MPI_Status*          status,
  std::source_location location
) {
  int result = PMPI_Iprobe(source, tag, communicator, flag, status);
  checkReturnValue(result, "MPI_Iprobe", location);
  return result;
}

int tarch::mpi::improbe(
  int                  source,
  int                  tag,
  MPI_Comm             communicator,
  int*                 flag,
  MPI_Message*         message,
  MPI_Status*          status,
  std::source_location location
) {
  int result = PMPI_Improbe(source, tag, communicator, flag, message, status);
  checkReturnValue(result, "MPI_Improbe", location);
  return result;
}

int tarch::mpi::mprobe(
  int                   source,
  int                   tag,
  MPI_Comm              communicator,
  MPI_Message*          message,
  std::function<void()> progressFunctor,
  MPI_Status*           status,
  std::source_location  location
) {
  if (not progressFunctor) {
    return mprobe(source, tag, communicator, message, status, location);
  }

  int          flag           = 0;
  bool         waitingStarted = false;
  TimeoutPoint startedWaiting;
  TimeoutPoint nextWarning;

  while (not flag) {
    improbe(source, tag, communicator, &flag, message, status, location);
    if (not flag) {
      reportBlockingOperation("MPI_Mprobe", location, source, tag, startedWaiting, nextWarning, waitingStarted);
      progressFunctor();
    }
  }

  return MPI_SUCCESS;
}

int tarch::mpi::mprobe(
  int                  source,
  int                  tag,
  MPI_Comm             communicator,
  MPI_Message*         message,
  MPI_Status*          status,
  std::source_location location
) {
  int          flag           = 0;
  bool         waitingStarted = false;
  TimeoutPoint startedWaiting;
  TimeoutPoint nextWarning;

  while (not flag) {
    improbe(source, tag, communicator, &flag, message, status, location);
    if (not flag) {
      reportBlockingOperation("MPI_Mprobe", location, source, tag, startedWaiting, nextWarning, waitingStarted);
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    }
  }

  return MPI_SUCCESS;
}

int tarch::mpi::mrecv(
  void*                buffer,
  int                  count,
  MPI_Datatype         datatype,
  MPI_Message*         message,
  MPI_Status*          status,
  std::source_location location
) {
  MPI_Request request = MPI_REQUEST_NULL;
  int         result  = PMPI_Imrecv(buffer, count, datatype, message, &request);
  checkReturnValue(result, "MPI_Imrecv", location);
  registerPendingRequest(&request, "MPI_Imrecv", -1, -1, location);
  return wait(&request, status, location);
}

int tarch::mpi::getCount(const MPI_Status* status, MPI_Datatype datatype, int* count, std::source_location location) {
  int result = PMPI_Get_count(status, datatype, count);
  checkReturnValue(result, "MPI_Get_count", location);
  return result;
}

#endif
