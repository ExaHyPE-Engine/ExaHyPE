// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SerialCommunicationManager.h"

void tarch::mpi::SerialCommunicationManager::start() {}

void tarch::mpi::SerialCommunicationManager::stop() {}

/** Runs one pass over the registered pollers on the calling thread. */
void tarch::mpi::SerialCommunicationManager::progress() { runPollersOnce(); }

/**
 * Polls until every registered concern is idle. The caller guarantees that
 * nothing re-arms a poller during the call, so an idle pass is final.
 */
void tarch::mpi::SerialCommunicationManager::finishPendingWork() {
  while (not runPollersOnce()) {
  }
}
