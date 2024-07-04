#include "../strategies/SplitOversizedTree.h"

#include "tarch/Assertions.h"
#include "tarch/tarch.h"
#include "peano4/grid/GridStatistics.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "peano4/utils/Globals.h"
#include "tarch/multicore/Core.h"
#include "toolbox/loadbalancing/loadbalancing.h"


tarch::logging::Log toolbox::loadbalancing::strategies::SplitOversizedTree::_log(
  "toolbox::loadbalancing::strategies::SplitOversizedTree"
);


toolbox::loadbalancing::strategies::SplitOversizedTree::SplitOversizedTree(
  Configuration* configuration, CostMetrics* costMetrics
):
  AbstractLoadBalancing(configuration, costMetrics),
  _roundRobinToken(0) {
  assertion(configuration != nullptr);
  assertion(costMetrics != nullptr);
  _state = State::InterRankDistribution;
  _statistics.notifyOfStateChange(_state);
}


toolbox::loadbalancing::strategies::SplitOversizedTree::~SplitOversizedTree() {}


std::string toolbox::loadbalancing::strategies::SplitOversizedTree::toString() const {
  std::ostringstream msg;

  msg
    << "(type=split-oversized-tree"
    << ",state=" << ::toString(_state) << ",statistics=" << _statistics.toString() << ",cost-metrics="
    << _costMetrics->toString() << ",round-robin-token=" << _roundRobinToken << ",blacklist=" << _blacklist.toString()
    << ",heaviest-local-tree=" << getIdOfHeaviestLocalSpacetree() << " (analysed)"
    << ",heaviest-local-weight=" << getWeightOfHeaviestLocalSpacetree() << " (analysed)"
    << ",#local-trees=" << peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()
    << ",is-inter-rank-balancing-bad=" << isInterRankBalancingBad()
    << ",is-intra-rank-balancing-bad=" << isIntraRankBalancingBad() << ",target-tree-cost=" << getTargetTreeCost()
    << ",min-tree-size=" << _configuration->getMinTreeSize(_state) << ")";

  return msg.str();
}


void toolbox::loadbalancing::strategies::SplitOversizedTree::updateState() {
  if (_state == State::SwitchedOff) {
    logDebug("updateState()", "don't update as we have switched off");
  } else if (areRanksUnemployed()) {
    _state = State::InterRankDistribution;
    logDebug("updateState()", "switch to state " << ::toString(_state));
  } else if (isInterRankBalancingBad() and _roundRobinToken == tarch::mpi::Rank::getInstance().getRank() and doesLocalTreeViolateThreshold()) {
    _state = State::InterRankBalancing;
    logDebug("updateState()", "switch to state " << ::toString(_state));
  } else if (isIntraRankBalancingBad() and doesLocalTreeViolateThreshold()) {
    _state = State::IntraRankBalancing;
    logDebug("updateState()", "switch to state " << ::toString(_state));
  } else {
    _state = State::WaitForRoundRobinToken;

    _roundRobinToken++;
    _roundRobinToken = _roundRobinToken % tarch::mpi::Rank::getInstance().getNumberOfRanks();
  }
}


bool toolbox::loadbalancing::strategies::SplitOversizedTree::doesLocalTreeViolateThreshold() const {
  if (not _statistics.hasConsistentViewOfWorld()) { //
    return false;
  } else if (peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() == 0) {
    return false;
  } else if (peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() == 1) {
    return true;
  }
  else if ( static_cast<int>(peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()) >= _configuration->getMaxLocalTreesPerRank(_state) ) {
    return false;
  } else {
    bool result = false;

    for (auto p : peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees()) {
      result |= _costMetrics->getCostOfLocalTree(p) > getTargetTreeCost();
    }

    return result;
  }
}


void toolbox::loadbalancing::strategies::SplitOversizedTree::updateLoadBalancing() {
#if PeanoDebug > 0
  logInfo("updateLoadBalancing()", "load balancing's state=" << toString());
#endif

  switch (_state) {
  case State::InterRankDistribution: {
    int    heaviestSpacetree = getIdOfHeaviestLocalSpacetree(_configuration->getWorstCaseBalancingRatio(_state));
    double weightOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
    if (
          fitsIntoMemory(_state)
          and
          heaviestSpacetree!=NoHeaviestTreeAvailable
          and
          not _blacklist.isBlacklisted(heaviestSpacetree)
          and
          weightOfHeaviestSpacetree > getTargetTreeCost()
          and
          _costMetrics->getLightestRank()!=tarch::mpi::Rank::getInstance().getRank()
          and
          computeNumberOfSplits(heaviestSpacetree)>0
        ) {
      logInfo(
        "updateLoadBalancing()",
        "Not all ranks are used. Split once and deploy to lightest global rank " << _costMetrics->getLightestRank()
      );
      triggerSplit(heaviestSpacetree, _costMetrics->getLightestRank(), 1);
    } else if (heaviestSpacetree != NoHeaviestTreeAvailable) {
      logInfo(
        "updateLoadBalancing()",
        "can't split heaviest tree " << heaviestSpacetree << " even though this tree is too heavy " << toString()
      );
    }
  } break;
  case State::InterRankBalancing: {
    int heaviestSpacetree         = getIdOfHeaviestLocalSpacetree();
    int weightOfHeaviestSpacetree = getWeightOfHeaviestLocalSpacetree();
    if (
      fitsIntoMemory(_state)
      and
      heaviestSpacetree != NoHeaviestTreeAvailable
      and
      not _blacklist.isBlacklisted(heaviestSpacetree)
      and
      weightOfHeaviestSpacetree > getTargetTreeCost()
      and
      computeNumberOfSplits(heaviestSpacetree) > 0
    ) {
      logInfo(
        "updateLoadBalancing()",
        "biggest local tree "
          << heaviestSpacetree << " is too heavy as it hosts " << weightOfHeaviestSpacetree
          << " cells. Try to split tree " << computeNumberOfSplits(heaviestSpacetree)
          << " times and to offload subsection(s) of tree to " << _costMetrics->getLightestRank() << " " << toString()
      );
      triggerSplit(heaviestSpacetree, _costMetrics->getLightestRank(), computeNumberOfSplits(heaviestSpacetree));
    } else if (heaviestSpacetree != NoHeaviestTreeAvailable) {
      logInfo(
        "updateLoadBalancing()",
        "can't split heaviest tree "
          << heaviestSpacetree << " and fork off to remote rank,as "
          << "heaviest tree is on blacklist " << toString()
      );
    }
  } break;
  case State::IntraRankBalancing: {
    for (auto p : peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees()) {
      if (
        fitsIntoMemory(_state)
        and
        not _blacklist.isBlacklisted(p)
        and
        _costMetrics->getCostOfLocalTree(p) > getTargetTreeCost()
        and
        computeNumberOfSplits(p) > 0
      ) {
        logInfo(
          "updateLoadBalancing()",
          "local tree "
            << p << " is too heavy as it has a weight of " << _costMetrics->getCostOfLocalTree(p)
            << ". Try to split tree " << computeNumberOfSplits(p) << " times on local rank " << toString()
            << " (max local trees=" << _configuration->getMaxLocalTreesPerRank(_state)
            << ", min-tree-size=" << _configuration->getMinTreeSize(_state) << ")"
        );
        triggerSplit(p, tarch::mpi::Rank::getInstance().getRank(), computeNumberOfSplits(p));
      }
    }
  } break;
  default:
    break;
  }
}


void toolbox::loadbalancing::strategies::SplitOversizedTree::finishStep() {
  _statistics.updateGlobalView();
  _costMetrics->updateGlobalView();
  _blacklist.update();

  updateLoadBalancing();
  updateState();

  _statistics.notifyOfStateChange(_state);

  dumpStatistics();
}


int toolbox::loadbalancing::strategies::SplitOversizedTree::computeNumberOfSplits(int sourceTree) const {
  int numberOfSplits = static_cast<int>(_costMetrics->getCostOfLocalTree(sourceTree) / getTargetTreeCost()) - 1;

  numberOfSplits = std::max(1, numberOfSplits);
  numberOfSplits = std::min(
    numberOfSplits,
    _configuration->getMaxLocalTreesPerRank(_state)
    -
    static_cast<int>(peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size())
  );

  int currentSourceTreeCells = peano4::parallel::SpacetreeSet::getInstance()
                                 .getGridStatistics(sourceTree)
                                 .getNumberOfLocalUnrefinedCells();

  while (
    currentSourceTreeCells / (numberOfSplits + 1) < _configuration->getMinTreeSize(_state)
    and
    numberOfSplits > 0
  ) {
    numberOfSplits--;
  }

  return numberOfSplits;
}


void toolbox::loadbalancing::strategies::SplitOversizedTree::triggerSplit(
  int sourceTree, int targetRank, int numberOfSplits
) {
  const int numberOfSplitCells = std::max(
    1,
    peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(sourceTree).getNumberOfLocalUnrefinedCells()
      / (1 + numberOfSplits)
  );
  for (int i = 0; i < numberOfSplits; i++) {
    bool success = peano4::parallel::SpacetreeSet::getInstance().split(
      sourceTree,
      peano4::SplitInstruction{numberOfSplitCells, _configuration->getMode(_state)},
      targetRank
    );
    if (not success) {
      logInfo("triggerSplit()", "wanted to split local rank " << sourceTree << " but failed");
    }

    _blacklist.triggeredSplit(sourceTree);
    _statistics.incLocalNumberOfSplits();
  }
}


double toolbox::loadbalancing::strategies::SplitOversizedTree::getTargetTreeCost() const {
  const int TargetNumberOfThreads = std::min(
    {tarch::multicore::Core::getInstance().getNumberOfThreads(),
     _configuration->getMaxLocalTreesPerRank(_state),
     static_cast<int>(peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size())}
  );

  return _costMetrics->getGlobalCost() / tarch::mpi::Rank::getInstance().getNumberOfRanks() / TargetNumberOfThreads;
}
