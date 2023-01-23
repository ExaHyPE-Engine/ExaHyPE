// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "toolbox/loadbalancing/RecursiveSubdivision.h"

namespace exahype2 {
  class LoadBalancingConfiguration;
}


/**
 * ExaHyPE 2-specific load balancing configuration
 *
 * We do not impose any constraint while we build up the mesh. Later on, we allow
 * users (via the constructor or Python interface, respectively) to restrict
 * both the size of the trees and the tree count.
 */
class exahype2::LoadBalancingConfiguration: public toolbox::loadbalancing::RecursiveSubdivision::Configuration {
  private:
    const double   _loadBalancingQuality;
    const int      _minSizeOfTree;
    const int      _maxNumberOfTreesThroughoutInitialDistribution;
    const int      _maxNumberOfTrees;
    const int      _maxSizeOfDeployedRemoteMPITree;
  public:
    static constexpr int  UseNumberOfThreads = -1;
    /**
     * @param minSizeOfTree If a partition (tree) is smaller than the given
     *   number of cells, we do not split it up further. If you set this
     *   argument to 0, you effectively set no threshold - Peano will try to
     *   split any tree if it thinks it would improve the load balancing.
     *
     * @param maxNumberOfTrees Set the maximum number of trees (per rank).
     *
     */
    LoadBalancingConfiguration(
      double loadBalancingQuality=0.9,
      int    minSizeOfTree = 0,
      int    maxNumberOfTreesThroughoutInitialDistribution=UseNumberOfThreads,
      int    maxNumberOfTrees=UseNumberOfThreads,
      int    maxSizeOfDeployedRemoteMPITree = std::numeric_limits<int>::max()
    );

    virtual ~LoadBalancingConfiguration() = default;

    bool makeSplitDependOnMemory(toolbox::loadbalancing::RecursiveSubdivision::State state) override;

    int getMaxLocalTreesPerRank(toolbox::loadbalancing::RecursiveSubdivision::State state) override;

    double getWorstCaseBalancingRatio(toolbox::loadbalancing::RecursiveSubdivision::State state) override;

    int getMinTreeSize(toolbox::loadbalancing::RecursiveSubdivision::State state) override;

    int getMaxTreeSize(toolbox::loadbalancing::RecursiveSubdivision::State state) override;
};


