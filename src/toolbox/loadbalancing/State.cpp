// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "State.h"

std::string toString(const toolbox::loadbalancing::State state) {
  switch (state) {
  case toolbox::loadbalancing::State::InterRankDistribution:
    return "InterRankDistribution";
  case toolbox::loadbalancing::State::IntraRankDistribution:
    return "IntraRankDistribution";
  case toolbox::loadbalancing::State::InterRankBalancing:
    return "InterRankBalancing";
  case toolbox::loadbalancing::State::IntraRankBalancing:
    return "IntraRankBalancing";
  case toolbox::loadbalancing::State::Stagnation:
    return "Stagnation";
  case toolbox::loadbalancing::State::SwitchedOff:
    return "SwitchedOff";
  }
  return "<undef>";
}

std::ostream& operator<<(std::ostream& out, const toolbox::loadbalancing::State state) {
  out << toString(state);
  return out;
}
