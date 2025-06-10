// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "State.h"

#include "peano4/grid/grid.h"

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
  virtual int getMaxLocalTreesPerRank(State state) = 0;

  /**
   * Control when to balance between ranks.
   */
  virtual double getWorstCaseBalancingRatio(State state) = 0;

  virtual peano4::SplitInstruction::Mode getMode(State state) = 0;

  /**
   * Minimum tree size.
   */
  virtual int getMinTreeSize(State state) = 0;
};

class toolbox::loadbalancing::DefaultConfiguration: public toolbox::loadbalancing::Configuration {
public:
  DefaultConfiguration() = default;
  virtual ~DefaultConfiguration() override = default;

  virtual int                            getMaxLocalTreesPerRank(State state) override;
  virtual double                         getWorstCaseBalancingRatio(State state) override;
  virtual int                            getMinTreeSize(State state) override;
  virtual peano4::SplitInstruction::Mode getMode(State state) override;
};
