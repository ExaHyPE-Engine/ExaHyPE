#include "acoustic2d.h"

#include "exahype2/RefinementControl.h"

tarch::logging::Log examples::exahype2::acoustic2d::acoustic2d::_log("examples::exahype2::acoustic2d::acoustic2d");

::exahype2::RefinementCommand examples::exahype2::acoustic2d::acoustic2d::refinementCriterion(
  const double* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t
) {
  logTraceInWith3Arguments("refinementCriterion(...)", volumeX, volumeH, t);

  // bool isRight = volumeX[0] > 0.5;
  // ::exahype2::RefinementCommand result = isRight ? ::exahype2::RefinementCommand::Refine :
  // ::exahype2::RefinementCommand::Keep;
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Refine;
  logTraceOutWith1Argument("refinementCriterion(...)", ::toString(result));
  return result;
}

void examples::exahype2::acoustic2d::acoustic2d::initialCondition(
  double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments("initialCondition(...)", volumeCentre, volumeH, gridIsConstructred);

#if Dimensions == 2
  tarch::la::Vector<Dimensions, double> centre = {0.5, 0.5};
#else
  tarch::la::Vector<Dimensions, double> centre = {0.5, 0.5, 0.5};
#endif

  tarch::la::Vector<Dimensions, double> secondCentre = {0.4, 0.5};
  bool                                  nearCentre   = tarch::la::norm2(volumeCentre - centre) < 0.05
                    || tarch::la::norm2(volumeCentre - secondCentre) < 0.05;

  Q[0] = (nearCentre ? 0.15 : 0.0); // p
  Q[1] = 0.0;                       // vx
  Q[2] = 0.0;                       // vy
  Q[3] = 0.0;                       // vz

  logTraceOut("initialCondition(...)");
}

void examples::exahype2::acoustic2d::acoustic2d::boundaryConditions(
  const double* __restrict__ Qinside, // Qinside[4+0]
  double* __restrict__ Qoutside,      // Qoutside[4+0]
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments("boundaryConditions(...)", faceCentre, volumeH, t, normal);

  Qoutside[0] = 0.0;
  Qoutside[1] = 0.0;
  Qoutside[2] = 0.0;
  Qoutside[3] = 0.0;

  logTraceOut("boundaryConditions(...)");
}

double ::examples::exahype2::acoustic2d::acoustic2d::maxEigenvalue(
  const double* __restrict__ Q, // Q[4+0],
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

void ::examples::exahype2::acoustic2d::acoustic2d::flux(
  const double* __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[4]
) {
  logTraceInWith4Arguments("flux(...)", faceCentre, volumeH, t, normal);

  double rho = 2.7;
  double c   = 1.0;
  double K_0 = rho * c * c;

  F[0]          = K_0 * Q[normal + 1];
  F[1]          = 0.0;
  F[2]          = 0.0;
  F[3]          = 0.0;
  F[normal + 1] = (1 / rho) * Q[0];

  logTraceOut("flux(...)");
}
