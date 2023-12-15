#include "../../selfgravitation/Poisson-as-hyperbolic-subcycling-scheme/Poisson.h"

#include "exahype2/RefinementControl.h"
#include "tarch/multicore/Lock.h"


tarch::logging::Log applications::exahype2::euler::selfgravitation::Poisson::_log("examples::exahype2::selfgravitation::Poisson");

tarch::multicore::BooleanSemaphore applications::exahype2::euler::selfgravitation::Poisson::_maxGradientDeltaSemaphore;

applications::exahype2::euler::selfgravitation::Poisson::Poisson():
  _currentMaxGradientDelta(1.0),
  _previousMaxGradientDelta(1.0) {}


void applications::exahype2::euler::selfgravitation::Poisson::startTimeStep(
  double globalMinTimeStamp, double globalMaxTimeStamp, double globalMinTimeStepSize, double globalMaxTimeStepSize
) {
  AbstractPoisson::startTimeStep(globalMinTimeStamp, globalMaxTimeStamp, globalMinTimeStepSize, globalMaxTimeStepSize);
  _previousMaxGradientDelta = _currentMaxGradientDelta;
  _currentMaxGradientDelta  = 0.0;
  if (_previousMaxGradientDelta > 1e-8) {
    logInfo(
      "startTimeStep(...)",
      "max gradient difference="
        << _previousMaxGradientDelta
        << ". Solver has not converged, i.e. do not use |dt|_max=" << AbstractPoisson::getAdmissibleTimeStepSize()
    );
  } else {
    logInfo(
      "startTimeStep(...)",
      "max gradient difference="
        << _previousMaxGradientDelta
        << ". Solver has converged. Use |dt|_max=" << AbstractPoisson::getAdmissibleTimeStepSize()
    );
  }
}


double applications::exahype2::euler::selfgravitation::Poisson::getAdmissibleTimeStepSize() const {
  double originalAdmissibleTimeStepSize = AbstractPoisson::getAdmissibleTimeStepSize();

  if (_previousMaxGradientDelta > 1.0e-8) {
    return tarch::la::NUMERICAL_ZERO_DIFFERENCE * 2.0; // not converged yet
  } else {
    return originalAdmissibleTimeStepSize;
  }
}


void applications::exahype2::euler::selfgravitation::Poisson::reportGradientDelta(double delta) {
  tarch::multicore::Lock lock(_maxGradientDeltaSemaphore);
  _currentMaxGradientDelta = std::max(_currentMaxGradientDelta, delta);
}


void applications::exahype2::euler::selfgravitation::Poisson::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  bool                                         gridIsConstructed
) {
  logTraceInWith3Arguments("initialCondition(...)", volumeCentre, volumeH, gridIsConstructed);

  // Trivial initial conditions
  Q[0] = 0.0;
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = 0.0;
  Q[4] = 1.0; // we need a proper initial density for the very first time step
              // just before the actual Poisson solve has already started.

  logTraceOut("initialCondition(...)");
}


void applications::exahype2::euler::selfgravitation::Poisson::boundaryConditions(
  const double* __restrict__ Qinside, // Qinside[4+0]
  double* __restrict__ Qoutside,      // Qoutside[4+0]
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments("boundaryConditions(...)", faceCentre, volumeH, t, normal);

  // Neumann conditions
  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];

  logTraceOut("boundaryConditions(...)");
}


double applications::exahype2::euler::selfgravitation::Poisson::maxEigenvalue(
  const double* __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
) {
  logTraceInWith4Arguments("maxEigenvalue(...)", faceCentre, volumeH, t, normal);

  // @todo needs cross-checking
  const double T_tau = 1.0 / 4.0 / tarch::la::PI / tarch::la::PI;

  double result = std::sqrt(1 / T_tau);

  logTraceOutWith1Argument("maxEigenvalue(...)", result);
  return result;
}


void applications::exahype2::euler::selfgravitation::Poisson::flux(
  const double* __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[4]
) {
  logTraceInWith4Arguments("flux(...)", faceCentre, volumeH, t, normal);

  const double T_tau = 1.0 / 4.0 / tarch::la::PI / tarch::la::PI;
  F[0]               = Q[normal + 1];
  F[1]               = normal == 0 ? -Q[0] / T_tau : 0.0;
  F[2]               = normal == 1 ? -Q[0] / T_tau : 0.0;
  F[3]               = normal == 2 ? -Q[0] / T_tau : 0.0;

  logTraceOut("flux(...)");
}


void applications::exahype2::euler::selfgravitation::Poisson::sourceTerm(
  const double* __restrict__ Q, // Q[4+1]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  double* __restrict__ S // S[4
) {
  logTraceInWith4Arguments("sourceTerm(...)", volumeX, volumeH, t, dt);

  const double T_tau = 1.0 / 4.0 / tarch::la::PI / tarch::la::PI;
  //  const double G     = 6.67e-11;
  const double G   = 6.67e-3; // @todo No clue what normalised constant belong in here. 1e-11 is definitely too small
  const double rho = Q[4];    // This is the input term

  S[0] = -tarch::la::PI * G / rho;
  S[1] = -Q[1] / T_tau;
  S[2] = -Q[2] / T_tau;
  S[3] = -Q[3] / T_tau;

  logTraceOut("sourceTerm(...)");
}
