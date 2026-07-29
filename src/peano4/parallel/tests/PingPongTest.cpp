// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PingPongTest.h"

#include "tarch/instrumentation/Log.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"
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

  if (tarch::mpi::Rank::getInstance().getProvidedThreadLevelSupport() == MPI_THREAD_MULTIPLE) {
    testMethod(testMultithreadedPingPongWithBlockingReceives);
    testMethod(testMultithreadedPingPongWithBlockingSends);
    testMethod(testMultithreadedPingPongWithBlockingSendsAndReceives);

    testMethod(testMultithreadedPingPongWithNonblockingReceives);
    testMethod(testMultithreadedPingPongWithNonblockingSends);
    testMethod(testMultithreadedPingPongWithNonblockingSendsAndReceives);
  }
}
