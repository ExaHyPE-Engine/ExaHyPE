// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TreeIdMapping.h"

#include "peano4/parallel/ExchangeChannels.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Log.h"

peano4::parallel::TreeIdMapping peano4::parallel::TreeIdMapping::_singleton;

peano4::parallel::TreeIdMapping& peano4::parallel::TreeIdMapping::getInstance() { return _singleton; }

/**
 * The global master hosts the first spacetree from the start, so its slot for
 * tree 0 is taken here rather than through a reservation.
 */
void peano4::parallel::TreeIdMapping::init() {
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    registerId(0);
  }
}

int peano4::parallel::TreeIdMapping::getNumberOfRegisteredTrees() const {
  return static_cast<int>(_occupiedTreeSlots.count());
}

int peano4::parallel::TreeIdMapping::reserveId() { return reserveIds(1).front(); }

/**
 * Reserve a batch of IDs in one pass over the local slots.
 *
 * Results are ordered by local tree ID, which makes a batched split
 * deterministic even when earlier slots have been released again.
 */
std::vector<int> peano4::parallel::TreeIdMapping::reserveIds(const int numberOfIds) {
  if (numberOfIds < 1) {
    tarch::terminateWithError("The number of tree IDs to reserve has to be positive.");
  }

  const int rank = tarch::mpi::Rank::getInstance().getRank();
  logDebug("Trying to reserve " << numberOfIds << " global IDs on rank " << rank);

  std::vector<int> result;
  result.reserve(numberOfIds);
  for (int localTreeId = 0; localTreeId < MAX_SPACETREES_PER_RANK and static_cast<int>(result.size()) < numberOfIds;
       ++localTreeId) {
    if (not _occupiedTreeSlots[localTreeId]) {
      const int treeId = getId(rank, localTreeId);
      registerId(treeId);
      result.push_back(treeId);
    }
  }

  if (static_cast<int>(result.size()) != numberOfIds) {
    tarch::terminateWithError(
      "Could not reserve " + std::to_string(numberOfIds) + " tree IDs because rank " + std::to_string(rank)
      + " has only " + std::to_string(result.size()) + " free local tree slots."
    );
  }

  return result;
}

/**
 * Occupy the slot and demand the data-exchange communicator of its group, so
 * the next collective creation round covers the new tree.
 */
void peano4::parallel::TreeIdMapping::registerId(const int treeId) {
  logDebug("Register new global id " << treeId);
  const int localTreeId = getLocalTreeId(treeId);
  assertion(not _occupiedTreeSlots[localTreeId], treeId, localTreeId);

  _occupiedTreeSlots[localTreeId] = true;
  peano4::parallel::ExchangeChannels::getInstance().requestCommunicator(localTreeId);
}

void peano4::parallel::TreeIdMapping::deregisterId(const int treeId) {
  const int localTreeId = getLocalTreeId(treeId);
  assertion(_occupiedTreeSlots[localTreeId], treeId, localTreeId);

  _occupiedTreeSlots[localTreeId] = false;

  logDebug("Removed tree " << treeId << " from list of trees");
}
