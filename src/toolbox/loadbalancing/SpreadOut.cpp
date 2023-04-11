#include "SpreadOut.h"

tarch::logging::Log  toolbox::loadbalancing::SpreadOut::_log( "toolbox::loadbalancing::SpreadOut" );


std::string toolbox::loadbalancing::SpreadOut::toString( Action action ) {
  switch ( action ) {
    case Action::None:
      return "none";
    case Action::SpreadEquallyOverAllRanks:
      return "spread-equally-over-all-ranks";
    case Action::SplitHeaviestLocalTreeMultipleTimes_UseLocalRank_UseRecursivePartitioning:
      return "split-heaviest-tree(multiple-times,use-local-rank,use-recursive-partitioning)";
    case Action::SplitHeaviestLocalTreeOnce_UseAllRanks_UseRecursivePartitioning:
      return "split-heaviest-tree(once,use-all-ranks,use-recursive-partitioning)";
    case Action::SplitHeaviestLocalTreeOnce_DontUseLocalRank_UseRecursivePartitioning:
      return "split-heaviest-tree(once,dont-use-local-rank,use-recursive-partitioning)";
    case Action::SplitHeaviestLocalTreeOnce_UseLocalRank_UseRecursivePartitioning:
      return "split-heaviest-tree(once,use-local-rank,use-recursive-partitioning)";
    case Action::Unspecified:
      return "unspecified";
  }
  return "<undef>";
}


toolbox::loadbalancing::SpreadOut::SpreadOut(Configuration* configuration):
  LoadBalancing( configuration ),
  _stepsToWaitForNextLoadBalancingDecision(0) {
  _state = State::InterRankDistribution;
}


toolbox::loadbalancing::SpreadOut::~SpreadOut() {
}
