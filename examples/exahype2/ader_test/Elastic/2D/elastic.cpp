#include "elastic.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::elastic::elastic::_log( "examples::exahype2::elastic::elastic" );




::exahype2::RefinementCommand examples::exahype2::elastic::elastic::refinementCriterion(
  const double * __restrict__ Q,               // Q[8+0]
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




void examples::exahype2::elastic::elastic::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructred
) {
  logTraceInWith2Arguments( "initialCondition(...)", x, gridIsConstructred );

  //stresses
  Q[0] = 0.0; //xx
  Q[1] = 0.0; //yy
  Q[2] = 0.0; //xy

  //velocities
  tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.5};
  bool inCircle = ( tarch::la::norm2( x-circleCentre ) < 0.15 );

  Q[3] = inCircle ? std::exp(-tarch::la::norm2( x-circleCentre )/2) * 2 : 0; //x
  Q[4] = Q[3]; //y

  //auxiliary variables
  Q[5] = inCircle ? 1.2 : 1.0; //rho
  Q[6] = inCircle ? 0.1 : 0.2; //cp
  Q[7] = inCircle ? 0.1 : 0.2; //cs

  logTraceOut( "initialCondition(...)" );
}





void examples::exahype2::elastic::elastic::boundaryValues(
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

  //stresses
  stateOut[0] = 0.0;
  stateOut[1] = 0.0;
  stateOut[2] = 0.0;

  //velocities
  stateOut[3] = 0.0;
  stateOut[4] = 0.0;

  //auxiliary variables
  stateOut[5] = stateIn[5]; //rho
  stateOut[6] = stateIn[6]; //cp
  stateOut[7] = stateIn[7]; //cs

//TODO: flux
  fluxOut[0] = 0.0;
  fluxOut[1] = 0.0;
  fluxOut[2] = 0.0;
  fluxOut[3] = 0.0; //Q[0] or Q[2] but 0 either way
  fluxOut[4] = 0.0; //Q[2] or Q[1] but 0 either way
  fluxOut[5] = 0.0;
  fluxOut[6] = 0.0;
  fluxOut[7] = 0.0;

  logTraceOut( "boundaryConditions(...)" );
}




  

void ::examples::exahype2::elastic::elastic::eigenvalues(
    const double* const Q, // Q[8+0]
    const int direction,
    double* const lambda
  )  {
  logTraceInWith3Arguments( "eigenvalue(...)", x, t, normal );

  //distinct eigenvalues are cp, -cp, cs, -cs, 0

  lambda[0] = Q[6];
  lambda[1] = -Q[6];
  lambda[2] = Q[7];
  lambda[3] = -Q[7];
  lambda[4] = 0.0;
  lambda[5] = 0.0;
  lambda[6] = 0.0;
  lambda[7] = 0.0;

  logTraceOut( "eigenvalue(...)" );
}




void ::examples::exahype2::elastic::elastic::flux(
  const double* const Q, // Q[8+0]
  double** const F // F[Dimensions*8]
)  {
  logTraceIn( "flux(...)");

  //stresses
  F[0][0] = 0.0;
  F[0][1] = 0.0;
  F[0][2] = 0.0;
  F[0][3] = Q[0]; //sigma_xx
  F[0][4] = Q[2]; //sigma_xy
  F[0][5] = 0.0;
  F[0][6] = 0.0;
  F[0][7] = 0.0;

  F[1][0] = 0.0;
  F[1][1] = 0.0;
  F[1][2] = 0.0;
  F[1][3] = Q[2]; //sigma_xy
  F[1][4] = Q[1]; //sigma_yy
  F[1][5] = 0.0;
  F[1][6] = 0.0;
  F[1][7] = 0.0;

  logTraceOut( "flux(...)" );
}




void ::examples::exahype2::elastic::elastic::nonConservativeProduct(
  const double* const Q,      //Q[8+0]
  const double* const gradQ,  //Q[8+0]
  double* const BgradQ       //BgradQ[8]
)  {
  logTraceInWith3Arguments( "nonConservativeProduct(...)", x, t, normal );

  //Lamé parameters
  double mu_x = Q[5] * Q[7] * Q[7]; //rho*cs^2
  double lambda_x = Q[5] * Q[6] * Q[6] - 2 * mu_x; //rho*cp^2 - 2 mu

  double mu_y = Q[8+5] * Q[8+7] * Q[8+7]; //rho*cs^2
  double lambda_y = Q[8+5] * Q[8+6] * Q[8+6] - 2 * mu_y; //rho*cp^2 - 2 mu

  BgradQ[0] = (lambda_x + 2*mu_x) * gradQ[3]
     + lambda_y * gradQ[8+4];
  BgradQ[1] =  lambda_x * gradQ[3]
    + (lambda_y + 2*mu_y) * gradQ[8+4];
  BgradQ[2] =  mu_x * gradQ[4]
    + mu_y * gradQ[8+3];

  BgradQ[3] = 0.0; //v0
  BgradQ[4] = 0.0; //v1
  BgradQ[5] = 0.0;
  BgradQ[6] = 0.0;
  BgradQ[7] = 0.0;

  logTraceOut( "nonConservativeProduct(...)" );
}




