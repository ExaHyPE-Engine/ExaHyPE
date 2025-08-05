// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridTraversalEvent.h"

#include <algorithm>
#include <sstream>

peano4::grid::GridTraversalEvent::GridTraversalEvent(
  const tarch::la::Vector<DIMENSIONS, double>& __x,
  const tarch::la::Vector<DIMENSIONS, double>& __h,
  const std::bitset<TWO_POWER_D>&                __hasBeenRefined,
  const std::bitset<TWO_POWER_D>&                __willBeRefined,
  const std::bitset<TWO_POWER_D>&                __isVertexLocal,
  const std::bitset<TWO_POWER_D>&                __isParentVertexLocal,
  const std::bitset<TWO_POWER_D>&                __isVertexParentOfSubtree,
  const std::bitset<TWO_TIMES_D>&                __isFaceLocal,
  const bool                                   __isCellLocal,
  const bool                                   __isParentCellLocal,
  const std::bitset<TWO_POWER_D>&                __isVertexAdjacentToParallelDomainBoundary,
  const std::bitset<TWO_TIMES_D>&                __isFaceAdjacentToParallelDomainBoundary,
  const tarch::la::Vector<TWO_POWER_D, int>&     __numberOfAdjacentTreesPerVertex,
  const std::bitset<THREE_POWER_D>&              __isAdjacentCellLocal,
  const tarch::la::Vector<TWO_POWER_D, int>&     __vertexDataFrom,
  const tarch::la::Vector<TWO_POWER_D, int>&     __vertexDataTo,
  const tarch::la::Vector<TWO_TIMES_D, int>&     __faceDataFrom,
  const tarch::la::Vector<TWO_TIMES_D, int>&     __faceDataTo,
  const int                                    __cellData,
  const tarch::la::Vector<DIMENSIONS, int>&    __relativePositionToFather,
  const int                                    __invokingSpacetree,
  const bool                                   __invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing
) {
  setX(__x);
  setH(__h);
  setHasBeenRefined(__hasBeenRefined);
  setWillBeRefined(__willBeRefined);
  setIsVertexLocal(__isVertexLocal);
  setIsParentVertexLocal(__isParentVertexLocal);
  setIsVertexParentOfSubtree(__isVertexParentOfSubtree);
  setIsFaceLocal(__isFaceLocal);
  setIsCellLocal(__isCellLocal);
  setIsParentCellLocal(__isParentCellLocal);
  setIsVertexAdjacentToParallelDomainBoundary(__isVertexAdjacentToParallelDomainBoundary);
  setIsFaceAdjacentToParallelDomainBoundary(__isFaceAdjacentToParallelDomainBoundary);
  setNumberOfAdjacentTreesPerVertex(__numberOfAdjacentTreesPerVertex);
  setIsAdjacentCellLocal(__isAdjacentCellLocal);
  setVertexDataFrom(__vertexDataFrom);
  setVertexDataTo(__vertexDataTo);
  setFaceDataFrom(__faceDataFrom);
  setFaceDataTo(__faceDataTo);
  setCellData(__cellData);
  setRelativePositionToFather(__relativePositionToFather);
  setInvokingSpacetree(__invokingSpacetree);
  setInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing(__invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing);
}

peano4::grid::GridTraversalEvent::GridTraversalEvent(const GridTraversalEvent& copy) {
  setX(copy.getX());
  setH(copy.getH());
  setHasBeenRefined(copy.getHasBeenRefined());
  setWillBeRefined(copy.getWillBeRefined());
  setIsVertexLocal(copy.getIsVertexLocal());
  setIsParentVertexLocal(copy.getIsParentVertexLocal());
  setIsVertexParentOfSubtree(copy.getIsVertexParentOfSubtree());
  setIsFaceLocal(copy.getIsFaceLocal());
  setIsCellLocal(copy.getIsCellLocal());
  setIsParentCellLocal(copy.getIsParentCellLocal());
  setIsVertexAdjacentToParallelDomainBoundary(copy.getIsVertexAdjacentToParallelDomainBoundary());
  setIsFaceAdjacentToParallelDomainBoundary(copy.getIsFaceAdjacentToParallelDomainBoundary());
  setNumberOfAdjacentTreesPerVertex(copy.getNumberOfAdjacentTreesPerVertex());
  setIsAdjacentCellLocal(copy.getIsAdjacentCellLocal());
  setVertexDataFrom(copy.getVertexDataFrom());
  setVertexDataTo(copy.getVertexDataTo());
  setFaceDataFrom(copy.getFaceDataFrom());
  setFaceDataTo(copy.getFaceDataTo());
  setCellData(copy.getCellData());
  setRelativePositionToFather(copy.getRelativePositionToFather());
  setInvokingSpacetree(copy.getInvokingSpacetree());
  setInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing(
    copy.getInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing()
  );
}

std::string peano4::grid::GridTraversalEvent::toString() const {
  std::ostringstream out;
  out << "(";
  out << "x=" << getX();
  out << ",";
  out << "h=" << getH();
  out << ",";
  out << "hasBeenRefined=" << getHasBeenRefined();
  out << ",";
  out << "willBeRefined=" << getWillBeRefined();
  out << ",";
  out << "isVertexLocal=" << getIsVertexLocal();
  out << ",";
  out << "isParentVertexLocal=" << getIsParentVertexLocal();
  out << ",";
  out << "isVertexParentOfSubtree=" << getIsVertexParentOfSubtree();
  out << ",";
  out << "isFaceLocal=" << getIsFaceLocal();
  out << ",";
  out << "isCellLocal=" << _isCellLocal;
  out << ",";
  out << "isParentCellLocal=" << _isParentCellLocal;
  out << ",";
  out << "isVertexAdjacentToParallelDomainBoundary=" << getIsVertexAdjacentToParallelDomainBoundary();
  out << ",";
  out << "isFaceAdjacentToParallelDomainBoundary=" << getIsFaceAdjacentToParallelDomainBoundary();
  out << ",";
  out << "numberOfAdjacentTreesPerVertex=" << _numberOfAdjacentTreesPerVertex;
  out << ",";
  out << "isAdjacentCellLocal=" << getIsAdjacentCellLocal();
  out << ",";
  out << "vertexDataFrom=" << _vertexDataFrom;
  out << ",";
  out << "vertexDataTo=" << _vertexDataTo;
  out << ",";
  out << "faceDataFrom=" << _faceDataFrom;
  out << ",";
  out << "faceDataTo=" << _faceDataTo;
  out << ",";
  out << "cellData=" << _cellData;
  out << ",";
  out << "relativePositionToFather=" << getRelativePositionToFather();
  out << ",";
  out << "invokingSpacetree=" << _invokingSpacetree;
  out << ",";
  out << "invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing="
      << _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing;
  out << ")";
  return out.str();
}

#ifdef WITH_MPI

#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
MPI_Datatype peano4::grid::GridTraversalEvent::Datatype = MPI_DATATYPE_NULL;
#endif

MPI_Datatype peano4::grid::GridTraversalEvent::getForkDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridTraversalEvent::getGlobalCommunicationDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridTraversalEvent::getJoinDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridTraversalEvent::getBoundaryExchangeDatatype() { return Datatype; }
MPI_Datatype peano4::grid::GridTraversalEvent::getMultiscaleDataExchangeDatatype() { return Datatype; }

void peano4::grid::GridTraversalEvent::freeForkDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridTraversalEvent::freeGlobalCommunicationDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridTraversalEvent::freeJoinDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridTraversalEvent::freeBoundaryExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

void peano4::grid::GridTraversalEvent::freeMultiscaleDataExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL) {
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}

int peano4::grid::GridTraversalEvent::getSenderRank() const { return _senderDestinationRank; }

void peano4::grid::GridTraversalEvent::initDatatype() {
#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  peano4::grid::GridTraversalEvent instances[2];

  int NumberOfAttributes = 0;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;

  MPI_Datatype* subtypes = new MPI_Datatype[NumberOfAttributes];
  int*          blocklen = new int[NumberOfAttributes];
  MPI_Aint*     disp     = new MPI_Aint[NumberOfAttributes];

  int counter       = 0;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = DIMENSIONS;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = DIMENSIONS;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = TWO_POWER_D;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = TWO_POWER_D;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = TWO_POWER_D;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = TWO_TIMES_D;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = TWO_TIMES_D;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = DIMENSIONS;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;

  MPI_Aint baseFirstInstance;
  MPI_Aint baseSecondInstance;
  MPI_Get_address(&instances[0], &baseFirstInstance);
  MPI_Get_address(&instances[1], &baseSecondInstance);

  counter = 0;
  MPI_Get_address(&(instances[0]._x.data()[0]), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._h.data()[0]), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._hasBeenRefined), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._willBeRefined), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._isVertexLocal), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._isParentVertexLocal), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._isVertexParentOfSubtree), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._isFaceLocal), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._isCellLocal), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._isParentCellLocal), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._isVertexAdjacentToParallelDomainBoundary), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._isFaceAdjacentToParallelDomainBoundary), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._numberOfAdjacentTreesPerVertex.data()[0]), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._isAdjacentCellLocal), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._vertexDataFrom.data()[0]), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._vertexDataTo.data()[0]), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._faceDataFrom.data()[0]), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._faceDataTo.data()[0]), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._cellData), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._relativePositionToFather.data()[0]), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._invokingSpacetree), &disp[counter]);
  counter++;
  MPI_Get_address(&(instances[0]._invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing), &disp[counter]);
  counter++;

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
  if (errorCode)
    std::cerr << "error constructing MPI datatype in " << __FILE__ << ":" << __LINE__ << std::endl;

  delete[] subtypes;
  delete[] blocklen;
  delete[] disp;

#else
  // invoke routine once to trigger lazy initialisation
  getForkDatatype();
  getJoinDatatype();
  getBoundaryExchangeDatatype();
  getMultiscaleDataExchangeDatatype();
  getGlobalCommunicationDatatype();
#endif
}

void peano4::grid::GridTraversalEvent::shutdownDatatype() {
#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  freeForkDatatype();
  freeJoinDatatype();
  freeBoundaryExchangeDatatype();
  freeMultiscaleDataExchangeDatatype();
  freeGlobalCommunicationDatatype();
#else
  MPI_Datatype type = Datatype;
  MPI_Type_free(&type);
#endif
}

void peano4::grid::GridTraversalEvent::
  send(const peano4::grid::GridTraversalEvent& buffer, int destination, int tag, MPI_Comm communicator) {
  MPI_Send(&buffer, 1, Datatype, destination, tag, communicator);
}

void peano4::grid::GridTraversalEvent::
  receive(peano4::grid::GridTraversalEvent& buffer, int source, int tag, MPI_Comm communicator) {
  MPI_Status status;
  MPI_Recv(&buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}

void peano4::grid::GridTraversalEvent::send(
  const peano4::grid::GridTraversalEvent& buffer,
  int                                     destination,
  int                                     tag,
  std::function<void()>                   startCommunicationFunctor,
  std::function<void()>                   waitFunctor,
  MPI_Comm                                communicator
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

void peano4::grid::GridTraversalEvent::receive(
  peano4::grid::GridTraversalEvent& buffer,
  int                               source,
  int                               tag,
  std::function<void()>             startCommunicationFunctor,
  std::function<void()>             waitFunctor,
  MPI_Comm                          communicator
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
  buffer._senderDestinationRank = status.MPI_SOURCE;
}
#endif

#ifdef WITH_MPI
void peano4::grid::GridTraversalEvent::sendAndPollDanglingMessages(
  const peano4::grid::GridTraversalEvent& message,
  int                                     destination,
  int                                     tag,
  MPI_Comm                                communicator
) {
  peano4::grid::GridTraversalEvent::send(
    message,
    destination,
    tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance()
        .writeTimeOutWarning("peano4::grid::GridTraversalEvent", "sendAndPollDanglingMessages()", destination, tag);
      tarch::mpi::Rank::getInstance()
        .triggerDeadlockTimeOut("peano4::grid::GridTraversalEvent", "sendAndPollDanglingMessages()", destination, tag);
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}

void peano4::grid::GridTraversalEvent::receiveAndPollDanglingMessages(
  peano4::grid::GridTraversalEvent& message,
  int                               source,
  int                               tag,
  MPI_Comm                          communicator
) {
  peano4::grid::GridTraversalEvent::receive(
    message,
    source,
    tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance()
        .writeTimeOutWarning("peano4::grid::GridTraversalEvent", "receiveAndPollDanglingMessages()", source, tag);
      tarch::mpi::Rank::getInstance()
        .triggerDeadlockTimeOut("peano4::grid::GridTraversalEvent", "receiveAndPollDanglingMessages()", source, tag);
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}

#endif
