// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/loadbalancing/Configuration.h"

namespace exahype2 {
  class LoadBalancingConfiguration;
} // namespace exahype2

/**
 * ExaHyPE 2-specific load balancing configuration
 */
class exahype2::LoadBalancingConfiguration: public toolbox::loadbalancing::Configuration {
private:
  const double                         _loadBalancingQuality;
  const int                            _minSizeOfTree;
  const int                            _maxNumberOfTreesThroughoutInitialDistribution;
  const int                            _maxNumberOfTrees;
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
  static constexpr int UseNumberOfThreads         = -1;
  static constexpr int UseTwiceTheNumberOfThreads = -2;

  /**
   * Configure load balancing
   *
   * @param minSizeOfTree If a partition (tree) is smaller than the given
   *   number of cells, we do not split it up further. If you set this
   *   argument to 0, you effectively set no threshold - Peano will try to
   *   split any tree if it thinks it would improve the load balancing.
   *
   * @param maxNumberOfTreesThroughoutInitialDistribution Set the maximum
   *   number of trees (per rank) that we use when we split up the domain
   *   of a rank for the first time. By default, I try to give each thread
   *   per rank exactly one subpartition to work on. You might want to
   *   reduce this value.
   *
   * @param maxNumberOfTrees This value is used after the initial
   *   decomposition. If you pick it higher than maxNumberOfTreesThroughoutInitialDistribution,
   *   then you give the load balancing the opportunity to split up the
   *   domain further even if all threads have already one subpartition.
   */
  LoadBalancingConfiguration(
    double                         loadBalancingQuality                          = 0.9,
    int                            minSizeOfTree                                 = 1,
    int                            maxNumberOfTreesThroughoutInitialDistribution = UseNumberOfThreads,
    int                            maxNumberOfTrees                              = UseTwiceTheNumberOfThreads,
    peano4::SplitInstruction::Mode mode = peano4::SplitInstruction::Mode::BottomUp
  );

  virtual ~LoadBalancingConfiguration() override = default;

  virtual int                            getMaxLocalTreesPerRank(toolbox::loadbalancing::State state) override;
  virtual double                         getWorstCaseBalancingRatio(toolbox::loadbalancing::State state) override;
  virtual int                            getMinTreeSize(toolbox::loadbalancing::State state) override;
  virtual peano4::SplitInstruction::Mode getMode(toolbox::loadbalancing::State state) override;
};
