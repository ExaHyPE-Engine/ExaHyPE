#include "advection.h"

#include "exahype2/RefinementControl.h"

tarch::logging::Log examples::exahype2::advection::advection::_log("examples::exahype2::advection::advection");

::exahype2::RefinementCommand examples::exahype2::advection::advection::refinementCriterion(
  const double* __restrict__ Q, // Q[3+0]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t
) {
  logTraceInWith3Arguments("refinementCriterion(...)", volumeX, volumeH, t);
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  // see comments in header file

  logTraceOutWith1Argument("refinementCriterion(...)", ::toString(result));
  return result;
}

void examples::exahype2::advection::advection::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments("initialCondition(...)", volumeCentre, volumeH, gridIsConstructred);

  // @todo Implement your stuff here, but ensure all entries are
  //   initialised incl all auxiliary variables. The snippet below
  //   sets everything to zero
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = volumeCentre[i];
  }

  logTraceOut("initialCondition(...)");
}

void examples::exahype2::advection::advection::boundaryConditions(
  const double* __restrict__ Qinside, // Qinside[3+0]
  double* __restrict__ Qoutside,      // Qoutside[3+0]
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments("boundaryConditions(...)", faceCentre, volumeH, t, normal);

  for (int i = 0; i < 3; i++) {
    Qoutside[i] = Qinside[i];
  }

  logTraceOut("boundaryConditions(...)");
}

double ::examples::exahype2::advection::advection::maxEigenvalue(
  const double* __restrict__ Q, // Q[3+0],
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
) {
  logTraceInWith4Arguments("maxEigenvalue(...)", faceCentre, volumeH, t, normal);

  return 1.0;

  logTraceOut("maxEigenvalue(...)");
}

void ::examples::exahype2::advection::advection::flux(
  const double* __restrict__ Q, // Q[3+0],
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[3]
) {
  logTraceInWith4Arguments("flux(...)", faceCentre, volumeH, t, normal);

  F[0] = 0.0;
  F[1] = 0.0;
  F[2] = 0.0;

  F[normal] = Q[normal];

  logTraceOut("flux(...)");
}
