// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace toolbox::loadbalancing {
  /**
   * Selects the decomposition stage used by strategies and phase-dependent configuration. The initial stages
   * populate MPI ranks before local tree slots. The balancing stages are reserved for dynamic rebalancing;
   * Stagnation and SwitchedOff leave no active strategy.
   */
  enum class State {
    InitialInterRankSplit,
    InitialIntraRankSplit,
    InterRankBalancing,
    IntraRankBalancing,
    Stagnation,
    SwitchedOff
  };
} // namespace toolbox::loadbalancing

std::ostream& operator<<(std::ostream& output, const toolbox::loadbalancing::State state);
