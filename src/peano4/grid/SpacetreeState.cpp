// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpacetreeState.h"

std::string peano4::grid::toString(SpacetreeState state) {
  switch (state) {
  case SpacetreeState::EmptyRun:
    return "EmptyRun";
  case SpacetreeState::NewRoot:
    return "NewRoot";
  case SpacetreeState::NewFromSplit:
    return "NewFromSplit";
  case SpacetreeState::Running:
    return "Running";
  case SpacetreeState::JoinTriggered:
    return "JoinTriggered";
  case SpacetreeState::Joining:
    return "Joining";
  case SpacetreeState::Joined:
    return "Joined";
  }
  return "<undef>";
}
