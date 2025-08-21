// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridVertex.h"

#include <algorithm>
#include <bitset>
#include <sstream>

#ifdef WITH_MPI
#include <mpi.h>
#endif

static_assert(std::is_standard_layout_v<peano4::grid::GridVertex>);
static_assert(std::is_trivially_copyable_v<peano4::grid::GridVertex>);

peano4::grid::GridVertex peano4::grid::createVertex(
  int                                 level,
  GridVertex::State                   state,
  const std::array<int, TWO_POWER_D>& adjacentRanks,
  bool                                isNewFineGridVertex
) {
  GridVertex newVertex;
  newVertex.level = level;
  newVertex.setState(state);
  newVertex.adjacentRanks         = adjacentRanks;
  newVertex.backupOfAdjacentRanks = adjacentRanks;
  newVertex.setFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep, not isNewFineGridVertex);
  newVertex.setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, not isNewFineGridVertex);
  newVertex.clearFlag(peano4::grid::GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep);
  newVertex.clearFlag(peano4::grid::GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep);
  return newVertex;
}

std::string peano4::grid::GridVertex::toString() const {
  std::ostringstream out;
  out << "GridVertex{state=" << static_cast<int>(state) << " (";

  switch (getState()) {
  case State::HangingVertex:
    out << "HangingVertex";
    break;
  case State::New:
    out << "New";
    break;
  case State::Unrefined:
    out << "Unrefined";
    break;
  case State::Refined:
    out << "Refined";
    break;
  case State::RefinementTriggered:
    out << "RefinementTriggered";
    break;
  case State::Refining:
    out << "Refining";
    break;
  case State::EraseTriggered:
    out << "EraseTriggered";
    break;
  case State::Erasing:
    out << "Erasing";
    break;
  case State::Delete:
    out << "Delete";
    break;
  }
  out << ")";

  out << ", flags=0b" << std::bitset<8>(flags) << " [";
  bool first   = true;
  auto addFlag = [&](const char* name, uint8_t mask) {
    if (getFlag(mask)) {
      if (!first) {
        out << ", ";
      }
      out << name;
      first = false;
    }
  };
  addFlag("HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep", HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep);
  addFlag("IsAntecessorOfRefinedVertexInCurrentTreeSweep", IsAntecessorOfRefinedVertexInCurrentTreeSweep);
  addFlag("HasBeenParentOfSubtreeVertexInPreviousTreeSweep", HasBeenParentOfSubtreeVertexInPreviousTreeSweep);
  addFlag("IsParentOfSubtreeVertexInCurrentTreeSweep", IsParentOfSubtreeVertexInCurrentTreeSweep);
  if (first) {
    out << "None"; // no flags set
  }
  out << "]";

  out << ", NumberOfAdjacentRefinedLocalCells=" << numberOfAdjacentRefinedLocalCells;
  out << ", Level=" << level;
  out << ", AdjacentRanks=(";
  for (size_t i = 0; i < adjacentRanks.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << adjacentRanks[i];
  }
  out << ")";

  out << "}";

  return out.str();
}

#ifdef WITH_MPI
MPI_Datatype peano4::grid::GridVertex::Datatype = MPI_DATATYPE_NULL;

MPI_Datatype peano4::grid::GridVertex::getForkDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridVertex::getGlobalCommunicationDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridVertex::getJoinDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridVertex::getBoundaryExchangeDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridVertex::getMultiscaleDataExchangeDatatype() { return Datatype; }

void peano4::grid::GridVertex::freeForkDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridVertex::freeGlobalCommunicationDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridVertex::freeJoinDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridVertex::freeBoundaryExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridVertex::freeMultiscaleDataExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridVertex::initDatatype() {
  peano4::grid::GridVertex instances[2];

  constexpr int                                NumberOfAttributes = 6;
  std::array<MPI_Datatype, NumberOfAttributes> subtypes{};
  std::array<int, NumberOfAttributes>          blocklen{};
  std::array<MPI_Aint, NumberOfAttributes>     disp{};

  int counter = 0;

  subtypes[counter] = MPI_UINT8_T;
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].flags, &disp[counter]);
  counter++;

  subtypes[counter] = MPI_INT8_T;
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].state, &disp[counter]);
  counter++;

  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].level, &disp[counter]);
  counter++;

  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].numberOfAdjacentRefinedLocalCells, &disp[counter]);
  counter++;

  subtypes[counter] = MPI_INT;
  blocklen[counter] = TWO_POWER_D;
  MPI_Get_address(instances[0].adjacentRanks.data(), &disp[counter]);
  counter++;

  subtypes[counter] = MPI_INT;
  blocklen[counter] = TWO_POWER_D;
  MPI_Get_address(instances[0].backupOfAdjacentRanks.data(), &disp[counter]);
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
  errorCode += MPI_Type_create_struct(NumberOfAttributes, blocklen.data(), disp.data(), subtypes.data(), &tmpType);
  errorCode += MPI_Type_create_resized(tmpType, offset, extent, &Datatype);
  errorCode += MPI_Type_commit(&Datatype);
  errorCode += MPI_Type_free(&tmpType);

  if (errorCode) {
    std::stringstream ss;
    ss << "Error constructing MPI datatype in " << __FILE__ << ":" << __LINE__ << std::endl;
    throw std::runtime_error(ss.str());
  }
}

void peano4::grid::GridVertex::shutdownDatatype() {
  freeForkDatatype();
  freeJoinDatatype();
  freeBoundaryExchangeDatatype();
  freeMultiscaleDataExchangeDatatype();
  freeGlobalCommunicationDatatype();
}

void peano4::grid::GridVertex::send(const GridVertex& buffer, int destination, int tag, MPI_Comm communicator) {
  MPI_Send(&buffer, 1, Datatype, destination, tag, communicator);
}

void peano4::grid::GridVertex::receive(GridVertex& buffer, int source, int tag, MPI_Comm communicator) {
  MPI_Status status;
  MPI_Recv(&buffer, 1, Datatype, source, tag, communicator, &status);
  buffer.senderDestinationRank = status.MPI_SOURCE;
}

void peano4::grid::GridVertex::send(
  const GridVertex&     buffer,
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

void peano4::grid::GridVertex::receive(
  GridVertex&           buffer,
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
  buffer.senderDestinationRank = status.MPI_SOURCE;
}

void peano4::grid::GridVertex::
  sendAndPollDanglingMessages(const GridVertex& message, int destination, int tag, MPI_Comm communicator) {
  GridVertex::send(
    message,
    destination,
    tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance()
        .writeTimeOutWarning("peano4::grid::GridVertex", "sendAndPollDanglingMessages()", destination, tag);
      tarch::mpi::Rank::getInstance()
        .triggerDeadlockTimeOut("peano4::grid::GridVertex", "sendAndPollDanglingMessages()", destination, tag);
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}

void peano4::grid::GridVertex::
  receiveAndPollDanglingMessages(GridVertex& message, int source, int tag, MPI_Comm communicator) {
  GridVertex::receive(
    message,
    source,
    tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance()
        .writeTimeOutWarning("peano4::grid::GridVertex", "receiveAndPollDanglingMessages()", source, tag);
      tarch::mpi::Rank::getInstance()
        .triggerDeadlockTimeOut("peano4::grid::GridVertex", "receiveAndPollDanglingMessages()", source, tag);
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
