// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackExchangeTest.h"

#include "peano4/grid/Stack.h"
#include "peano4/grid/StackExchange.h"
#include "peano4/grid/tests/SelfSerialisingRecord.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tests/TestMacros.h"

namespace {
  constexpr int NumberOfElements = 5;
  constexpr int ValuesPerRecord  = 32;
  constexpr int Tag              = 4321;

  /**
   * Record whose bytes are the bytes to send, so it travels on the direct path.
   */
  struct PlainRecord {
    static constexpr int Cardinality = 16;

    std::array<int, Cardinality> values{};
  };

  /**
   * Polls both transfers until neither has anything left to do. Send and
   * receive are driven from the same thread, so neither may be waited on
   * exclusively.
   */
  template <class T>
  void completeTransfer(
    peano4::grid::Stack<T>&            sender,
    peano4::grid::StackExchangeJob<T>& sendJob,
    peano4::grid::Stack<T>&            receiver,
    peano4::grid::StackExchangeJob<T>& receiveJob
  ) {
    bool sendFinished    = false;
    bool receiveFinished = false;
    while (not sendFinished or not receiveFinished) {
      sendFinished    = sendFinished or peano4::grid::tryToFinishSendOrReceive(sender, sendJob);
      receiveFinished = receiveFinished or peano4::grid::tryToFinishSendOrReceive(receiver, receiveJob);
    }
  }
} // namespace

peano4::grid::tests::StackExchangeTest::StackExchangeTest() :
  TestCase("peano4::grid::tests::StackExchangeTest") {}

void peano4::grid::tests::StackExchangeTest::run() {
  testMethod(testSendAndReceiveToSelf);
  testMethod(testSendAndReceiveInARing);
}

/**
 * A rank exchanges with itself, so both ends of the transfer are observable in
 * one test. A completed send releases its stack's slots while a completed
 * receive holds the payload.
 */
void peano4::grid::tests::StackExchangeTest::testSendAndReceiveToSelf() {
  const auto&    rank         = tarch::mpi::Rank::getInstance();
  const MPI_Comm communicator = rank.getCommunicator();
  const int      localRank    = rank.getRank();

  {
    peano4::grid::Stack<PlainRecord> sender;
    peano4::grid::Stack<PlainRecord> receiver;
    StackExchangeJob<PlainRecord>    sendJob;
    StackExchangeJob<PlainRecord>    receiveJob;

    sender.resize(NumberOfElements);
    for (int element = 0; element < NumberOfElements; element++) {
      for (int index = 0; index < PlainRecord::Cardinality; index++) {
        sender[element].values[index] = element * index;
      }
    }

    startReceive(receiver, receiveJob, localRank, Tag, communicator, NumberOfElements);
    startSend(sender, sendJob, localRank, Tag, communicator);
    completeTransfer(sender, sendJob, receiver, receiveJob);

    validate(sender.empty());
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int element = 0; element < NumberOfElements; element++) {
      for (int index = 0; index < PlainRecord::Cardinality; index++) {
        validateEquals(receiver[element].values[index], element * index);
      }
    }
  }

  {
    peano4::grid::Stack<SelfSerialisingRecord> sender;
    peano4::grid::Stack<SelfSerialisingRecord> receiver;
    StackExchangeJob<SelfSerialisingRecord>    sendJob;
    StackExchangeJob<SelfSerialisingRecord>    receiveJob;

    sender.resize(NumberOfElements);
    for (int element = 0; element < NumberOfElements; element++) {
      sender[element] = SelfSerialisingRecord(ValuesPerRecord, element * 10);
    }

    startReceive(receiver, receiveJob, localRank, Tag, communicator, NumberOfElements);
    startSend(sender, sendJob, localRank, Tag, communicator);
    completeTransfer(sender, sendJob, receiver, receiveJob);

    validate(sender.empty());
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int element = 0; element < NumberOfElements; element++) {
      validateEquals(receiver[element].size(), static_cast<std::size_t>(ValuesPerRecord));
      for (int index = 0; index < ValuesPerRecord; index++) {
        validateEquals(receiver[element].at(index), element * 10);
      }
    }
  }
}

/**
 * Every rank sends to its successor and receives from its predecessor, so the
 * payload has to cross a real rank boundary to arrive. Even ranks send first
 * and odd ranks receive first, which keeps a rank from waiting on a partner
 * that is itself waiting.
 */
void peano4::grid::tests::StackExchangeTest::testSendAndReceiveInARing() {
  const auto& rank = tarch::mpi::Rank::getInstance();
  if (rank.getNumberOfRanks() <= 1) {
    return;
  }

  const MPI_Comm communicator  = rank.getCommunicator();
  const int      localRank     = rank.getRank();
  const int      numberOfRanks = rank.getNumberOfRanks();
  const int      successor     = (localRank + 1) % numberOfRanks;
  const int      predecessor   = (localRank - 1 + numberOfRanks) % numberOfRanks;

  {
    peano4::grid::Stack<PlainRecord> sender;
    peano4::grid::Stack<PlainRecord> receiver;
    StackExchangeJob<PlainRecord>    sendJob;
    StackExchangeJob<PlainRecord>    receiveJob;

    sender.resize(NumberOfElements);
    for (int element = 0; element < NumberOfElements; element++) {
      for (int index = 0; index < PlainRecord::Cardinality; index++) {
        sender[element].values[index] = localRank * 1000 + element * 10 + index;
      }
    }

    if (localRank % 2 == 0) {
      startSend(sender, sendJob, successor, Tag, communicator);
      startReceive(receiver, receiveJob, predecessor, Tag, communicator, NumberOfElements);
    } else {
      startReceive(receiver, receiveJob, predecessor, Tag, communicator, NumberOfElements);
      startSend(sender, sendJob, successor, Tag, communicator);
    }
    completeTransfer(sender, sendJob, receiver, receiveJob);

    validate(sender.empty());
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int element = 0; element < NumberOfElements; element++) {
      for (int index = 0; index < PlainRecord::Cardinality; index++) {
        validateEquals(receiver[element].values[index], predecessor * 1000 + element * 10 + index);
      }
    }
  }

  {
    peano4::grid::Stack<SelfSerialisingRecord> sender;
    peano4::grid::Stack<SelfSerialisingRecord> receiver;
    StackExchangeJob<SelfSerialisingRecord>    sendJob;
    StackExchangeJob<SelfSerialisingRecord>    receiveJob;

    sender.resize(NumberOfElements);
    for (int element = 0; element < NumberOfElements; element++) {
      sender[element] = SelfSerialisingRecord(ValuesPerRecord, localRank * 1000 + element * 10);
    }

    if (localRank % 2 == 0) {
      startSend(sender, sendJob, successor, Tag, communicator);
      startReceive(receiver, receiveJob, predecessor, Tag, communicator, NumberOfElements);
    } else {
      startReceive(receiver, receiveJob, predecessor, Tag, communicator, NumberOfElements);
      startSend(sender, sendJob, successor, Tag, communicator);
    }
    completeTransfer(sender, sendJob, receiver, receiveJob);

    validate(sender.empty());
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int element = 0; element < NumberOfElements; element++) {
      for (int index = 0; index < ValuesPerRecord; index++) {
        validateEquals(receiver[element].at(index), predecessor * 1000 + element * 10);
      }
    }
  }
}
