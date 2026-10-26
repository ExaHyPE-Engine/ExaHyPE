// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AbstractCommunicationManager.h"

#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/SerialCommunicationManager.h"
#include "tarch/mpi/ThreadedCommunicationManager.h"

bool tarch::mpi::AbstractCommunicationManager::_dedicatedThreadIsUsed = false;

/**
 * Returns the implementation the process runs with. Both implementations exist in every binary.
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
 * a thread. The setting applies to the whole run.
 */
void
tarch::mpi::AbstractCommunicationManager::configure(const bool communicationThreadEnabled) {
  _dedicatedThreadIsUsed = communicationThreadEnabled and tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1;
}

bool
tarch::mpi::AbstractCommunicationManager::usesDedicatedThread() {
  return _dedicatedThreadIsUsed;
}

/**
 * Registers one poller. A caller registers every poller before start(). The polling thread then
 * reads the list without a lock.
 */
void
tarch::mpi::AbstractCommunicationManager::registerPoller(Poller poller) {
  ASSERTION(static_cast<bool>(poller));
  _pollers.push_back(std::move(poller));
}

/**
 * Runs every registered poller once and returns whether all of them were idle.
 */
bool
tarch::mpi::AbstractCommunicationManager::runPollersOnce() {
  bool allPollersIdle = true;
  for (auto& poller : _pollers) {
    allPollersIdle = poller() and allPollersIdle;
  }
  return allPollersIdle;
}
