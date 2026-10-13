// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SerialCommunicationManager.h"

void
tarch::mpi::SerialCommunicationManager::start() {}

void
tarch::mpi::SerialCommunicationManager::stop() {}

/**
 * Runs one pass over the registered pollers on the calling thread.
 */
void
tarch::mpi::SerialCommunicationManager::progress() {
  runPollersOnce();
}

/**
 * Polls until every poller is idle. A caller re-arms no poller during the call.
 */
void
tarch::mpi::SerialCommunicationManager::finishPendingWork() {
  while (not runPollersOnce()) {
  }
}
