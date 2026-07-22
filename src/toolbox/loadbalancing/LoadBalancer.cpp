// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadBalancer.h"

#include "peano4/Globals.h"
#include "peano4/grid/GridStatistics.h"
#include "peano4/grid/SplitInstruction.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/Termination.h"
#include "tarch/mpi/Rank.h"
#include "toolbox/loadbalancing/InitialSplitPlan.h"

toolbox::loadbalancing::LoadBalancer::LoadBalancer(
  std::unique_ptr<toolbox::loadbalancing::Configuration> configuration
) :
  _configuration(
    configuration ? std::move(configuration) : throw std::invalid_argument("Configuration must not be null.")
  ),
  _initialTreesPerRank(
    std::max(1, _configuration->getMaxLocalTreesPerRank(toolbox::loadbalancing::State::InitialInterRankSplit))
  ),
  _blacklist{},
  _statistics{},
  _state(toolbox::loadbalancing::State::SwitchedOff),
  _strategy{} {
  setState(toolbox::loadbalancing::State::InitialInterRankSplit);
}

toolbox::loadbalancing::LoadBalancer::~LoadBalancer() { shutdown(); }

void toolbox::loadbalancing::LoadBalancer::shutdown() {
  _statistics.completePendingReduction();
  toolbox::loadbalancing::Statistics::shutdownMPI();
}

void toolbox::loadbalancing::LoadBalancer::setState(const toolbox::loadbalancing::State state) {
  _state = state;
  switch (state) {
  case toolbox::loadbalancing::State::InitialInterRankSplit:
    _strategy = makeInitialInterRankSplit();
    break;
  case toolbox::loadbalancing::State::InitialIntraRankSplit:
    _strategy = makeInitialIntraRankSplit();
    break;
  default:
    _strategy.reset();
    break;
  }
}

/**
 * Re-enabling starts from the last coherent global view. A settled partial inter-rank split is rejected
 * because its rank imbalance has no bound.
 */
void toolbox::loadbalancing::LoadBalancer::enable(const bool value) {
  if (value and _state == toolbox::loadbalancing::State::SwitchedOff) {
    _statistics.resetInitialDecompositionState();
    _blacklist = toolbox::loadbalancing::Blacklist{};
    setState(toolbox::loadbalancing::State::InitialInterRankSplit);
  } else if (not value and _state != toolbox::loadbalancing::State::SwitchedOff) {
    setState(toolbox::loadbalancing::State::SwitchedOff);
  }
}

/**
 * Consumes the preceding global exchange before the strategy runs and starts the next exchange after the decision.
 * This order publishes forks issued in this sweep together with the statistics that led to them.
 */
void toolbox::loadbalancing::LoadBalancer::update() {
  _statistics.updateGlobalView();
  if (_strategy) {
    updateBlacklist();
  }

  if (_state == toolbox::loadbalancing::State::InitialInterRankSplit
      or _state == toolbox::loadbalancing::State::InitialIntraRankSplit) {
    const auto& spacetreeSet = peano4::parallel::SpacetreeSet::getInstance();
    const auto  localTreeIds = spacetreeSet.getLocalSpacetrees();
    for (const auto treeId : localTreeIds) {
      if (spacetreeSet.getGridStatistics(treeId).getRemovedEmptySubtree()) {
        const auto message = std::string("An initial split produced an empty tree. ") + describeLocalDecomposition();
        tarch::terminateWithError(message);
      }
    }
  }

  updateStrategy();
  _statistics.startGlobalViewExchange();
}

/** Completes the current global exchange before a controller or summary requires a current snapshot. */
void toolbox::loadbalancing::LoadBalancer::synchroniseGlobalView() { _statistics.synchroniseGlobalView(); }

void toolbox::loadbalancing::LoadBalancer::updateBlacklist() {
  _blacklist.update([](const int treeId) {
    const auto& spacetreeSet   = peano4::parallel::SpacetreeSet::getInstance();
    const auto& treeStatistics = spacetreeSet.getGridStatistics(treeId);
    return toolbox::loadbalancing::Blacklist::TreeState{
      treeStatistics.numberOfLocalUnrefinedCells,
      treeStatistics.getRemovedEmptySubtree()
    };
  });
}

void toolbox::loadbalancing::LoadBalancer::updateStrategy() {
  if (not _strategy) {
    return;
  }

  const auto configuredNumberOfTrees = getConfiguredNumberOfTrees();
  const auto availableNumberOfCells  = _statistics.getGlobalNumberOfInnerUnrefinedCells();
  if (toolbox::loadbalancing::InitialSplitPlan::classifyDecomposition(
        configuredNumberOfTrees,
        availableNumberOfCells,
        hasUniformGridCompleted()
      )
      == toolbox::loadbalancing::InitialSplitPlan::Decomposition::InsufficientCells) {
    tarch::terminateWithError(
      "Initial decomposition requires " + std::to_string(configuredNumberOfTrees)
      + " trees, but the completed uniform grid contains only " + std::to_string(availableNumberOfCells) + " cells."
    );
  }

  const auto next = _strategy->update(*this);
  if (next != _state) {
    setState(next);
  }
}

int toolbox::loadbalancing::LoadBalancer::getMaxPermittedTreesPerRank() const { return _initialTreesPerRank; }

bool toolbox::loadbalancing::LoadBalancer::isStagnating() const {
  return _state == toolbox::loadbalancing::State::Stagnation or _state == toolbox::loadbalancing::State::SwitchedOff;
}

/**
 * The caller holds a uniform level while a fork settles or can be issued. Below the relevant piece-size threshold,
 * another refinement level is required before a split can make progress, so intermediate settling sweeps are skipped.
 */
bool toolbox::loadbalancing::LoadBalancer::needsGridToStayStationary() const {
  if (_statistics.getGlobalNumberOfSplits() > 0 or _statistics.getGlobalNumberOfStationarySweeps() < 0) {
    return true;
  }

  if (isStagnating()) {
    return false;
  }

  const auto uniformGridComplete = hasUniformGridCompleted();
  if (_statistics.hasAdaptiveMesh() and not uniformGridComplete) {
    return false;
  }

  const auto globalNumberOfTrees = _statistics.getGlobalNumberOfTrees();
  if (globalNumberOfTrees >= getConfiguredNumberOfTrees()) {
    return false;
  }
  if (uniformGridComplete) {
    return true;
  }

  const auto numberOfRanks        = std::max(1, tarch::mpi::Rank::getInstance().getNumberOfRanks());
  const auto numberOfRanksForLoad = static_cast<std::uint64_t>(numberOfRanks);
  const auto globalLoad           = _statistics.getGlobalNumberOfInnerUnrefinedCells();
  const auto minimumPieceSize     = static_cast<std::uint64_t>(minPieceSizeForInitialSplits());
  if (_statistics.getGlobalNumberOfRanksWithAtLeastOneTree() < numberOfRanks) {
    return globalLoad / numberOfRanksForLoad >= minimumPieceSize;
  }
  return globalLoad / static_cast<std::uint64_t>(getConfiguredNumberOfTrees()) >= minimumPieceSize;
}

/**
 * Uniform refinement cannot provide more splittable cells after this notification.
 *
 * The phase controller derives this event from a globally reduced grid snapshot on the master. Strategies therefore
 * use hasUniformGridCompleted(), which lets the source rank observe the local notification immediately while the
 * asynchronous statistics reduction forwards it to the workers.
 */
void toolbox::loadbalancing::LoadBalancer::notifyUniformGridComplete() { _statistics.notifyUniformGridComplete(); }

bool toolbox::loadbalancing::LoadBalancer::hasUniformGridCompleted() const {
  return _statistics.hasUniformGridCompleted();
}

/**
 * A fork starts only from a coherent tree set without another fork in flight. A negative stationary-sweep count marks
 * a new tree whose vertices are still becoming persistent, while one local stationary sweep makes the source safe to
 * cut.
 */
bool toolbox::loadbalancing::LoadBalancer::mayTriggerSplits() const {
  return _statistics.getGlobalNumberOfSplits() == 0 and _statistics.hasConsistentViewOfWorld()
    and _statistics.getLocalNumberOfStationarySweeps() >= 1 and _statistics.getGlobalNumberOfStationarySweeps() >= 0;
}

/**
 * One coarse cell refines into three children per axis. Waiting for that many cells per target piece limits the
 * floor/ceil imbalance introduced by integer cell counts.
 */
int toolbox::loadbalancing::LoadBalancer::minPieceSizeForInitialSplits() const {
  return static_cast<int>(THREE_POWER_D);
}

/**
 * All ranks use the cached per-rank target because configuration may depend on transient state.
 */
int toolbox::loadbalancing::LoadBalancer::getConfiguredNumberOfTrees() const {
  return tarch::mpi::Rank::getInstance().getNumberOfRanks() * _initialTreesPerRank;
}

const toolbox::loadbalancing::Statistics& toolbox::loadbalancing::LoadBalancer::getStatistics() const {
  return _statistics;
}

/** Submit the complete deterministic plan through one batched tree-management operation. */
void toolbox::loadbalancing::LoadBalancer::triggerSplits(
  const int                                       sourceTreeId,
  const toolbox::loadbalancing::InitialSplitPlan& plan
) {
  auto& spacetreeSet = peano4::parallel::SpacetreeSet::getInstance();
  std::vector<peano4::parallel::SpacetreeSet::SplitRequest> requests;
  requests.reserve(plan.requests.size());
  const auto mode = _configuration->getMode(_state);
  for (const auto& request : plan.requests) {
    requests.push_back({peano4::grid::SplitInstruction{request.numberOfCells, mode}, request.targetRank});
  }

  if (not spacetreeSet.split(sourceTreeId, requests)) {
    tarch::terminateWithError(
      "Failed to register " + std::to_string(requests.size()) + " exact splits from source tree "
      + std::to_string(sourceTreeId) + "."
    );
  }

  if (not requests.empty()) {
    _blacklist.notifySplit(sourceTreeId, spacetreeSet.getGridStatistics(sourceTreeId).numberOfLocalUnrefinedCells);
    for (std::size_t split = 0; split < requests.size(); ++split) {
      _statistics.incLocalNumberOfSplits();
    }
  }
}

std::string toolbox::loadbalancing::LoadBalancer::describeLocalDecomposition() {
  std::ostringstream message;
  message << "Trees on this rank: ";

  const auto& spacetreeSet = peano4::parallel::SpacetreeSet::getInstance();
  const auto  localTreeIds = spacetreeSet.getLocalSpacetrees();
  if (localTreeIds.empty()) {
    message << "none";
    return message.str();
  }

  message << localTreeIds.size();
  std::uint64_t numberOfLocalCells   = 0;
  std::uint64_t numberOfVirtualCells = 0;
  for (const auto treeId : localTreeIds) {
    const auto& statistics = spacetreeSet.getGridStatistics(treeId);
    message << " · #" << treeId << ": " << statistics.numberOfLocalUnrefinedCells << "/"
            << statistics.numberOfRemoteUnrefinedCells;
    numberOfLocalCells += static_cast<std::uint64_t>(statistics.numberOfLocalUnrefinedCells);
    numberOfVirtualCells += static_cast<std::uint64_t>(statistics.numberOfRemoteUnrefinedCells);
  }
  message << " · Total: " << numberOfLocalCells << "/" << numberOfVirtualCells << " (local/virtual cells)";

  return message.str();
}

int toolbox::loadbalancing::LoadBalancer::getHeaviestNonBlacklistedLocalSpacetreeId() const {
  int heaviestTreeId       = -1;
  int maximumNumberOfCells = 0;

  const auto& spacetreeSet = peano4::parallel::SpacetreeSet::getInstance();
  const auto  localTreeIds = spacetreeSet.getLocalSpacetrees();
  for (const int treeId : localTreeIds) {
    if (_blacklist.isBlacklisted(treeId) or not spacetreeSet.maySplit(treeId)) {
      continue;
    }

    const auto currentNumberOfCells = spacetreeSet.getGridStatistics(treeId).numberOfLocalUnrefinedCells;
    if (currentNumberOfCells > maximumNumberOfCells) {
      maximumNumberOfCells = currentNumberOfCells;
      heaviestTreeId       = treeId;
    }
  }

  return heaviestTreeId;
}
