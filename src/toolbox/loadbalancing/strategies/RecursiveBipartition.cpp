#include "../strategies/RecursiveBipartition.h"

#include "tarch/Assertions.h"
#include "tarch/tarch.h"
#include "peano4/grid/GridStatistics.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "peano4/utils/Globals.h"
#include "tarch/multicore/Core.h"
#include "toolbox/loadbalancing/loadbalancing.h"


tarch::logging::Log toolbox::loadbalancing::strategies::RecursiveBipartition::_log(
  "toolbox::loadbalancing::strategies::RecursiveBipartition"
);


toolbox::loadbalancing::strategies::RecursiveBipartition::RecursiveBipartition(
  Configuration* configuration, CostMetrics* costMetrics
):
  AbstractLoadBalancing(configuration, costMetrics),
  _roundRobinToken(0) {
  assertion(configuration != nullptr);
  assertion(costMetrics != nullptr);
  _state = State::SwitchedOff;
  _statistics.notifyOfStateChange(_state);
}


toolbox::loadbalancing::strategies::RecursiveBipartition::~RecursiveBipartition() {}


std::string toolbox::loadbalancing::strategies::RecursiveBipartition::toString() const {
  std::ostringstream msg;

  msg
    << "(type=recursive-bipartition"
    << ",state=" << ::toString(_state) << ",statistics=" << _statistics.toString() << ",cost-metrics="
    << _costMetrics->toString() << ",round-robin-token=" << _roundRobinToken << ",blacklist=" << _blacklist.toString()
    << ",heaviest-local-tree=" << getIdOfHeaviestLocalSpacetree() << " (analysed)"
    << ",heaviest-local-weight=" << getWeightOfHeaviestLocalSpacetree() << " (analysed)"
    << ",#local-trees=" << peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()
    << ",is-inter-rank-balancing-bad=" << isInterRankBalancingBad()
    << ",is-intra-rank-balancing-bad=" << isIntraRankBalancingBad() << ")";

  return msg.str();
}


void toolbox::loadbalancing::strategies::RecursiveBipartition::updateState() {
  if (_state == State::SwitchedOff) {
    logDebug("updateState()", "don't update as we have switched off");
  } else if (areRanksUnemployed()) {
    _state = State::InterRankDistribution;
    logDebug("updateState()", "switch to state " << ::toString(_state));
  } else if (isInterRankBalancingBad() and _roundRobinToken == tarch::mpi::Rank::getInstance().getRank()) {
    _state = State::InterRankBalancing;
    logDebug("updateState()", "switch to state " << ::toString(_state));
  } else if (isIntraRankBalancingBad()) {
    _state = State::IntraRankBalancing;
    logDebug("updateState()", "switch to state " << ::toString(_state));
  } else {
    _state = State::WaitForRoundRobinToken;

    _roundRobinToken++;
    _roundRobinToken = _roundRobinToken % tarch::mpi::Rank::getInstance().getNumberOfRanks();
  }
}


void toolbox::loadbalancing::strategies::RecursiveBipartition::updateLoadBalancing() {
#if PeanoDebug > 0
  logInfo("updateLoadBalancing()", "load balancing's state=" << toString());
#endif

  switch (_state) {
  case State::InterRankDistribution: {
    int heaviestSpacetree = getIdOfHeaviestLocalSpacetree(_configuration->getWorstCaseBalancingRatio(_state));
    int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
    if (
          fitsIntoMemory(_state)
          and
          heaviestSpacetree!=NoHeaviestTreeAvailable
          and
          not _blacklist.isBlacklisted(heaviestSpacetree)
          and
          numberOfLocalUnrefinedCellsOfHeaviestSpacetree > _configuration->getMinTreeSize(_state)
          and
          _costMetrics->getLightestRank()!=tarch::mpi::Rank::getInstance().getRank()
        ) {
      int cellsPerCore = std::max(
        {1, numberOfLocalUnrefinedCellsOfHeaviestSpacetree / 2, _configuration->getMinTreeSize(_state)}
      );

      logInfo(
        "updateLoadBalancing()",
        "Not all ranks are used. Lightest global rank is rank " << _costMetrics->getLightestRank(
        ) << ", so assign this rank " << cellsPerCore << " cell(s)"
      );
      triggerSplit(heaviestSpacetree, cellsPerCore, _costMetrics->getLightestRank());
    } else if (heaviestSpacetree != NoHeaviestTreeAvailable) {
      logInfo(
        "updateLoadBalancing()",
        "can't split heaviest tree " << heaviestSpacetree << " even though this tree is too heavy"
      );
    }
  } break;
  case State::InterRankBalancing: {
    int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
    int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
    if (
          fitsIntoMemory(_state)
          and
          heaviestSpacetree!=NoHeaviestTreeAvailable
          and
          not _blacklist.isBlacklisted(heaviestSpacetree)
          and
          numberOfLocalUnrefinedCellsOfHeaviestSpacetree > _configuration->getMinTreeSize(_state)
        ) {
      logInfo(
        "updateLoadBalancing()",
        "biggest local tree "
          << heaviestSpacetree << " is too heavy as it hosts " << numberOfLocalUnrefinedCellsOfHeaviestSpacetree
          << " cells"
      );
      int cellsPerCore = std::max(
        {1, numberOfLocalUnrefinedCellsOfHeaviestSpacetree / 2, _configuration->getMinTreeSize(_state)}
      );

      logInfo(
        "updateLoadBalancing()",
        "lightest global rank is rank "
          << _costMetrics->getLightestRank() << ", so assign this rank " << cellsPerCore << " cell(s)"
      );
      triggerSplit(heaviestSpacetree, cellsPerCore, _costMetrics->getLightestRank());
    } else if (heaviestSpacetree != NoHeaviestTreeAvailable) {
      logInfo(
        "updateLoadBalancing()",
        "can't split heaviest tree "
          << heaviestSpacetree
          << " and fork off to remote rank as this tree is on the blacklist or split does not fit into local memory anymore"
      );
    }
  } break;
  case State::IntraRankBalancing: {
    int heaviestSpacetree                              = getIdOfHeaviestLocalSpacetree();
    int numberOfLocalUnrefinedCellsOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
    if (
          fitsIntoMemory(_state)
          and
          heaviestSpacetree!=NoHeaviestTreeAvailable
          and
          not _blacklist.isBlacklisted(heaviestSpacetree)
          and
          numberOfLocalUnrefinedCellsOfHeaviestSpacetree > _configuration->getMinTreeSize(_state)
        ) {
      int cellsPerCore = std::max(
        {1, numberOfLocalUnrefinedCellsOfHeaviestSpacetree / 2, _configuration->getMinTreeSize(_state)}
      );
      logInfo(
        "updateLoadBalancing()", "split tree on local rank and assign it " << cellsPerCore << " cell(s)" << toString()
      );
      triggerSplit(heaviestSpacetree, cellsPerCore, tarch::mpi::Rank::getInstance().getRank());
    } else if (heaviestSpacetree != NoHeaviestTreeAvailable) {
      logInfo(
        "updateLoadBalancing()",
        "can't split heaviest tree "
          << heaviestSpacetree
          << " on local rank as this tree is on the blacklist or split does not fit into local memory anymore"
      );
    }
  } break;
  default:
    break;
  }
}


void toolbox::loadbalancing::strategies::RecursiveBipartition::finishStep() {
  _statistics.updateGlobalView();
  _costMetrics->updateGlobalView();
  _blacklist.update();

  updateLoadBalancing();
  updateState();

  _statistics.notifyOfStateChange(_state);

  dumpStatistics();
}


void toolbox::loadbalancing::strategies::RecursiveBipartition::triggerSplit(
  int sourceTree, int numberOfCells, int targetRank
) {
  bool success = peano4::parallel::SpacetreeSet::getInstance().split(
    sourceTree,
    peano4::SplitInstruction{numberOfCells, _configuration->getMode(_state) },
    targetRank
  );
  if (not success) {
    logInfo("triggerSplit()", "wanted to split local rank " << sourceTree << " but failed");
  }

  _blacklist.triggeredSplit(sourceTree);
  _statistics.incLocalNumberOfSplits();
}
