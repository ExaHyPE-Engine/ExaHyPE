// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Engine.h"

#include "peano4/topology/TopologyManager.h"
#include "runner/GlobalStatistics.h"
#include "runner/Orchestrator.h"
#include "runner/Phase.h"
#include "runner/RunControl.h"
#include "runner/RuntimeParameters.h"
#include "tarch/FloatingPointExceptions.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Watch.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

#include <mpi.h>

namespace {
  void recordStepTime(
    std::vector<std::pair<std::string, tarch::instrumentation::Measurement>>& stepTimes,
    const std::string&                                                        name,
    const double                                                              elapsedSeconds
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

  void recordStepTaskSeconds(
    std::vector<std::pair<std::string, double>>& stepTaskSeconds,
    const std::string&                           name,
    const double                                 taskSeconds
  ) {
    for (auto& [stepName, accumulated] : stepTaskSeconds) {
      if (stepName == name) {
        accumulated += taskSeconds;
        return;
      }
    }
    stepTaskSeconds.emplace_back(name, taskSeconds);
  }
} // namespace

runner::Engine::Engine(
  tarch::la::Vector<DIMENSIONS, double> domainOffset,
  tarch::la::Vector<DIMENSIONS, double> domainSize,
  std::bitset<DIMENSIONS>               periodicBC,
  runner::RepositoryCallbacks           callbacks,
  std::vector<runner::Step>             steps
) :
  _domainOffset(domainOffset),
  _domainSize(domainSize),
  _periodicBC(periodicBC),
  _callbacks(std::move(callbacks)),
  _steps(std::move(steps)) {}

const runner::Step& runner::Engine::findStep(const int programStep) const {
  for (const auto& step : _steps) {
    if (step.programStep == programStep) {
      return step;
    }
  }
  tarch::terminateWithError("No step registered for Peano program step " + std::to_string(programStep) + ".");
}

void runner::Engine::executeStepBody(const int programStep) const {
  const tarch::FloatingPointExceptions::ScopedTrap scopedTrap(__FILE__, __LINE__);
  findStep(programStep).body();
}

/**
 * Broadcasts the step, executes it and reduces the global statistics. Only the
 * global master calls this. Returns the wall time in seconds of the broadcast,
 * the traversal, the reduction and the post-step callback.
 */
double runner::Engine::runStep(const int programStep) {
  tarch::instrumentation::Watch watch("runner::Engine", "runStep", false, false);
  watch.start();
  const auto taskSecondsBefore = peano4::topology::TopologyManager::getInstance().getAccumulatedTaskSeconds();

  runner::RunControl::getInstance().setNextProgramStep(programStep);
  runner::RunControl::getInstance().continueToRun();

  executeStepBody(programStep);

  runner::globalStatistics().synchronise();
  if (_callbacks.afterStep) {
    _callbacks.afterStep();
  }

  watch.stop();
  const auto  elapsed  = watch.getCalendarTime();
  const auto& stepName = findStep(programStep).name;
  recordStepTime(_measurements.stepTimes, stepName, elapsed);
  recordStepTaskSeconds(
    _measurements.stepTaskSeconds,
    stepName,
    peano4::topology::TopologyManager::getInstance().getAccumulatedTaskSeconds() - taskSecondsBefore
  );
  return elapsed;
}

void runner::Engine::runWorker() const {
  while (runner::RunControl::getInstance().continueToRun()) {
    const auto programStep = runner::RunControl::getInstance().getCurrentProgramStep();
    executeStepBody(programStep);
    runner::globalStatistics().synchronise();
    if (_callbacks.afterStep) {
      _callbacks.afterStep();
    }
  }
}

void runner::Engine::assertConsistentStepRegistryAcrossRanks() const {
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() <= 1) {
    return;
  }

  constexpr auto FnvOffsetBasis = 1469598103934665603ULL;
  constexpr auto FnvPrime       = 1099511628211ULL;
  constexpr auto StepSeparator  = 0xffffffffULL;

  unsigned long long localHash = FnvOffsetBasis;
  const auto         mix       = [&localHash](const unsigned long long value) {
    localHash ^= value;
    localHash *= FnvPrime;
  };
  for (const auto& step : _steps) {
    mix(static_cast<unsigned long long>(step.programStep));
    for (const char character : step.name) {
      mix(static_cast<unsigned long long>(static_cast<unsigned char>(character)));
    }
    mix(StepSeparator);
  }

  unsigned long long minHash      = 0;
  unsigned long long maxHash      = 0;
  const auto         communicator = tarch::mpi::Rank::getInstance().getCommunicator();
  tarch::mpi::allreduce(&localHash, &minHash, 1, MPI_UNSIGNED_LONG_LONG, MPI_MIN, communicator);
  tarch::mpi::allreduce(&localHash, &maxHash, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, communicator);

  if (minHash != maxHash) {
    tarch::terminateWithError(
      "The registered step set differs across MPI ranks. Every rank must register "
      "identical steps (same ids and names) so that a broadcast program step resolves everywhere."
    );
  }
}

int runner::Engine::run() {
  assertConsistentStepRegistryAcrossRanks();

  if (not tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    runWorker();
    return EXIT_SUCCESS;
  }

  runner::PhaseContext context{
    *this,
    _box,
    _callbacks,
    runner::RuntimeParameters::getInstance(),
    _measurements,
    _domainOffset,
    _domainSize,
    _periodicBC
  };

  const bool stoppedAfterNonCriticalAssertion = runner::Orchestrator(_steps).run(context);

  // Workers wait inside continueToRun(), so the master broadcasts a final termination step.
  runner::RunControl::getInstance().setNextProgramStep(runner::RunControl::Terminate);
  runner::RunControl::getInstance().continueToRun();

  return stoppedAfterNonCriticalAssertion ? EXIT_FAILURE : EXIT_SUCCESS;
}
