#include "testing.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::testing::testing::_log( "examples::exahype2::testing::testing" );




::exahype2::RefinementCommand examples::exahype2::testing::testing::refinementCriterion(
  const double * __restrict__ Q,               // Q[3+2]
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




void examples::exahype2::testing::testing::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
  bool                                         gridIsConstructred
) {
  logTraceInWith2Arguments( "initialCondition(...)", x, gridIsConstructred );

  //pressure
  double distanceFromCentre1 = std::sqrt((x[0]-0.6)*(x[0]-0.6)+(x[1]-0.5)*(x[1]-0.5));
  double distanceFromCentre2 = std::sqrt((x[0]-0.4)*(x[0]-0.4)+(x[1]-0.5)*(x[1]-0.5));
  bool nearCentre = (distanceFromCentre1<0.08||distanceFromCentre2<0.08);

  Q[0] = (nearCentre? 0.1 : 0.0);

  //velocities
  Q[1] = 0.0;
  Q[2] = 0.0;

  //auxiliary variables
  Q[3] = 2.7; //density
  Q[4] = 3.0; //wave speed

  logTraceOut( "initialCondition(...)" );
}





void examples::exahype2::testing::testing::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[3+2]
  double * __restrict__                        Qoutside, // Qoutside[3+2]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
) {
  logTraceInWith3Arguments( "boundaryConditions(...)", x, t, normal );

  Qoutside[0] = 0.0;
  Qoutside[1] = 0.0;
  Qoutside[2] = 0.0;
  Qoutside[3] = 2.7;
  Qoutside[4] = 3.0;

  logTraceOut( "boundaryConditions(...)" );
}





double ::examples::exahype2::testing::testing::maxEigenvalue(
  const double * __restrict__ Q, // Q[3+2],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
)  {
  logTraceInWith3Arguments( "maxEigenvalue(...)", x, t, normal );

  return std::abs(Q[4]);

  logTraceOut( "maxEigenvalue(...)" );
}




void ::examples::exahype2::testing::testing::flux(
  const double * __restrict__ Q, // Q[3+2],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal,
  double * __restrict__ F // F[3]
)  {
  logTraceInWith3Arguments( "flux(...)", x, t, normal );

  double rho    = Q[3];
  double c      = Q[4];
  double K_0    = rho*c*c;

  switch(normal){
  case 0:
    F[0] = K_0 * Q[1];
    F[1] = (1/rho) * Q[0];
    F[2] = 0.0;
    break;
  case 1:
    F[0] = K_0 * Q[2];
    F[1] = 0.0;
    F[2] = (1/rho) * Q[0];
    break;
  }

  logTraceOut( "flux(...)" );
}
























