// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

#ifdef PEANO_WITH_MPI
#include <mpi.h>
#endif

namespace tarch {
  namespace mpi {
    struct StringMessage;
  } // namespace mpi
} // namespace tarch

/**
 * An MPI message type which carries a string of arbitrary length. The receiver
 * does not know the length upfront, so receive() probes for the byte count
 * instead of using a fixed MPI datatype.
 *
 * @author Tobias Weinzierl
 */
struct tarch::mpi::StringMessage {
public:
  StringMessage() {}
  StringMessage(const std::string& value);

  std::string getData() const;
  void        setData(const std::string& value);

#ifdef PEANO_WITH_MPI
  static void sendAndPollDanglingMessages(const tarch::mpi::StringMessage& message, int destination, int tag);
  static void receiveAndPollDanglingMessages(tarch::mpi::StringMessage& message, int source, int tag);
#endif

#ifdef PEANO_WITH_MPI
  /**
   * @return The rank that sent the object. Only meaningful after a receive
   *         with MPI_ANY_SOURCE.
   */
  int getSenderRank() const;

  /**
   * Send and receive are static because a non-static member cannot expose the
   * object's layout to MPI: the this pointer does not see the vtable that MPI
   * has to skip. Both delegate to the blocking tarch::mpi::send() and
   * tarch::mpi::receive() wrappers.
   */
  static void send(const tarch::mpi::StringMessage& buffer, int destination, int tag, MPI_Comm communicator);
  static void receive(tarch::mpi::StringMessage& buffer, int source, int tag, MPI_Comm communicator);

  /**
   * Non-blocking alternative to the other send() and receive(). The message
   * goes out through tarch::mpi, and tarch::mpi::wait() runs the wait functor
   * while it is still pending. Under heavy MPI load this helps to avoid
   * deadlocks.
   */
  static void send(
    const tarch::mpi::StringMessage& buffer,
    int                              destination,
    int                              tag,
    std::function<void()>            startCommunicationFunctor,
    std::function<void()>            waitFunctor,
    MPI_Comm                         communicator
  );
  static void receive(
    tarch::mpi::StringMessage& buffer,
    int                        source,
    int                        tag,
    std::function<void()>      startCommunicationFunctor,
    std::function<void()>      waitFunctor,
    MPI_Comm                   communicator
  );

  /**
   * @return Whether a message is pending on the given tag.
   *
   * The probe matches on the tag alone. A byte count cannot identify a message
   * type, so a caller has to know which type arrives on a tag.
   */
  static bool isMessageInQueue(int tag, MPI_Comm communicator);
#endif

  std::string toString() const;

private:
  std::string _data;

  int _senderDestinationRank;
};
