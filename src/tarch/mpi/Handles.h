// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <mpi.h>

/**
 * Creates, queries and frees MPI communicators, info objects, datatypes and reduction operations.
 * Every call checks the return value of MPI and terminates with the operation and the source
 * location of its caller where MPI fails. duplicateCommunicator() completes like the collectives of
 * Collectives.h, so a rank that never joins it warns and aborts. MPI offers no non-blocking split,
 * so a split that a rank never joins blocks without a report.
 */
namespace tarch::mpi {
  int splitCommunicator(
    MPI_Comm             communicator,
    int                  colour,
    int                  key,
    MPI_Comm*            newCommunicator,
    std::source_location location = std::source_location::current()
  );

  int splitCommunicatorByType(
    MPI_Comm             communicator,
    int                  splitType,
    int                  key,
    MPI_Info             info,
    MPI_Comm*            newCommunicator,
    std::source_location location = std::source_location::current()
  );

  int duplicateCommunicator(
    MPI_Comm             communicator,
    MPI_Comm*            newCommunicator,
    std::source_location location = std::source_location::current()
  );

  int freeCommunicator(MPI_Comm* communicator, std::source_location location = std::source_location::current());

  int getRank(MPI_Comm communicator, int* rank, std::source_location location = std::source_location::current());

  int getNumberOfRanks(
    MPI_Comm             communicator,
    int*                 numberOfRanks,
    std::source_location location = std::source_location::current()
  );

  int getAttribute(
    MPI_Comm             communicator,
    int                  key,
    void*                value,
    int*                 found,
    std::source_location location = std::source_location::current()
  );

  int createInfo(MPI_Info* info, std::source_location location = std::source_location::current());

  int setInfo(
    MPI_Info             info,
    const char*          key,
    const char*          value,
    std::source_location location = std::source_location::current()
  );

  int freeInfo(MPI_Info* info, std::source_location location = std::source_location::current());

  int createContiguousType(
    int                  count,
    MPI_Datatype         elementType,
    MPI_Datatype*        newType,
    std::source_location location = std::source_location::current()
  );

  int commitType(MPI_Datatype* type, std::source_location location = std::source_location::current());

  int freeType(MPI_Datatype* type, std::source_location location = std::source_location::current());

  int createOperation(
    MPI_User_function*   function,
    bool                 commutative,
    MPI_Op*              operation,
    std::source_location location = std::source_location::current()
  );

  int freeOperation(MPI_Op* operation, std::source_location location = std::source_location::current());
} // namespace tarch::mpi
