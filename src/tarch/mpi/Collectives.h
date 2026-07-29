// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

#include <mpi.h>

namespace tarch::mpi {
  int barrier(MPI_Comm communicator, std::source_location location = std::source_location::current());

  int allreduce(
    const void*          sendBuffer,
    void*                receiveBuffer,
    int                  count,
    MPI_Datatype         datatype,
    MPI_Op               operation,
    MPI_Comm             communicator,
    std::source_location location = std::source_location::current()
  );

  int allgather(
    const void*          sendBuffer,
    int                  sendCount,
    MPI_Datatype         sendDatatype,
    void*                receiveBuffer,
    int                  receiveCount,
    MPI_Datatype         receiveDatatype,
    MPI_Comm             communicator,
    std::source_location location = std::source_location::current()
  );

  int allgatherv(
    const void*          sendBuffer,
    int                  sendCount,
    MPI_Datatype         sendDatatype,
    void*                receiveBuffer,
    const int*           receiveCounts,
    const int*           receiveDisplacements,
    MPI_Datatype         receiveDatatype,
    MPI_Comm             communicator,
    std::source_location location = std::source_location::current()
  );

  int alltoall(
    const void*          sendBuffer,
    int                  sendCount,
    MPI_Datatype         sendDatatype,
    void*                receiveBuffer,
    int                  receiveCount,
    MPI_Datatype         receiveDatatype,
    MPI_Comm             communicator,
    std::source_location location = std::source_location::current()
  );

  int alltoallv(
    const void*          sendBuffer,
    const int*           sendCounts,
    const int*           sendDisplacements,
    MPI_Datatype         sendDatatype,
    void*                receiveBuffer,
    const int*           receiveCounts,
    const int*           receiveDisplacements,
    MPI_Datatype         receiveDatatype,
    MPI_Comm             communicator,
    std::source_location location = std::source_location::current()
  );

  int reduce(
    const void*          sendBuffer,
    void*                receiveBuffer,
    int                  count,
    MPI_Datatype         datatype,
    MPI_Op               operation,
    int                  root,
    MPI_Comm             communicator,
    std::source_location location = std::source_location::current()
  );

  int scan(
    const void*          sendBuffer,
    void*                receiveBuffer,
    int                  count,
    MPI_Datatype         datatype,
    MPI_Op               operation,
    MPI_Comm             communicator,
    std::source_location location = std::source_location::current()
  );
} // namespace tarch::mpi
