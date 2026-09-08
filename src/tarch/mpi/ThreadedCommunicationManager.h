// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/mpi/AbstractCommunicationManager.h"
#include "tarch/mpi/HelperThread.h"

namespace tarch::mpi {
  class ThreadedCommunicationManager;
} // namespace tarch::mpi

/**
 * Drives the registered pollers on a thread of its own.
 *
 * The thread runs between start() and stop(), so every registered concern
 * progresses regardless of what the task scheduler's threads do. It reserves a
 * core: The task runtime shrinks its team by one thread while this manager is
 * selected.
 */
class tarch::mpi::ThreadedCommunicationManager final : public tarch::mpi::AbstractCommunicationManager {
public:
  ThreadedCommunicationManager()                                               = default;
  ThreadedCommunicationManager(const ThreadedCommunicationManager&)            = delete;
  ThreadedCommunicationManager(ThreadedCommunicationManager&&)                 = delete;
  ThreadedCommunicationManager& operator=(const ThreadedCommunicationManager&) = delete;
  ThreadedCommunicationManager& operator=(ThreadedCommunicationManager&&)      = delete;
  ~ThreadedCommunicationManager() override                                     = default;

  void start() override;
  void stop() override;
  void progress() override;
  void finishPendingWork() override;

private:
  /** Consecutive passes that found work pending without finishing it. */
  static constexpr int BusyPassesBeforePacing = 16;

  /** How long a paced pass sleeps, which is the completion latency the pacing may add. */
  static constexpr std::chrono::microseconds PacedPassSleep{20};

  std::atomic<bool>          _allPollersIdle{false};
  std::atomic<unsigned long> _numberOfCompletedPasses{0};
  bool                       _threadIsRunning{false};
  int                        _consecutiveBusyPasses{0};

  /**
   * Every pass tests the outstanding requests. A message in flight takes many
   * passes, and each test contends with the compute threads' own MPI calls
   * inside the library, so a long busy streak paces itself down to a short
   * sleep. An idle pass costs no MPI call and needs no pacing. The streak
   * counter stops at the threshold, as a streak lasts for as long as the rank
   * has messages in flight.
   */
  tarch::mpi::HelperThread _pollingThread{[this]() -> bool {
    const bool allPollersIdle = runPollersOnce();
    _allPollersIdle.store(allPollersIdle, std::memory_order_relaxed);
    _numberOfCompletedPasses.fetch_add(1, std::memory_order_release);
    if (allPollersIdle) {
      _consecutiveBusyPasses = 0;
    } else if (_consecutiveBusyPasses < BusyPassesBeforePacing) {
      _consecutiveBusyPasses++;
    } else {
      std::this_thread::sleep_for(PacedPassSleep);
    }
    return false;
  }};
};
