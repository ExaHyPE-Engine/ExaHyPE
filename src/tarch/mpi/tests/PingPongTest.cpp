// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PingPongTest.h"

#include "tarch/instrumentation/Debug.h"
#include "tarch/instrumentation/Error.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"
#include "tarch/tasking/Runtime.h"
#include "tarch/tasking/Task.h"
#include "tarch/tasking/TaskGraph.h"

tarch::mpi::tests::PingPongTest::PingPongTest() :
  tarch::tests::TestCase("tarch::mpi::tests::PingPongTest") {}

void
tarch::mpi::tests::PingPongTest::testBuiltInType() {
  const int tag           = tarch::mpi::Rank::reserveFreeTag("tarch::mpi::tests::PingPongTest::testBuiltInType");
  const int outgoingValue = 23;
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    tarch::mpi::send(&outgoingValue, 1, MPI_INT, 1, tag, MPI_COMM_WORLD);
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    int incomingValue{25};
    tarch::mpi::receive(&incomingValue, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    validateEquals(incomingValue, outgoingValue);
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
}

namespace {
  class PingPongSendTask final : public tarch::tasking::Task {
  public:
    PingPongSendTask(const int payloadOffset, const int tag, const bool useBlockingCall) :
      _payloadOffset(payloadOffset),
      _tag(tag),
      _useBlockingCall(useBlockingCall) {}
    PingPongSendTask(const PingPongSendTask&)            = delete;
    PingPongSendTask(PingPongSendTask&&)                 = delete;
    PingPongSendTask& operator=(const PingPongSendTask&) = delete;
    PingPongSendTask& operator=(PingPongSendTask&&)      = delete;
    ~PingPongSendTask() override                         = default;

    void run() override {
      const int outgoingValue = 23 + _payloadOffset;
      if (_useBlockingCall) {
        tarch::mpi::send(&outgoingValue, 1, MPI_INT, 1, _tag, MPI_COMM_WORLD);
        logDebug("Sent blocking message " << outgoingValue << " to rank 1 with tag " << _tag << ".");
      } else {
        MPI_Request sendRequest{MPI_REQUEST_NULL};
        tarch::mpi::isend(&outgoingValue, 1, MPI_INT, 1, _tag, MPI_COMM_WORLD, &sendRequest);
        logDebug("Posted non-blocking message " << outgoingValue << " to rank 1 with tag " << _tag << ".");
        tarch::mpi::wait(&sendRequest, MPI_STATUS_IGNORE);
      }
    }

  private:
    const int  _payloadOffset;
    const int  _tag;
    const bool _useBlockingCall;
  };

  class PingPongReceiveTask final : public tarch::tasking::Task {
  public:
    PingPongReceiveTask(
      std::atomic<int>& errorCount,
      const int         expectedPayloadOffset,
      const int         tag,
      const bool        useBlockingCall
    ) :
      _errorCount(errorCount),
      _expectedPayloadOffset(expectedPayloadOffset),
      _tag(tag),
      _useBlockingCall(useBlockingCall) {}
    PingPongReceiveTask(const PingPongReceiveTask&)            = delete;
    PingPongReceiveTask(PingPongReceiveTask&&)                 = delete;
    PingPongReceiveTask& operator=(const PingPongReceiveTask&) = delete;
    PingPongReceiveTask& operator=(PingPongReceiveTask&&)      = delete;
    ~PingPongReceiveTask() override                            = default;

    void run() override {
      int incomingValue{-12};
      logDebug("Receiving a message from rank 0 with tag " << _tag << ".");
      if (_useBlockingCall) {
        tarch::mpi::receive(&incomingValue, 1, MPI_INT, 0, _tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      } else {
        MPI_Request receiveRequest{MPI_REQUEST_NULL};
        tarch::mpi::irecv(&incomingValue, 1, MPI_INT, 0, _tag, MPI_COMM_WORLD, &receiveRequest);
        tarch::mpi::wait(&receiveRequest, MPI_STATUS_IGNORE);
      }
      logDebug("Received value " << incomingValue << ".");
      if (incomingValue != 23 + _expectedPayloadOffset) {
        logError(
          "The ping-pong test expects " << (23 + _expectedPayloadOffset) << " and receives " << incomingValue
                                        << " (blocking mode " << _useBlockingCall << ", tag " << _tag << ")."
        );
        _errorCount.fetch_add(1, std::memory_order_relaxed);
      }
    }

  private:
    std::atomic<int>& _errorCount;
    const int         _expectedPayloadOffset;
    const int         _tag;
    const bool        _useBlockingCall;
  };

  template <typename TaskFactory>
  void spawnTasksAndWaitForDependencies(const int numberOfTasks, TaskFactory&& taskFactory) {
    std::vector<char>  completionTokens(numberOfTasks);
    std::vector<void*> completionDependencies(numberOfTasks);
    for (int taskIndex = 0; taskIndex < numberOfTasks; taskIndex++) {
      completionDependencies[taskIndex] = &completionTokens[taskIndex];
      tarch::tasking::spawnTask(taskFactory(taskIndex), {}, {completionDependencies[taskIndex]});
    }
    tarch::tasking::waitForDependencies(completionDependencies);
  }
} // namespace

void
tarch::mpi::tests::PingPongTest::testMultithreadedPingPongWithBlockingReceives() {
  std::atomic<int> errorCount{0};
  const int        numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
  const int        baseTag         = tarch::mpi::Rank::reserveFreeTag(
    "tarch::mpi::tests::PingPongTest::testMultithreadedPingPongWithBlockingReceives",
    numberOfThreads
  );
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    for (int threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
      const int outgoingValue = 23 + threadIndex;
      tarch::mpi::send(&outgoingValue, 1, MPI_INT, 1, baseTag + threadIndex, MPI_COMM_WORLD);
    }
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
      return new PingPongReceiveTask(errorCount, threadIndex, baseTag + threadIndex, true);
    });
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
  validate(errorCount.load(std::memory_order_relaxed) == 0);
}

void
tarch::mpi::tests::PingPongTest::testMultithreadedPingPongWithNonblockingReceives() {
  std::atomic<int> errorCount{0};
  const int        numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
  const int        baseTag         = tarch::mpi::Rank::reserveFreeTag(
    "tarch::mpi::tests::PingPongTest::testMultithreadedPingPongWithNonblockingReceives",
    numberOfThreads
  );
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    for (int threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
      const int outgoingValue = 23 + threadIndex;
      tarch::mpi::send(&outgoingValue, 1, MPI_INT, 1, baseTag + threadIndex, MPI_COMM_WORLD);
    }
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
      return new PingPongReceiveTask(errorCount, threadIndex, baseTag + threadIndex, false);
    });
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
  validate(errorCount.load(std::memory_order_relaxed) == 0);
}

void
tarch::mpi::tests::PingPongTest::testMultithreadedPingPongWithBlockingSends() {
  const int numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
  const int baseTag         = tarch::mpi::Rank::reserveFreeTag(
    "tarch::mpi::tests::PingPongTest::testMultithreadedPingPongWithBlockingSends",
    numberOfThreads
  );
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
      return new PingPongSendTask(threadIndex, baseTag + threadIndex, true);
    });
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    for (int threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
      int incomingValue{-12};
      tarch::mpi::receive(&incomingValue, 1, MPI_INT, 0, baseTag + threadIndex, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      validateEquals(incomingValue, 23 + threadIndex);
    }
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
}

void
tarch::mpi::tests::PingPongTest::testMultithreadedPingPongWithNonblockingSends() {
  const int numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
  const int baseTag         = tarch::mpi::Rank::reserveFreeTag(
    "tarch::mpi::tests::PingPongTest::testMultithreadedPingPongWithNonblockingSends",
    numberOfThreads
  );
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
      return new PingPongSendTask(threadIndex, baseTag + threadIndex, false);
    });
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    for (int threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
      int incomingValue{-12};
      tarch::mpi::receive(&incomingValue, 1, MPI_INT, 0, baseTag + threadIndex, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      validateEquals(incomingValue, 23 + threadIndex);
    }
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
}

void
tarch::mpi::tests::PingPongTest::testMultithreadedPingPongWithBlockingSendsAndReceives() {
  std::atomic<int> errorCount{0};
  const int        numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
  const int        baseTag         = tarch::mpi::Rank::reserveFreeTag(
    "tarch::mpi::tests::PingPongTest::"
    "testMultithreadedPingPongWithBlockingSendsAndReceives",
    numberOfThreads
  );
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
      return new PingPongSendTask(threadIndex, baseTag + threadIndex, true);
    });
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
      return new PingPongReceiveTask(errorCount, threadIndex, baseTag + threadIndex, true);
    });
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
  validate(errorCount.load(std::memory_order_relaxed) == 0);
}

void
tarch::mpi::tests::PingPongTest::testMultithreadedPingPongWithNonblockingSendsAndReceives() {
  std::atomic<int> errorCount{0};
  const int        numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
  const int        baseTag         = tarch::mpi::Rank::reserveFreeTag(
    "tarch::mpi::tests::PingPongTest::"
    "testMultithreadedPingPongWithNonblockingSendsAndReceives",
    numberOfThreads
  );
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
    spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
      return new PingPongSendTask(threadIndex, baseTag + threadIndex, false);
    });
  }
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
    spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
      return new PingPongReceiveTask(errorCount, threadIndex, baseTag + threadIndex, false);
    });
  }
  tarch::mpi::barrier(MPI_COMM_WORLD);
  validate(errorCount.load(std::memory_order_relaxed) == 0);
}

void
tarch::mpi::tests::PingPongTest::run() {
  testMethod(testBuiltInType);
  testMethod(testMultithreadedPingPongWithBlockingReceives);
  testMethod(testMultithreadedPingPongWithBlockingSends);
  testMethod(testMultithreadedPingPongWithBlockingSendsAndReceives);
  testMethod(testMultithreadedPingPongWithNonblockingReceives);
  testMethod(testMultithreadedPingPongWithNonblockingSends);
  testMethod(testMultithreadedPingPongWithNonblockingSendsAndReceives);
}
