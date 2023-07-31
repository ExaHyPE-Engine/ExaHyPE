#include "SWE.h"

#include "exahype2/RefinementControl.h"

tarch::logging::Log examples::exahype2::heat::SWE::_log("examples::exahype2::heat::SWE");

/*
 * Enables the usage of shortcuts to access variables, e.g. use Q[s.h] instead of Q[0]
 */
examples::exahype2::heat::VariableShortcuts s;

static constexpr double grav = 9.81;

::exahype2::RefinementCommand examples::exahype2::heat::SWE::refinementCriterion(
  const double* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  return result;
}

void examples::exahype2::heat::SWE::initialCondition(
  double* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  const tarch::la::Vector<Dimensions, int>&    point,
  bool                                         gridIsConstructed
) {

  double x_0 = x[0] - 0.5;
  x_0        = x_0 * x_0;
  double y_0 = x[1] - 0.5;
  y_0        = y_0 * y_0;

  double distanceFromCenter = std::sqrt(x_0 + y_0);

  Q[s.b] = (distanceFromCenter < 0.1 ? 1.0 : 1.1);

  Q[s.h] = 2.0;
  //  Q[s.h] = 2.0 - Q[s.b];

  Q[s.v + 0] = 0.0;
  Q[s.v + 1] = 0.0;
}

void examples::exahype2::heat::SWE::boundaryConditions(
  const double* __restrict__ Qinside, // Qinside[4+0]
  double* __restrict__ Qoutside,      // Qoutside[4+0]
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  int                                          normal
) {

  Qoutside[s.h]     = Qinside[s.h];
  Qoutside[s.v + 0] = Qinside[s.v + 0];
  Qoutside[s.v + 1] = Qinside[s.v + 1];
  Qoutside[s.b]     = Qinside[s.b];
}

double ::examples::exahype2::heat::SWE::maxEigenvalue(
  const double* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
) {

  double u = Q[s.v + normal] / Q[s.h];
  double c = std::sqrt(grav * Q[s.h]);

  return std::max(std::abs(u + c), std::abs(u - c));
}

void ::examples::exahype2::heat::SWE::flux(
  const double* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[4]
) {

  double ih = 1.0 / Q[s.h];

  F[s.h]     = Q[s.v + normal];
  F[s.v + 0] = Q[s.v + normal] * Q[s.v + 0] * ih;
  F[s.v + 1] = Q[s.v + normal] * Q[s.v + 1] * ih;
}

void ::examples::exahype2::heat::SWE::nonconservativeProduct(
  const double* __restrict__ Q,      // Q[4+0]
  const double* __restrict__ deltaQ, // deltaQ[4+0]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ BgradQ // BgradQ[4]
) {

  BgradQ[s.h] = 0.0;
  switch (normal) {
  case 0:
    BgradQ[s.v + 0] = grav * Q[s.h] * (deltaQ[s.h] + deltaQ[s.b]);
    BgradQ[s.v + 1] = 0.0;
    break;
  case 1:
    BgradQ[s.v + 0] = 0.0;
    BgradQ[s.v + 1] = grav * Q[s.h] * (deltaQ[s.h] + deltaQ[s.b]);
    break;
  }
}
