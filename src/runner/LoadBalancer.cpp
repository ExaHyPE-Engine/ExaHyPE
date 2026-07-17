// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadBalancer.h"

#include "peano4/grid/SplitInstruction.h"
#include "runner/RuntimeParameters.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/multicore/Core.h"
#include "toolbox/loadbalancing/Configuration.h"

namespace {
  /** Initial-split thresholds require the same trees-per-rank target on every MPI rank. */
  int getMaxNumberOfTreesPerRank() {
    const auto configuredNumberOfTreesPerRank = runner::RuntimeParameters::getInstance().getMaxNumberOfTreesPerRank();
    const auto maxNumberOfTreesPerRank        = configuredNumberOfTreesPerRank > 0
             ? configuredNumberOfTreesPerRank
             : tarch::multicore::Core::getInstance().getNumberOfThreads();
#ifdef PEANO_WITH_MPI
    if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
      auto minimumNumberOfTreesPerRank = maxNumberOfTreesPerRank;
      auto maximumNumberOfTreesPerRank = maxNumberOfTreesPerRank;
      tarch::mpi::allreduce(
        &maxNumberOfTreesPerRank,
        &minimumNumberOfTreesPerRank,
        1,
        MPI_INT,
        MPI_MIN,
        tarch::mpi::Rank::getInstance().getCommunicator()
      );
      tarch::mpi::allreduce(
        &maxNumberOfTreesPerRank,
        &maximumNumberOfTreesPerRank,
        1,
        MPI_INT,
        MPI_MAX,
        tarch::mpi::Rank::getInstance().getCommunicator()
      );
      if (minimumNumberOfTreesPerRank != maximumNumberOfTreesPerRank) {
        tarch::terminateWithError(
          "MPI ranks resolved different trees-per-rank targets (" + std::to_string(minimumNumberOfTreesPerRank) + " .. "
          + std::to_string(maximumNumberOfTreesPerRank) + "). Pass one common -trees value."
        );
      }
    }
#endif
    return maxNumberOfTreesPerRank;
  }
} // namespace

namespace runner {
  class RunnerLoadBalancingConfiguration;
} // namespace runner

class runner::RunnerLoadBalancingConfiguration final : public toolbox::loadbalancing::Configuration {
public:
  explicit RunnerLoadBalancingConfiguration(const int maxNumberOfTreesPerRank) :
    _maxNumberOfTreesPerRank(maxNumberOfTreesPerRank) {}

  int getMaxLocalTreesPerRank([[maybe_unused]] const toolbox::loadbalancing::State state) const override {
    return _maxNumberOfTreesPerRank;
  }

  double getWorstCaseBalancingRatio([[maybe_unused]] const toolbox::loadbalancing::State state) const override {
    return runner::RuntimeParameters::getInstance().getLoadBalancingQuality();
  }

  peano4::grid::SplitInstruction::Mode
  getMode([[maybe_unused]] const toolbox::loadbalancing::State state) const override {
    return peano4::grid::SplitInstruction::Mode::BottomUp;
  }

private:
  const int _maxNumberOfTreesPerRank;
};

namespace {
  std::unique_ptr<toolbox::loadbalancing::LoadBalancer> loadBalancerInstance;
} // namespace

void runner::initLoadBalancer() {
  loadBalancerInstance = std::make_unique<toolbox::loadbalancing::LoadBalancer>(
    std::make_unique<RunnerLoadBalancingConfiguration>(getMaxNumberOfTreesPerRank())
  );
}

toolbox::loadbalancing::LoadBalancer& runner::loadBalancer() {
  // The launcher creates the instance before any step or statistics access.
  assertion(loadBalancerInstance);
  return *loadBalancerInstance;
}
