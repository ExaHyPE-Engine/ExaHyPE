// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadBalancer.h"

#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/multicore/Core.h"

#include <sstream>

tarch::logging::Log toolbox::loadbalancing::LoadBalancer::_log("toolbox::loadbalancing::LoadBalancer");

toolbox::loadbalancing::LoadBalancer::LoadBalancer(Configuration* configuration):
  _configuration(configuration),
  _blacklist(),
  _statistics(),
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

  if (_state == State::InterRankDistribution) {
    updateInterRankLoadBalancing();
  } else if (_state == State::IntraRankDistribution) {
    updateIntraRankLoadBalancing();
  }

  _statistics.notifyOfStateChange(_state);
}

int toolbox::loadbalancing::LoadBalancer::getGlobalNumberOfTrees() const {
  return _statistics.getGlobalNumberOfTrees();
}

int toolbox::loadbalancing::LoadBalancer::getGlobalNumberOfInnerUnrefinedCells() const {
  return _statistics.getGlobalNumberOfInnerUnrefinedCells();
}

bool toolbox::loadbalancing::LoadBalancer::hasStagnated() const {
  return _state == State::Stagnation or _state == State::SwitchedOff;
}

bool toolbox::loadbalancing::LoadBalancer::hasSplitRecently() const {
  return _statistics.getNumberOfStateUpdatesWithoutAnySplit() < 3;
}

void toolbox::loadbalancing::LoadBalancer::updateInterRankLoadBalancing() {
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  if (numberOfRanks <= 1 or _statistics.getGlobalNumberOfTrees() > 1) {
    _state = State::IntraRankDistribution;
    return;
  }

  if (hasSplitRecently() or not _statistics.hasConsistentViewOfWorld()
      or not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    return; // Nothing to do yet
  }

  const int numberOfCells                         = _statistics.getGlobalNumberOfInnerUnrefinedCells();
  const int minTreeSize                           = _configuration->getMinTreeSize(_state);
  const int numberOfTreesToAccommodateMinTreeSize = std::max(numberOfCells / minTreeSize, 1);
  if (numberOfTreesToAccommodateMinTreeSize < numberOfRanks) {
    return; // Nothing to do (yet)
  }

  const int numberOfCellsPerTree = std::max(static_cast<int>(std::round(numberOfCells / numberOfRanks)), 1);
  logDebug(
    "updateInterRankLoadBalancing()",
    "Mesh has "
      << numberOfCells << " cells, so split " << numberOfRanks - 1 << " times "
      << "("
      << "MinTreeSize=" << minTreeSize << ", NumberOfTreesToAccommodateMinTreeSize="
      << numberOfTreesToAccommodateMinTreeSize << ", NumberOfCellsPerTree=" << numberOfCellsPerTree << ")"
  );

  for (int targetRank = 1; targetRank < numberOfRanks; targetRank++) {
    int numberOfCellsPerTreeWithRemainder = numberOfCellsPerTree;
    if (numberOfCells % numberOfRanks >= targetRank) {
      numberOfCellsPerTreeWithRemainder++;
    }
    triggerSplit(numberOfCellsPerTreeWithRemainder, targetRank);
  }
}

void toolbox::loadbalancing::LoadBalancer::updateIntraRankLoadBalancing() {
  const int numberOfThreads          = tarch::multicore::Core::getInstance().getNumberOfThreads();
  const int maxPermittedTreesPerRank = _configuration->getMaxLocalTreesPerRank(_state);
  if (numberOfThreads <= 1 or _statistics.getLocalNumberOfTrees() >= maxPermittedTreesPerRank) {
    _state = State::Stagnation;
    return;
  }

  const int minTreeSize                           = _configuration->getMinTreeSize(_state);
  const int numberOfCells                         = _statistics.getLocalNumberOfInnerUnrefinedCells();
  const int numberOfTreesToAccommodateMinTreeSize = std::max(numberOfCells / minTreeSize, 1);
  const int numberOfAllowedTrees                  = std::min({numberOfTreesToAccommodateMinTreeSize, maxPermittedTreesPerRank, numberOfThreads});
  if (numberOfTreesToAccommodateMinTreeSize < numberOfThreads or not _statistics.hasConsistentViewOfWorld()) {
    return; // Nothing to do (yet)
  }

  unsigned int numberOfSplits = numberOfAllowedTrees - 1;
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
        triggerSplit(numberOfCellsPerTree + 1, tarch::mpi::Rank::getInstance().getRank());
        remainder--;
      } else {
        triggerSplit(numberOfCellsPerTree, tarch::mpi::Rank::getInstance().getRank());
      }
      numberOfSplits--;
    }
  }
}

void toolbox::loadbalancing::LoadBalancer::triggerSplit(int numberOfCells, int targetRank) {
  const int  sourceTree = *(peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().begin());
  const bool success    = peano4::parallel::SpacetreeSet::getInstance().split(
    sourceTree,
    peano4::SplitInstruction{numberOfCells, _configuration->getMode(_state)},
    targetRank
  );

  if (not success) {
    logError("triggerSplit()", "Wanted to split tree " << sourceTree << " but failed");
  }

  _blacklist.triggeredSplit(sourceTree);
  _statistics.incLocalNumberOfSplits();
}

std::string toolbox::loadbalancing::LoadBalancer::toString() const {
  std::ostringstream msg;
  msg
    << "(State=" << _state << ",Statistics=" << _statistics.toString() << ",Blacklist=" << _blacklist.toString() << ")";
  return msg.str();
}

std::string toolbox::loadbalancing::LoadBalancer::getStatistics() {
  std::ostringstream msg;

  int           sumLocal             = 0;
  int           sumRemote            = 0;
  std::set<int> idsOfLocalSpacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();

  msg << idsOfLocalSpacetrees.size() << " tree(s): ";

  for (auto p : idsOfLocalSpacetrees) {
    msg
      << "(#" << p << ":"
      << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells() << "/"
      << peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfRemoteUnrefinedCells() << ")";
    sumLocal += peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfLocalUnrefinedCells();
    sumRemote += peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(p).getNumberOfRemoteUnrefinedCells();
  }
  if (idsOfLocalSpacetrees.empty()) {
    msg << "no trees hosted on this node - therefore no load";
  } else {
    msg << " total=" << sumLocal << "/" << sumRemote << " (local/virtual)";
  }

  return msg.str();
}
