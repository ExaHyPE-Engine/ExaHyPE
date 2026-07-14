// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadBalancer.h"

#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"

toolbox::loadbalancing::LoadBalancer::LoadBalancer(Configuration* configuration) :
  _configuration(configuration),
  _blacklist{},
  _statistics{},
  _strategy(makeInterRankDistributionStrategy()) {
  setState(State::InterRankDistribution);
}

toolbox::loadbalancing::LoadBalancer::~LoadBalancer() {
  shutdown();

  delete _configuration;
  _configuration = nullptr;
}

void toolbox::loadbalancing::LoadBalancer::shutdown() {
  _statistics.completePendingReduction();
  Statistics::shutdownMPI();
}

void toolbox::loadbalancing::LoadBalancer::setState(State state) {
  _state = state;
  switch (state) {
  case State::InterRankDistribution:
    _strategy = makeInterRankDistributionStrategy();
    break;
  case State::IntraRankDistribution:
    _strategy = makeIntraRankDistributionStrategy();
    break;
  default:
    _strategy.reset();
    break;
  }
}

void toolbox::loadbalancing::LoadBalancer::enable(bool value) {
  if (value and _state == State::SwitchedOff) {
    setState(State::InterRankBalancing);
  } else if (not value and _state != State::SwitchedOff) {
    setState(State::SwitchedOff);
  }
}

void toolbox::loadbalancing::LoadBalancer::update() {
  _statistics.updateGlobalView();
  _blacklist.update();

  if (_strategy) {
    const State next = _strategy->update(*this);
    if (next != _state) {
      setState(next);
    }
  }
}

int toolbox::loadbalancing::LoadBalancer::getMaxPermittedTreesPerRank() const {
  return _configuration->getMaxLocalTreesPerRank(_state);
}

bool toolbox::loadbalancing::LoadBalancer::isStagnating() const {
  return _state == State::Stagnation or _state == State::SwitchedOff;
}

bool toolbox::loadbalancing::LoadBalancer::hasSplitRecently() const {
  return _statistics.getNumberOfStateUpdatesWithoutAnySplit() < 3; // TODO: Don't use hardcoded magic numbers
}

const toolbox::loadbalancing::Statistics& toolbox::loadbalancing::LoadBalancer::getStatistics() const {
  return _statistics;
}

void toolbox::loadbalancing::LoadBalancer::triggerSplit(int numberOfCells, int targetRank, int sourceTree) {
  const bool success = peano4::parallel::SpacetreeSet::getInstance().split(
    sourceTree,
    peano4::grid::SplitInstruction{numberOfCells, _configuration->getMode(_state)},
    targetRank
  );

  if (not success) {
    logError("Wanted to split tree " << sourceTree << " but failed");
    assertion(false);
  }

  _blacklist.notifySplit(sourceTree);
  _statistics.incLocalNumberOfSplits();
}

std::string toolbox::loadbalancing::LoadBalancer::getLocalLoadBalancing() {
  std::ostringstream msg;

  int         sumLocal             = 0;
  int         sumRemote            = 0;
  const auto& idsOfLocalSpacetrees = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();

  msg << idsOfLocalSpacetrees.size() << " tree(s): ";

  for (auto p : idsOfLocalSpacetrees) {
    msg << "(#" << p << ":"
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

    const int currentCells =
      peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(treeId).numberOfLocalUnrefinedCells;
    if (currentCells > maxCells) {
      maxCells       = currentCells;
      heaviestTreeId = treeId;
    }
  }

  return heaviestTreeId;
}
