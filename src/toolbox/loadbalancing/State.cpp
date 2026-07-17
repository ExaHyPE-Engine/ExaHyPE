// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "State.h"

std::ostream& operator<<(std::ostream& output, const toolbox::loadbalancing::State state) {
  switch (state) {
  case toolbox::loadbalancing::State::InitialInterRankSplit:
    return output << "InitialInterRankSplit";
  case toolbox::loadbalancing::State::InitialIntraRankSplit:
    return output << "InitialIntraRankSplit";
  case toolbox::loadbalancing::State::InterRankBalancing:
    return output << "InterRankBalancing";
  case toolbox::loadbalancing::State::IntraRankBalancing:
    return output << "IntraRankBalancing";
  case toolbox::loadbalancing::State::Stagnation:
    return output << "Stagnation";
  case toolbox::loadbalancing::State::SwitchedOff:
    return output << "SwitchedOff";
  }
  return output << "Undefined";
}
