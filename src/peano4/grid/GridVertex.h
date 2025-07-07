// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>

#ifdef Parallel
#include <functional>
#include <mpi.h>
#endif

#include "peano4/grid/LoadStoreComputeFlag.h"
#include "peano4/grid/TraversalObserver.h"
#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/services/ServiceRepository.h"

namespace peano4 {
  namespace grid {
    struct GridVertex;
  } // namespace grid
} // namespace peano4

struct peano4::grid::GridVertex {
public:
  enum class State : int {
    HangingVertex       = 0,
    New                 = 1,
    Unrefined           = 2,
    Refined             = 3,
    RefinementTriggered = 4,
    Refining            = 5,
    EraseTriggered      = 6,
    Erasing             = 7,
    Delete              = 8
  };

  GridVertex() = default;
  GridVertex(
    const State                                  __state,
    const tarch::la::Vector<TwoPowerD, int>&     __adjacentRanks,
    const tarch::la::Vector<TwoPowerD, int>&     __backupOfAdjacentRanks,
    const bool                                   __hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep,
    const bool                                   __isAntecessorOfRefinedVertexInCurrentTreeSweep,
    const bool                                   __hasBeenParentOfSubtreeVertexInPreviousTreeSweep,
    const bool                                   __isParentOfSubtreeVertexInCurrentTreeSweep,
    const int                                    __numberOfAdjacentRefinedLocalCells,
    const tarch::la::Vector<Dimensions, double>& __x,
    const int                                    __level
  );
  GridVertex(const GridVertex& copy);
  GridVertex& operator=(const GridVertex& other);

  inline peano4::grid::GridVertex::State getState() const { return _state; }
  inline void                            setState(State value) { _state = value; }

  inline tarch::la::Vector<TwoPowerD, int> getAdjacentRanks() const { return _adjacentRanks; }
  inline void setAdjacentRanks(const tarch::la::Vector<TwoPowerD, int>& value) { _adjacentRanks = value; }
  inline int  getAdjacentRanks(int index) const { return _adjacentRanks[index]; }
  inline void setAdjacentRanks(int index, int value) { _adjacentRanks[index] = value; }

  inline tarch::la::Vector<TwoPowerD, int> getBackupOfAdjacentRanks() const { return _backupOfAdjacentRanks; }
  inline void                              setBackupOfAdjacentRanks(const tarch::la::Vector<TwoPowerD, int>& value) {
    _backupOfAdjacentRanks = value;
  }
  inline int  getBackupOfAdjacentRanks(int index) const { return _backupOfAdjacentRanks(index); }
  inline void setBackupOfAdjacentRanks(int index, int value) { _backupOfAdjacentRanks(index) = value; }

  inline bool getHasBeenAntecessorOfRefinedVertexInPreviousTreeSweep() const {
    return _hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep;
  }
  inline void setHasBeenAntecessorOfRefinedVertexInPreviousTreeSweep(bool value) {
    _hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep = value;
  }

  inline bool getIsAntecessorOfRefinedVertexInCurrentTreeSweep() const {
    return _isAntecessorOfRefinedVertexInCurrentTreeSweep;
  }
  inline void setIsAntecessorOfRefinedVertexInCurrentTreeSweep(bool value) {
    _isAntecessorOfRefinedVertexInCurrentTreeSweep = value;
  }

  inline bool getHasBeenParentOfSubtreeVertexInPreviousTreeSweep() const {
    return _hasBeenParentOfSubtreeVertexInPreviousTreeSweep;
  }
  inline void setHasBeenParentOfSubtreeVertexInPreviousTreeSweep(bool value) {
    _hasBeenParentOfSubtreeVertexInPreviousTreeSweep = value;
  }

  inline bool getIsParentOfSubtreeVertexInCurrentTreeSweep() const {
    return _isParentOfSubtreeVertexInCurrentTreeSweep;
  }
  inline void setIsParentOfSubtreeVertexInCurrentTreeSweep(bool value) {
    _isParentOfSubtreeVertexInCurrentTreeSweep = value;
  }

  inline int  getNumberOfAdjacentRefinedLocalCells() const { return _numberOfAdjacentRefinedLocalCells; }
  inline void setNumberOfAdjacentRefinedLocalCells(int value) { _numberOfAdjacentRefinedLocalCells = value; }

#if PeanoDebug > 0
  inline tarch::la::Vector<Dimensions, double> getX() const { return _x; }
  inline void                                  setX(const tarch::la::Vector<Dimensions, double>& value) { _x = value; }
  inline double                                getX(int index) const { return _x(index); }
  inline void                                  setX(int index, double value) { _x(index) = value; }
#endif

  inline int  getLevel() const { return _level; }
  inline void setLevel(int value) { _level = value; }

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
  static MPI_Datatype getGlobalCommunicationDatatype();

  static void freeForkDatatype();
  static void freeJoinDatatype();
  static void freeBoundaryExchangeDatatype();
  static void freeMultiscaleDataExchangeDatatype();
  static void freeGlobalCommunicationDatatype();

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
  static void send(const peano4::grid::GridVertex& buffer, int destination, int tag, MPI_Comm communicator);
  static void receive(peano4::grid::GridVertex& buffer, int source, int tag, MPI_Comm communicator);

  /**
   * Alternative to the other send() where I trigger a non-blocking send an
   * then invoke the functor until the corresponding MPI_Test tells me that
   * the message went through. In systems with heavy MPI usage, this can
   * help to avoid deadlocks.
   */
  static void send(
    const peano4::grid::GridVertex& buffer,
    int                             destination,
    int                             tag,
    std::function<void()>           startCommunicationFunctor,
    std::function<void()>           waitFunctor,
    MPI_Comm                        communicator
  );
  static void receive(
    peano4::grid::GridVertex& buffer,
    int                       source,
    int                       tag,
    std::function<void()>     startCommunicationFunctor,
    std::function<void()>     waitFunctor,
    MPI_Comm                  communicator
  );
#endif

  enum ObjectConstruction { NoData };

  GridVertex(ObjectConstruction):
    GridVertex() {}

#ifdef Parallel
  static void sendAndPollDanglingMessages(
    const peano4::grid::GridVertex& message,
    int                             destination,
    int                             tag,
    MPI_Comm                        communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
  static void receiveAndPollDanglingMessages(
    peano4::grid::GridVertex& message,
    int                       source,
    int                       tag,
    MPI_Comm                  communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
#endif

  std::string toString() const;

private:
  State                             _state;
  tarch::la::Vector<TwoPowerD, int> _adjacentRanks;
  tarch::la::Vector<TwoPowerD, int> _backupOfAdjacentRanks;
  bool                              _hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep;
  bool                              _isAntecessorOfRefinedVertexInCurrentTreeSweep;
  bool                              _hasBeenParentOfSubtreeVertexInPreviousTreeSweep;
  bool                              _isParentOfSubtreeVertexInCurrentTreeSweep;
  int                               _numberOfAdjacentRefinedLocalCells;

#if PeanoDebug > 0
  tarch::la::Vector<Dimensions, double> _x;
#endif

  int _level;

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
