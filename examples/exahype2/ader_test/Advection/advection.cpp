#include "advection.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   exahype2::advection::advection::_log( "exahype2::advection::advection" );




void exahype2::advection::advection::initialCondition(
  double * __restrict__ Q, //Q[3+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {
  logTraceInWith2Arguments( "initialCondition(...)", x, gridIsConstructed );

  Q[0] = x[0];
  Q[1] = x[1];
  Q[2] = x[2];

  logTraceOut( "initialCondition(...)" );
}





void exahype2::advection::advection::boundaryValues(
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

  stateOut[0] = 0.0;
  stateOut[1] = 0.0;
  stateOut[2] = 0.0;

  fluxOut[0] = 0.0;
  fluxOut[1] = 0.0;
  fluxOut[2] = 0.0;

  logTraceOut( "boundaryConditions(...)" );
}




  

void ::exahype2::advection::advection::eigenvalues(
    const double* const Q, // Q[3+0]
    const int direction,
    double* const lambda
  )  {
  logTraceInWith3Arguments( "eigenvalue(...)", x, t, normal );

  lambda[0] = 1.0;
  lambda[1] = 0.0;
  lambda[2] = 0.0;

  logTraceOut( "eigenvalue(...)" );
}




void ::exahype2::advection::advection::flux(
  const double* const Q, // Q[3+0]
  double** const F // F[Dimensions*3]
)  {
  logTraceIn( "flux(...)");

  F[0][0] = Q[0];
  F[0][1] = 0.0;
  F[0][2] = 0.0;

  F[1][0] = 0.0;
  F[1][1] = Q[1];
  F[1][2] = 0.0;

  #if Dimensions==3
  F[2][0] = 0.0;
  F[2][1] = 0.0;
  F[2][2] = Q[2];
  #endif

  logTraceOut( "flux(...)" );
}






::exahype2::RefinementCommand exahype2::advection::advection::refinementCriterion(
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




