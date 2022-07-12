#include "nonlinear_acoustic.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::nonlinear_acoustic::nonlinear_acoustic::_log( "examples::exahype2::nonlinear_acoustic::nonlinear_acoustic" );




::exahype2::RefinementCommand examples::exahype2::nonlinear_acoustic::nonlinear_acoustic::refinementCriterion(
  const double * __restrict__ Q,               // Q[3+0]
  const tarch::la::Vector<Dimensions,double>&  x,
	const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {
	logTraceInWith3Arguments( "refinementCriterion(...)", x, h, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;



  logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
  return result;
}




void examples::exahype2::nonlinear_acoustic::nonlinear_acoustic::initialCondition(
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
  Q[1] = 0.0;
  Q[2] = 0.0;

  logTraceOut( "initialCondition(...)" );
}





void examples::exahype2::nonlinear_acoustic::nonlinear_acoustic::boundaryValues(
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

  fluxOut[0] = fluxIn[0];
  fluxOut[1] = fluxIn[1];
  fluxOut[2] = fluxIn[2];

  logTraceOut( "boundaryConditions(...)" );
}





void ::examples::exahype2::nonlinear_acoustic::nonlinear_acoustic::eigenvalues(
    const double* const Q, // Q[3+0]
    const int direction,
    double* const lambda
  )  {
  logTraceInWith3Arguments( "eigenvalue(...)", x, t, normal );

  lambda[0] = 1.0;
  lambda[1] = -1.0;
  lambda[2] = 0.0;

  logTraceOut( "eigenvalue(...)" );
}




void ::examples::exahype2::nonlinear_acoustic::nonlinear_acoustic::flux(
  const double* const Q, // Q[3+0]
  double** const F // F[Dimensions*3]
)  {
  logTraceIn( "flux(...)");

  double rho = 2.7;
  double c = 1.0;
  double K_0    = rho*c*c;

  F[0][0] = K_0 * Q[1];
  F[0][1] = (1/rho) * Q[0];
  F[0][2] = 0.0;

  F[1][0] = K_0 * Q[2];
  F[1][1] = 0.0;
  F[1][2] = (1/rho) * Q[0];

  logTraceOut( "flux(...)" );
}
























