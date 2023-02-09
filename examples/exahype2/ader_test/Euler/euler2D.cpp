#include "euler2D.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::euler2D::euler2D::_log( "examples::exahype2::euler2D::euler2D" );




::exahype2::RefinementCommand examples::exahype2::euler2D::euler2D::refinementCriterion(
  const double * __restrict__ Q,               // Q[4+0]
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




void examples::exahype2::euler2D::euler2D::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructred
) {
  logTraceInWith2Arguments( "initialCondition(...)", x, gridIsConstructred );

  Q[0] = 1.0;
  Q[1] = 0.0;
  Q[2] = 0.0;

  double x_0 = x[0]-0.5;
  x_0 = x_0*x_0;
  double y_0 = x[1]-0.5;
  y_0 = y_0*y_0;

  double distanceFromCenter=std::sqrt(x_0+y_0);

  Q[3] = (distanceFromCenter<0.1 ? 1.0 : 1.01);

  logTraceOut( "initialCondition(...)" );
}





void examples::exahype2::euler2D::euler2D::boundaryValues(
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
  stateOut[3] = stateIn[3];

  fluxOut[0] = fluxIn[0];
  fluxOut[1] = fluxIn[1];
  fluxOut[2] = fluxIn[2];
  fluxOut[3] = fluxIn[3];

  logTraceOut( "boundaryConditions(...)" );
}





void ::examples::exahype2::euler2D::euler2D::eigenvalues(
    const double* const Q, // Q[4+0]
    const int direction,
    double* const lambda
  )  {
  logTraceInWith3Arguments( "eigenvalue(...)", x, t, normal );

  const double irho = 1.0/Q[0];

  // based on the assumption that the fluid is an ideal gas, gamma chosen for dry air
  const double gamma = 1.4;
  const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  const double c   = std::sqrt(gamma * p * irho);

  const double u = Q[direction+1]*irho;

  lambda[0] = u-c;
  lambda[1] = u;
  lambda[2] = u+c;
  lambda[3] = 0.0;

  logTraceOut( "eigenvalue(...)" );
}




void ::examples::exahype2::euler2D::euler2D::flux(
  const double* const Q, // Q[4+0]
  double** const F // F[Dimensions*4]
)  {
  logTraceIn( "flux(...)");

  double irho = 1.0 / Q[0];
  const double gamma = 1.4;
  const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  F[0][0] = Q[1];
  F[0][1] = Q[1]*Q[1]*irho+p;
  F[0][2] = Q[1]*Q[2]*irho;
  F[0][3] = Q[1]*irho*(Q[3]+p);

  F[1][0] = Q[2];
  F[1][1] = Q[2]*Q[1]*irho;
  F[1][2] = Q[2]*Q[2]*irho+p;
  F[1][3] = Q[2]*irho*(Q[3]+p);

  logTraceOut( "flux(...)" );
}
























