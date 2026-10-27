// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Request.h"

#include "tarch/Termination.h"
#include "tarch/instrumentation/Warning.h"
#include "tarch/mpi/Diagnostics.h"
#include "tarch/mpi/PollPacing.h"

namespace {
  using TimeoutPoint = std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>>;

  /**
   * Warns about and aborts a wait on a request that the pending-request table does not track.
   */
  void reportUntrackedWait(
    const char*                 operation,
    const std::source_location& location,
    TimeoutPoint&               startedWaiting,
    TimeoutPoint&               nextWarning,
    bool&                       waitingStarted
  ) {
    auto& diagnostics = tarch::mpi::Diagnostics::getInstance();
    if (not diagnostics.useTimeoutChecks()) {
      return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (not waitingStarted) {
      waitingStarted = true;
      startedWaiting = now;
      nextWarning    = now + diagnostics.getTimeoutWarning();
    }

    if (diagnostics.getTimeoutWarning() > std::chrono::seconds(0) and now >= nextWarning) {
      logWarning(
        operation << " at " << location.file_name() << ":" << location.line() << " still waits for MPI request"
      );
      nextWarning = now + diagnostics.getTimeoutWarning();
    }

    if (
      diagnostics.getDeadlockTimeout() > std::chrono::seconds(0)
      and now - startedWaiting >= diagnostics.getDeadlockTimeout()
    ) {
      std::ostringstream message;
      message << operation << " at " << location.file_name() << ":" << location.line()
              << " deadlocked while waiting for MPI request after " << diagnostics.getDeadlockTimeout().count()
              << " second(s)";
      diagnostics.plotMessageQueues();
      tarch::terminateWithError(message.str(), tarch::mpi::Diagnostics::DeadlockExitCode, location);
    }
  }
} // namespace

int
tarch::mpi::isend(
  const void*          buffer,
  int                  count,
  MPI_Datatype         datatype,
  int                  destination,
  int                  tag,
  MPI_Comm             communicator,
  MPI_Request*         request,
  std::source_location location
) {
  const int result = MPI_Isend(buffer, count, datatype, destination, tag, communicator, request);
  tarch::mpi::checkReturnValue(result, "MPI_Isend", location);
  tarch::mpi::Diagnostics::getInstance().registerRequest(request, "MPI_Isend", destination, tag, location);
  return result;
}

int
tarch::mpi::irecv(
  void*                buffer,
  int                  count,
  MPI_Datatype         datatype,
  int                  source,
  int                  tag,
  MPI_Comm             communicator,
  MPI_Request*         request,
  std::source_location location
) {
  const int result = MPI_Irecv(buffer, count, datatype, source, tag, communicator, request);
  tarch::mpi::checkReturnValue(result, "MPI_Irecv", location);
  tarch::mpi::Diagnostics::getInstance().registerRequest(request, "MPI_Irecv", source, tag, location);
  return result;
}

int
tarch::mpi::test(MPI_Request* request, int* flag, MPI_Status* status, std::source_location location) {
  const int result = MPI_Test(request, flag, status);
  tarch::mpi::checkReturnValue(result, "MPI_Test", location);

  if (flag != nullptr and *flag) {
    tarch::mpi::Diagnostics::getInstance().clearRequest(request);
  }

  return result;
}

/**
 * Waits for one request in a loop of MPI_Test() calls. A wait that never finishes reports its
 * operation and its partner. The reports need a timeout that a caller has configured.
 */
int
tarch::mpi::wait(MPI_Request* request, MPI_Status* status, std::source_location location) {
  int                    flag           = 0;
  bool                   waitingStarted = false;
  TimeoutPoint           startedWaiting;
  TimeoutPoint           nextWarning;
  tarch::mpi::PollPacing pacing;
  while (not flag) {
    tarch::mpi::test(request, &flag, status, location);
    if (not flag) {
      // The timeout check costs more than the request test and runs on the stride of the pacing.
      if (pacing.reportProgressNow()) {
        if (not tarch::mpi::Diagnostics::getInstance().reportRequest(request, "MPI_Wait", location)) {
          reportUntrackedWait("MPI_Wait", location, startedWaiting, nextWarning, waitingStarted);
        }
      }
    }
  }

  tarch::mpi::Diagnostics::getInstance().clearRequest(request);
  return MPI_SUCCESS;
}

int
tarch::mpi::send(
  const void*          buffer,
  int                  count,
  MPI_Datatype         datatype,
  int                  destination,
  int                  tag,
  MPI_Comm             communicator,
  std::source_location location
) {
  MPI_Request request = MPI_REQUEST_NULL;
  tarch::mpi::isend(buffer, count, datatype, destination, tag, communicator, &request, location);
  return tarch::mpi::wait(&request, MPI_STATUS_IGNORE, location);
}

int
tarch::mpi::receive(
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
  tarch::mpi::irecv(buffer, count, datatype, source, tag, communicator, &request, location);
  return tarch::mpi::wait(&request, status, location);
}

int
tarch::mpi::sendrecv(
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

  tarch::mpi::irecv(
    receiveBuffer,
    receiveCount,
    receiveDatatype,
    source,
    receiveTag,
    communicator,
    &receiveRequest,
    location
  );
  tarch::mpi::isend(sendBuffer, sendCount, sendDatatype, destination, sendTag, communicator, &sendRequest, location);

  tarch::mpi::wait(&sendRequest, MPI_STATUS_IGNORE, location);
  return tarch::mpi::wait(&receiveRequest, status, location);
}
