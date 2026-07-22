// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef PEANO_WITH_MPI
#include <mpi.h>
#endif

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/services/ServiceRepository.h"

namespace tarch {
  namespace mpi {
    struct DoubleMessage;
  } // namespace mpi
} // namespace tarch

struct tarch::mpi::DoubleMessage {
public:
  DoubleMessage() {}
  DoubleMessage(double __value);

  double getValue() const;
  void   setValue(double value);
  DoubleMessage(const DoubleMessage& copy) = default;

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
   * @return The rank that sent the object. Only meaningful after a receive
   *         with MPI_ANY_SOURCE.
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
   * Send and receive are static because a non-static member cannot expose the
   * object's layout to MPI: the this pointer does not see the vtable that MPI
   * has to skip. Both delegate to the blocking tarch::mpi::send() and
   * tarch::mpi::receive() wrappers.
   */
  static void send(const tarch::mpi::DoubleMessage& buffer, int destination, int tag, MPI_Comm communicator);
  static void receive(tarch::mpi::DoubleMessage& buffer, int source, int tag, MPI_Comm communicator);

  /**
   * Non-blocking alternative to the other send() and receive(). The message
   * goes out through tarch::mpi, and tarch::mpi::wait() runs the wait functor
   * while it is still pending. Under heavy MPI load this helps to avoid
   * deadlocks.
   */
  static void send(
    const tarch::mpi::DoubleMessage& buffer,
    int                              destination,
    int                              tag,
    std::function<void()>            startCommunicationFunctor,
    std::function<void()>            waitFunctor,
    MPI_Comm                         communicator
  );
  static void receive(
    tarch::mpi::DoubleMessage& buffer,
    int                        source,
    int                        tag,
    std::function<void()>      startCommunicationFunctor,
    std::function<void()>      waitFunctor,
    MPI_Comm                   communicator
  );
#endif

#ifdef PEANO_WITH_MPI
  static void sendAndPollDanglingMessages(
    const tarch::mpi::DoubleMessage& message,
    int                              destination,
    int                              tag,
    MPI_Comm                         communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
  static void receiveAndPollDanglingMessages(
    tarch::mpi::DoubleMessage& message,
    int                        source,
    int                        tag,
    MPI_Comm                   communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
#endif

  std::string toString() const;

private:
  double _value;

#ifdef PEANO_WITH_MPI
private:
  int                 _senderDestinationRank;
  static MPI_Datatype Datatype;
#endif
};
