// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Engine.h"

#include "peano4/parallel/Node.h"
#include "runner/Instrumentation.h"
#include "runner/LoadBalancer.h"
#include "runner/Orchestrator.h"
#include "runner/Phase.h"
#include "runner/PhaseNames.h"
#include "runner/RuntimeParameters.h"
#include "tarch/FloatingPointExceptions.h"
#include "tarch/instrumentation/Watch.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"

namespace {
  void recordStepTime(
    std::vector<std::pair<std::string, tarch::instrumentation::Measurement>>& stepTimes,
    const std::string&                                                        name,
    double                                                                    elapsedSeconds
  ) {
    for (auto& [stepName, measurement] : stepTimes) {
      if (stepName == name) {
        measurement.setValue(elapsedSeconds);
        return;
      }
    }
    stepTimes.emplace_back(name, tarch::instrumentation::Measurement());
    stepTimes.back().second.setValue(elapsedSeconds);
  }
} // namespace

runner::Engine::Engine(
  tarch::la::Vector<DIMENSIONS, double> domainOffset,
  tarch::la::Vector<DIMENSIONS, double> domainSize,
  std::bitset<DIMENSIONS>               periodicBC,
  RepositoryCallbacks                   callbacks,
  std::vector<Step>                     steps
) :
  _domainOffset(domainOffset),
  _domainSize(domainSize),
  _periodicBC(periodicBC),
  _callbacks(std::move(callbacks)),
  _steps(std::move(steps)),
  _measurements(),
  _box() {}

const runner::Step& runner::Engine::findStep(int programStep) const {
  for (const auto& step : _steps) {
    if (step.programStep == programStep) {
      return step;
    }
  }
  throw std::runtime_error(
    "runner::Engine: no step registered for Peano program step " + std::to_string(programStep) + "."
  );
}

void runner::Engine::executeStepBody(int programStep) const {
  tarch::FloatingPointExceptions::ScopedTrap scopedTrap(__FILE__, __LINE__);
  findStep(programStep).body();
}

double runner::Engine::runStep(int programStep) {
  // Measure the whole step (broadcast, traversal, load-balancer update) so the global collectives at
  // both ends are counted.
  tarch::instrumentation::Watch watch("runner::Engine", "runStep", false, false);
  watch.start();

  peano4::parallel::Node::getInstance().setNextProgramStep(programStep);
  peano4::parallel::Node::getInstance().continueToRun();

  executeStepBody(programStep);

  // Refresh the load balancer after the step so the next decision sees current statistics.
  {
    // The load balancer's own post-sweep decision refresh, distinct from the LoadBalancing program
    // step (which is a grid sweep). Hence a separate region name.
    SCOREP_USER_REGION(PhaseName::LoadBalancerUpdate, SCOREP_USER_REGION_TYPE_COMMON)
    loadBalancer().update();
  }
  if (_callbacks.afterStep) {
    _callbacks.afterStep();
  }

  watch.stop();
  const double elapsed = watch.getCalendarTime();
  recordStepTime(_measurements.stepTimes, findStep(programStep).name, elapsed);
  return elapsed;
}

void runner::Engine::runWorker() const {
  while (peano4::parallel::Node::getInstance().continueToRun()) {
    executeStepBody(peano4::parallel::Node::getInstance().getCurrentProgramStep());
    {
      // The load balancer's own post-sweep decision refresh, distinct from the LoadBalancing program
      // step.
      SCOREP_USER_REGION(PhaseName::LoadBalancerUpdate, SCOREP_USER_REGION_TYPE_COMMON)
      loadBalancer().update();
    }
    if (_callbacks.afterStep) {
      _callbacks.afterStep();
    }
  }
}

void runner::Engine::assertConsistentStepRegistryAcrossRanks() const {
#ifdef PEANO_WITH_MPI
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() <= 1) {
    return;
  }

  unsigned long long localHash = 1469598103934665603ULL; // FNV-1a offset basis
  const auto         mix       = [&localHash](unsigned long long value) {
    localHash ^= value;
    localHash *= 1099511628211ULL; // FNV-1a prime
  };
  for (const auto& step : _steps) {
    mix(static_cast<unsigned long long>(step.programStep));
    for (const char character : step.name) {
      mix(static_cast<unsigned long long>(static_cast<unsigned char>(character)));
    }
    mix(0xffffffffULL);
  }

  unsigned long long minHash      = 0;
  unsigned long long maxHash      = 0;
  const auto         communicator = tarch::mpi::Rank::getInstance().getCommunicator();
  MPI_Allreduce(&localHash, &minHash, 1, MPI_UNSIGNED_LONG_LONG, MPI_MIN, communicator);
  MPI_Allreduce(&localHash, &maxHash, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, communicator);

  if (minHash != maxHash) {
    throw std::runtime_error(
      "runner::Engine: the registered step set differs across MPI ranks. Every rank must register "
      "identical steps (same ids and names) so that a broadcast program step resolves everywhere."
    );
  }
#endif
}

int runner::Engine::run() {
  assertConsistentStepRegistryAcrossRanks();

  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    runWorker();
    return EXIT_SUCCESS;
  }

  PhaseContext context{
    *this,
    _box,
    _callbacks,
    RuntimeParameters::getInstance(),
    _measurements,
    _domainOffset,
    _domainSize,
    _periodicBC
  };

  const bool terminatedGracefully = Orchestrator(_steps).run(context);

  peano4::parallel::Node::getInstance().setNextProgramStep(peano4::parallel::Node::Terminate);
  peano4::parallel::Node::getInstance().continueToRun(); // Send terminate instruction to other ranks

  return terminatedGracefully ? EXIT_FAILURE : EXIT_SUCCESS;
}
