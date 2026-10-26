// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/instrumentation/Debug.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"
#include "tarch/tasking/Runtime.h"
#include "tarch/tasking/Task.h"
#include "tarch/tasking/TaskGraph.h"
#include "tarch/testing/UnitTest.h"

namespace {
  class PingPongSendTask final : public tarch::tasking::Task {
  public:
    PingPongSendTask(const int payloadOffset, const int tag, const bool useBlockingCall) :
      _payloadOffset(payloadOffset),
      _tag(tag),
      _useBlockingCall(useBlockingCall) {}
    PingPongSendTask(const PingPongSendTask& other)            = delete;
    PingPongSendTask(PingPongSendTask&& other)                 = delete;
    PingPongSendTask& operator=(const PingPongSendTask& other) = delete;
    PingPongSendTask& operator=(PingPongSendTask&& other)      = delete;
    ~PingPongSendTask() override                               = default;

    void run() override {
      const int outgoingValue = 23 + _payloadOffset;
      if (_useBlockingCall) {
        tarch::mpi::send(&outgoingValue, 1, MPI_INT, 1, _tag, MPI_COMM_WORLD);
        LOG_DEBUG("Sent blocking message " << outgoingValue << " to rank 1 with tag " << _tag << ".");
      } else {
        MPI_Request sendRequest{MPI_REQUEST_NULL};
        tarch::mpi::isend(&outgoingValue, 1, MPI_INT, 1, _tag, MPI_COMM_WORLD, &sendRequest);
        LOG_DEBUG("Posted non-blocking message " << outgoingValue << " to rank 1 with tag " << _tag << ".");
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
    PingPongReceiveTask(const int tag, const bool useBlockingCall, int& incomingValue) :
      _tag(tag),
      _useBlockingCall(useBlockingCall),
      _incomingValue(incomingValue) {}
    PingPongReceiveTask(const PingPongReceiveTask& other)            = delete;
    PingPongReceiveTask(PingPongReceiveTask&& other)                 = delete;
    PingPongReceiveTask& operator=(const PingPongReceiveTask& other) = delete;
    PingPongReceiveTask& operator=(PingPongReceiveTask&& other)      = delete;
    ~PingPongReceiveTask() override                                  = default;

    void run() override {
      LOG_DEBUG("Receiving a message from rank 0 with tag " << _tag << ".");
      if (_useBlockingCall) {
        tarch::mpi::receive(&_incomingValue, 1, MPI_INT, 0, _tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      } else {
        MPI_Request receiveRequest{MPI_REQUEST_NULL};
        tarch::mpi::irecv(&_incomingValue, 1, MPI_INT, 0, _tag, MPI_COMM_WORLD, &receiveRequest);
        tarch::mpi::wait(&receiveRequest, MPI_STATUS_IGNORE);
      }
      LOG_DEBUG("Received value " << _incomingValue << ".");
    }

  private:
    const int  _tag;
    const bool _useBlockingCall;
    int&       _incomingValue;
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

UNIT_TEST("An integer that rank 0 sends arrives unchanged at rank 1") {
  GIVEN("A reserved tag, the value 23 to send and a receive buffer with the value 25") {
    constexpr int OutgoingValue = 23;
    const int     tag           = tarch::mpi::Rank::reserveFreeTag("tarch/mpi/PingPongTest built-in type");
    int           incomingValue{25};

    WHEN("Rank 0 sends the value with a blocking call, rank 1 receives it and every rank enters a barrier") {
      const bool hasPartner = tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2;
      if (hasPartner and tarch::mpi::Rank::getInstance().getRank() == 0) {
        tarch::mpi::send(&OutgoingValue, 1, MPI_INT, 1, tag, MPI_COMM_WORLD);
      }
      if (hasPartner and tarch::mpi::Rank::getInstance().getRank() == 1) {
        tarch::mpi::receive(&incomingValue, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      tarch::mpi::barrier(MPI_COMM_WORLD);

      THEN("Rank 1 receives the value 23") {
        if (hasPartner and tarch::mpi::Rank::getInstance().getRank() == 1) {
          VALIDATE_EQUALS(incomingValue, OutgoingValue);
        }
      }
    }
  }
}

UNIT_TEST("Concurrent tasks of rank 1 receive with blocking calls what the main thread of rank 0 sends") {
  GIVEN("One reserved tag per thread") {
    const int numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
    const int baseTag =
      tarch::mpi::Rank::reserveFreeTag("tarch/mpi/PingPongTest blocking receives in tasks", numberOfThreads);

    WHEN("Rank 0 sends 23 plus the thread index under every tag and one task per tag on rank 1 receives it") {
      std::vector<int> incomingValues;
      if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
        for (int threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
          const int outgoingValue = 23 + threadIndex;
          tarch::mpi::send(&outgoingValue, 1, MPI_INT, 1, baseTag + threadIndex, MPI_COMM_WORLD);
        }
      }
      if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
        incomingValues.assign(numberOfThreads, -12);
        spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
          return new PingPongReceiveTask(baseTag + threadIndex, true, incomingValues[threadIndex]);
        });
      }
      tarch::mpi::barrier(MPI_COMM_WORLD);

      THEN("Rank 1 receives 23 plus the thread index under every tag") {
        for (int threadIndex = 0; threadIndex < static_cast<int>(incomingValues.size()); threadIndex++) {
          VALIDATE_EQUALS(incomingValues[threadIndex], 23 + threadIndex, threadIndex);
        }
      }
    }
  }
}

UNIT_TEST("The main thread of rank 1 receives what concurrent tasks of rank 0 send with blocking calls") {
  GIVEN("One reserved tag per thread") {
    const int numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
    const int baseTag =
      tarch::mpi::Rank::reserveFreeTag("tarch/mpi/PingPongTest blocking sends in tasks", numberOfThreads);

    WHEN("One task per tag on rank 0 sends 23 plus the thread index and the main thread of rank 1 receives it") {
      std::vector<int> incomingValues;
      if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
        spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
          return new PingPongSendTask(threadIndex, baseTag + threadIndex, true);
        });
      }
      if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
        for (int threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
          int incomingValue{-12};
          tarch::mpi::receive(&incomingValue, 1, MPI_INT, 0, baseTag + threadIndex, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          incomingValues.push_back(incomingValue);
        }
      }
      tarch::mpi::barrier(MPI_COMM_WORLD);

      THEN("Rank 1 receives 23 plus the thread index under every tag") {
        for (int threadIndex = 0; threadIndex < static_cast<int>(incomingValues.size()); threadIndex++) {
          VALIDATE_EQUALS(incomingValues[threadIndex], 23 + threadIndex, threadIndex);
        }
      }
    }
  }
}

UNIT_TEST("Concurrent tasks of rank 1 receive what concurrent tasks of rank 0 send, all with blocking calls") {
  GIVEN("One reserved tag per thread") {
    const int numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
    const int baseTag =
      tarch::mpi::Rank::reserveFreeTag("tarch/mpi/PingPongTest blocking sends and receives in tasks", numberOfThreads);

    WHEN("One task per tag on rank 0 sends 23 plus the thread index and one task per tag on rank 1 receives it") {
      std::vector<int> incomingValues;
      if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
        spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
          return new PingPongSendTask(threadIndex, baseTag + threadIndex, true);
        });
      }
      if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
        incomingValues.assign(numberOfThreads, -12);
        spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
          return new PingPongReceiveTask(baseTag + threadIndex, true, incomingValues[threadIndex]);
        });
      }
      tarch::mpi::barrier(MPI_COMM_WORLD);

      THEN("Rank 1 receives 23 plus the thread index under every tag") {
        for (int threadIndex = 0; threadIndex < static_cast<int>(incomingValues.size()); threadIndex++) {
          VALIDATE_EQUALS(incomingValues[threadIndex], 23 + threadIndex, threadIndex);
        }
      }
    }
  }
}

UNIT_TEST("Concurrent tasks of rank 1 receive with non-blocking calls what the main thread of rank 0 sends") {
  GIVEN("One reserved tag per thread") {
    const int numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
    const int baseTag =
      tarch::mpi::Rank::reserveFreeTag("tarch/mpi/PingPongTest non-blocking receives in tasks", numberOfThreads);

    WHEN("Rank 0 sends 23 plus the thread index under every tag and one task per tag on rank 1 receives it") {
      std::vector<int> incomingValues;
      if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
        for (int threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
          const int outgoingValue = 23 + threadIndex;
          tarch::mpi::send(&outgoingValue, 1, MPI_INT, 1, baseTag + threadIndex, MPI_COMM_WORLD);
        }
      }
      if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
        incomingValues.assign(numberOfThreads, -12);
        spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
          return new PingPongReceiveTask(baseTag + threadIndex, false, incomingValues[threadIndex]);
        });
      }
      tarch::mpi::barrier(MPI_COMM_WORLD);

      THEN("Rank 1 receives 23 plus the thread index under every tag") {
        for (int threadIndex = 0; threadIndex < static_cast<int>(incomingValues.size()); threadIndex++) {
          VALIDATE_EQUALS(incomingValues[threadIndex], 23 + threadIndex, threadIndex);
        }
      }
    }
  }
}

UNIT_TEST("The main thread of rank 1 receives what concurrent tasks of rank 0 send with non-blocking calls") {
  GIVEN("One reserved tag per thread") {
    const int numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
    const int baseTag =
      tarch::mpi::Rank::reserveFreeTag("tarch/mpi/PingPongTest non-blocking sends in tasks", numberOfThreads);

    WHEN("One task per tag on rank 0 sends 23 plus the thread index and the main thread of rank 1 receives it") {
      std::vector<int> incomingValues;
      if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
        spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
          return new PingPongSendTask(threadIndex, baseTag + threadIndex, false);
        });
      }
      if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
        for (int threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
          int incomingValue{-12};
          tarch::mpi::receive(&incomingValue, 1, MPI_INT, 0, baseTag + threadIndex, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          incomingValues.push_back(incomingValue);
        }
      }
      tarch::mpi::barrier(MPI_COMM_WORLD);

      THEN("Rank 1 receives 23 plus the thread index under every tag") {
        for (int threadIndex = 0; threadIndex < static_cast<int>(incomingValues.size()); threadIndex++) {
          VALIDATE_EQUALS(incomingValues[threadIndex], 23 + threadIndex, threadIndex);
        }
      }
    }
  }
}

UNIT_TEST("Concurrent tasks of rank 1 receive what concurrent tasks of rank 0 send, all with non-blocking calls") {
  GIVEN("One reserved tag per thread") {
    const int numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
    const int baseTag         = tarch::mpi::Rank::reserveFreeTag(
      "tarch/mpi/PingPongTest non-blocking sends and receives in tasks",
      numberOfThreads
    );

    WHEN("One task per tag on rank 0 sends 23 plus the thread index and one task per tag on rank 1 receives it") {
      std::vector<int> incomingValues;
      if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 0) {
        spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
          return new PingPongSendTask(threadIndex, baseTag + threadIndex, false);
        });
      }
      if (tarch::mpi::Rank::getInstance().getNumberOfRanks() >= 2 and tarch::mpi::Rank::getInstance().getRank() == 1) {
        incomingValues.assign(numberOfThreads, -12);
        spawnTasksAndWaitForDependencies(numberOfThreads, [&](const int threadIndex) {
          return new PingPongReceiveTask(baseTag + threadIndex, false, incomingValues[threadIndex]);
        });
      }
      tarch::mpi::barrier(MPI_COMM_WORLD);

      THEN("Rank 1 receives 23 plus the thread index under every tag") {
        for (int threadIndex = 0; threadIndex < static_cast<int>(incomingValues.size()); threadIndex++) {
          VALIDATE_EQUALS(incomingValues[threadIndex], 23 + threadIndex, threadIndex);
        }
      }
    }
  }
}
