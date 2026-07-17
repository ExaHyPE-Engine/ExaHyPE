// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef PEANO_WITH_MPI
#include <mpi.h>
#endif

#include "peano4/Globals.h"
#include "peano4/grid/GridTraversalObserver.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/services/ServiceRepository.h"

namespace peano4 {
  namespace parallel {
    struct TreeManagementMessage;
  } // namespace parallel
} // namespace peano4

struct peano4::parallel::TreeManagementMessage {
public:
  enum class Action : int {
    RequestNewRemoteTrees                      = 0,
    CreateNewRemoteTrees                       = 1,
    RemoveChildTreeFromBooksAsChildBecameEmpty = 2,
    JoinWithWorker                             = 3,
    Acknowledgement                            = 4
  };

  TreeManagementMessage() = default;
  TreeManagementMessage(int masterSpacetreeId, int workerSpacetreeId, Action action, int numberOfSpacetrees = 1);

  int  getMasterSpacetreeId() const;
  void setMasterSpacetreeId(int value);
  int  getWorkerSpacetreeId() const;
  void setWorkerSpacetreeId(int value);
  /** Number of workers covered by a batched reservation or creation message. */
  int                                             getNumberOfSpacetrees() const;
  void                                            setNumberOfSpacetrees(int value);
  peano4::parallel::TreeManagementMessage::Action getAction() const;
  void                                            setAction(Action value);

#ifdef PEANO_WITH_MPI
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
  int  getSenderRank() const;
  void setSenderRank(int rank);

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
   * including a vtable. So this routine now delegates to the blocking
   * tarch::mpi::send/receive wrappers.
   */
  static void
  send(const peano4::parallel::TreeManagementMessage& buffer, int destination, int tag, MPI_Comm communicator);
  static void receive(peano4::parallel::TreeManagementMessage& buffer, int source, int tag, MPI_Comm communicator);

  /**
   * Alternative to the other send()/receive() where I issue non-blocking
   * MPI through tarch::mpi and let tarch::mpi::wait invoke the functor while
   * the message is pending. In systems with heavy MPI usage, this can help
   * to avoid deadlocks.
   */
  static void send(
    const peano4::parallel::TreeManagementMessage& buffer,
    int                                            destination,
    int                                            tag,
    std::function<void()>                          startCommunicationFunctor,
    std::function<void()>                          waitFunctor,
    MPI_Comm                                       communicator
  );
  static void receive(
    peano4::parallel::TreeManagementMessage& buffer,
    int                                      source,
    int                                      tag,
    std::function<void()>                    startCommunicationFunctor,
    std::function<void()>                    waitFunctor,
    MPI_Comm                                 communicator
  );
#endif

#ifdef PEANO_WITH_MPI
  static void sendAndPollDanglingMessages(
    const peano4::parallel::TreeManagementMessage& message,
    int                                            destination,
    int                                            tag,
    MPI_Comm                                       communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
  static void receiveAndPollDanglingMessages(
    peano4::parallel::TreeManagementMessage& message,
    int                                      source,
    int                                      tag,
    MPI_Comm                                 communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
#endif

  std::string toString() const;

private:
  int    _masterSpacetreeId  = -1;
  int    _workerSpacetreeId  = -1;
  int    _numberOfSpacetrees = 1;
  Action _action             = Action::Acknowledgement;

#ifdef PEANO_WITH_MPI
private:
  int                 _senderDestinationRank = -1;
  static MPI_Datatype Datatype;
#endif
};
