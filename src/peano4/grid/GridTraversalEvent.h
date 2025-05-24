// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>

#ifdef Parallel
#include <functional>
#include <mpi.h>
#endif

#include "LoadStoreComputeFlag.h"
#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/services/ServiceRepository.h"
#include "TraversalObserver.h"

namespace peano4 {
  namespace grid {
    struct GridTraversalEvent;
  } // namespace grid
} // namespace peano4


struct peano4::grid::GridTraversalEvent {
public:
  GridTraversalEvent() = default;
  GridTraversalEvent(
    const tarch::la::Vector<Dimensions, double>& __x,
    const tarch::la::Vector<Dimensions, double>& __h,
    const std::bitset<TwoPowerD>&                __hasBeenRefined,
    const std::bitset<TwoPowerD>&                __willBeRefined,
    const std::bitset<TwoPowerD>&                __isVertexLocal,
    const std::bitset<TwoPowerD>&                __isParentVertexLocal,
    const std::bitset<TwoPowerD>&                __isVertexParentOfSubtree,
    const std::bitset<TwoTimesD>&                __isFaceLocal,
    const bool                                   __isCellLocal,
    const bool                                   __isParentCellLocal,
    const std::bitset<TwoPowerD>&                __isVertexAdjacentToParallelDomainBoundary,
    const std::bitset<TwoTimesD>&                __isFaceAdjacentToParallelDomainBoundary,
    const tarch::la::Vector<TwoPowerD, int>&     __numberOfAdjacentTreesPerVertex,
    const std::bitset<ThreePowerD>&              __isAdjacentCellLocal,
    const tarch::la::Vector<TwoPowerD, int>&     __vertexDataFrom,
    const tarch::la::Vector<TwoPowerD, int>&     __vertexDataTo,
    const tarch::la::Vector<TwoTimesD, int>&     __faceDataFrom,
    const tarch::la::Vector<TwoTimesD, int>&     __faceDataTo,
    const int                                    __cellData,
    const tarch::la::Vector<Dimensions, int>&    __relativePositionToFather,
    const int                                    __invokingSpacetree,
    const bool                                   __invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing
  );
  GridTraversalEvent(const GridTraversalEvent& copy);

  inline tarch::la::Vector<Dimensions, double> getX() const { return _x; }
  inline double                                getX(int index) const { return _x[index]; }
  inline void                                  setX(int index, double value) { _x[index] = value; }
  inline void                                  setX(const tarch::la::Vector<Dimensions, double>& value) { _x = value; }

  inline tarch::la::Vector<Dimensions, double> getH() const { return _h; }
  inline double                                getH(int index) const { return _h[index]; }
  inline void                                  setH(int index, double value) { _h[index] = value; }
  inline void                                  setH(const tarch::la::Vector<Dimensions, double>& value) { _h = value; }

  inline std::bitset<TwoPowerD> getHasBeenRefined() const { return _hasBeenRefined; }
  inline bool                   getHasBeenRefined(int index) const { return _hasBeenRefined[index]; }
  inline void                   setHasBeenRefined(int index, bool value) { _hasBeenRefined[index] = value; }
  inline void                   setHasBeenRefined(const std::bitset<TwoPowerD>& value) { _hasBeenRefined = value; }
  inline void                   flipHasBeenRefined(int index) { _hasBeenRefined[index] = not _hasBeenRefined[index]; }

  inline std::bitset<TwoPowerD> getWillBeRefined() const { return _willBeRefined; }
  inline bool                   getWillBeRefined(int index) const { return _willBeRefined[index]; }
  inline void                   setWillBeRefined(int index, bool value) { _willBeRefined[index] = value; }
  inline void                   setWillBeRefined(const std::bitset<TwoPowerD>& value) { _willBeRefined = value; }
  inline void                   flipWillBeRefined(int index) { _willBeRefined[index] = not _willBeRefined[index]; }

  inline std::bitset<TwoPowerD> getIsVertexLocal() const { return _isVertexLocal; }
  inline bool                   getIsVertexLocal(int index) const { return _isVertexLocal[index]; }
  inline void                   setIsVertexLocal(int index, bool value) { _isVertexLocal[index] = value; }
  inline void                   setIsVertexLocal(const std::bitset<TwoPowerD>& value) { _isVertexLocal = value; }
  inline void                   flipIsVertexLocal(int index) { _isVertexLocal[index] = not _isVertexLocal[index]; }

  inline std::bitset<TwoPowerD> getIsParentVertexLocal() const { return _isParentVertexLocal; }
  inline bool                   getIsParentVertexLocal(int index) const { return _isParentVertexLocal[index]; }
  inline void                   setIsParentVertexLocal(int index, bool value) { _isParentVertexLocal[index] = value; }
  inline void setIsParentVertexLocal(const std::bitset<TwoPowerD>& value) { _isParentVertexLocal = value; }
  inline void flipIsParentVertexLocal(int index) { _isParentVertexLocal[index] = not _isParentVertexLocal[index]; }

  inline std::bitset<TwoPowerD> getIsVertexParentOfSubtree() const { return _isVertexParentOfSubtree; }
  inline bool                   getIsVertexParentOfSubtree(int index) const { return _isVertexParentOfSubtree[index]; }
  inline void setIsVertexParentOfSubtree(int index, bool value) { _isVertexParentOfSubtree[index] = value; }
  inline void setIsVertexParentOfSubtree(const std::bitset<TwoPowerD>& value) { _isVertexParentOfSubtree = value; }
  inline void flipIsVertexParentOfSubtree(int index) {
    _isVertexParentOfSubtree[index] = not _isVertexParentOfSubtree[index];
  }

  inline std::bitset<TwoTimesD> getIsFaceLocal() const { return _isFaceLocal; }
  inline bool                   getIsFaceLocal(int index) const { return _isFaceLocal[index]; }
  inline void                   setIsFaceLocal(int index, bool value) { _isFaceLocal[index] = value; }
  inline void                   setIsFaceLocal(const std::bitset<TwoTimesD>& value) { _isFaceLocal = value; }
  inline void                   flipIsFaceLocal(int index) { _isFaceLocal[index] = not _isFaceLocal[index]; }

  inline bool getIsCellLocal() const { return _isCellLocal; }
  inline void setIsCellLocal(bool value) { _isCellLocal = value; }

  inline bool getIsParentCellLocal() const { return _isParentCellLocal; }
  inline void setIsParentCellLocal(bool value) { _isParentCellLocal = value; }

  inline std::bitset<TwoPowerD> getIsVertexAdjacentToParallelDomainBoundary() const {
    return _isVertexAdjacentToParallelDomainBoundary;
  }
  inline bool getIsVertexAdjacentToParallelDomainBoundary(int index) const {
    return _isVertexAdjacentToParallelDomainBoundary[index];
  }
  inline void setIsVertexAdjacentToParallelDomainBoundary(int index, bool value) {
    _isVertexAdjacentToParallelDomainBoundary[index] = value;
  }
  inline void setIsVertexAdjacentToParallelDomainBoundary(const std::bitset<TwoPowerD>& value) {
    _isVertexAdjacentToParallelDomainBoundary = value;
  }
  inline void flipIsVertexAdjacentToParallelDomainBoundary(int index) {
    _isVertexAdjacentToParallelDomainBoundary[index] = not _isVertexAdjacentToParallelDomainBoundary[index];
  }

  inline std::bitset<TwoTimesD> getIsFaceAdjacentToParallelDomainBoundary() const {
    return _isFaceAdjacentToParallelDomainBoundary;
  }
  inline void setIsFaceAdjacentToParallelDomainBoundary(const std::bitset<TwoTimesD>& value) {
    _isFaceAdjacentToParallelDomainBoundary = value;
  }
  inline bool getIsFaceAdjacentToParallelDomainBoundary(int index) const {
    return _isFaceAdjacentToParallelDomainBoundary[index];
  }
  inline void setIsFaceAdjacentToParallelDomainBoundary(int index, bool value) {
    _isFaceAdjacentToParallelDomainBoundary[index] = value;
  }
  inline void flipIsFaceAdjacentToParallelDomainBoundary(int index) {
    _isFaceAdjacentToParallelDomainBoundary[index] = not _isFaceAdjacentToParallelDomainBoundary[index];
  }

  inline tarch::la::Vector<TwoPowerD, int> getNumberOfAdjacentTreesPerVertex() const {
    return _numberOfAdjacentTreesPerVertex;
  }
  inline void setNumberOfAdjacentTreesPerVertex(const tarch::la::Vector<TwoPowerD, int>& value) {
    _numberOfAdjacentTreesPerVertex = value;
  }
  inline int  getNumberOfAdjacentTreesPerVertex(int index) const { return _numberOfAdjacentTreesPerVertex[index]; }
  inline void setNumberOfAdjacentTreesPerVertex(int index, int value) {
    _numberOfAdjacentTreesPerVertex[index] = value;
  }

  inline std::bitset<ThreePowerD> getIsAdjacentCellLocal() const { return _isAdjacentCellLocal; }
  inline void setIsAdjacentCellLocal(const std::bitset<ThreePowerD>& value) { _isAdjacentCellLocal = value; }
  inline bool getIsAdjacentCellLocal(int index) const { return _isAdjacentCellLocal[index]; }
  inline void setIsAdjacentCellLocal(int index, bool value) { _isAdjacentCellLocal[index] = value; }
  inline void flipIsAdjacentCellLocal(int index) { _isAdjacentCellLocal[index] = not _isAdjacentCellLocal[index]; }

  inline tarch::la::Vector<TwoPowerD, int> getVertexDataFrom() const { return _vertexDataFrom; }
  inline void setVertexDataFrom(const tarch::la::Vector<TwoPowerD, int>& value) { _vertexDataFrom = value; }
  inline int  getVertexDataFrom(int index) const { return _vertexDataFrom[index]; }
  inline void setVertexDataFrom(int index, int value) { _vertexDataFrom[index] = value; }

  inline tarch::la::Vector<TwoPowerD, int> getVertexDataTo() const { return _vertexDataTo; }
  inline void setVertexDataTo(const tarch::la::Vector<TwoPowerD, int>& value) { _vertexDataTo = value; }
  inline int  getVertexDataTo(int index) const { return _vertexDataTo[index]; }
  inline void setVertexDataTo(int index, int value) { _vertexDataTo[index] = value; }

  inline tarch::la::Vector<TwoTimesD, int> getFaceDataFrom() const { return _faceDataFrom; }
  inline void setFaceDataFrom(const tarch::la::Vector<TwoTimesD, int>& value) { _faceDataFrom = value; }
  inline int  getFaceDataFrom(int index) const { return _faceDataFrom[index]; }
  inline void setFaceDataFrom(int index, int value) { _faceDataFrom[index] = value; }

  inline tarch::la::Vector<TwoTimesD, int> getFaceDataTo() const { return _faceDataTo; }
  inline void setFaceDataTo(const tarch::la::Vector<TwoTimesD, int>& value) { _faceDataTo = value; }
  inline int  getFaceDataTo(int index) const { return _faceDataTo[index]; }
  inline void setFaceDataTo(int index, int value) { _faceDataTo[index] = value; }

  inline int  getCellData() const { return _cellData; }
  inline void setCellData(int value) { _cellData = value; }

  inline tarch::la::Vector<Dimensions, int> getRelativePositionToFather() const { return _relativePositionToFather; }
  inline void setRelativePositionToFather(const tarch::la::Vector<Dimensions, int>& value) {
    _relativePositionToFather = value;
  }
  inline int  getRelativePositionToFather(int index) const { return _relativePositionToFather[index]; }
  inline void setRelativePositionToFather(int index, int value) { _relativePositionToFather[index] = value; }

  inline int  getInvokingSpacetree() const { return _invokingSpacetree; }
  inline void setInvokingSpacetree(int value) { _invokingSpacetree = value; }
  inline bool getInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing() const {
    return _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing;
  }
  inline void setInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing(bool value) {
    _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing = value;
  }

#ifdef Parallel
  /**
   * Hands out MPI datatype if we work without the LLVM MPI extension.
   * If we work with this additional feature, this is the routine where
   * the lazy initialisation is done and the datatype is also cached.
   */
  static MPI_Datatype getForkDatatype();
  static MPI_Datatype getJoinDatatype();
  static MPI_Datatype getBoundaryExchangeDatatype();
  static MPI_Datatype getMultiscaleDataExchangeDatatype();
  static MPI_Datatype getGlobalCommunciationDatatype();

  static void freeForkDatatype();
  static void freeJoinDatatype();
  static void freeBoundaryExchangeDatatype();
  static void freeMultiscaleDataExchangeDatatype();
  static void freeGlobalCommunciationDatatype();

  /**
   * @return The rank of the sender of an object. It only make ssense to call
   *         this routine after you've invoked receive with MPI_ANY_SOURCE.
   */
  int getSenderRank() const;

  /**
   * Wrapper around getDatatype() to trigger lazy evaluation if we
   * use the lazy initialisation.
   */
  static void initDatatype();

  /**
   * Free the underlying MPI datatype.
   */
  static void shutdownDatatype();

  /**
   * In DaStGen (the first version), I had a non-static version of the send
   * as well as the receive. However, this did not work with newer C++11
   * versions, as a member function using this as pointer usually doesn't
   * see the vtable while the init sees the object from outside, i.e.
   * including a vtable. So this routine now is basically an alias for a
   * blocking MPI_Send.
   */
  static void send(const peano4::grid::GridTraversalEvent& buffer, int destination, int tag, MPI_Comm communicator);
  static void receive(peano4::grid::GridTraversalEvent& buffer, int source, int tag, MPI_Comm communicator);

  /**
   * Alternative to the other send() where I trigger a non-blocking send an
   * then invoke the functor until the corresponding MPI_Test tells me that
   * the message went through. In systems with heavy MPI usage, this can
   * help to avoid deadlocks.
   */
  static void send(
    const peano4::grid::GridTraversalEvent& buffer,
    int                                     destination,
    int                                     tag,
    std::function<void()>                   startCommunicationFunctor,
    std::function<void()>                   waitFunctor,
    MPI_Comm                                communicator
  );
  static void receive(
    peano4::grid::GridTraversalEvent& buffer,
    int                               source,
    int                               tag,
    std::function<void()>             startCommunicationFunctor,
    std::function<void()>             waitFunctor,
    MPI_Comm                          communicator
  );
#endif


  enum ObjectConstruction { NoData };

  GridTraversalEvent(ObjectConstruction):
    GridTraversalEvent() {}

#ifdef Parallel
  static void sendAndPollDanglingMessages(
    const peano4::grid::GridTraversalEvent& message,
    int                                     destination,
    int                                     tag,
    MPI_Comm                                communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
  static void receiveAndPollDanglingMessages(
    peano4::grid::GridTraversalEvent& message,
    int                               source,
    int                               tag,
    MPI_Comm                          communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
#endif

  std::string toString() const;

private:
  tarch::la::Vector<Dimensions, double> _x;
  tarch::la::Vector<Dimensions, double> _h;
  std::bitset<TwoPowerD>                _hasBeenRefined;
  std::bitset<TwoPowerD>                _willBeRefined;
  std::bitset<TwoPowerD>                _isVertexLocal;
  std::bitset<TwoPowerD>                _isParentVertexLocal;
  std::bitset<TwoPowerD>                _isVertexParentOfSubtree;
  std::bitset<TwoTimesD>                _isFaceLocal;
  bool                                  _isCellLocal;
  bool                                  _isParentCellLocal;
  std::bitset<TwoPowerD>                _isVertexAdjacentToParallelDomainBoundary;
  std::bitset<TwoTimesD>                _isFaceAdjacentToParallelDomainBoundary;
  tarch::la::Vector<TwoPowerD, int>     _numberOfAdjacentTreesPerVertex;
  std::bitset<ThreePowerD>              _isAdjacentCellLocal;
  tarch::la::Vector<TwoPowerD, int>     _vertexDataFrom;
  tarch::la::Vector<TwoPowerD, int>     _vertexDataTo;
  tarch::la::Vector<TwoTimesD, int>     _faceDataFrom;
  tarch::la::Vector<TwoTimesD, int>     _faceDataTo;
  int                                   _cellData;
  tarch::la::Vector<Dimensions, int>    _relativePositionToFather;
  int                                   _invokingSpacetree;
  bool                                  _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing;

#ifdef Parallel
private:
  int _senderDestinationRank;

#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  /**
   * Whenever we use LLVM's MPI extension (DaStGe), we rely on lazy
   * initialisation of the datatype. However, Peano calls init explicitly
   * in most cases. Without the LLVM extension which caches the MPI
   * datatype once constructed, this field stores the type.
   */
  static MPI_Datatype Datatype;
#endif
#endif
};
