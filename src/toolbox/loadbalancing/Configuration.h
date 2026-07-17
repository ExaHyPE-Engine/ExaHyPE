// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/SplitInstruction.h"
#include "toolbox/loadbalancing/State.h"

namespace toolbox::loadbalancing {
  class Configuration;
  class DefaultConfiguration;
} // namespace toolbox::loadbalancing

class toolbox::loadbalancing::Configuration {
public:
  Configuration()                                = default;
  Configuration(const Configuration&)            = default;
  Configuration(Configuration&&)                 = default;
  Configuration& operator=(const Configuration&) = default;
  Configuration& operator=(Configuration&&)      = default;
  virtual ~Configuration()                       = default;

  virtual int    getMaxLocalTreesPerRank(toolbox::loadbalancing::State state) const               = 0;
  virtual double getWorstCaseBalancingRatio(toolbox::loadbalancing::State state) const            = 0;
  virtual peano4::grid::SplitInstruction::Mode getMode(toolbox::loadbalancing::State state) const = 0;
};

class toolbox::loadbalancing::DefaultConfiguration final : public toolbox::loadbalancing::Configuration {
public:
  int                                  getMaxLocalTreesPerRank(toolbox::loadbalancing::State state) const override;
  double                               getWorstCaseBalancingRatio(toolbox::loadbalancing::State state) const override;
  peano4::grid::SplitInstruction::Mode getMode(toolbox::loadbalancing::State state) const override;
};
