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
  const double                               _loadBalancingQuality;
  const int                                  _minSizeOfTree;
  const int                                  _maxNumberOfTrees;
  const peano4::grid::SplitInstruction::Mode _mode;

  int translateSetMaxNumberOfTreesIntoRealNumberOfTrees(int value) const;

public:
  /**
   * Configure load balancing
   */
  LoadBalancingConfiguration(
    double                               loadBalancingQuality = 0.9,
    int                                  minSizeOfTree        = 1,
    int                                  maxNumberOfTrees     = -1,
    peano4::grid::SplitInstruction::Mode mode                 = peano4::grid::SplitInstruction::Mode::BottomUp
  );

  virtual ~LoadBalancingConfiguration() override = default;

  virtual int                                  getMaxLocalTreesPerRank(toolbox::loadbalancing::State state) override;
  virtual double                               getWorstCaseBalancingRatio(toolbox::loadbalancing::State state) override;
  virtual int                                  getMinTreeSize(toolbox::loadbalancing::State state) override;
  virtual peano4::grid::SplitInstruction::Mode getMode(toolbox::loadbalancing::State state) override;
};
