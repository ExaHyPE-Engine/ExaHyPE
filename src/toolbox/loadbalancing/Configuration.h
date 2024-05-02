// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "State.h"
#include "peano4/grid/grid.h"


namespace toolbox {
  namespace loadbalancing {
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

    virtual peano4::SplitInstruction::Mode getMode(State state) = 0;

    /**
     * Minimum tree size.
     */
    virtual int getMinTreeSize(State state) = 0;

    virtual std::string toString() const;
};


class toolbox::loadbalancing::DefaultConfiguration: public toolbox::loadbalancing::Configuration {
  private:
    const int _maxTreesPerRank;
  public:
    /**
     * Create default configuration
     *
     * If you don't specify how many trees you allow per rank, then the code
     * will try to maximise the utilisation.
     */
    DefaultConfiguration(int maxTreesPerRank=0);

    virtual bool makeSplitDependOnMemory(State state) override;

    virtual int getMaxLocalTreesPerRank(State state) override;

    virtual double getWorstCaseBalancingRatio(State state) override;

    virtual int getMinTreeSize(State state) override;

    virtual peano4::SplitInstruction::Mode getMode(State state) override;
};
