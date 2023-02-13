#include "SWE.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   examples::exahype2::heat::SWE::_log( "examples::exahype2::heat::SWE" );


/*
 * Enables the usage of shortcuts to access variables, e.g. use Q[s.h] instead of Q[s.h]
 */
examples::exahype2::heat::VariableShortcuts s;



::exahype2::RefinementCommand examples::exahype2::heat::SWE::refinementCriterion(
  const double * __restrict__ Q,               // Q[3+1]
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
  double * __restrict__ Q, //Q[3+1]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {
  logTraceInWith2Arguments( "initialCondition(...)", x, gridIsConstructed );

  double x_0 = x[0]-0.5;
  x_0 = x_0*x_0;
  double y_0 = x[1]-0.5;
  y_0 = y_0*y_0;

  double distanceFromCenter=std::sqrt(x_0+y_0);

  Q[s.b] = (distanceFromCenter<0.1 ? 1.0 : 1.1);

  Q[s.h] = 2.0 - Q[s.b];

  Q[s.v+0] = 0.0;
  Q[s.v+1] = 0.0;
  


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

  stateOut[s.h]   = stateIn[s.h];
  stateOut[s.v+0] = stateIn[s.v+0];
  stateOut[s.v+1] = stateIn[s.v+1];
  stateOut[s.b]   = stateIn[s.b];

  fluxOut[s.h]    = fluxIn[s.h];
  fluxOut[s.v+0]  = fluxIn[s.v+0];
  fluxOut[s.v+1]  = fluxIn[s.v+1];
  fluxOut[s.b]    = 0.0;

  logTraceOut( "boundaryConditions(...)" );
}




  

void ::examples::exahype2::heat::SWE::eigenvalues(
    const double* const Q, // Q[3+1]
    const int direction,
    double* const lambda
  )  {
  logTraceInWith3Arguments( "eigenvalue(...)", x, t, normal );

  double u = Q[s.v+direction]/Q[s.h];
  double c = std::sqrt(Q[s.h]);

  lambda[0] = u-c;
  lambda[1] = u;
  lambda[2] = u+c;
  lambda[3] = 0.0;

  logTraceOut( "eigenvalue(...)" );
}




void ::examples::exahype2::heat::SWE::flux(
  const double* const Q, // Q[3+1]
  double** const F // F[Dimensions*3]
)  {
  logTraceIn( "flux(...)");

  double ih = 1.0 / Q[s.h];

  F[0][s.h]   = Q[s.v+0];
  F[0][s.v+0] = Q[s.v+0]*Q[s.v+0]*ih;
  F[0][s.v+1] = Q[s.v+0]*Q[s.v+1]*ih;
  F[0][s.b]   = 0.0;

  F[1][s.h]   = Q[s.v+1];
  F[1][s.v+0] = Q[s.v+0]*Q[s.v+1]*ih;
  F[1][s.v+1] = Q[s.v+1]*Q[s.v+1]*ih;
  F[1][s.b]   = 0.0;

  logTraceOut( "flux(...)" );
}





void ::examples::exahype2::heat::SWE::nonConservativeProduct(
  const double* const Q,      //Q[3+1]
  const double* const gradQ,  //Q[3+1]
  double* const BgradQ       //BgradQ[s.h]
)  {
  logTraceInWith3Arguments( "nonConservativeProduct(...)", x, t, normal );

  static const double grav = 9.81;
  
  BgradQ[s.h]   = 0.0;
  BgradQ[s.v+0] = grav*Q[s.h]*(gradQ[s.h]+gradQ[s.b]);
  BgradQ[s.v+1] = grav*Q[s.h]*(gradQ[s.Size+s.h]+gradQ[s.Size+s.b]);
  BgradQ[s.b]   = 0.0;

  logTraceOut( "nonConservativeProduct(...)" );
}









