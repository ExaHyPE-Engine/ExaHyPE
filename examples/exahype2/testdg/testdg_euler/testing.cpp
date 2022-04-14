#include "testing.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::testing::testing::_log( "examples::exahype2::testing::testing" );




::exahype2::RefinementCommand examples::exahype2::testing::testing::refinementCriterion(
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




void examples::exahype2::testing::testing::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
  bool                                         gridIsConstructred
) {
  logTraceInWith2Arguments( "initialCondition(...)", x, gridIsConstructred );

  // @todo Implement your stuff here


  double distanceFromCenter = std::sqrt((x[0]-0.5)*(x[0]-0.5)+(x[1]-0.5)*(x[1]-0.5));
  /*
  Q[0] = (distanceFromCenter < 0.1 ? 1.0 : 0.9);
  */

  //density
  Q[0] = 1.0;

  //momentums
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = 0.0;

  //inner energy
  Q[4] = (distanceFromCenter < 0.1 ? 0.9 : 1.0);

  logTraceOut( "initialCondition(...)" );
}





void examples::exahype2::testing::testing::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[5+0]
  double * __restrict__                        Qoutside, // Qoutside[5+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
) {
  logTraceInWith3Arguments( "boundaryConditions(...)", x, t, normal );

  for(int i=0; i<5; i++){
    Qoutside[i] = Qinside[i];
  }

  logTraceOut( "boundaryConditions(...)" );
}





double ::examples::exahype2::testing::testing::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
)  {
  logTraceInWith3Arguments( "maxEigenvalue(...)", x, t, normal );

  const double irho = 1.0/Q[0];

  if(Q[0]<0.0){
    std::cout << "Negative pressure: " << Q[0] << std::endl;
    assert(false);
  }

  // based on the assumption that the fluid is an ideal gas, gamma chosen for dry air
  const double gamma = 1.4;
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  if(p<0.0){
    std::cout << "Negative pressure p=" << p << " with start parameters [";
    for(int i=0; i<4; i++){
      std::cout << Q[i] << ", ";
    }
    std::cout << Q[4] << "]" << std::endl;
    assert(false);
  }

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




void ::examples::exahype2::testing::testing::flux(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal,
  double * __restrict__ F // F[5]
)  {
  logTraceInWith3Arguments( "flux(...)", x, t, normal );

  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  const double velocity = irho*Q[normal+1];
  F[0] = velocity*Q[0];
  F[1] = velocity*Q[1];
  F[2] = velocity*Q[2];
  F[3] = velocity*Q[3];
  F[normal+1] += p;
  F[4] = velocity*(Q[4]+p);

  logTraceOut( "flux(...)" );
}
























