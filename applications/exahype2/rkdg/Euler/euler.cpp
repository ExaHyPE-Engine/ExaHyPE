#include "euler.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::euler::euler::_log( "examples::exahype2::euler::euler" );




::exahype2::RefinementCommand examples::exahype2::euler::euler::refinementCriterion(
  const double * __restrict__ Q,               // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {
  logTraceInWith3Arguments( "refinementCriterion(...)", x, h, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  // see comments in header file

  logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
  return result;
}




void examples::exahype2::euler::euler::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
  bool                                         gridIsConstructred
) {

  Q[0] = 1.0;
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = 0.0;

  tarch::la::Vector<Dimensions,double> circleCentre = {0.5, 0.5};
  double distanceFromCenter=tarch::la::norm2(x-circleCentre);

  Q[4] = (distanceFromCenter<0.1 ? 0.9 : 1.1);

}





void examples::exahype2::euler::euler::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[5+0]
  double * __restrict__                        Qoutside, // Qoutside[5+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
) {

  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
  Qoutside[4] = Qinside[4];

}





double ::examples::exahype2::euler::euler::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {

  const double irho = 1.0/Q[0];

  // based on the assumption that the fluid is an ideal gas, gamma chosen for dry air
  const double gamma = 1.4;
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));

  const double c   = std::sqrt(gamma * p * irho);
  const double u = Q[normal+1]*irho;

  return std::max(std::abs(u-c), std::abs(u+c));

}



void ::examples::exahype2::euler::euler::flux(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[5]
)  {

  double irho = 1.0 / Q[0];
  const double gamma = 1.4;
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));

  F[0] = Q[normal+1];
  F[1] = Q[normal+1]*Q[1]*irho;
  F[2] = Q[normal+1]*Q[2]*irho;
  F[3] = Q[normal+1]*Q[3]*irho;
  F[4] = Q[normal+1]*irho*(Q[4]+p);

  F[normal+1] += p;
  
}












