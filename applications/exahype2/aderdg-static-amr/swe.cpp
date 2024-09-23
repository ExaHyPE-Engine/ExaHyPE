
#include "swe.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   applications::exahype2::aderdg::swe::_log( "applications::exahype2::aderdg::swe" );

::exahype2::RefinementCommand applications::exahype2::aderdg::swe::refinementCriterion(
    const double * __restrict__                  Q,    //Q[4+0]
    const tarch::la::Vector<Dimensions,double>&  x,
    const tarch::la::Vector<Dimensions,double>&  h,
    double                                       t
){

  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  if(x[0]<0.3){
    result = ::exahype2::RefinementCommand::Refine;
  }

  return result;
}

void applications::exahype2::aderdg::swe::initialCondition(
  double * __restrict__ Q, //Q[3+1]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {

  double r = std::sqrt((x[0]-0.5)*(x[0]-0.5)+(x[1]-0.5)*(x[1]-0.5));

  // Q[0] = 1. + r;
  Q[0] = 1.;

  Q[1] = 0.;
  Q[2] = 0.;
  // Q[3] = 0.;

  Q[3] = r < 0.1 ? 0.0 : 0.05;

}

void applications::exahype2::aderdg::swe::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[3+1]
  double * __restrict__                        Qoutside, // Qoutside[3+1]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
){

  // double r = std::sqrt((x[0]-0.5)*(x[0]-0.5)+(x[1]-0.5)*(x[1]-0.5));

  // Qoutside[0] = 1. + r;
  // Qoutside[1] = 0.;
  // Qoutside[2] = 0.;
  // Qoutside[3] = 0.;

  initialCondition(Qoutside, x, volumeH, {0, 0}, true);

}

double applications::exahype2::aderdg::swe::maxEigenvalue(
  const double* __restrict__ Q, // Q[3+1]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
){
    constexpr double grav = 9.81;

    const double u = Q[1 + normal] / Q[0];
    const double c = std::sqrt(grav * Q[0]);

    return std::max(std::abs(u + c), std::abs(u - c));

  // constexpr double c = std::sqrt(4.);
  // return c;
}

void applications::exahype2::aderdg::swe::flux(
  const double* __restrict__ Q, // Q[3+1]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[3]
) {
    double ih = 1.0 / Q[0];

    F[0] = Q[1 + normal];
    F[1] = Q[1 + normal] * Q[1] * ih;
    F[2] = Q[1 + normal] * Q[2] * ih;
    F[3] = 0.0;

  // constexpr double K0     = 4.;
  // constexpr double inv_rho = 1.0;

  // F[0] = K0 * Q[normal+1];
  // F[1] = 0.0;
  // F[2] = 0.0;
  // F[3] = 0.0;

  // F[normal+1] = inv_rho * Q[0];
}

void applications::exahype2::aderdg::swe::nonconservativeProduct(
  const double *__restrict__ Q, //Q[3+1]
  const double *__restrict__ deltaQ, //deltaQ[3+1]
  const tarch::la::Vector<Dimensions, double> &volumeX,
  const tarch::la::Vector<Dimensions, double> &volumeH,
  double t,
  double dt,
  int normal,
  double *__restrict__ BTimesDeltaQ //BTimesDeltaQ[3]
){
    constexpr double grav = 9.81;
    BTimesDeltaQ[0] = 0.0;
    switch (normal) {
    case 0:
      BTimesDeltaQ[1] = grav * Q[0] * (deltaQ[0] + deltaQ[3]);
      BTimesDeltaQ[2] = 0.0;
      break;
    case 1:
      BTimesDeltaQ[1] = 0.0;
      BTimesDeltaQ[2] = grav * Q[0] * (deltaQ[0] + deltaQ[3]);
      break;
    }
    BTimesDeltaQ[3] = 0.0;

    // BTimesDeltaQ[0] = 0.;
    // BTimesDeltaQ[1] = 0.;
    // BTimesDeltaQ[2] = 0.;
    // BTimesDeltaQ[3] = 0.;
}
