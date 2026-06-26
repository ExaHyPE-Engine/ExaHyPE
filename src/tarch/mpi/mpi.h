// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

#include <functional>
#include <source_location>
#include <string>

#ifdef PEANO_WITH_MPI
#include <mpi.h>
#endif

namespace tarch {
  namespace mpi {
#ifdef PEANO_WITH_MPI
    enum class RequestTestMode { TestOnly, WaitForCompletion };

    std::string MPIStatusToString(const MPI_Status& status);
    std::string MPIReturnValueToString(int result);

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
      RequestTestMode      mode     = RequestTestMode::TestOnly,
      std::source_location location = std::source_location::current()
    );

    int wait(
      MPI_Request*         request,
      MPI_Status*          status   = MPI_STATUS_IGNORE,
      std::source_location location = std::source_location::current()
    );

    int wait(
      MPI_Request*          request,
      std::function<void()> progressFunctor,
      MPI_Status*           status   = MPI_STATUS_IGNORE,
      std::source_location  location = std::source_location::current()
    );

    int cancel(MPI_Request* request, std::source_location location = std::source_location::current());

    int barrier(MPI_Comm communicator, std::source_location location = std::source_location::current());

    int barrier(
      MPI_Comm              communicator,
      std::function<void()> progressFunctor,
      std::source_location  location = std::source_location::current()
    );

    int allreduce(
      const void*          sendBuffer,
      void*                receiveBuffer,
      int                  count,
      MPI_Datatype         datatype,
      MPI_Op               operation,
      MPI_Comm             communicator,
      std::source_location location = std::source_location::current()
    );

    int allreduce(
      const void*           sendBuffer,
      void*                 receiveBuffer,
      int                   count,
      MPI_Datatype          datatype,
      MPI_Op                operation,
      MPI_Comm              communicator,
      std::function<void()> progressFunctor,
      std::source_location  location = std::source_location::current()
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

    int allgather(
      const void*           sendBuffer,
      int                   sendCount,
      MPI_Datatype          sendDatatype,
      void*                 receiveBuffer,
      int                   receiveCount,
      MPI_Datatype          receiveDatatype,
      MPI_Comm              communicator,
      std::function<void()> progressFunctor,
      std::source_location  location = std::source_location::current()
    );

    int iallgather(
      const void*          sendBuffer,
      int                  sendCount,
      MPI_Datatype         sendDatatype,
      void*                receiveBuffer,
      int                  receiveCount,
      MPI_Datatype         receiveDatatype,
      MPI_Comm             communicator,
      MPI_Request*         request,
      std::source_location location = std::source_location::current()
    );

    int allgatherv(
      const void*          sendBuffer,
      int                  sendCount,
      MPI_Datatype         sendDatatype,
      void*                receiveBuffer,
      const int*           receiveCounts,
      const int*           displacements,
      MPI_Datatype         receiveDatatype,
      MPI_Comm             communicator,
      std::source_location location = std::source_location::current()
    );

    int allgatherv(
      const void*           sendBuffer,
      int                   sendCount,
      MPI_Datatype          sendDatatype,
      void*                 receiveBuffer,
      const int*            receiveCounts,
      const int*            displacements,
      MPI_Datatype          receiveDatatype,
      MPI_Comm              communicator,
      std::function<void()> progressFunctor,
      std::source_location  location = std::source_location::current()
    );

    int iallgatherv(
      const void*          sendBuffer,
      int                  sendCount,
      MPI_Datatype         sendDatatype,
      void*                receiveBuffer,
      const int*           receiveCounts,
      const int*           displacements,
      MPI_Datatype         receiveDatatype,
      MPI_Comm             communicator,
      MPI_Request*         request,
      std::source_location location = std::source_location::current()
    );

    int iallreduce(
      const void*          sendBuffer,
      void*                receiveBuffer,
      int                  count,
      MPI_Datatype         datatype,
      MPI_Op               operation,
      MPI_Comm             communicator,
      MPI_Request*         request,
      std::source_location location = std::source_location::current()
    );

    int ireduce(
      const void*          sendBuffer,
      void*                receiveBuffer,
      int                  count,
      MPI_Datatype         datatype,
      MPI_Op               operation,
      int                  root,
      MPI_Comm             communicator,
      MPI_Request*         request,
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

    int reduce(
      const void*           sendBuffer,
      void*                 receiveBuffer,
      int                   count,
      MPI_Datatype          datatype,
      MPI_Op                operation,
      int                   root,
      MPI_Comm              communicator,
      std::function<void()> progressFunctor,
      std::source_location  location = std::source_location::current()
    );

    int ibarrier(
      MPI_Comm             communicator,
      MPI_Request*         request,
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

    int iprobe(
      int                  source,
      int                  tag,
      MPI_Comm             communicator,
      int*                 flag,
      MPI_Status*          status   = MPI_STATUS_IGNORE,
      std::source_location location = std::source_location::current()
    );

    int improbe(
      int                  source,
      int                  tag,
      MPI_Comm             communicator,
      int*                 flag,
      MPI_Message*         message,
      MPI_Status*          status   = MPI_STATUS_IGNORE,
      std::source_location location = std::source_location::current()
    );

    int mprobe(
      int                   source,
      int                   tag,
      MPI_Comm              communicator,
      MPI_Message*          message,
      std::function<void()> progressFunctor,
      MPI_Status*           status   = MPI_STATUS_IGNORE,
      std::source_location  location = std::source_location::current()
    );

    int mprobe(
      int                  source,
      int                  tag,
      MPI_Comm             communicator,
      MPI_Message*         message,
      MPI_Status*          status   = MPI_STATUS_IGNORE,
      std::source_location location = std::source_location::current()
    );

    int mrecv(
      void*                buffer,
      int                  count,
      MPI_Datatype         datatype,
      MPI_Message*         message,
      MPI_Status*          status   = MPI_STATUS_IGNORE,
      std::source_location location = std::source_location::current()
    );

    int getCount(
      const MPI_Status*    status,
      MPI_Datatype         datatype,
      int*                 count,
      std::source_location location = std::source_location::current()
    );
#endif
  } // namespace mpi
} // namespace tarch
