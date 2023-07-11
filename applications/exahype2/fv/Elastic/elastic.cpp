#include "elastic.h"

#include "exahype2/RefinementControl.h"

tarch::logging::Log examples::exahype2::elastic::elastic::_log("examples::exahype2::elastic::elastic");

::exahype2::RefinementCommand examples::exahype2::elastic::elastic::refinementCriterion(
  const double* __restrict__ Q, // Q[9+3]
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  double                                       t
) {

  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Refine;
  //  ::exahype2::RefinementCommand result = x[0] > 0.5 ? ::exahype2::RefinementCommand::Refine :
  //  ::exahype2::RefinementCommand::Keep;

  return result;
}

void examples::exahype2::elastic::elastic::initialCondition(
  double* __restrict__ Q, // Q[9+3]
  const tarch::la::Vector<Dimensions, double>& x,
  const tarch::la::Vector<Dimensions, double>& h,
  bool                                         gridIsConstructed
) {

  // stresses
  Q[0 + 0] = 0.0; // xx
  Q[0 + 1] = 0.0; // yy
  Q[0 + 2] = 0.0; // xy
  Q[0 + 3] = 0.0; // xy
  Q[0 + 4] = 0.0; // xz
  Q[0 + 5] = 0.0; // yz

  // velocities
#if Dimensions == 2
  tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.5};
  tarch::la::Vector<Dimensions, double> position     = {x[0], x[1]};
#else
  tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.5, 0.5};
  tarch::la::Vector<Dimensions, double> position     = {x[0], x[1], x[2]};
#endif
  bool inCircle = tarch::la::norm2(position - circleCentre) < 0.15;

  Q[6 + 0] = inCircle ? 0.1 : 0.0; // inCircle ? std::exp(-tarch::la::norm2( position-circleCentre )/10) : 0; //x
  // Q[6+0] = 0.1;
  Q[6 + 1] = Q[6 + 0]; // y
  Q[6 + 2] = Q[6 + 0]; // z

  // auxiliary variables
  Q[9]  = 2.7;   // rho
  Q[10] = 6.0;   // cp
  Q[11] = 3.464; // cs
}

void examples::exahype2::elastic::elastic::boundaryConditions(
  const double* __restrict__ Qinside, // Qinside[9+3]
  double* __restrict__ Qoutside,      // Qoutside[9+3]
  const tarch::la::Vector<Dimensions, double>& faceCentre,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  int                                          normal
) {

  // stresses
  Qoutside[0 + 0] = 0.0;
  Qoutside[0 + 1] = 0.0;
  Qoutside[0 + 2] = 0.0;
  Qoutside[0 + 3] = 0.0;
  Qoutside[0 + 4] = 0.0;
  Qoutside[0 + 5] = 0.0;

  // velocities
  Qoutside[6 + 0] = 0.0;
  Qoutside[6 + 1] = 0.0;
  Qoutside[6 + 2] = 0.0;

  // auxiliary variables
  Qoutside[9]  = Qinside[9];
  Qoutside[10] = Qinside[10];
  Qoutside[11] = Qinside[11];
}

double ::examples::exahype2::elastic::elastic::maxEigenvalue(
  const double* __restrict__ Q, // Q[9+3]
  const tarch::la::Vector<Dimensions, double>& volumeCentre,
  const tarch::la::Vector<Dimensions, double>& volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
) {

  return std::max(std::abs(Q[10]), std::abs(Q[11]));
}

void ::examples::exahype2::elastic::elastic::flux(
  const double* __restrict__ Q, // Q[9+3]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[9]
) {

  for (int i = 0; i < 6; i++) {
    F[0 + i] = 0.0;
  }

  switch (normal) {
  case 0:
    F[6 + 0] = Q[0 + 0]; // sigma_xx
    F[6 + 1] = Q[0 + 3]; // sigma_xy
    F[6 + 2] = Q[0 + 4]; // sigma_xz
    break;
  case 1:
    F[6 + 0] = Q[0 + 3]; // sigma_xy
    F[6 + 1] = Q[0 + 1]; // sigma_yy
    F[6 + 2] = Q[0 + 5]; // sigma_yz
    break;
  case 2:
    F[6 + 0] = Q[0 + 4]; // sigma_xz
    F[6 + 1] = Q[0 + 5]; // sigma_yz
    F[6 + 2] = Q[0 + 2]; // sigma_zz
  }
}

void ::examples::exahype2::elastic::elastic::nonconservativeProduct(
  const double* __restrict__ Q,      // Q[9+3]
  const double* __restrict__ deltaQ, // deltaQ[9+3]
  const tarch::la::Vector<Dimensions, double>& volumeX,
  const tarch::la::Vector<Dimensions, double>& volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ BgradQ // BgradQ[9]
) {

  // Lamé parameters
  double mu     = Q[9] * Q[11] * Q[11];          // rho*cs^2
  double lambda = Q[9] * Q[10] * Q[10] - 2 * mu; // rho*cp^2 - 2 mu

  switch (normal) {
  case 0:
    BgradQ[0 + 0] = (lambda + 2 * mu) * deltaQ[6 + 0]; // xx
    BgradQ[0 + 1] = lambda * deltaQ[6 + 0];            // yy
    BgradQ[0 + 2] = lambda * deltaQ[6 + 0];            // zz
    BgradQ[0 + 3] = mu * deltaQ[6 + 1];                // xy
    BgradQ[0 + 4] = mu * deltaQ[6 + 2];                // xz
    BgradQ[0 + 5] = 0.0;                               // yz
    break;
  case 1:
    BgradQ[0 + 0] = lambda * deltaQ[6 + 1];            // xx
    BgradQ[0 + 1] = (lambda + 2 * mu) * deltaQ[6 + 1]; // yy
    BgradQ[0 + 2] = lambda * deltaQ[6 + 1];            // zz
    BgradQ[0 + 3] = mu * deltaQ[6 + 0];                // xy
    BgradQ[0 + 4] = 0.0;                               // xz
    BgradQ[0 + 5] = mu * deltaQ[6 + 2];                // yz
    break;
  case 2:
    BgradQ[0 + 0] = lambda * deltaQ[6 + 2];            // xx
    BgradQ[0 + 1] = lambda * deltaQ[6 + 2];            // yy
    BgradQ[0 + 2] = (lambda + 2 * mu) * deltaQ[6 + 2]; // zz
    BgradQ[0 + 3] = 0.0;                               // xy
    BgradQ[0 + 4] = mu * deltaQ[6 + 0];                // xz
    BgradQ[0 + 5] = mu * deltaQ[6 + 1];                // yz
  }

  for (int i = 0; i < 3; i++) {
    BgradQ[6 + i] = 0.0;
  }
}
