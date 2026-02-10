// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CCZ4SBH_FD4.h"
#include "SingleBlackHoleScenario.h"

#include "exahype2/RefinementControl.h"
#include "repositories/SolverRepository.h"

tarch::logging::Log benchmarks::exahype2::ccz4::CCZ4SBH_FD4::_log("benchmarks::exahype2::ccz4::CCZ4SBH_FD4");


benchmarks::exahype2::ccz4::CCZ4SBH_FD4::CCZ4SBH_FD4() {
  prepareTwoPunctures();
}


void benchmarks::exahype2::ccz4::CCZ4SBH_FD4::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& meshCellCentre,
  const tarch::la::Vector<Dimensions, double>& meshCellH,
  bool                                         gridIsConstructed
) {
  logTraceInWith3Arguments("initialCondition(...)", meshCellCentre, meshCellH, gridIsConstructed);

  applications::exahype2::ccz4::ApplyTwoPunctures(Q, meshCellCentre, 0, twoPunctures, not gridIsConstructed);

  logTraceOut("initialCondition(...)");
}

void benchmarks::exahype2::ccz4::CCZ4SBH_FD4::startTimeStep(
  double globalMinTimeStamp, double globalMaxTimeStamp, double globalMinTimeStepSize, double globalMaxTimeStepSize
) {
  AbstractCCZ4SBH_FD4::startTimeStep(
    globalMinTimeStamp, globalMaxTimeStamp, globalMinTimeStepSize, globalMaxTimeStepSize
  );

#if defined(CoupleWithFV)
  if (isFirstGridSweepOfTimeStep()) {
    double fvTimeStepSize  = repositories::instanceOfCCZ4SBH_FV.getAdmissibleTimeStepSize();
    double newTimeStepSize = std::min(fvTimeStepSize, _admissibleTimeStepSize);
    if (tarch::la::smaller(newTimeStepSize, fvTimeStepSize)) {
      logInfo(
        "startTimeStep(...)",
        "reduce admissible time step size from dt_{dg}="
          << _admissibleTimeStepSize << " vs dt_{fv}=" << fvTimeStepSize << " to dt=" << newTimeStepSize
          << " (previously reported time step sizes for this step are invalid)"
      );
    }
    _admissibleTimeStepSize = newTimeStepSize;
    repositories::instanceOfCCZ4SBH_FV.reduceAdmissibleTimeStepSize(newTimeStepSize);
  }
#endif
}
