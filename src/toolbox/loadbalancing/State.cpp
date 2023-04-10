#include "State.h"



std::string toString( toolbox::loadbalancing::State state ) {
  switch (state) {
    case toolbox::loadbalancing::State::Undefined:
      return "undefined";
    case toolbox::loadbalancing::State::InterRankDistribution:
      return "inter-rank-distribution";
    case toolbox::loadbalancing::State::IntraRankDistribution:
      return "intra-rank-distribution";
    case toolbox::loadbalancing::State::InterRankBalancing:
      return "inter-rank-balancing";
    case toolbox::loadbalancing::State::IntraRankBalancing:
      return "intra-rank-balancing";
    case toolbox::loadbalancing::State::WaitForRoundRobinToken:
      return "wait-for-round-robin";
    case toolbox::loadbalancing::State::Stagnation:
      return "stagnation";
    case toolbox::loadbalancing::State::SwitchedOff:
      return "switched-off";
  }
  return "<undef>";
}



