// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "State.h"
#include "peano4/grid/SplitInstruction.h"

namespace toolbox::loadbalancing {
  class Configuration;
  class DefaultConfiguration;
} // namespace toolbox::loadbalancing

/**
 * Abstract interface to tweak the behaviour of the load balancer.
 */
class toolbox::loadbalancing::Configuration {
public:
  Configuration()          = default;
  virtual ~Configuration() = default;

  /**
   * Constraint on the number of trees per rank.
   */
  virtual int getMaxLocalTreesPerRank(State state) const = 0;

  /**
   * Control when to balance between ranks.
   */
  virtual double getWorstCaseBalancingRatio(State state) const = 0;

  virtual peano4::grid::SplitInstruction::Mode getMode(State state) const = 0;

  /**
   * Minimum tree size.
   */
  virtual int getMinTreeSize(State state) const = 0;
};

class toolbox::loadbalancing::DefaultConfiguration final: public toolbox::loadbalancing::Configuration {
public:
  DefaultConfiguration()           = default;
  ~DefaultConfiguration() override = default;

  int                                  getMaxLocalTreesPerRank(State state) const override;
  double                               getWorstCaseBalancingRatio(State state) const override;
  int                                  getMinTreeSize(State state) const override;
  peano4::grid::SplitInstruction::Mode getMode(State state) const override;
};
