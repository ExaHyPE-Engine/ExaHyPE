// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AutomatonState.h"

#include "PeanoCurve.h"
#include "tarch/services/ServiceRepository.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

static_assert(std::is_standard_layout_v<peano4::grid::AutomatonState>);
static_assert(std::is_trivially_copyable_v<peano4::grid::AutomatonState>);

std::ostream& operator<<(std::ostream& out, const peano4::grid::AutomatonState& state) {
  return (out << state.toString());
}

std::string peano4::grid::AutomatonState::toString() const {
  std::ostringstream out;
  out << "AutomatonState{";

  out << "Level=" << level;

  out << ", x=[";
  for (auto i = 0; i < x.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << std::setprecision(6) << x[i];
  }
  out << "]";

  out << ", h=[";
  for (auto i = 0; i < h.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << std::setprecision(6) << h[i];
  }
  out << "]";

  out << ", Inverted=" << (getInverted() ? "true" : "false");

  out << ", EvenFlags=[";
  for (int d = 0; d < DIMENSIONS; d++) {
    if (d > 0) {
      out << ",";
    }
    out << (getEvenFlag(d) ? "1" : "0");
  }
  out << "]";

  out << ", AccessNumber=[";
  for (std::size_t i = 0; i < accessNumber.size(); i++) {
    if (i > 0) {
      out << ",";
    }
    out << static_cast<int>(accessNumber[i]);
  }
  out << "]";

  out << "}";

  return out.str();
}

void peano4::grid::AutomatonState::refineState(
  const AutomatonState&              coarseGrid,
  AutomatonState                     fineGridStates[THREE_POWER_D],
  tarch::la::Vector<DIMENSIONS, int> fineGridStatesPosition,
  int                                axis
) {
  if (axis == -1) {
    int arrayIndex                   = peano4::dLinearised(fineGridStatesPosition, 3);
    fineGridStates[arrayIndex]       = coarseGrid;
    fineGridStates[arrayIndex].level = coarseGrid.level + 1;
  } else {
    assertion(axis >= 0);
    assertion(axis < DIMENSIONS);

    AutomatonState firstCell  = coarseGrid;
    AutomatonState secondCell = coarseGrid;
    AutomatonState thirdCell  = coarseGrid;

    if (PeanoCurve::isTraversePositiveAlongAxis(coarseGrid, axis)) {
      PeanoCurve::setExitFace(firstCell, axis);
    } else {
      PeanoCurve::setEntryFace(firstCell, axis);
    }
    firstCell.h[axis]            = coarseGrid.h[axis] / 3.0;
    fineGridStatesPosition(axis) = 0;
    firstCell.x[axis]            = coarseGrid.x[axis];
    refineState(firstCell, fineGridStates, fineGridStatesPosition, axis - 1);

    PeanoCurve::invertEvenFlag(secondCell, axis);
    PeanoCurve::setEntryFace(secondCell, axis);
    PeanoCurve::setExitFace(secondCell, axis);
    secondCell.h[axis]           = coarseGrid.h[axis] / 3.0;
    fineGridStatesPosition(axis) = 1;
    secondCell.x[axis]           = coarseGrid.x[axis] + coarseGrid.h[axis] / 3.0;
    refineState(secondCell, fineGridStates, fineGridStatesPosition, axis - 1);

    if (PeanoCurve::isTraversePositiveAlongAxis(coarseGrid, axis)) {
      PeanoCurve::setEntryFace(thirdCell, axis);
    } else {
      PeanoCurve::setExitFace(thirdCell, axis);
    }
    thirdCell.h[axis]            = coarseGrid.h[axis] / 3.0;
    fineGridStatesPosition(axis) = 2;
    thirdCell.x[axis]            = coarseGrid.x[axis] + 2.0 * coarseGrid.h[axis] / 3.0;
    refineState(thirdCell, fineGridStates, fineGridStatesPosition, axis - 1);
  }

  if (axis == DIMENSIONS - 1) {
    for (int i = 0; i < THREE_POWER_D; i++) {
      for (int d = 0; d < TWO_TIMES_D; d++) {
        assertion(
          fineGridStates[i].accessNumber[d] > -2 * DIMENSIONS,
          i,
          d,
          fineGridStates[i].toString(),
          coarseGrid.toString(),
          THREE_POWER_D,
          TWO_TIMES_D
        );
        assertion(
          fineGridStates[i].accessNumber[d] < 2 * DIMENSIONS,
          i,
          d,
          fineGridStates[i].toString(),
          coarseGrid.toString(),
          THREE_POWER_D,
          TWO_TIMES_D
        );
      }
    }
  }
}

#ifdef PEANO_WITH_MPI
MPI_Datatype peano4::grid::AutomatonState::Datatype = MPI_DATATYPE_NULL;

MPI_Datatype peano4::grid::AutomatonState::getForkDatatype() { return Datatype; }
MPI_Datatype peano4::grid::AutomatonState::getGlobalCommunicationDatatype() { return Datatype; }
MPI_Datatype peano4::grid::AutomatonState::getJoinDatatype() { return Datatype; }
MPI_Datatype peano4::grid::AutomatonState::getBoundaryExchangeDatatype() { return Datatype; }
MPI_Datatype peano4::grid::AutomatonState::getMultiscaleDataExchangeDatatype() { return Datatype; }

void peano4::grid::AutomatonState::freeForkDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::AutomatonState::freeGlobalCommunicationDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::AutomatonState::freeJoinDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::AutomatonState::freeBoundaryExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::AutomatonState::freeMultiscaleDataExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::AutomatonState::initDatatype() {
  AutomatonState instances[2];

  constexpr int NumberOfAttributes = 5;
  MPI_Datatype  subtypes[NumberOfAttributes];
  int           blocklen[NumberOfAttributes];
  MPI_Aint      disp[NumberOfAttributes];

  int counter = 0;

  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].level, &disp[counter]);
  counter++;

  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = DIMENSIONS;
  MPI_Get_address(&(instances[0].x.data()[0]), &disp[counter]);
  counter++;

  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = DIMENSIONS;
  MPI_Get_address(&(instances[0].h.data()[0]), &disp[counter]);
  counter++;

  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].flags, &disp[counter]);
  counter++;

  subtypes[counter] = MPI_INT8_T;
  blocklen[counter] = TWO_TIMES_D;
  MPI_Get_address(&(instances[0].accessNumber[0]), &disp[counter]);
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

void peano4::grid::AutomatonState::shutdownDatatype() {
  freeForkDatatype();
  freeJoinDatatype();
  freeBoundaryExchangeDatatype();
  freeMultiscaleDataExchangeDatatype();
  freeGlobalCommunicationDatatype();
}

void peano4::grid::AutomatonState::send(const AutomatonState& buffer, int destination, int tag, MPI_Comm communicator) {
  MPI_Send(&buffer, 1, Datatype, destination, tag, communicator);
}

void peano4::grid::AutomatonState::receive(AutomatonState& buffer, int source, int tag, MPI_Comm communicator) {
  MPI_Status status;
  MPI_Recv(&buffer, 1, Datatype, source, tag, communicator, &status);
}

void peano4::grid::AutomatonState::send(
  const AutomatonState& buffer,
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

void peano4::grid::AutomatonState::receive(
  AutomatonState&       buffer,
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

void peano4::grid::AutomatonState::
  sendAndPollDanglingMessages(const AutomatonState& message, int destination, int tag, MPI_Comm communicator) {
  AutomatonState::send(
    message,
    destination,
    tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance()
        .writeTimeOutWarning("peano4::grid::AutomatonState", "sendAndPollDanglingMessages()", destination, tag);
      tarch::mpi::Rank::getInstance()
        .triggerDeadlockTimeOut("peano4::grid::AutomatonState", "sendAndPollDanglingMessages()", destination, tag);
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}

void peano4::grid::AutomatonState::
  receiveAndPollDanglingMessages(AutomatonState& message, int source, int tag, MPI_Comm communicator) {
  AutomatonState::receive(
    message,
    source,
    tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance()
        .writeTimeOutWarning("peano4::grid::AutomatonState", "receiveAndPollDanglingMessages()", source, tag);
      tarch::mpi::Rank::getInstance()
        .triggerDeadlockTimeOut("peano4::grid::AutomatonState", "receiveAndPollDanglingMessages()", source, tag);
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
