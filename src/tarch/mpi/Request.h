// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <mpi.h>

namespace tarch::mpi {
  int send(
    const void*          buffer,
    int                  count,
    MPI_Datatype         datatype,
    int                  destination,
    int                  tag,
    MPI_Comm             communicator,
    std::source_location location = std::source_location::current()
  );

  int receive(
    void*                buffer,
    int                  count,
    MPI_Datatype         datatype,
    int                  source,
    int                  tag,
    MPI_Comm             communicator,
    MPI_Status*          status   = MPI_STATUS_IGNORE,
    std::source_location location = std::source_location::current()
  );

  int isend(
    const void*          buffer,
    int                  count,
    MPI_Datatype         datatype,
    int                  destination,
    int                  tag,
    MPI_Comm             communicator,
    MPI_Request*         request,
    std::source_location location = std::source_location::current()
  );

  int irecv(
    void*                buffer,
    int                  count,
    MPI_Datatype         datatype,
    int                  source,
    int                  tag,
    MPI_Comm             communicator,
    MPI_Request*         request,
    std::source_location location = std::source_location::current()
  );

  int sendrecv(
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
    MPI_Status*          status   = MPI_STATUS_IGNORE,
    std::source_location location = std::source_location::current()
  );

  int test(
    MPI_Request*         request,
    int*                 flag,
    MPI_Status*          status   = MPI_STATUS_IGNORE,
    std::source_location location = std::source_location::current()
  );

  int wait(
    MPI_Request*         request,
    MPI_Status*          status   = MPI_STATUS_IGNORE,
    std::source_location location = std::source_location::current()
  );
} // namespace tarch::mpi
