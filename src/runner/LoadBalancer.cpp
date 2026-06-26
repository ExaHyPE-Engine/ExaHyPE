// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadBalancer.h"

#include "peano4/Globals.h"
#include "runner/RuntimeParameters.h"
#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"
#include "toolbox/loadbalancing/Configuration.h"

#include <algorithm>
#include <memory>

namespace {
  class RuntimeConfiguration final : public toolbox::loadbalancing::Configuration {
  public:
    explicit RuntimeConfiguration(bool hasPeriodicBC) :
      _hasPeriodicBC(hasPeriodicBC) {}

    int getMaxLocalTreesPerRank(toolbox::loadbalancing::State) const override {
      const int configured = runner::RuntimeParameters::getInstance().getMaxNumberOfTreesPerRank();
      return configured > 0 ? configured : tarch::multicore::Core::getInstance().getNumberOfThreads();
    }

    double getWorstCaseBalancingRatio(toolbox::loadbalancing::State) const override {
      return runner::RuntimeParameters::getInstance().getLoadBalancingQuality();
    }

    int getMinTreeSize(toolbox::loadbalancing::State) const override {
      const int minSizeOfTree = runner::RuntimeParameters::getInstance().getMinSizeOfTree();
      return _hasPeriodicBC ? std::max(minSizeOfTree, static_cast<int>(THREE_POWER_D)) : minSizeOfTree;
    }

    peano4::grid::SplitInstruction::Mode getMode(toolbox::loadbalancing::State) const override {
      return peano4::grid::SplitInstruction::Mode::BottomUp;
    }

  private:
    bool _hasPeriodicBC;
  };

  std::unique_ptr<toolbox::loadbalancing::LoadBalancer> theLoadBalancer;
} // namespace

void runner::initLoadBalancer(bool hasPeriodicBoundaryConditions) {
  theLoadBalancer =
    std::make_unique<toolbox::loadbalancing::LoadBalancer>(new RuntimeConfiguration(hasPeriodicBoundaryConditions));
}

toolbox::loadbalancing::LoadBalancer& runner::loadBalancer() {
  // initLoadBalancer() runs in runner::run() before any step or statistics touches the
  // balancer, so the instance must exist by the time this is called.
  assertion(theLoadBalancer);
  return *theLoadBalancer;
}
