// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/logging/Log.h"
#include "toolbox/loadbalancing/Configuration.h"


namespace exahype2 {
  class LoadBalancingConfiguration;
}


/**
 * ExaHyPE 2-specific load balancing configuration
 *
 */
class exahype2::LoadBalancingConfiguration: public toolbox::loadbalancing::Configuration {
  private:
    static tarch::logging::Log  _log;

    const double   _loadBalancingQuality;
    const int      _minSizeOfTree;
    const bool     _assumePeriodicBoundaryConditions;
    const int      _maxNumberOfTreesThroughoutInitialDistribution;
    const int      _maxNumberOfTrees;
    const peano4::SplitInstruction::Mode _mode;

    /**
     * Return how many trees to use if value is picked by user
     *
     * If the users sets a real number >0 for the upper number of trees, then
     * we pick this value. However, if the users picks a magic constant, then
     * we have to read out the real system configuration and translate it into
     * a real number, i.e. if the user has picked UseNumberOfThreads for
     * example, then we have to ask the core how many threads we really have
     * and return that value.
     */
    int translateSetMaxNumberOfTreesIntoRealNumberOfTrees(int value) const;

  public:
    static constexpr int  UseNumberOfThreads         = -1;
    static constexpr int  UseTwiceTheNumberOfThreads = -2;

    /**
     * Configure load balancing
     *
     * @param minSizeOfTree If a partition (tree) is smaller than the given
     *   number of cells, we do not split it up further. If you set this
     *   argument to 0, you effectively set no threshold - Peano will try to
     *   split any tree if it thinks it would improve the load balancing.
     *
     *   This value is not taken into account for the initial distribution.
     *   Here, you have to use maxNumberOfTreesThroughoutInitialDistribution
     *   to constrain over-ambitious domain splitting.
     *
     * @param maxNumberOfTreesThroughoutInitialDistribution Set the maximum
     *   number of trees (per rank) that we use when we split up the domain
     *   no a rank for the first time. By default, I try to give each thread
     *   per rank exactly one subpartition to work on. You might want to
     *   reduce this value.
     *
     * @param maxNumberOfTrees This value is used after the initial
     *   decomposition. If you pick it higher than maxNumberOfTreesThroughoutInitialDistribution,
     *   then you give the load balancing the opportunity to split up the
     *   domain further even if all threads have already one subpartition.
     *   I recommend to to pick it higher than maxNumberOfTreesThroughoutInitialDistribution,
     *   as the load balancer kicks in throughout the grid construction. By
     *   the time is uses maxNumberOfTreesThroughoutInitialDistribution threads,
     *   local adaptivity might not yet be established, i.e. subsequent grid
     *   refinements will yield imbalances that only further splits can
     *   compensate.
     *
     */
    LoadBalancingConfiguration(
      double loadBalancingQuality=0.9,
      int    minSizeOfTree = 0,
      bool   assumePeriodicBoundaryConditions = false,
      int    maxNumberOfTreesThroughoutInitialDistribution=UseNumberOfThreads,
      int    maxNumberOfTrees=UseTwiceTheNumberOfThreads,
      peano4::SplitInstruction::Mode mode = peano4::SplitInstruction::Mode::BottomUp
    );

    virtual ~LoadBalancingConfiguration() = default;

    virtual bool makeSplitDependOnMemory(toolbox::loadbalancing::State state) override;

    virtual int getMaxLocalTreesPerRank(toolbox::loadbalancing::State state) override;

    virtual double getWorstCaseBalancingRatio(toolbox::loadbalancing::State state) override;

    /**
     * If we do the initial distribution in-between ranks, then there
     * should be no such thing as a min tree size.
     */
    virtual int getMinTreeSize(toolbox::loadbalancing::State state) override;

    virtual std::string toString() const override;

    virtual peano4::SplitInstruction::Mode getMode(toolbox::loadbalancing::State state) override;
};


