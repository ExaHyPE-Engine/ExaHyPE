#include "testing.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::testing::testing::_log( "examples::exahype2::testing::testing" );


const double grav = 9.81;

::exahype2::RefinementCommand examples::exahype2::testing::testing::refinementCriterion(
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




void examples::exahype2::testing::testing::initialCondition(
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

  Q[0] = 1.0;
//  Q[0] = (x[0]<0.5 ? 1.0 : 1.01);
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = (distanceFromCenter<0.1 ? 1.0 : 1.01);


  logTraceOut( "initialCondition(...)" );
}





void examples::exahype2::testing::testing::boundaryValues(
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

  double _F[2][NumberOfVariables]={0.0};
  double* F[2] = {_F[0], _F[1]};
  flux(stateOut,F);
  std::copy_n(F[direction], 4, fluxOut);

  logTraceOut( "boundaryConditions(...)" );
}





void ::examples::exahype2::testing::testing::eigenvalues(
    const double* const Q, // Q[3+1]
    const int direction,
    double* const lambda
  )  {
  logTraceInWith3Arguments( "eigenvalue(...)", x, t, normal );

  const double c = std::sqrt(grav*Q[0]);
  const double ih = 1./Q[0];
  double u_n = Q[direction + 1] * ih;

  lambda[0] = u_n + c;
  lambda[1] = u_n - c;
  lambda[2] = u_n;
  lambda[3] = 0.0;

  logTraceOut( "eigenvalue(...)" );
}




void ::examples::exahype2::testing::testing::flux(
  const double* const Q, // Q[3+1]
  double** const F // F[Dimensions*3]
)  {
  logTraceIn( "flux(...)");

  double ih = 1.0 / Q[0];
  double* f = F[0];
  double* g = F[1];

  f[0] = Q[1];
  f[1] = Q[1]*Q[1]*ih + 0.5*grav*Q[0]*Q[0];
  f[2] = Q[1]*Q[2]*ih;

  g[0] = Q[2];
  g[1] = Q[1]*Q[2]*ih;
  g[2] = Q[2]*Q[2]*ih + 0.5*grav*Q[0]*Q[0];

  logTraceOut( "flux(...)" );
}




void ::examples::exahype2::testing::testing::nonConservativeProduct(
  const double* const Q,      //Q[3+1]
  const double* const gradQ,  //Q[3+1]
  double* const BgradQ       //BgradQ[3]
)  {
  logTraceInWith3Arguments( "nonConservativeProduct(...)", x, t, normal );

  kernels::idx2 idx_gradQ(Dimensions,4);

  BgradQ[0] = 0.0;
  BgradQ[1] = grav*Q[0]*gradQ[idx_gradQ(0,3)];
  BgradQ[2] = grav*Q[0]*gradQ[idx_gradQ(1,3)];
  BgradQ[3] = 0.0;

  logTraceOut( "nonConservativeProduct(...)" );
}





















