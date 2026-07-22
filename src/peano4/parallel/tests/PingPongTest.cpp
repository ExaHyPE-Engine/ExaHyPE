// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PingPongTest.h"

#ifdef PEANO_WITH_MPI

#include "peano4/parallel/StartTraversalMessage.h"
#include "peano4/parallel/TreeManagementMessage.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/IntegerMessage.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Task.h"

peano4::parallel::tests::PingPongTest::PingPongTest() :
  TestCase("peano4::parallel::tests::PingPongTest") {}

void peano4::parallel::tests::PingPongTest::testBuiltInType() {
  const int tag = tarch::mpi::Rank::reserveFreeTag("peano4::parallel::tests::PingPongTest::testBuiltInType");
  int       out = 23;
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    tarch::mpi::send(&out, 1, MPI_INT, 1, tag, MPI_COMM_WORLD);
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    int in = 25;
    tarch::mpi::receive(&in, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    validateEquals(in, out);
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
}

namespace {
  int testErrors = 0;

  class PingPongSendTask : public tarch::multicore::Task {
  private:
    const int  _payloadOffset;
    const int  _tag;
    const bool _blockingMPI;

  public:
    PingPongSendTask(int payloadOffset, int tag, bool blockingMPI) :
      Task(Task::NoTaskType, Task::DefaultPriority),
      _payloadOffset(payloadOffset),
      _tag(tag),
      _blockingMPI(blockingMPI) {}

    bool run() {
      const int out = 23 + _payloadOffset;
      if (_blockingMPI) {
        tarch::mpi::send(&out, 1, MPI_INT, 1, _tag, MPI_COMM_WORLD);
        logDebug("sent blocking message " << out << " to rank 1 with tag " << _tag);
      } else {
        MPI_Request request;
        tarch::mpi::isend(&out, 1, MPI_INT, 1, _tag, MPI_COMM_WORLD, &request);
        logDebug("sent non-blocking message " << out << " to rank 1 with tag " << _tag);
        tarch::mpi::wait(&request, MPI_STATUS_IGNORE);
      }
      return false;
    }
  };

  class PingPongReceiveTask : public tarch::multicore::Task {
  private:
    const int  _expectedPayloadOffset;
    const int  _tag;
    const bool _blockingMPI;

  public:
    PingPongReceiveTask(int expectedPayloadOffset, int tag, bool blockingMPI) :
      Task(Task::NoTaskType, Task::DefaultPriority),
      _expectedPayloadOffset(expectedPayloadOffset),
      _tag(tag),
      _blockingMPI(blockingMPI) {}

    bool run() {
      int in = -12;
      logDebug("receive message from rank 1 with tag " << _tag);
      if (_blockingMPI) {
        tarch::mpi::receive(&in, 1, MPI_INT, 0, _tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      } else {
        MPI_Request request;
        tarch::mpi::irecv(&in, 1, MPI_INT, 0, _tag, MPI_COMM_WORLD, &request);
        tarch::mpi::wait(&request, MPI_STATUS_IGNORE);
      }
      logDebug("got content " << in);
      if (in != 23 + _expectedPayloadOffset) {
        logError(
          "received " << in << " instead of " << (23 + _expectedPayloadOffset) << " (blocking mode=" << _blockingMPI
                      << ", tag=" << _tag << ")"
        );
        testErrors++;
      }
      return false;
    }
  };
} // namespace

void peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithBlockingReceives() {
  const int baseTag = tarch::mpi::Rank::reserveFreeTag(
    "peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithBlockingReceives",
    tarch::multicore::Core::getInstance().getNumberOfThreads()
  );
  int out;
  testErrors = 0;
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    for (int i = 0; i < tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      out = 23 + i;
      tarch::mpi::send(&out, 1, MPI_INT, 1, baseTag + i, MPI_COMM_WORLD);
    }
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    std::vector<tarch::multicore::Task*> tasks;
    for (int i = 0; i < tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back(new PingPongReceiveTask(i, baseTag + i, true));
    }
    tarch::multicore::spawnTasks(tasks);
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
  validate(testErrors == 0);
}

void peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithNonblockingReceives() {
  const int baseTag = tarch::mpi::Rank::reserveFreeTag(
    "peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithNonblockingReceives",
    tarch::multicore::Core::getInstance().getNumberOfThreads()
  );
  int out;
  testErrors = 0;
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    for (int i = 0; i < tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      out = 23 + i;
      tarch::mpi::send(&out, 1, MPI_INT, 1, baseTag + i, MPI_COMM_WORLD);
    }
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    std::vector<tarch::multicore::Task*> tasks;
    for (int i = 0; i < tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back(new PingPongReceiveTask(i, baseTag + i, false));
    }
    tarch::multicore::spawnTasks(tasks);
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
  validate(testErrors == 0);
}

void peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithBlockingSends() {
  const int baseTag = tarch::mpi::Rank::reserveFreeTag(
    "peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithBlockingSends",
    tarch::multicore::Core::getInstance().getNumberOfThreads()
  );
  int out    = 23;
  testErrors = 0;
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    std::vector<tarch::multicore::Task*> tasks;
    for (int i = 0; i < tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back(new PingPongSendTask(i, baseTag + i, true));
    }
    tarch::multicore::spawnTasks(tasks);
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    int in = -12;
    for (int i = 0; i < tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tarch::mpi::receive(&in, 1, MPI_INT, 0, baseTag + i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      validateEquals(in, out + i);
    }
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
  validate(testErrors == 0);
}

void peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithNonblockingSends() {
  const int baseTag = tarch::mpi::Rank::reserveFreeTag(
    "peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithNonblockingSends",
    tarch::multicore::Core::getInstance().getNumberOfThreads()
  );
  int out    = 23;
  testErrors = 0;
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    std::vector<tarch::multicore::Task*> tasks;
    for (int i = 0; i < tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back(new PingPongSendTask(i, baseTag + i, false));
    }
    tarch::multicore::spawnTasks(tasks);
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    int in = -12;
    for (int i = 0; i < tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tarch::mpi::receive(&in, 1, MPI_INT, 0, baseTag + i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      validateEquals(in, out + i);
    }
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
  validate(testErrors == 0);
}

void peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithBlockingSendsAndReceives() {
  const int baseTag = tarch::mpi::Rank::reserveFreeTag(
    "peano4::parallel::tests::PingPongTest::"
    "testMultithreadedPingPongWithBlockingSendsAndReceives",
    tarch::multicore::Core::getInstance().getNumberOfThreads()
  );
  // int out    = 23;
  testErrors = 0;
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    std::vector<tarch::multicore::Task*> tasks;
    for (int i = 0; i < tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back(new PingPongSendTask(i, baseTag + i, true));
    }
    tarch::multicore::spawnTasks(tasks);
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    std::vector<tarch::multicore::Task*> tasks;
    for (int i = 0; i < tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back(new PingPongReceiveTask(i, baseTag + i, true));
    }
    tarch::multicore::spawnTasks(tasks);
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
  validate(testErrors == 0);
}

void peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithNonblockingSendsAndReceives() {
  const int baseTag = tarch::mpi::Rank::reserveFreeTag(
    "peano4::parallel::tests::PingPongTest::"
    "testMultithreadedPingPongWithNonblockingSendsAndReceives",
    tarch::multicore::Core::getInstance().getNumberOfThreads()
  );
  // int out    = 23;
  testErrors = 0;
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    std::vector<tarch::multicore::Task*> tasks;
    for (int i = 0; i < tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back(new PingPongSendTask(i, baseTag + i, false));
    }
    tarch::multicore::spawnTasks(tasks);
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    std::vector<tarch::multicore::Task*> tasks;
    for (int i = 0; i < tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back(new PingPongReceiveTask(i, baseTag + i, false));
    }
    tarch::multicore::spawnTasks(tasks);
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
  validate(testErrors == 0);
}

void peano4::parallel::tests::PingPongTest::testStartTraversalMessageExchange() {
  const int tag =
    tarch::mpi::Rank::reserveFreeTag("peano4::parallel::tests::PingPongTest::testStartTraversalMessageExchange");
  StartTraversalMessage out;
  out.setStepIdentifier(23);
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    StartTraversalMessage::send(out, 1, tag, MPI_COMM_WORLD);
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    StartTraversalMessage in;
    StartTraversalMessage::receive(in, 0, tag, MPI_COMM_WORLD);
    validateEqualsWithParams2(in.getStepIdentifier(), out.getStepIdentifier(), in.toString(), out.toString());
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
}

/**
 * Exchange a message type through its own send() and through MPI_Send
 *
 * A message type with a vtable carries an additional four bytes at the front of
 * the object, and those bytes are not necessarily reflected in the this pointer.
 * Types with vtables are therefore not meant to be exchanged, and mixing a raw
 * MPI_Send with a send() call on the same object surfaces the resulting bugs.
 */
void peano4::parallel::tests::PingPongTest::testIntegerMessageExchange() {
  const int tag =
    tarch::mpi::Rank::reserveFreeTag("peano4::parallel::tests::PingPongTest::testIntegerMessageExchange");
  tarch::mpi::IntegerMessage out;
  out.setValue(23);
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    tarch::mpi::send(&out, 1, tarch::mpi::IntegerMessage::getGlobalCommunicationDatatype(), 1, tag, MPI_COMM_WORLD);
    tarch::mpi::IntegerMessage::send(out, 1, tag, tarch::mpi::Rank::getInstance().getCommunicator());
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    tarch::mpi::IntegerMessage inThroughMPI;
    inThroughMPI.setValue(72);
    tarch::mpi::receive(
      &inThroughMPI,
      1,
      tarch::mpi::IntegerMessage::getGlobalCommunicationDatatype(),
      0,
      tag,
      MPI_COMM_WORLD,
      MPI_STATUS_IGNORE
    );
    validateEqualsWithParams2(inThroughMPI.getValue(), out.getValue(), inThroughMPI.toString(), out.toString());

    tarch::mpi::IntegerMessage inThroughOwnReceive;
    inThroughOwnReceive.setValue(73);
    tarch::mpi::IntegerMessage::receive(inThroughOwnReceive, 0, tag, tarch::mpi::Rank::getInstance().getCommunicator());
    validateEqualsWithParams2(inThroughOwnReceive.getValue(), out.getValue(), inThroughOwnReceive.toString(), out.toString());
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
}

void peano4::parallel::tests::PingPongTest::testMessageArrayExchange() {
  const int tag = tarch::mpi::Rank::reserveFreeTag("peano4::parallel::tests::PingPongTest::testMessageArrayExchange");
  StartTraversalMessage out[10];
  out[0].setStepIdentifier(23);
  out[1].setStepIdentifier(24);
  out[2].setStepIdentifier(25);
  out[3].setStepIdentifier(26);
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    tarch::mpi::send(out, 4, StartTraversalMessage::getGlobalCommunicationDatatype(), 1, tag, MPI_COMM_WORLD);
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    StartTraversalMessage in[10];
    tarch::mpi::receive(
      in,
      4,
      StartTraversalMessage::getGlobalCommunicationDatatype(),
      0,
      tag,
      MPI_COMM_WORLD,
      MPI_STATUS_IGNORE
    );
    validateEqualsWithParams8(
      in[0].getStepIdentifier(),
      out[0].getStepIdentifier(),
      in[0].toString(),
      out[0].toString(),
      in[1].toString(),
      out[1].toString(),
      in[2].toString(),
      out[2].toString(),
      in[3].toString(),
      out[3].toString()
    );
    validateEqualsWithParams8(
      in[1].getStepIdentifier(),
      out[1].getStepIdentifier(),
      in[0].toString(),
      out[0].toString(),
      in[1].toString(),
      out[1].toString(),
      in[2].toString(),
      out[2].toString(),
      in[3].toString(),
      out[3].toString()
    );
    validateEqualsWithParams8(
      in[2].getStepIdentifier(),
      out[2].getStepIdentifier(),
      in[0].toString(),
      out[0].toString(),
      in[1].toString(),
      out[1].toString(),
      in[2].toString(),
      out[2].toString(),
      in[3].toString(),
      out[3].toString()
    );
    validateEqualsWithParams8(
      in[3].getStepIdentifier(),
      out[3].getStepIdentifier(),
      in[0].toString(),
      out[0].toString(),
      in[1].toString(),
      out[1].toString(),
      in[2].toString(),
      out[2].toString(),
      in[3].toString(),
      out[3].toString()
    );
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
}

void peano4::parallel::tests::PingPongTest::testTreeManagementMessageArrayExchange() {
  peano4::parallel::TreeManagementMessage message;
  const int                               Tag =
    tarch::mpi::Rank::reserveFreeTag("peano4::parallel::tests::PingPongTest::testTreeManagementMessageArrayExchange");
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    message.setAction(TreeManagementMessage::Action::Acknowledgement);
    peano4::parallel::TreeManagementMessage::send(message, 1, Tag, MPI_COMM_WORLD);
  } else if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and
             tarch::mpi::Rank::getInstance().getRank() == 1) {
    peano4::parallel::TreeManagementMessage::receive(message, 0, Tag, MPI_COMM_WORLD);
    validateWithParams1(message.getAction() == TreeManagementMessage::Action::Acknowledgement, message.toString());
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
}

/**
 * Build up suite of test routines
 *
 * The multithreaded calls are guarded by a comparison of
 * tarch::mpi::Rank::getInstance().getProvidedThreadLevelSupport() to
 * MPI_THREAD_MULTIPLE. Peano cannot work properly without MPI's
 * multithreaded support, but tools such as MUST
 * (http://www.itc.rwth-aachen.de/MUST) artificially lower the reported
 * thread level for their correctness checks and would then report a
 * deadlock here. Skipping the affected tests keeps the thread checkers
 * usable on this test suite.
 */
void peano4::parallel::tests::PingPongTest::run() {
  testMethod(testBuiltInType);

  testMethod(testIntegerMessageExchange);
  testMethod(testStartTraversalMessageExchange);
  testMethod(testMessageArrayExchange);

  testMethod(testTreeManagementMessageArrayExchange);

  if (tarch::mpi::Rank::getInstance().getProvidedThreadLevelSupport() == MPI_THREAD_MULTIPLE) {
    testMethod(testMultithreadedPingPongWithBlockingReceives);
    testMethod(testMultithreadedPingPongWithBlockingSends);
    testMethod(testMultithreadedPingPongWithBlockingSendsAndReceives);

    testMethod(testMultithreadedPingPongWithNonblockingReceives);
    testMethod(testMultithreadedPingPongWithNonblockingSends);
    testMethod(testMultithreadedPingPongWithNonblockingSendsAndReceives);
  }
}

#endif
