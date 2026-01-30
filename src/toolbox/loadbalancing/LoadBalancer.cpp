// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadBalancer.h"

#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/multicore/Core.h"

#include <sstream>

tarch::logging::Log toolbox::loadbalancing::LoadBalancer::_log("toolbox::loadbalancing::LoadBalancer");

toolbox::loadbalancing::LoadBalancer::LoadBalancer(Configuration* configuration):
  _configuration(configuration),
  _blacklist{},
  _statistics{},
  _state(State::InterRankDistribution) {

  _statistics.notifyOfStateChange(_state);
}

toolbox::loadbalancing::LoadBalancer::~LoadBalancer() {
  delete _configuration;
  _configuration = nullptr;
}

void toolbox::loadbalancing::LoadBalancer::enable(bool value) {
  if (value and _state == State::SwitchedOff) {
    _state = State::InterRankBalancing;
    logDebug("enable()", "Switched load balancing on (again) and therefore change to state " << _state);
  } else if (not value and _state != State::SwitchedOff) {
    logDebug("enable()", "Switched load balancing off");
    _state = State::SwitchedOff;
  }

  _statistics.notifyOfStateChange(_state);
}

bool toolbox::loadbalancing::LoadBalancer::isEnabled(bool globally) const {
  return globally ? (_statistics.getGlobalNumberOfRanksWithEnabledLoadBalancing() > 0) : (_state != State::SwitchedOff);
}

void toolbox::loadbalancing::LoadBalancer::update() {
  _statistics.updateGlobalView();
  _blacklist.update();

  logDebug("update()", toString());

  // TODO: Get rid of the states, combine inter-rank and intra-rank into one function? Does that make sense?
  // TODO: Introduce new mechanism to detect different kind of meshes: Regular, static AMR, dynamic AMR
  if (_state == State::InterRankDistribution) {
    updateInterRankLoadBalancing();
  } else if (_state == State::IntraRankDistribution) {
    updateIntraRankLoadBalancing();
  }

  _statistics.notifyOfStateChange(_state);
}

int toolbox::loadbalancing::LoadBalancer::getMaxPermittedTreesPerRank() const {
  return _configuration->getMaxLocalTreesPerRank(_state);
}

int toolbox::loadbalancing::LoadBalancer::getGlobalNumberOfTrees() const {
  return _statistics.getGlobalNumberOfTrees();
}

int toolbox::loadbalancing::LoadBalancer::getGlobalNumberOfInnerUnrefinedCells() const {
  return _statistics.getGlobalNumberOfInnerUnrefinedCells();
}

int toolbox::loadbalancing::LoadBalancer::getGlobalNumberOfOuterUnrefinedCells() const {
  return _statistics.getGlobalNumberOfOuterUnrefinedCells();
}

int toolbox::loadbalancing::LoadBalancer::getGlobalNumberOfVertices() const {
  return _statistics.getGlobalNumberOfVertices();
}

int toolbox::loadbalancing::LoadBalancer::getGlobalNumberOfHangingVertices() const {
  return _statistics.getGlobalNumberOfHangingVertices();
}

int toolbox::loadbalancing::LoadBalancer::getGlobalMinTreeLoad() const { return _statistics.getGlobalMinTreeLoad(); }

int toolbox::loadbalancing::LoadBalancer::getGlobalMaxTreeLoad() const { return _statistics.getGlobalMaxTreeLoad(); }

std::size_t toolbox::loadbalancing::LoadBalancer::getGlobalMemoryUsageInBytes() const {
  return _statistics.getGlobalMemoryUsageInBytes();
}

int toolbox::loadbalancing::LoadBalancer::getGlobalMinDepth() const { return _statistics.getGlobalMinDepth(); }

int toolbox::loadbalancing::LoadBalancer::getGlobalMaxDepth() const { return _statistics.getGlobalMaxDepth(); }

double toolbox::loadbalancing::LoadBalancer::getGlobalMinH() const { return _statistics.getGlobalMinH(); }

int toolbox::loadbalancing::LoadBalancer::getGlobalNumberOfStationarySweeps() const {
  return _statistics.getGlobalNumberOfStationarySweeps();
}

bool toolbox::loadbalancing::LoadBalancer::hasStagnated() const {
  return _state == State::Stagnation or _state == State::SwitchedOff;
}

bool toolbox::loadbalancing::LoadBalancer::hasSplitRecently() const {
  return _statistics.getNumberOfStateUpdatesWithoutAnySplit() < 3; // TODO: Don't use hardcoded magic numbers
}

int toolbox::loadbalancing::LoadBalancer::getNumberOfSettlingIterations() const {
  // Define the necessary settling time
  // - 3 iterations: Required for a tree to physically split and clear the Blacklist.
  // - 1 iteration:  Required for local statistics to update after the physical change.
  // - 1 iteration:  Required for global MPI reduction to reflect the new local stats.
  constexpr int RequiredSettlingIterations = 5;
  int           result                     = RequiredSettlingIterations;
  return result;
}

void toolbox::loadbalancing::LoadBalancer::updateInterRankLoadBalancing() {
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();

  if (numberOfRanks <= 1 or _statistics.getGlobalNumberOfTrees() >= numberOfRanks) {
    _state = State::IntraRankDistribution;
    return;
  }

  // Break if we cannot further split the mesh although we still have ranks that are not satisfied
  if (_statistics.getNumberOfStateUpdatesWithoutAnySplit() > getNumberOfSettlingIterations()) {
    logWarning(
      "updateInterRankLoadBalancing",
      "Current global mesh is too small or poorly balanced to split further to ranks. Please consider increasing the mesh size or reduce the number of ranks."
    );
    _state = State::IntraRankDistribution;
    return;
  }

  if (hasSplitRecently() or not _statistics.hasConsistentViewOfWorld()) {
    return; // Nothing to do yet
  }

  const int localHeaviestTreeId = getHeaviestNonBlacklistedLocalSpacetreeID();
  if (localHeaviestTreeId == -1) {
    logDebug("updateInterRankLoadBalancing()", "No valid local tree found to split");
    return;
  }

  const int localHeaviestTreeLoad = peano4::parallel::SpacetreeSet::getInstance()
                                      .getGridStatistics(localHeaviestTreeId)
                                      .numberOfLocalUnrefinedCells;

  // We compare against the global max load. If I'm not close to the heaviest global tree,
  // I should yield to let the heavier tree split first.
  const int globalMaxLoad = _statistics.getGlobalMaxTreeLoad();
  if (localHeaviestTreeLoad < globalMaxLoad * 0.9) {
    // Someone else is significantly heavier, let them balance first.
    logDebug(
      "updateInterRankLoadBalancing()",
      "Yielding: Load "
        << localHeaviestTreeLoad << " of local tree " << localHeaviestTreeId << " is smaller than global max "
        << globalMaxLoad
    );
    return;
  }

  const int numberOfCells     = _statistics.getGlobalNumberOfInnerUnrefinedCells();
  const int minTreeSize       = _configuration->getMinTreeSize(_state);
  const int idealCellsPerTree = std::max(static_cast<int>(std::round(numberOfCells / numberOfRanks)), 1);

  logDebug(
    "updateInterRankLoadBalancing()",
    "Evaluating Split. Ranks: "
      << numberOfRanks << ", Global Trees: " << _statistics.getGlobalNumberOfTrees()
      << ", Total Cells: " << numberOfCells << ", Ideal/Tree: " << idealCellsPerTree << ", MinTreeSize: " << minTreeSize
  );

  int nextTargetRank = _statistics.getGlobalNumberOfTrees() + tarch::mpi::Rank::getInstance().getRank();
  if (nextTargetRank >= numberOfRanks) {
    logDebug("updateInterRankLoadBalancing()", "Next target rank " << nextTargetRank << " is out of bounds");
    return;
  }

  int splitsTriggered = 0;
  while (nextTargetRank < numberOfRanks) {
    const int currentProjectedLoad = localHeaviestTreeLoad - (splitsTriggered * idealCellsPerTree);

    // Stop if the tree would become too small
    if (currentProjectedLoad - idealCellsPerTree < minTreeSize) {
      logDebug(
        "updateInterRankLoadBalancing()",
        "Stopping split sequence: Remaining load ("
          << currentProjectedLoad << ") minus split (" << idealCellsPerTree << ") would be less than MinTreeSize ("
          << minTreeSize << ")"
      );
      break;
    }

    // TODO: Adjust for remainder cells on the very last rank
    const int cellsToSplit = idealCellsPerTree;

    logDebug(
      "updateInterRankLoadBalancing()",
      "Local heaviest tree "
        << localHeaviestTreeId << " has " << localHeaviestTreeLoad << " cells. " << "Splitting " << cellsToSplit
        << " cells to rank " << nextTargetRank << " (hierarchical split #" << splitsTriggered + 1 << ")"
    );

    triggerSplit(cellsToSplit, nextTargetRank, localHeaviestTreeId);
    splitsTriggered++;

    // If we split multiple times, we must jump over all other ranks to find our next unique slot.
    // E.g. if there are 9 existing trees, I target 9 + myRank, then 9 + myRank + 9, etc.
    // This ensures hierarchy.
    nextTargetRank += _statistics.getGlobalNumberOfTrees();
  }
}

void toolbox::loadbalancing::LoadBalancer::updateIntraRankLoadBalancing() {
  const int numberOfThreads          = tarch::multicore::Core::getInstance().getNumberOfThreads();
  const int maxPermittedTreesPerRank = getMaxPermittedTreesPerRank();

  if (numberOfThreads <= 1 or _statistics.getLocalNumberOfTrees() >= maxPermittedTreesPerRank) {
    _state = State::Stagnation;
    return;
  }

  if (_statistics.getNumberOfStateUpdatesWithoutAnySplit() > getNumberOfSettlingIterations()) {
    logWarning(
      "updateIntraRankLoadBalancing",
      "Current local mesh is too small or poorly balanced to split further into tasks. Please consider increasing the mesh size or reduce the number of local trees."
    );
    _state = State::Stagnation;
  }

  if (hasSplitRecently() or not _statistics.hasConsistentViewOfWorld()) {
    return; // Nothing to do yet
  }

  const int minTreeSize                           = _configuration->getMinTreeSize(_state);
  const int numberOfCells                         = _statistics.getLocalNumberOfInnerUnrefinedCells();
  const int numberOfTreesToAccommodateMinTreeSize = std::max(numberOfCells / minTreeSize, 1);

  if (numberOfTreesToAccommodateMinTreeSize < maxPermittedTreesPerRank) {
    return; // Nothing to do (yet)
  }

  const int    numberOfAllowedTrees = std::min({numberOfTreesToAccommodateMinTreeSize, maxPermittedTreesPerRank});
  unsigned int numberOfSplits       = numberOfAllowedTrees - 1;
  if (numberOfSplits > 0) {
    const int numberOfCellsPerTree = std::max(static_cast<int>(std::round(numberOfCells / numberOfAllowedTrees)), 1);

    logDebug(
      "updateIntraRankLoadBalancing()",
      "Mesh has "
        << numberOfCells << " cells, so split " << numberOfSplits << " times "
        << "("
        << "MinTreeSize=" << minTreeSize << ", MaxPermittedTreesPerRank=" << maxPermittedTreesPerRank
        << ", NumberOfTreesToAccommodateMinTreeSize=" << numberOfTreesToAccommodateMinTreeSize
        << ", NumberOfCellsPerTree=" << numberOfCellsPerTree << ")"
    );

    unsigned int remainder = numberOfCells - (numberOfCellsPerTree * numberOfAllowedTrees);
    while (numberOfSplits) {
      if (remainder > 0) {
        triggerSplit(
          numberOfCellsPerTree + 1,
          tarch::mpi::Rank::getInstance().getRank(),
          *(peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().begin())
        );
        remainder--;
      } else {
        triggerSplit(
          numberOfCellsPerTree,
          tarch::mpi::Rank::getInstance().getRank(),
          *(peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().begin())
        );
      }
      numberOfSplits--;
    }
  }
}

void toolbox::loadbalancing::LoadBalancer::triggerSplit(int numberOfCells, int targetRank, int sourceTree) {
  const bool success = peano4::parallel::SpacetreeSet::getInstance().split(
    sourceTree,
    peano4::grid::SplitInstruction{numberOfCells, _configuration->getMode(_state)},
    targetRank
  );

  if (not success) {
    logError("triggerSplit()", "Wanted to split tree " << sourceTree << " but failed");
    assertion(false);
  }

  _blacklist.triggeredSplit(sourceTree);
  _statistics.incLocalNumberOfSplits();
}

std::string toolbox::loadbalancing::LoadBalancer::toString() const {
  std::ostringstream msg;
  msg
    << "{State=" << _state << ",Statistics=" << _statistics.toString() << ",Blacklist=" << _blacklist.toString() << "}";
  return msg.str();
}

std::string toolbox::loadbalancing::LoadBalancer::getStatistics() {
  std::ostringstream msg;

  int         sumLocal             = 0;
  int         sumRemote            = 0;
  const auto& idsOfLocalSpacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();

  msg << idsOfLocalSpacetrees.size() << " tree(s): ";

  for (auto p : idsOfLocalSpacetrees) {
    msg
      << "(#" << p << ":"
      << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).numberOfLocalUnrefinedCells << "/"
      << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).numberOfRemoteUnrefinedCells << ")";
    sumLocal += peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).numberOfLocalUnrefinedCells;
    sumRemote += peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).numberOfRemoteUnrefinedCells;
  }
  if (idsOfLocalSpacetrees.empty()) {
    msg << "no trees hosted on this node - therefore no load";
  } else {
    msg << " total=" << sumLocal << "/" << sumRemote << " (local/virtual)";
  }

  return msg.str();
}

int toolbox::loadbalancing::LoadBalancer::getHeaviestNonBlacklistedLocalSpacetreeID() const {
  int heaviestTreeId = -1;
  int maxCells       = -1;

  const auto& localTrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();

  for (int treeId : localTrees) {
    if (_blacklist.isBlacklisted(treeId)) {
      continue;
    }

    const int currentCells = peano4::parallel::SpacetreeSet::getInstance()
                               .getGridStatistics(treeId)
                               .numberOfLocalUnrefinedCells;
    if (currentCells > maxCells) {
      maxCells       = currentCells;
      heaviestTreeId = treeId;
    }
  }

  return heaviestTreeId;
}
