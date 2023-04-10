// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <string>


namespace toolbox {
  namespace loadbalancing {
    /**
     * State descriptor of load balancing
     *
     * Any load balancing will run through different states, aka "what will I
     * try to achieve next". Not every load balancing strategy runs through the
     * states in the same order or passes through all states, but these states
     * are kind of a superset of different phases.
     *
     * We have to define them here to give the configuration objects the
     * opportunity to make state-dependent decisions if and how to refine.
     */
    enum class State {
      Undefined,

      /**
       * Code has not yet spread out over all ranks but would like to do so
       * now.
       */
      InterRankDistribution,

      /**
       * Code has spread over all ranks, but it has not spread over all cores
       * yet, i.e. there's around one tree per rank and that's it w.r.t. the
       * global decomposition.
       */
      IntraRankDistribution,

      /**
       * We have completed the first two phases and try to balance between the
       * ranks now to meet the load balancing quality.
       */
      InterRankBalancing,

      /**
       * The code is satisfied with the load balancing quality between the
       * ranks and now spawns more partitions per rank to increase the per-rank
       * concurrency.
       */
      IntraRankBalancing,

      /**
       * You usually don't get this state when we query the configuration, i.e.
       * if the lb on a rank is in this state, then it will not ask the
       * configuration.
       */
      WaitForRoundRobinToken,

      /**
       * You usually don't get this state when we query the configuration, i.e.
       * if the lb on a rank is in this state, then it will not ask the
       * configuration.
       */
      Stagnation,

      /**
       * You usually don't get this state when we query the configuration, i.e.
       * if the lb on a rank is in this state, then it will not ask the
       * configuration.
       */
      SwitchedOff
    };
  }
}


std::string toString( toolbox::loadbalancing::State state);
