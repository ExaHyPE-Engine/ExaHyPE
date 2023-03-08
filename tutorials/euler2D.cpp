#include "euler2D.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::euler2D::euler2D::_log( "examples::exahype2::euler2D::euler2D" );




::exahype2::RefinementCommand examples::exahype2::euler2D::euler2D::refinementCriterion(
  const double * __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  logTraceInWith3Arguments( "refinementCriterion(...)", volumeX, volumeH, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  // see comments in header file

  logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
  return result;
}




void examples::exahype2::euler2D::euler2D::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments( "initialCondition(...)", volumeCentre, volumeH, gridIsConstructred );

  Q[0] = 0.1;  // rho
  Q[1] = 0.0;    // momentum in x direction
  Q[2] = 0.0;    // momentum in y direction

  bool isLeft = (volumeCentre[0] < 0.5);
  Q[3] = (isLeft ? 1.0 : 0.0);

  logTraceOut( "initialCondition(...)" );
}





void examples::exahype2::euler2D::euler2D::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[4+0]
  double * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );

  Qoutside[0] = Qinside[0]; //density
  Qoutside[1] = Qinside[1];      //velocity in x direction
  Qoutside[2] = Qinside[2];      //velocity in y direction
  Qoutside[3] = Qinside[3];      //Energy

  logTraceOut( "boundaryConditions(...)" );
}





double ::examples::exahype2::euler2D::euler2D::maxEigenvalue(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );

  const double irho = 1.0/Q[0];

  // based on the assumption that the fluid is an ideal gas, gamma chosen for dry air
  const double gamma = 1.4;
  const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  const double c   = std::sqrt(gamma * p * irho);

  double result = 1.0;
  switch(normal){
  case 0: //x
    result = std::max( std::abs(Q[1] * irho - c), std::abs(Q[1] * irho + c) );
    break;
  case 1: //y
    result = std::max( std::abs(Q[2] * irho - c), std::abs(Q[2] * irho + c) );
    break;
  }

  logTraceOut( "maxEigenvalue(...)" );

  return result;
}




void ::examples::exahype2::euler2D::euler2D::flux(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[4]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );

  const double irho = 1.0/Q[0];

  // based on the assumption that the fluid is an ideal gas, gamma chosen for dry air
  const double gamma = 1.4;
  const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  switch(normal){
  case 0: //in x direction
    F[0] = Q[1]; //rho
    F[1] = irho * Q[1] * Q[1] + p;
    F[2] = irho * Q[1] * Q[2];
    F[3] = irho * Q[1] *(Q[3] + p);
    break;
  case 1: //in y direction
    F[0] = Q[2];
    F[1] = irho * Q[2] * Q[1];
    F[2] = irho * Q[2] * Q[2] + p;
    F[3] = irho * Q[2] *(Q[3] + p);
    break;
  }

  logTraceOut( "flux(...)" );
}






















