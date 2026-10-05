// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Handles.h"

#include "tarch/mpi/Diagnostics.h"
#include "tarch/mpi/Request.h"

int
tarch::mpi::splitCommunicator(
  MPI_Comm             communicator,
  const int            colour,
  const int            key,
  MPI_Comm*            newCommunicator,
  std::source_location location
) {
  const int result = MPI_Comm_split(communicator, colour, key, newCommunicator);
  tarch::mpi::checkReturnValue(result, "MPI_Comm_split", location);
  return result;
}

int
tarch::mpi::splitCommunicatorByType(
  MPI_Comm             communicator,
  const int            splitType,
  const int            key,
  MPI_Info             info,
  MPI_Comm*            newCommunicator,
  std::source_location location
) {
  const int result = MPI_Comm_split_type(communicator, splitType, key, info, newCommunicator);
  tarch::mpi::checkReturnValue(result, "MPI_Comm_split_type", location);
  return result;
}

/**
 * Posts the non-blocking duplication and completes it with tarch::mpi::wait(), like the collectives.
 */
int
tarch::mpi::duplicateCommunicator(MPI_Comm communicator, MPI_Comm* newCommunicator, std::source_location location) {
  MPI_Request request = MPI_REQUEST_NULL;
  const int   result  = MPI_Comm_idup(communicator, newCommunicator, &request);
  tarch::mpi::checkReturnValue(result, "MPI_Comm_idup", location);
  tarch::mpi::Diagnostics::getInstance().registerRequest(&request, "MPI_Comm_idup", -1, -1, location);
  // tarch::mpi::wait() completes the request in its MPI_Test loop.
  return tarch::mpi::wait(&request, MPI_STATUS_IGNORE, location); // NOLINT(clang-analyzer-optin.mpi.MPI-Checker)
}

int
tarch::mpi::freeCommunicator(MPI_Comm* communicator, std::source_location location) {
  const int result = MPI_Comm_free(communicator);
  tarch::mpi::checkReturnValue(result, "MPI_Comm_free", location);
  return result;
}

int
tarch::mpi::getRank(MPI_Comm communicator, int* rank, std::source_location location) {
  const int result = MPI_Comm_rank(communicator, rank);
  tarch::mpi::checkReturnValue(result, "MPI_Comm_rank", location);
  return result;
}

int
tarch::mpi::getNumberOfRanks(MPI_Comm communicator, int* numberOfRanks, std::source_location location) {
  const int result = MPI_Comm_size(communicator, numberOfRanks);
  tarch::mpi::checkReturnValue(result, "MPI_Comm_size", location);
  return result;
}

int
tarch::mpi::getAttribute(MPI_Comm communicator, const int key, void* value, int* found, std::source_location location) {
  const int result = MPI_Comm_get_attr(communicator, key, value, found);
  tarch::mpi::checkReturnValue(result, "MPI_Comm_get_attr", location);
  return result;
}

int
tarch::mpi::createInfo(MPI_Info* info, std::source_location location) {
  const int result = MPI_Info_create(info);
  tarch::mpi::checkReturnValue(result, "MPI_Info_create", location);
  return result;
}

int
tarch::mpi::setInfo(MPI_Info info, const char* key, const char* value, std::source_location location) {
  const int result = MPI_Info_set(info, key, value);
  tarch::mpi::checkReturnValue(result, "MPI_Info_set", location);
  return result;
}

int
tarch::mpi::freeInfo(MPI_Info* info, std::source_location location) {
  const int result = MPI_Info_free(info);
  tarch::mpi::checkReturnValue(result, "MPI_Info_free", location);
  return result;
}

int
tarch::mpi::createContiguousType(
  const int            count,
  MPI_Datatype         elementType,
  MPI_Datatype*        newType,
  std::source_location location
) {
  const int result = MPI_Type_contiguous(count, elementType, newType);
  tarch::mpi::checkReturnValue(result, "MPI_Type_contiguous", location);
  return result;
}

int
tarch::mpi::commitType(MPI_Datatype* type, std::source_location location) {
  const int result = MPI_Type_commit(type);
  tarch::mpi::checkReturnValue(result, "MPI_Type_commit", location);
  return result;
}

int
tarch::mpi::freeType(MPI_Datatype* type, std::source_location location) {
  const int result = MPI_Type_free(type);
  tarch::mpi::checkReturnValue(result, "MPI_Type_free", location);
  return result;
}

int
tarch::mpi::createOperation(
  MPI_User_function*   function,
  const bool           commutative,
  MPI_Op*              operation,
  std::source_location location
) {
  const int result = MPI_Op_create(function, commutative ? 1 : 0, operation);
  tarch::mpi::checkReturnValue(result, "MPI_Op_create", location);
  return result;
}

int
tarch::mpi::freeOperation(MPI_Op* operation, std::source_location location) {
  const int result = MPI_Op_free(operation);
  tarch::mpi::checkReturnValue(result, "MPI_Op_free", location);
  return result;
}
