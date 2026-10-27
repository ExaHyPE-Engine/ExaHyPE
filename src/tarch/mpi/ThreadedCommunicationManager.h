// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/mpi/AbstractCommunicationManager.h"
#include "tarch/mpi/HelperThread.h"

namespace tarch::mpi {
  class ThreadedCommunicationManager;
} // namespace tarch::mpi

/**
 * Drives the registered pollers on a thread of its own between start() and stop(). The thread
 * reserves a core. The task runtime shrinks its team by one thread while a run uses this manager.
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
  static constexpr int BusyPassesBeforePacing = 16;

  static constexpr std::chrono::microseconds PacedPassSleep{20};

  std::atomic<bool>          _allPollersIdle{false};
  std::atomic<unsigned long> _numberOfCompletedPasses{0};
  bool                       _threadIsRunning{false};
  int                        _consecutiveBusyPasses{0};

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
