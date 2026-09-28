// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AbstractCommunicationManager.h"

#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/SerialCommunicationManager.h"
#include "tarch/mpi/ThreadedCommunicationManager.h"

bool tarch::mpi::AbstractCommunicationManager::_dedicatedThreadIsUsed = false;

/**
 * Returns the implementation the process runs with. Both implementations exist in every binary, and
 * configure() selects one at run time.
 */
tarch::mpi::AbstractCommunicationManager&
tarch::mpi::AbstractCommunicationManager::getInstance() {
  static tarch::mpi::SerialCommunicationManager   serialManager;
  static tarch::mpi::ThreadedCommunicationManager threadedManager;
  if (usesDedicatedThread()) {
    return threadedManager;
  }
  return serialManager;
}

/**
 * Sets whether this process dedicates a thread to communication. A single-rank run never dedicates
 * a thread. The task runtime sizes its thread team by this decision, so the decision holds for the
 * whole run.
 */
void
tarch::mpi::AbstractCommunicationManager::configure(const bool communicationThreadEnabled) {
  _dedicatedThreadIsUsed = communicationThreadEnabled and tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1;
}

/**
 * Returns whether the process drives its communication on a thread of its own.
 */
bool
tarch::mpi::AbstractCommunicationManager::usesDedicatedThread() {
  return _dedicatedThreadIsUsed;
}

/**
 * Registers one poller. A caller registers every poller before start(), so the polling thread reads
 * the list without a lock.
 */
void
tarch::mpi::AbstractCommunicationManager::registerPoller(Poller poller) {
  assertion(static_cast<bool>(poller));
  _pollers.push_back(std::move(poller));
}

/**
 * Runs every registered poller once and reports whether all of them were idle.
 */
bool
tarch::mpi::AbstractCommunicationManager::runPollersOnce() {
  bool allPollersIdle = true;
  for (auto& poller : _pollers) {
    allPollersIdle = poller() and allPollersIdle;
  }
  return allPollersIdle;
}
