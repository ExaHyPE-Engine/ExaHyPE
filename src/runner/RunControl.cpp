// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RunControl.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

#include <mpi.h>

runner::RunControl runner::RunControl::_singleton;

runner::RunControl& runner::RunControl::getInstance() { return _singleton; }

/**
 * Distributes the next program step and any non-critical assertion in one
 * maximum reduction. Every rank but the global master contributes a program
 * step below every legal one, so the maximum is the master's choice. Any rank
 * may raise a non-critical assertion, and the maximum hands the highest such
 * rank to every rank.
 */
bool runner::RunControl::continueToRun() {
  constexpr int NoProgramStepDecision = Terminate - 1;

  const auto&              rank = tarch::mpi::Rank::getInstance();
  const std::array<int, 2> localControl{
    rank.isGlobalMaster() ? _currentProgramStep : NoProgramStepDecision,
    tarch::getNonCriticalAssertionRank()
  };
  std::array<int, 2> globalControl{};

  tarch::mpi::allreduce(localControl.data(), globalControl.data(), 2, MPI_INT, MPI_MAX, rank.getCommunicator());

  _currentProgramStep = globalControl[0];
  tarch::adoptNonCriticalAssertionRank(globalControl[1]);
  logDebug("Program step is " << _currentProgramStep);

  return _currentProgramStep != Terminate;
}

/**
 * Chooses the program step that continueToRun() distributes next. Only the
 * global master's choice reaches the other ranks.
 */
void runner::RunControl::setNextProgramStep(const int programStep) {
  assertion(programStep == Terminate or programStep >= 0, programStep);
  _currentProgramStep = programStep;
}

int runner::RunControl::getCurrentProgramStep() const { return _currentProgramStep; }
