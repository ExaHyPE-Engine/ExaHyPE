#include "SWESolver.h"

tarch::logging::Log tutorials::exahype2::swe::SWESolver::_log( "tutorials::exahype2::swe::SWESolver" );

void tutorials::exahype2::swe::SWESolver::initialCondition(
  double * __restrict__                         Q,
  const tarch::la::Vector<Dimensions,double>&   volumeCentre,
  const tarch::la::Vector<Dimensions,double>&   volumeH,
  bool                                          gridIsConstructred
) {
  const bool nearCentre = tarch::la::norm2(volumeCentre) < 0.40;

  Q[0] = 1.0; // Water height (h)
  Q[1] = 0; // x-momentum (hu)
  Q[2] = 0; // y-momentum (hv)
  Q[3] = (nearCentre ? 0.0 : 0.1); // Bathymetry (b)
  //Q[3] = 0;
}

void tutorials::exahype2::swe::SWESolver::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[3+1]
  double * __restrict__                        Qoutside, // Qoutside[3+1]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  // Reflecting boundary conditions
  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
}

double tutorials::exahype2::swe::SWESolver::maxEigenvalue(
  const double * __restrict__                   Q, // Q[3+1],
  const tarch::la::Vector<Dimensions,double>&   faceCentre,
  const tarch::la::Vector<Dimensions,double>&   volumeH,
  double                                        t,
  double                                        dt,
  int                                           normal
) {
  double result = 1.0;
  const double ih = 1.0 / Q[0]; // Inverse water height
  constexpr double g = 9.81; // Gravity [m/s^2]
  // We need to ensure that h+b is not negative.
  // This can happen if a negative bathymetry is chosen.
  // This should be avoided!
  assertion((Q[0] + Q[3]) >= 0.0);
  const double c = std::sqrt( g * (Q[0] + Q[3]) );
  double u = 0.0;

  switch (normal) {
    case 0: // x
     // Q[1] is the momentum in x-direction but we need the velocity here.
     // Therefore, multiply by the inverse water height (i.e., we divide by the height).
      u = ih * Q[1];
      result = std::max(std::abs(u-c), std::abs(u+c));
      break;
    case 1: // y
      u = ih * Q[2];
      result = std::max(std::abs(u-c), std::abs(u+c));
      break;
  }

  return result;
}

void tutorials::exahype2::swe::SWESolver::flux(
  const double * __restrict__                   Q, // Q[3+1],
  const tarch::la::Vector<Dimensions,double>&   faceCentre,
  const tarch::la::Vector<Dimensions,double>&   volumeH,
  double                                        t,
  double                                        dt,
  int                                           normal,
  double * __restrict__                         F // F[3]
) {
  const double ih = 1.0 / Q[0];

  switch (normal) {
    case 0:
      F[0] = Q[1]; // hu_1
      F[1] = ih * Q[1] * Q[1]; // h(u_1)^2
      F[2] = ih * Q[1] * Q[2]; // hu_1u_2
      break;
    case 1:
      F[0] = Q[2]; // hu_2
      F[1] = ih * Q[2] * Q[1]; // hu_1u_2
      F[2] = ih * Q[2] * Q[2]; // h(u_2)^2
      break;
  }
}

void tutorials::exahype2::swe::SWESolver::nonconservativeProduct(
  const double * __restrict__                  Q,      // Q[3+1],
  const double * __restrict__                  deltaQ, // deltaQ[3+1]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__                        BTimesDeltaQ  // BTimesDeltaQ[3]
) {
  constexpr double g = 9.81;

  switch (normal) {
  case 0: // x
    BTimesDeltaQ[0] = 0.0;
    BTimesDeltaQ[1] = g * Q[0] * (deltaQ[0] + deltaQ[3]);
    BTimesDeltaQ[2] = 0.0;
    break;
  case 1: // y
    BTimesDeltaQ[0] = 0.0;
    BTimesDeltaQ[1] = 0.0;
    BTimesDeltaQ[2] = g * Q[0] * (deltaQ[0] + deltaQ[3]);
    break;
  }
}

::exahype2::RefinementCommand tutorials::exahype2::swe::SWESolver::refinementCriterion(
  const double * __restrict__                   Q, // Q[3+1]
  const tarch::la::Vector<Dimensions,double>&   volumeX,
  const tarch::la::Vector<Dimensions,double>&   volumeH,
  double                                        t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  result = volumeX[0] < 0.0 ? ::exahype2::RefinementCommand::Refine : result;
  return result;
}
