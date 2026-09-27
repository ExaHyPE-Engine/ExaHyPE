// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ThreadedCommunicationManager.h"

void
tarch::mpi::ThreadedCommunicationManager::start() {
  _pollingThread.start();
  _threadIsRunning = true;
}

void
tarch::mpi::ThreadedCommunicationManager::stop() {
  _pollingThread.stop();
  _threadIsRunning = false;
}

/** The polling thread runs every poller, so this function does nothing. */
void
tarch::mpi::ThreadedCommunicationManager::progress() {}

/**
 * Waits until every poller is idle, and a caller re-arms no poller during the call. A pass that
 * began before the call may report a state older than the arming of the caller, so the wait trusts
 * the second pass that completes after entry. After stop(), the pollers run on the calling thread.
 */
void
tarch::mpi::ThreadedCommunicationManager::finishPendingWork() {
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
