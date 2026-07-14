// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadBalancer.h"

#include "peano4/Globals.h"
#include "runner/RuntimeParameters.h"
#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"
#include "toolbox/loadbalancing/Configuration.h"

namespace {
  class RuntimeConfiguration final : public toolbox::loadbalancing::Configuration {
  public:
    int getMaxLocalTreesPerRank(toolbox::loadbalancing::State) const override {
      const int configured = runner::RuntimeParameters::getInstance().getMaxNumberOfTreesPerRank();
      return configured > 0 ? configured : tarch::multicore::Core::getInstance().getNumberOfThreads();
    }

    double getWorstCaseBalancingRatio(toolbox::loadbalancing::State) const override {
      return runner::RuntimeParameters::getInstance().getLoadBalancingQuality();
    }

    int getMinTreeSize(toolbox::loadbalancing::State) const override {
      return runner::RuntimeParameters::getInstance().getMinSizeOfTree();
    }

    peano4::grid::SplitInstruction::Mode getMode(toolbox::loadbalancing::State) const override {
      return peano4::grid::SplitInstruction::Mode::BottomUp;
    }
  };

  std::unique_ptr<toolbox::loadbalancing::LoadBalancer> theLoadBalancer;
} // namespace

void runner::initLoadBalancer() {
  theLoadBalancer = std::make_unique<toolbox::loadbalancing::LoadBalancer>(new RuntimeConfiguration());
}

toolbox::loadbalancing::LoadBalancer& runner::loadBalancer() {
  // initLoadBalancer() runs in runner::launch() before any step or statistics touches the
  // balancer, so the instance must exist by the time this is called.
  assertion(theLoadBalancer);
  return *theLoadBalancer;
}
