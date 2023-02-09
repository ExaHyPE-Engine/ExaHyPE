#include "elastic.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   examples::exahype2::elastic::elastic::_log( "examples::exahype2::elastic::elastic" );


/*
 * Enables the usage of shortcuts to access variables, e.g. use Q[s.v] instead of Q[0]
 */
examples::exahype2::elastic::VariableShortcuts s;



::exahype2::RefinementCommand examples::exahype2::elastic::elastic::refinementCriterion(
  const double * __restrict__ Q,               // Q[5+3]
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
  double * __restrict__ Q, //Q[5+3]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {
  logTraceInWith2Arguments( "initialCondition(...)", x, gridIsConstructed );

    //stresses
    Q[s.sigma+0] = 0.0; //xx
    Q[s.sigma+1] = 0.0; //yy
    Q[s.sigma+2] = 0.0; //xy
    Q[s.sigma+3] = 0.0; //xy
    Q[s.sigma+4] = 0.0; //xz
    Q[s.sigma+5] = 0.0; //yz

    //velocities
#if Dimensions==2
    tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.5};
    tarch::la::Vector<Dimensions,double> position = {x[0],x[1]};
#else
    tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.5,0.5};
    tarch::la::Vector<Dimensions,double> position = {x[0],x[1],x[2]};
#endif    
    bool inCircle = tarch::la::norm2( position-circleCentre ) < 0.15;

    Q[s.v+0] = inCircle ? 0.1 : 0.0;//inCircle ? std::exp(-tarch::la::norm2( position-circleCentre )/10) : 0; //x
    Q[s.v+1] = Q[s.v+0]; //y
    Q[s.v+2] = Q[s.v+0]; //z

    //auxiliary variables
    Q[s.rho]  = 2.7; //rho
    Q[s.cp]   = 6.0; //cp
    Q[s.cs]   = 3.464; //cs

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
  // @todo Please implement/augment if required
  stateOut[s.sigma+0] = 0.0;
  stateOut[s.sigma+1] = 0.0;
  stateOut[s.sigma+2] = 0.0;
  stateOut[s.sigma+3] = 0.0;
  stateOut[s.sigma+4] = 0.0;
  stateOut[s.sigma+5] = 0.0;
  
  //velocities
  stateOut[s.v+0] = 0.0;
  stateOut[s.v+1] = 0.0;
  stateOut[s.v+2] = 0.0;
  
  //auxiliary variables
  stateOut[s.rho] = stateIn[s.rho]; //rho
  stateOut[s.cp] = stateIn[s.cp]; //cp
  stateOut[s.cs] = stateIn[s.cs]; //cs

  fluxOut[s.sigma+0] = 0.0;
  fluxOut[s.sigma+1] = 0.0;
  fluxOut[s.sigma+2] = 0.0;
  fluxOut[s.sigma+3] = 0.0;
  fluxOut[s.sigma+4] = 0.0;
  fluxOut[s.sigma+5] = 0.0;

  fluxOut[s.v+0] = 0.0;
  fluxOut[s.v+1] = 0.0;
  fluxOut[s.v+2] = 0.0;

  logTraceOut( "boundaryConditions(...)" );
}

  

void ::examples::exahype2::elastic::elastic::eigenvalues(
    const double* const Q, // Q[5+3]
    const int direction,
    double* const lambda
  )  {
  logTraceInWith3Arguments( "eigenvalue(...)", x, t, normal );
  // @todo Please implement/augment if required

  lambda[0] = Q[s.cp];
  lambda[1] = -Q[s.cp];
  lambda[2] = Q[s.cs];
  lambda[3] = -Q[s.cs];
  for(int i=4; i<s.SizeVariables; i++){
    lambda[i] = 0.0;
  }


  logTraceOut( "eigenvalue(...)" );
}



void ::examples::exahype2::elastic::elastic::flux(
  const double* const Q, // Q[5+3]
  double** const F // F[Dimensions*5]
)  {
  logTraceIn( "flux(...)");

  // @todo Please implement/augment if required
  F[0][s.sigma+0] = 0.0;
  F[0][s.sigma+1] = 0.0;
  F[0][s.sigma+2] = 0.0;
  F[0][s.sigma+3] = 0.0;
  F[0][s.sigma+4] = 0.0;
  F[0][s.sigma+5] = 0.0;
  F[0][s.v+0] = Q[s.sigma+0]; //sigma_xx
  F[0][s.v+1] = Q[s.sigma+3]; //sigma_xy
  F[0][s.v+2] = Q[s.sigma+4]; //sigma_xz
  
  F[1][s.sigma+0] = 0.0;
  F[1][s.sigma+1] = 0.0;
  F[1][s.sigma+2] = 0.0;
  F[1][s.sigma+3] = 0.0;
  F[1][s.sigma+4] = 0.0;
  F[1][s.sigma+5] = 0.0;
  F[1][s.v+0] = Q[s.sigma+3]; //sigma_xy
  F[1][s.v+1] = Q[s.sigma+1]; //sigma_yy
  F[1][s.v+2] = Q[s.sigma+5]; //sigma_yz

#if Dimensions==3
  F[2][s.sigma+0] = 0.0;
  F[2][s.sigma+1] = 0.0;
  F[2][s.sigma+2] = 0.0;
  F[2][s.sigma+3] = 0.0;
  F[2][s.sigma+4] = 0.0;
  F[2][s.sigma+5] = 0.0;
  F[2][s.v+0] = Q[s.sigma+4]; //sigma_xz
  F[2][s.v+1] = Q[s.sigma+5]; //sigma_yz
  F[2][s.v+2] = Q[s.sigma+2]; //sigma_zz
#endif

  logTraceOut( "flux(...)" );
}


void ::examples::exahype2::elastic::elastic::nonConservativeProduct(
  const double* const Q,      //Q[8+0]
  const double* const gradQ,  //Q[8+0]
  double* const BgradQ       //BgradQ[8]
)  {
  logTraceInWith3Arguments( "nonConservativeProduct(...)", x, t, normal );

  double mu = Q[s.rho] * Q[s.cs] * Q[s.cs]; //rho*cs^2
  double lambda = Q[s.rho] * Q[s.cp] * Q[s.cp] - 2 * mu; //rho*cp^2 - 2 mu

  BgradQ[s.sigma+0] = (lambda + 2*mu) * gradQ[s.v+0]; //xx
  BgradQ[s.sigma+1] = lambda * gradQ[s.v+0];          //yy
  BgradQ[s.sigma+2] = lambda * gradQ[s.v+0];          //zz
  BgradQ[s.sigma+3] = mu * gradQ[s.v+1];              //xy
  BgradQ[s.sigma+4] = mu * gradQ[s.v+2];              //xz
  BgradQ[s.sigma+5] = 0.0;                               //yz
  BgradQ[s.v+0] = 0.0;                                   //vx
  BgradQ[s.v+1] = 0.0;                                   //vy
  BgradQ[s.v+2] = 0.0;                                   //vz

  int pos = s.SizeVariables; //used to access elements of gradient in next direction

  BgradQ[s.sigma+0] += lambda * gradQ[pos+s.v+1];          //xx
  BgradQ[s.sigma+1] += (lambda + 2*mu) * gradQ[pos+s.v+1]; //yy
  BgradQ[s.sigma+2] += lambda * gradQ[pos+s.v+1];          //zz
  BgradQ[s.sigma+3] += mu * gradQ[pos+s.v+0];              //xy
  BgradQ[s.sigma+4] += 0.0;                               //xz
  BgradQ[s.sigma+5] += mu * gradQ[pos+s.v+2];              //yz

#if Dimensions==3
  pos += pos;
  BgradQ[s.sigma+0] += lambda * gradQ[pos+s.v+2];          //xx
  BgradQ[s.sigma+1] += lambda * gradQ[pos+s.v+2];          //yy
  BgradQ[s.sigma+2] += (lambda + 2*mu) * gradQ[pos+s.v+2]; //zz
  BgradQ[s.sigma+3] += 0.0;                               //xy
  BgradQ[s.sigma+4] += mu * gradQ[pos+s.v+0];              //xz
  BgradQ[s.sigma+5] += mu * gradQ[pos+s.v+1];              //yz
#endif


  logTraceOut( "nonConservativeProduct(...)" );
}




