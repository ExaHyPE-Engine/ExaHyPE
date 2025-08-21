// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>

#ifdef WITH_MPI
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
    struct GridControlEvent;
  } // namespace grid
} // namespace peano4

struct peano4::grid::GridControlEvent {
public:
  enum class RefinementControl : int { Refine = 0, Erase = 1 };

  GridControlEvent() = default;
  GridControlEvent(
    const RefinementControl                      __refinementControl,
    const tarch::la::Vector<DIMENSIONS, double>& __offset,
    const tarch::la::Vector<DIMENSIONS, double>& __width,
    const tarch::la::Vector<DIMENSIONS, double>& __h
  );
  GridControlEvent(const GridControlEvent& copy) = default;
  ~GridControlEvent()                            = default;

  inline peano4::grid::GridControlEvent::RefinementControl getRefinementControl() const { return _refinementControl; }
  inline void setRefinementControl(RefinementControl value) { _refinementControl = value; }

  inline tarch::la::Vector<DIMENSIONS, double> getOffset() const { return _offset; }
  inline void   setOffset(const tarch::la::Vector<DIMENSIONS, double>& value) { _offset = value; }
  inline double getOffset(int index) const { return _offset(index); }
  inline void   setOffset(int index, double value) { _offset(index) = value; }

  inline tarch::la::Vector<DIMENSIONS, double> getWidth() const { return _width; }
  inline void   setWidth(const tarch::la::Vector<DIMENSIONS, double>& value) { _width = value; }
  inline double getWidth(int index) const { return _width(index); }
  inline void   setWidth(int index, double value) { _width(index) = value; }

  inline tarch::la::Vector<DIMENSIONS, double> getH() const { return _h; }
  inline void                                  setH(const tarch::la::Vector<DIMENSIONS, double>& value) { _h = value; }
  inline double                                getH(int index) const { return _h(index); }
  inline void                                  setH(int index, double value) { _h(index) = value; }

#ifdef WITH_MPI
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
  static void send(const peano4::grid::GridControlEvent& buffer, int destination, int tag, MPI_Comm communicator);
  static void receive(peano4::grid::GridControlEvent& buffer, int source, int tag, MPI_Comm communicator);

  /**
   * Alternative to the other send() where I trigger a non-blocking send an
   * then invoke the functor until the corresponding MPI_Test tells me that
   * the message went through. In systems with heavy MPI usage, this can
   * help to avoid deadlocks.
   */
  static void send(
    const peano4::grid::GridControlEvent& buffer,
    int                                   destination,
    int                                   tag,
    std::function<void()>                 startCommunicationFunctor,
    std::function<void()>                 waitFunctor,
    MPI_Comm                              communicator
  );
  static void receive(
    peano4::grid::GridControlEvent& buffer,
    int                             source,
    int                             tag,
    std::function<void()>           startCommunicationFunctor,
    std::function<void()>           waitFunctor,
    MPI_Comm                        communicator
  );
#endif

  enum ObjectConstruction { NoData };

  GridControlEvent(ObjectConstruction):
    GridControlEvent() {}

#ifdef WITH_MPI
  static void sendAndPollDanglingMessages(
    const peano4::grid::GridControlEvent& message,
    int                                   destination,
    int                                   tag,
    MPI_Comm                              communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
  static void receiveAndPollDanglingMessages(
    peano4::grid::GridControlEvent& message,
    int                             source,
    int                             tag,
    MPI_Comm                        communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
#endif

  std::string toString() const;

private:
  RefinementControl                     _refinementControl;
  tarch::la::Vector<DIMENSIONS, double> _offset;
  tarch::la::Vector<DIMENSIONS, double> _width;
  tarch::la::Vector<DIMENSIONS, double> _h;

#ifdef WITH_MPI
private:
  int                 _senderDestinationRank;
  static MPI_Datatype Datatype;
#endif
};
