#include "SWE.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::heat::SWE::_log( "examples::exahype2::heat::SWE" );




::exahype2::RefinementCommand examples::exahype2::heat::SWE::refinementCriterion(
  const double * __restrict__ Q,               // Q[3+0]
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




void examples::exahype2::heat::SWE::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructred
) {
  logTraceInWith2Arguments( "initialCondition(...)", x, gridIsConstructred );

  double x_0 = x[0]-0.5;
  x_0 = x_0*x_0;
  double y_0 = x[1]-0.5;
  y_0 = y_0*y_0;

  double distanceFromCenter=std::sqrt(x_0+y_0);

  Q[0] = (distanceFromCenter<0.1 ? 1.0 : 1.01);
//  Q[0] = (x[0]<0.5 ? 1.0 : 1.01);
  Q[1] = 0.0;
  Q[2] = 0.0;

  logTraceOut( "initialCondition(...)" );
}





void examples::exahype2::heat::SWE::boundaryValues(
    const double* const x,
    const double        t,
    const double        dt,
    const int           faceIndex,
    const int           direction,
    const double* const fluxIn,
    const double* const stateIn,
    const double* const gradStateIn,
    double* const       fluxOut,
    double* const       stateOut
) {
  logTraceInWith3Arguments( "boundaryConditions(...)", x, t, direction );

  stateOut[0] = stateIn[0];
  stateOut[1] = stateIn[1];
  stateOut[2] = stateIn[2];

//  stateOut[direction+1] = -stateIn[direction+1];

  fluxOut[0] = fluxIn[0];
  fluxOut[1] = fluxIn[1];
  fluxOut[2] = fluxIn[2];

  logTraceOut( "boundaryConditions(...)" );
}





void ::examples::exahype2::heat::SWE::eigenvalues(
    const double* const Q, // Q[3+0]
    const int direction,
    double* const lambda
  )  {
  logTraceInWith3Arguments( "eigenvalue(...)", x, t, normal );

  double u = Q[direction+1]/Q[0];
  double c = std::sqrt(Q[0]);

  lambda[0] = u-c;
  lambda[1] = u;
  lambda[2] = u+c;

  logTraceOut( "eigenvalue(...)" );
}




void ::examples::exahype2::heat::SWE::flux(
  const double* const Q, // Q[3+0]
  double** const F // F[Dimensions*3]
)  {
  logTraceIn( "flux(...)");

  double ih = 1.0 / Q[0];
  F[0][0] = Q[1];
  F[0][1] = Q[1]*Q[1]*ih + 0.5*Q[0]*Q[0];
  F[0][2] = Q[1]*Q[2]*ih;

  F[1][0] = Q[2];
  F[1][1] = Q[1]*Q[2]*ih;
  F[1][2] = Q[2]*Q[2]*ih + 0.5*Q[0]*Q[0];

  logTraceOut( "flux(...)" );
}
























