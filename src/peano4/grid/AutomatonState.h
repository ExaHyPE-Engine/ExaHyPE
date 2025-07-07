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
    struct AutomatonState;
  } // namespace grid
} // namespace peano4

struct peano4::grid::AutomatonState {
public:
  AutomatonState() = default;
  AutomatonState(
    const int                                         __level,
    const tarch::la::Vector<Dimensions, double>&      __x,
    const tarch::la::Vector<Dimensions, double>&      __h,
    const bool                                        __inverted,
    const std::bitset<Dimensions>&                    __evenFlags,
    const tarch::la::Vector<DimensionsTimesTwo, int>& __accessNumber
  );

  AutomatonState(const AutomatonState& copy);
  AutomatonState& operator=(const AutomatonState& other);

  ~AutomatonState() = default;

  enum ObjectConstruction { NoData };

  AutomatonState(ObjectConstruction):
    AutomatonState() {}

  inline int  getLevel() const { return _level; }
  inline void setLevel(int value) { _level = value; }

  inline tarch::la::Vector<Dimensions, double> getX() const { return _x; }
  inline void                                  setX(const tarch::la::Vector<Dimensions, double>& value) { _x = value; }
  inline double                                getX(int index) const { return _x(index); }
  inline void                                  setX(int index, double value) { _x(index) = value; }

  inline tarch::la::Vector<Dimensions, double> getH() const { return _h; }
  inline void                                  setH(const tarch::la::Vector<Dimensions, double>& value) { _h = value; }
  inline double                                getH(int index) const { return _h(index); }
  inline void                                  setH(int index, double value) { _h(index) = value; }

  inline bool getInverted() const { return _inverted; }
  inline void setInverted(bool value) { _inverted = value; }

  inline std::bitset<Dimensions> getEvenFlags() const { return _evenFlags; }
  inline void                    setEvenFlags(const std::bitset<Dimensions>& value) { _evenFlags = value; }
  inline bool                    getEvenFlags(int index) const { return _evenFlags[index]; }
  inline void                    setEvenFlags(int index, bool value) { _evenFlags[index] = value; }
  inline void                    flipEvenFlags(int index) { _evenFlags[index] = not _evenFlags[index]; }

  inline tarch::la::Vector<DimensionsTimesTwo, int> getAccessNumber() const { return _accessNumber; }
  inline void setAccessNumber(const tarch::la::Vector<DimensionsTimesTwo, int>& value) { _accessNumber = value; }
  inline int  getAccessNumber(int index) const { return _accessNumber(index); }
  inline void setAccessNumber(int index, int value) { _accessNumber(index) = value; }

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
  static void send(const peano4::grid::AutomatonState& buffer, int destination, int tag, MPI_Comm communicator);
  static void receive(peano4::grid::AutomatonState& buffer, int source, int tag, MPI_Comm communicator);

  /**
   * Alternative to the other send() where I trigger a non-blocking send an
   * then invoke the functor until the corresponding MPI_Test tells me that
   * the message went through. In systems with heavy MPI usage, this can
   * help to avoid deadlocks.
   */
  static void send(
    const peano4::grid::AutomatonState& buffer,
    int                                 destination,
    int                                 tag,
    std::function<void()>               startCommunicationFunctor,
    std::function<void()>               waitFunctor,
    MPI_Comm                            communicator
  );
  static void receive(
    peano4::grid::AutomatonState& buffer,
    int                           source,
    int                           tag,
    std::function<void()>         startCommunicationFunctor,
    std::function<void()>         waitFunctor,
    MPI_Comm                      communicator
  );
#endif

#ifdef Parallel
  static void sendAndPollDanglingMessages(
    const peano4::grid::AutomatonState& message,
    int                                 destination,
    int                                 tag,
    MPI_Comm                            communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
  static void receiveAndPollDanglingMessages(
    peano4::grid::AutomatonState& message,
    int                           source,
    int                           tag,
    MPI_Comm                      communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
#endif

  std::string toString() const;

private:
  int                                        _level;
  tarch::la::Vector<Dimensions, double>      _x;
  tarch::la::Vector<Dimensions, double>      _h;
  bool                                       _inverted;
  std::bitset<Dimensions>                    _evenFlags;
  tarch::la::Vector<DimensionsTimesTwo, int> _accessNumber;

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
