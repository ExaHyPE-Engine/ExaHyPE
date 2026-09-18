// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Collectives.h"

#include "tarch/mpi/Diagnostics.h"
#include "tarch/mpi/Request.h"

/**
 * Every blocking collective is its non-blocking counterpart followed by the
 * watched wait of tarch::mpi::wait(), so a collective that never completes
 * warns and eventually aborts like a point-to-point message. Each collective
 * is created and completed inside one call: An unfinished collective makes an
 * MPI implementation run its collective progress engine inside every MPI call
 * of every thread for as long as it lives.
 */
namespace {
  int ibarrier(
    MPI_Comm             communicator,
    MPI_Request*         request,
    std::source_location location = std::source_location::current()
  ) {
    const int result = MPI_Ibarrier(communicator, request);
    tarch::mpi::checkReturnValue(result, "MPI_Ibarrier", location);
    tarch::mpi::Diagnostics::getInstance().registerRequest(request, "MPI_Ibarrier", -1, -1, location);
    return result;
  }

  int iallreduce(
    const void*          sendBuffer,
    void*                receiveBuffer,
    const int            count,
    MPI_Datatype         datatype,
    MPI_Op               operation,
    MPI_Comm             communicator,
    MPI_Request*         request,
    std::source_location location = std::source_location::current()
  ) {
    const int result = MPI_Iallreduce(sendBuffer, receiveBuffer, count, datatype, operation, communicator, request);
    tarch::mpi::checkReturnValue(result, "MPI_Iallreduce", location);
    tarch::mpi::Diagnostics::getInstance().registerRequest(request, "MPI_Iallreduce", -1, -1, location);
    return result;
  }

  int ireduce(
    const void*          sendBuffer,
    void*                receiveBuffer,
    const int            count,
    MPI_Datatype         datatype,
    MPI_Op               operation,
    const int            root,
    MPI_Comm             communicator,
    MPI_Request*         request,
    std::source_location location = std::source_location::current()
  ) {
    const int result = MPI_Ireduce(sendBuffer, receiveBuffer, count, datatype, operation, root, communicator, request);
    tarch::mpi::checkReturnValue(result, "MPI_Ireduce", location);
    tarch::mpi::Diagnostics::getInstance().registerRequest(request, "MPI_Ireduce", root, -1, location);
    return result;
  }

  int iallgather(
    const void*          sendBuffer,
    const int            sendCount,
    MPI_Datatype         sendDatatype,
    void*                receiveBuffer,
    const int            receiveCount,
    MPI_Datatype         receiveDatatype,
    MPI_Comm             communicator,
    MPI_Request*         request,
    std::source_location location = std::source_location::current()
  ) {
    const int result = MPI_Iallgather(
      sendBuffer,
      sendCount,
      sendDatatype,
      receiveBuffer,
      receiveCount,
      receiveDatatype,
      communicator,
      request
    );
    tarch::mpi::checkReturnValue(result, "MPI_Iallgather", location);
    tarch::mpi::Diagnostics::getInstance().registerRequest(request, "MPI_Iallgather", -1, -1, location);
    return result;
  }

  int iallgatherv(
    const void*          sendBuffer,
    const int            sendCount,
    MPI_Datatype         sendDatatype,
    void*                receiveBuffer,
    const int*           receiveCounts,
    const int*           receiveDisplacements,
    MPI_Datatype         receiveDatatype,
    MPI_Comm             communicator,
    MPI_Request*         request,
    std::source_location location = std::source_location::current()
  ) {
    const int result = MPI_Iallgatherv(
      sendBuffer,
      sendCount,
      sendDatatype,
      receiveBuffer,
      receiveCounts,
      receiveDisplacements,
      receiveDatatype,
      communicator,
      request
    );
    tarch::mpi::checkReturnValue(result, "MPI_Iallgatherv", location);
    tarch::mpi::Diagnostics::getInstance().registerRequest(request, "MPI_Iallgatherv", -1, -1, location);
    return result;
  }

  int ialltoall(
    const void*          sendBuffer,
    const int            sendCount,
    MPI_Datatype         sendDatatype,
    void*                receiveBuffer,
    const int            receiveCount,
    MPI_Datatype         receiveDatatype,
    MPI_Comm             communicator,
    MPI_Request*         request,
    std::source_location location = std::source_location::current()
  ) {
    const int result = MPI_Ialltoall(
      sendBuffer,
      sendCount,
      sendDatatype,
      receiveBuffer,
      receiveCount,
      receiveDatatype,
      communicator,
      request
    );
    tarch::mpi::checkReturnValue(result, "MPI_Ialltoall", location);
    tarch::mpi::Diagnostics::getInstance().registerRequest(request, "MPI_Ialltoall", -1, -1, location);
    return result;
  }

  int ialltoallv(
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
    std::source_location location = std::source_location::current()
  ) {
    const int result = MPI_Ialltoallv(
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
    tarch::mpi::checkReturnValue(result, "MPI_Ialltoallv", location);
    tarch::mpi::Diagnostics::getInstance().registerRequest(request, "MPI_Ialltoallv", -1, -1, location);
    return result;
  }
} // namespace

int tarch::mpi::barrier(MPI_Comm communicator, std::source_location location) {
  MPI_Request request = MPI_REQUEST_NULL;
  ibarrier(communicator, &request, location);
  return tarch::mpi::wait(&request, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::allreduce(
  const void*          sendBuffer,
  void*                receiveBuffer,
  const int            count,
  MPI_Datatype         datatype,
  MPI_Op               operation,
  MPI_Comm             communicator,
  std::source_location location
) {
  MPI_Request request = MPI_REQUEST_NULL;
  iallreduce(sendBuffer, receiveBuffer, count, datatype, operation, communicator, &request, location);
  // tarch::mpi::wait() completes the request in its MPI_Test loop.
  return tarch::mpi::wait(&request, MPI_STATUS_IGNORE, location); // NOLINT(clang-analyzer-optin.mpi.MPI-Checker)
}

int tarch::mpi::allgather(
  const void*          sendBuffer,
  const int            sendCount,
  MPI_Datatype         sendDatatype,
  void*                receiveBuffer,
  const int            receiveCount,
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
  // tarch::mpi::wait() completes the request in its MPI_Test loop.
  return tarch::mpi::wait(&request, MPI_STATUS_IGNORE, location); // NOLINT(clang-analyzer-optin.mpi.MPI-Checker)
}

int tarch::mpi::allgatherv(
  const void*          sendBuffer,
  const int            sendCount,
  MPI_Datatype         sendDatatype,
  void*                receiveBuffer,
  const int*           receiveCounts,
  const int*           receiveDisplacements,
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
    receiveDisplacements,
    receiveDatatype,
    communicator,
    &request,
    location
  );
  return tarch::mpi::wait(&request, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::alltoall(
  const void*          sendBuffer,
  const int            sendCount,
  MPI_Datatype         sendDatatype,
  void*                receiveBuffer,
  const int            receiveCount,
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
  // tarch::mpi::wait() completes the request in its MPI_Test loop.
  return tarch::mpi::wait(&request, MPI_STATUS_IGNORE, location); // NOLINT(clang-analyzer-optin.mpi.MPI-Checker)
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
  return tarch::mpi::wait(&request, MPI_STATUS_IGNORE, location);
}

int tarch::mpi::reduce(
  const void*          sendBuffer,
  void*                receiveBuffer,
  const int            count,
  MPI_Datatype         datatype,
  MPI_Op               operation,
  const int            root,
  MPI_Comm             communicator,
  std::source_location location
) {
  MPI_Request request = MPI_REQUEST_NULL;
  ireduce(sendBuffer, receiveBuffer, count, datatype, operation, root, communicator, &request, location);
  // tarch::mpi::wait() completes the request in its MPI_Test loop.
  return tarch::mpi::wait(&request, MPI_STATUS_IGNORE, location); // NOLINT(clang-analyzer-optin.mpi.MPI-Checker)
}

int tarch::mpi::scan(
  const void*          sendBuffer,
  void*                receiveBuffer,
  const int            count,
  MPI_Datatype         datatype,
  MPI_Op               operation,
  MPI_Comm             communicator,
  std::source_location location
) {
  MPI_Request request = MPI_REQUEST_NULL;
  const int   result  = MPI_Iscan(sendBuffer, receiveBuffer, count, datatype, operation, communicator, &request);
  tarch::mpi::checkReturnValue(result, "MPI_Iscan", location);
  tarch::mpi::Diagnostics::getInstance().registerRequest(&request, "MPI_Iscan", -1, -1, location);
  return tarch::mpi::wait(&request, MPI_STATUS_IGNORE, location);
}
