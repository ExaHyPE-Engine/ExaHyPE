// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RunControl.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

peano4::parallel::RunControl peano4::parallel::RunControl::_singleton;

peano4::parallel::RunControl& peano4::parallel::RunControl::getInstance() { return _singleton; }

/**
 * Distribute the next program step and any non-critical assertion
 *
 * Both values travel in one maximum reduction. The global master alone decides the
 * program step, so every other rank contributes a value below every legal step and
 * the maximum is the master's choice. A non-critical assertion travels the opposite
 * way: any rank may have raised one, and the maximum hands the highest such rank to
 * everybody, so each rank enters the sweep knowing that the run has to wind down.
 */
bool peano4::parallel::RunControl::continueToRun() {
  constexpr int NoProgramStepDecision = Terminate - 1;

  const auto&              rank = tarch::mpi::Rank::getInstance();
  const std::array<int, 2> localControl{
    rank.isGlobalMaster() ? _currentProgramStep : NoProgramStepDecision,
    tarch::getNonCriticalAssertionRank()
  };
  std::array<int, 2> globalControl{NoProgramStepDecision, -1};

  tarch::mpi::allreduce(localControl.data(), globalControl.data(), 2, MPI_INT, MPI_MAX, rank.getCommunicator());

  _currentProgramStep = globalControl[0];
  tarch::adoptNonCriticalAssertionRank(globalControl[1]);
  logDebug("Program step is " << _currentProgramStep);

  return _currentProgramStep != Terminate;
}

void peano4::parallel::RunControl::setNextProgramStep(const int number) {
  assertion(number == Terminate or number >= 0, number);
  _currentProgramStep = number;
}

int peano4::parallel::RunControl::getCurrentProgramStep() const { return _currentProgramStep; }
