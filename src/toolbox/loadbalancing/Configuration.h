// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


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


    class Configuration;
    class DefaultConfiguration;
  }
}


/**
 * Abstract interface to tweak the behaviour of the recursive subdivision.
 *
 * The idea is that the recursive subdivision internally makes decisions what it
 * would do. These decisions then are tweaked/tailored via the configuration.
 *
 * My default implementations all are stateless, but you can always add a state
 * to the configuration and thus make it alter its answers depending on where
 * you are in your code.
 */
class toolbox::loadbalancing::Configuration {
  public:
    virtual ~Configuration() = default;

    /**
     * Use the operating system's memory queries to get the memory out and to
     * veto too many local splits. Each split creates some overhead, so it can
     * happen that we run out of memory.
     */
    virtual bool   makeSplitDependOnMemory(State state) = 0;

    /**
     * Constraint on the number of trees per rank. The number is always
     * constrained by peano4::parallel::Node::MaxSpacetreesPerRank, but you
     * can cut it down further.
     *
     * You can use an arbitary large value if you don't care bout a maximum
     * number of subpartitions per rank. I however do recommend that you
     * return peano4::parallel::Node::MaxSpacetreesPerRank.
     */
    virtual int    getMaxLocalTreesPerRank(State state) = 0;

    /**
     * Control when to balance between ranks.
     */
    virtual double getWorstCaseBalancingRatio(State state) = 0;

    /**
     * Minimum tree size.
     */
    virtual int getMinTreeSize(State state) = 0;
};



class toolbox::loadbalancing::DefaultConfiguration: public toolbox::loadbalancing::Configuration {
  bool makeSplitDependOnMemory(State state) override;

  int getMaxLocalTreesPerRank(State state) override;

  double getWorstCaseBalancingRatio(State state) override;

  int getMinTreeSize(State state);
};
