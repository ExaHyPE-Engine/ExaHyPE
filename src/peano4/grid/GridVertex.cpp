// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridVertex.h"

#include "peano4/Loop.h"
#include "tarch/mpi/mpi.h"
#include "tarch/services/ServiceRepository.h"

static_assert(std::is_standard_layout_v<peano4::grid::GridVertex>);
static_assert(std::is_trivially_copyable_v<peano4::grid::GridVertex>);

#if PEANO_DEBUG
tarch::la::Vector<DIMENSIONS, double> peano4::grid::GridVertex::getVertexCoordinate(
  const tarch::la::Vector<DIMENSIONS, double>& cellX,
  const tarch::la::Vector<DIMENSIONS, double>& cellH,
  int                                          vertexIndex
) {
  tarch::la::Vector<DIMENSIONS, double> result = cellX;
  for (int d = 0; d < DIMENSIONS; ++d) {
    if ((vertexIndex & (1 << d)) != 0) {
      result[d] += cellH[d];
    }
  }
  return result;
}
#endif

peano4::grid::GridVertex peano4::grid::GridVertex::create(
  int                                          level,
  GridVertex::State                            state,
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const std::array<int, TWO_POWER_D>&          adjacentRanks,
  bool                                         isNewFineGridVertex
) {
  GridVertex newVertex;
  newVertex.level = level;
  newVertex.setState(state);
#if PEANO_DEBUG
  newVertex.setX(x);
#endif
  newVertex.adjacentRanks         = adjacentRanks;
  newVertex.backupOfAdjacentRanks = adjacentRanks;
  newVertex.setFlag(GridVertex::HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep, not isNewFineGridVertex);
  newVertex.setFlag(GridVertex::IsAntecessorOfRefinedVertexInCurrentTreeSweep, not isNewFineGridVertex);
  newVertex.clearFlag(peano4::grid::GridVertex::HasBeenParentOfSubtreeVertexInPreviousTreeSweep);
  newVertex.clearFlag(peano4::grid::GridVertex::IsParentOfSubtreeVertexInCurrentTreeSweep);
  newVertex.clearFlag(peano4::grid::GridVertex::HasBeenVertexTypeHangingInPreviousTreeSweep);
  return newVertex;
}

peano4::grid::GridVertex peano4::grid::GridVertex::create(
  int                                          level,
  GridVertex::State                            state,
  const tarch::la::Vector<DIMENSIONS, double>& cellX,
  const tarch::la::Vector<DIMENSIONS, double>& cellH,
  int                                          vertexIndex,
  const std::array<int, TWO_POWER_D>&          adjacentRanks,
  bool                                         isNewFineGridVertex
) {
#if PEANO_DEBUG
  return create(level, state, getVertexCoordinate(cellX, cellH, vertexIndex), adjacentRanks, isNewFineGridVertex);
#else
  return create(level, state, tarch::la::Vector<DIMENSIONS, double>(0.0), adjacentRanks, isNewFineGridVertex);
#endif
}

bool peano4::grid::GridVertex::areAllVerticesRefined(GridVertex vertices[TWO_POWER_D]) {
  bool result = true;
  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, int kScalar) {
    result &= (vertices[kScalar].getState() == GridVertex::State::Refined);
  });
  return result;
}

bool peano4::grid::GridVertex::areAllVerticesUnrefined(GridVertex vertices[TWO_POWER_D]) {
  bool result = true;
  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, int kScalar) {
    result &= (vertices[kScalar].getState() == GridVertex::State::Unrefined);
  });
  return result;
}

/**
 * A vertex will be refined if it is already refined or currently refining. It
 * also will be refined if the erase is only triggered.
 */
bool peano4::grid::GridVertex::willVertexBeRefined(const GridVertex& vertex) {
  return vertex.getState() == GridVertex::State::Refining or vertex.getState() == GridVertex::State::Refined
    or vertex.getState() == GridVertex::State::EraseTriggered;
}

/**
 * A vertex has been refined if it is (already) refined or is erasing or the
 * erase has been triggered.
 */
bool peano4::grid::GridVertex::hasVertexBeenRefined(const GridVertex& vertex) {
  return vertex.getState() == GridVertex::State::Refined or vertex.getState() == GridVertex::State::EraseTriggered
    or vertex.getState() == GridVertex::State::Erasing;
}

std::bitset<TWO_POWER_D> peano4::grid::GridVertex::willVerticesBeRefined(GridVertex vertices[TWO_POWER_D]) {
  std::bitset<TWO_POWER_D> bitset;
  for (int i = 0; i < TWO_POWER_D; i++) {
    assertion(not willVertexBeRefined(vertices[i]) or vertices[i].getState() != GridVertex::State::HangingVertex);
    bitset.set(i, willVertexBeRefined(vertices[i]));
  }
  return bitset;
}

std::bitset<TWO_POWER_D> peano4::grid::GridVertex::haveVerticesBeenRefined(GridVertex vertices[TWO_POWER_D]) {
  std::bitset<TWO_POWER_D> bitset;
  for (int i = 0; i < TWO_POWER_D; i++) {
    assertion(not hasVertexBeenRefined(vertices[i]) or vertices[i].getState() != GridVertex::State::HangingVertex);
    bitset.set(i, hasVertexBeenRefined(vertices[i]));
  }
  return bitset;
}

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridVertex& vertex) {
  return (out << vertex.toString());
}

std::string peano4::grid::GridVertex::toString() const {
  std::ostringstream out;
  out << "GridVertex{State=" << static_cast<int>(state) << " (";

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

  out << ", Flags=0b" << std::bitset<8>(flags) << " [";
  bool first   = true;
  auto addFlag = [&](const char* name, std::uint8_t mask) {
    if (getFlag(mask)) {
      if (not first) {
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
  addFlag("HasBeenVertexTypeHangingInPreviousTreeSweep", HasBeenVertexTypeHangingInPreviousTreeSweep);
  addFlag("HasBeenSurroundedByRefinedCellsInPreviousTreeSweep", HasBeenSurroundedByRefinedCellsInPreviousTreeSweep);
  if (first) {
    out << "None";
  }
  out << "]";

  out << ", NumberOfAdjacentRefinedLocalCells=" << numberOfAdjacentRefinedLocalCells;
  out << ", NumberOfAdjacentRefinedRemoteCells=" << numberOfAdjacentRefinedRemoteCells;
  out << ", Level=" << level;
#if PEANO_DEBUG
  out << ", X=" << x;
#endif
  out << ", AdjacentRanks=(";
  for (std::size_t i = 0; i < adjacentRanks.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << adjacentRanks[i];
  }
  out << ")";

  out << "}";

  return out.str();
}

#ifdef PEANO_WITH_MPI
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
  GridVertex instances[2];

  constexpr int NumberOfAttributes =
#if PEANO_DEBUG
    8;
#else
    7;
#endif
  MPI_Datatype subtypes[NumberOfAttributes];
  int          blocklen[NumberOfAttributes];
  MPI_Aint     disp[NumberOfAttributes];

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
  blocklen[counter] = 1;
  MPI_Get_address(&instances[0].numberOfAdjacentRefinedRemoteCells, &disp[counter]);
  counter++;

#if PEANO_DEBUG
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = DIMENSIONS;
  MPI_Get_address(&(instances[0].getX().data()[0]), &disp[counter]);
  counter++;
#endif

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

void peano4::grid::GridVertex::shutdownDatatype() {
  freeForkDatatype();
  freeJoinDatatype();
  freeBoundaryExchangeDatatype();
  freeMultiscaleDataExchangeDatatype();
  freeGlobalCommunicationDatatype();
}

void peano4::grid::GridVertex::send(const GridVertex& buffer, int destination, int tag, MPI_Comm communicator) {
  tarch::mpi::send(&buffer, 1, Datatype, destination, tag, communicator);
}

void peano4::grid::GridVertex::receive(GridVertex& buffer, int source, int tag, MPI_Comm communicator) {
  MPI_Status status;
  tarch::mpi::receive(&buffer, 1, Datatype, source, tag, communicator, &status);
}

void peano4::grid::GridVertex::send(
  const GridVertex&     buffer,
  int                   destination,
  int                   tag,
  std::function<void()> startCommunicationFunctor,
  std::function<void()> waitFunctor,
  MPI_Comm              communicator
) {
  MPI_Request sendRequestHandle = MPI_REQUEST_NULL;
  tarch::mpi::isend(&buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle);
  startCommunicationFunctor();
  tarch::mpi::wait(&sendRequestHandle, waitFunctor, MPI_STATUS_IGNORE);
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
  MPI_Request receiveRequestHandle = MPI_REQUEST_NULL;
  tarch::mpi::irecv(&buffer, 1, Datatype, source, tag, communicator, &receiveRequestHandle);
  startCommunicationFunctor();
  tarch::mpi::wait(&receiveRequestHandle, waitFunctor, &status);
}

void peano4::grid::GridVertex::sendAndPollDanglingMessages(
  const GridVertex& message,
  int               destination,
  int               tag,
  MPI_Comm          communicator
) {
  GridVertex::send(
    message,
    destination,
    tag,
    []() {},
    [&]() { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); },
    communicator
  );
}

void peano4::grid::GridVertex::receiveAndPollDanglingMessages(
  GridVertex& message,
  int         source,
  int         tag,
  MPI_Comm    communicator
) {
  GridVertex::receive(
    message,
    source,
    tag,
    []() {},
    [&]() { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); },
    communicator
  );
}
#endif
