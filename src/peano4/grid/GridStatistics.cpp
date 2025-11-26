// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridStatistics.h"

#include "AutomatonState.h"
#include "GridVertex.h"
#include "tarch/services/ServiceRepository.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

static_assert(std::is_standard_layout_v<peano4::grid::GridStatistics>);
static_assert(std::is_trivially_copyable_v<peano4::grid::GridStatistics>);

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridStatistics& statistics) {
  return (out << statistics.toString());
}

peano4::grid::GridStatistics operator+(
  const peano4::grid::GridStatistics& lhs,
  const peano4::grid::GridStatistics& rhs
) {
  peano4::grid::GridStatistics result;

  result.numberOfLocalUnrefinedCells  = lhs.numberOfLocalUnrefinedCells + rhs.numberOfLocalUnrefinedCells;
  result.numberOfRemoteUnrefinedCells = lhs.numberOfRemoteUnrefinedCells + rhs.numberOfRemoteUnrefinedCells;
  result.numberOfLocalRefinedCells    = lhs.numberOfLocalRefinedCells + rhs.numberOfLocalRefinedCells;
  result.numberOfRemoteRefinedCells   = lhs.numberOfRemoteRefinedCells + rhs.numberOfRemoteRefinedCells;
  result.numberOfStationarySweeps     = std::min(lhs.numberOfStationarySweeps, rhs.numberOfStationarySweeps);
  result.minH                         = tarch::la::min(lhs.minH, rhs.minH);
  result.flags                        = static_cast<std::uint8_t>(lhs.flags | rhs.flags);

  return result;
}

peano4::grid::GridStatistics operator+=(peano4::grid::GridStatistics& lhs, const peano4::grid::GridStatistics& rhs) {
  lhs = lhs + rhs;
  return lhs;
}

void peano4::grid::GridStatistics::clear(bool isGlobalMasterTree) {
  maxDepth                     = 0;
  minDepth                     = 0;
  numberOfLocalUnrefinedCells  = 0;
  numberOfRemoteUnrefinedCells = 0;
  numberOfLocalRefinedCells    = isGlobalMasterTree ? 1 : 0;
  numberOfRemoteRefinedCells   = isGlobalMasterTree ? 0 : 1;
  numberOfStationarySweeps     = numberOfStationarySweeps + 1;
  minH                         = tarch::la::Vector<DIMENSIONS, double>(std::numeric_limits<double>::max());
  setRemovedEmptySubtree(false);
  setCoarseningHasBeenVetoed(false);
}

std::size_t peano4::grid::GridStatistics::getMemoryUsageInBytes() const {
  // Count local and remote cells -> overestimate.
  const std::size_t numberOfCells = static_cast<std::size_t>(numberOfLocalRefinedCells)
                                    + static_cast<std::size_t>(numberOfLocalUnrefinedCells)
                                    + static_cast<std::size_t>(numberOfRemoteRefinedCells)
                                    + static_cast<std::size_t>(numberOfRemoteUnrefinedCells);

  // No cells -> no memory consumed by cells/vertices.
  if (numberOfCells == 0) {
    return 0;
  }

  // Rough estimate.
  const std::size_t numberOfVertices = 2 * (numberOfCells + 1);

  // Memory: one AutomatonState per cell, one GridVertex per vertex.
  const std::size_t numberOfBytes = numberOfCells * sizeof(AutomatonState) + numberOfVertices * sizeof(GridVertex);

  return numberOfBytes;
}

std::string peano4::grid::GridStatistics::toString() const {
  std::ostringstream out;
  out << "GridStatistics{";
  out << "Max Depth=" << maxDepth;
  out << ", Min Depth=" << minDepth;
  out << ", Leaves=" << numberOfLocalUnrefinedCells;
  out << ", Remote Leaves=" << numberOfRemoteUnrefinedCells;
  out << ", MinH=[";
  for (int d = 0; d < DIMENSIONS; ++d) {
    if (d) {
      out << ",";
    }
    out << std::setprecision(6) << minH[d];
  }
  out << "]";
  const auto memoryUsageInBytes = getMemoryUsageInBytes();
  const auto memoryUsageFormat  = tarch::chooseMemoryUsageFormat(memoryUsageInBytes);
  out << ", Memory=" << std::fixed << std::setprecision(2)
      << tarch::convertMemorySize(memoryUsageInBytes, memoryUsageFormat) << " " << tarch::toString(memoryUsageFormat);
  out << "}";
  return out.str();
}

#ifdef WITH_MPI
MPI_Datatype peano4::grid::GridStatistics::Datatype = MPI_DATATYPE_NULL;

MPI_Datatype peano4::grid::GridStatistics::getForkDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridStatistics::getGlobalCommunicationDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridStatistics::getJoinDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridStatistics::getBoundaryExchangeDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridStatistics::getMultiscaleDataExchangeDatatype() { return Datatype; }

void peano4::grid::GridStatistics::freeForkDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridStatistics::freeGlobalCommunicationDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridStatistics::freeJoinDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridStatistics::freeBoundaryExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridStatistics::freeMultiscaleDataExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridStatistics::initDatatype() {
  GridStatistics instances[2];

  constexpr int NumberOfAttributes = 9;
  MPI_Datatype  subtypes[NumberOfAttributes];
  int           blocklen[NumberOfAttributes];
  MPI_Aint      disp[NumberOfAttributes];

  int counter = 0;

  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].maxDepth, &disp[counter]);
  counter++;

  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].minDepth, &disp[counter]);
  counter++;

  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].numberOfLocalUnrefinedCells, &disp[counter]);
  counter++;

  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].numberOfRemoteUnrefinedCells, &disp[counter]);
  counter++;

  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].numberOfLocalRefinedCells, &disp[counter]);
  counter++;

  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].numberOfRemoteRefinedCells, &disp[counter]);
  counter++;

  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].numberOfStationarySweeps, &disp[counter]);
  counter++;

  subtypes[counter] = MPI_UINT8_T;
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].flags, &disp[counter]);
  counter++;

  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = DIMENSIONS;
  MPI_Get_address(&(instances[0].minH.data()[0]), &disp[counter]);
  counter++;

  MPI_Aint baseFirstInstance;
  MPI_Aint baseSecondInstance;
  MPI_Get_address(&instances[0], &baseFirstInstance);
  MPI_Get_address(&instances[1], &baseSecondInstance);
  MPI_Aint offset = disp[0] - baseFirstInstance;
  MPI_Aint extent = baseSecondInstance - baseFirstInstance - offset;
  for (int i = NumberOfAttributes - 1; i >= 0; i--) {
    disp[i] = disp[i] - disp[0];
  }

  int          errorCode = 0;
  MPI_Datatype tmpType;
  errorCode += MPI_Type_create_struct(NumberOfAttributes, blocklen, disp, subtypes, &tmpType);
  errorCode += MPI_Type_create_resized(tmpType, offset, extent, &Datatype);
  errorCode += MPI_Type_commit(&Datatype);
  errorCode += MPI_Type_free(&tmpType);

  if (errorCode) {
    std::stringstream ss;
    ss << "Error constructing MPI datatype in " << __FILE__ << ":" << __LINE__ << std::endl;
    throw std::runtime_error(ss.str());
  }
}

void peano4::grid::GridStatistics::shutdownDatatype() {
  freeForkDatatype();
  freeJoinDatatype();
  freeBoundaryExchangeDatatype();
  freeMultiscaleDataExchangeDatatype();
  freeGlobalCommunicationDatatype();
}

void peano4::grid::GridStatistics::send(const GridStatistics& buffer, int destination, int tag, MPI_Comm communicator) {
  MPI_Send(&buffer, 1, Datatype, destination, tag, communicator);
}

void peano4::grid::GridStatistics::receive(GridStatistics& buffer, int source, int tag, MPI_Comm communicator) {
  MPI_Status status;
  MPI_Recv(&buffer, 1, Datatype, source, tag, communicator, &status);
}

void peano4::grid::GridStatistics::send(
  const GridStatistics& buffer,
  int                   destination,
  int                   tag,
  std::function<void()> startCommunicationFunctor,
  std::function<void()> waitFunctor,
  MPI_Comm              communicator
) {
  MPI_Request sendRequestHandle;
  int         flag = 0;
  MPI_Isend(&buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle);
  MPI_Test(&sendRequestHandle, &flag, MPI_STATUS_IGNORE);
  startCommunicationFunctor();
  while (!flag) {
    waitFunctor();
    MPI_Test(&sendRequestHandle, &flag, MPI_STATUS_IGNORE);
  }
}

void peano4::grid::GridStatistics::receive(
  GridStatistics&       buffer,
  int                   source,
  int                   tag,
  std::function<void()> startCommunicationFunctor,
  std::function<void()> waitFunctor,
  MPI_Comm              communicator
) {
  MPI_Status  status;
  MPI_Request receiveRequestHandle;
  int         flag = 0;
  MPI_Irecv(&buffer, 1, Datatype, source, tag, communicator, &receiveRequestHandle);
  MPI_Test(&receiveRequestHandle, &flag, &status);
  startCommunicationFunctor();
  while (!flag) {
    waitFunctor();
    MPI_Test(&receiveRequestHandle, &flag, &status);
  }
}

void peano4::grid::GridStatistics::
  sendAndPollDanglingMessages(const GridStatistics& message, int destination, int tag, MPI_Comm communicator) {
  GridStatistics::send(
    message,
    destination,
    tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance()
        .writeTimeOutWarning("peano4::grid::GridStatistics", "sendAndPollDanglingMessages()", destination, tag);
      tarch::mpi::Rank::getInstance()
        .triggerDeadlockTimeOut("peano4::grid::GridStatistics", "sendAndPollDanglingMessages()", destination, tag);
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}

void peano4::grid::GridStatistics::
  receiveAndPollDanglingMessages(GridStatistics& message, int source, int tag, MPI_Comm communicator) {
  GridStatistics::receive(
    message,
    source,
    tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance()
        .writeTimeOutWarning("peano4::grid::GridStatistics", "receiveAndPollDanglingMessages()", source, tag);
      tarch::mpi::Rank::getInstance()
        .triggerDeadlockTimeOut("peano4::grid::GridStatistics", "receiveAndPollDanglingMessages()", source, tag);
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
