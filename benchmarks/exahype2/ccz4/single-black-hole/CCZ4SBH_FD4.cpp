// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CCZ4SBH_FD4.h"

#include "exahype2/RefinementControl.h"
#include "repositories/SolverRepository.h"

tarch::logging::Log benchmarks::exahype2::ccz4::CCZ4SBH_FD4::_log("benchmarks::exahype2::ccz4::CCZ4SBH_FD4");

TP::TwoPunctures* benchmarks::exahype2::ccz4::twoPunctures = nullptr;

void benchmarks::exahype2::ccz4::prepareTwoPunctures() {
  if (twoPunctures == nullptr) {
    twoPunctures = new TP::TwoPunctures();

    // first we set the parameter. TODO:find a way to read parameter from python script
    // int swi=0;//0--single black hole, 1--BBH hoc, 2--BBH rotation, 3--GW150914

    twoPunctures->par_b             = 1.0;
    twoPunctures->center_offset[0]  = -1.0;
    twoPunctures->center_offset[1]  = 0.0;
    twoPunctures->center_offset[2]  = 0.0;
    twoPunctures->target_M_plus     = 1.0; // adm mass
    twoPunctures->par_P_plus[0]     = 0.0;
    twoPunctures->par_P_plus[1]     = 0.0;
    twoPunctures->par_P_plus[2]     = 0.0; // linear momentum
    twoPunctures->par_S_plus[0]     = 0.0;
    twoPunctures->par_S_plus[1]     = 0.0;
    twoPunctures->par_S_plus[2]     = 0.0; // spin
    twoPunctures->target_M_minus    = 0.0; // adm mass
    twoPunctures->par_P_minus[0]    = 0.0;
    twoPunctures->par_P_minus[1]    = 0.0;
    twoPunctures->par_P_minus[2]    = 0.0; // linear momentum
    twoPunctures->par_S_minus[0]    = 0.0;
    twoPunctures->par_S_minus[1]    = 0.0;
    twoPunctures->par_S_minus[2]    = 0.0;          // spin
    twoPunctures->grid_setup_method = "evaluation"; // evaluation or Taylor expansion
    twoPunctures->TP_epsilon        = 1e-6;

    twoPunctures->PrintParameters();

    // then solve the equation
    twoPunctures->Run();
  }
}

::exahype2::RefinementCommand benchmarks::exahype2::ccz4::CCZ4SBH_FD4::refinementCriterion(
  const double* __restrict__ Q, // Q[59+0]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& meshCellH,
  double                                       t
) {
  return tarch::la::norm2(volumeX) - std::sqrt(Dimensions)*meshCellH(0) < BlackHoleFVRegion
       ? ::exahype2::RefinementCommand::Refine
       : ::exahype2::RefinementCommand::Keep;
}

benchmarks::exahype2::ccz4::CCZ4SBH_FD4::CCZ4SBH_FD4() { prepareTwoPunctures(); }

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
