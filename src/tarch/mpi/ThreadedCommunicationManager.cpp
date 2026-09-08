// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ThreadedCommunicationManager.h"

void tarch::mpi::ThreadedCommunicationManager::start() {
  _pollingThread.start();
  _threadIsRunning = true;
}

void tarch::mpi::ThreadedCommunicationManager::stop() {
  _pollingThread.stop();
  _threadIsRunning = false;
}

/** The polling thread owns all progress, so there is nothing to do inline. */
void tarch::mpi::ThreadedCommunicationManager::progress() {}

/**
 * Waits until every registered concern is idle. The caller guarantees that
 * nothing re-arms a poller during the call.
 *
 * A pass that began before the call may report a state older than the
 * caller's arming, so the wait only trusts a pass that started after entry:
 * The second pass completed from here on is such a pass. After stop() the
 * pollers run inline, as no thread polls them then.
 */
void tarch::mpi::ThreadedCommunicationManager::finishPendingWork() {
  if (not _threadIsRunning) {
    while (not runPollersOnce()) {
    }
    return;
  }

  const auto passesOnEntry = _numberOfCompletedPasses.load(std::memory_order_acquire);
  while (_numberOfCompletedPasses.load(std::memory_order_acquire) < passesOnEntry + 2
         or not _allPollersIdle.load(std::memory_order_relaxed)) {
  }
}
