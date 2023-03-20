#include "elastic.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   examples::exahype2::elastic::elastic::_log( "examples::exahype2::elastic::elastic" );


/*
 * Enables the usage of shortcuts to access variables, e.g. use Q[s.sigma] instead of Q[0]
 */
examples::exahype2::elastic::VariableShortcuts s;



::exahype2::RefinementCommand examples::exahype2::elastic::elastic::refinementCriterion(
  const double * __restrict__ Q,               // Q[9+3]
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
  double * __restrict__ Q, //Q[9+3]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {

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

}





void examples::exahype2::elastic::elastic::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[9+3]
  double * __restrict__                        Qoutside, // Qoutside[9+3]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
){

  //stresses
  Qoutside[s.sigma+0] = 0.0;
  Qoutside[s.sigma+1] = 0.0;
  Qoutside[s.sigma+2] = 0.0;
  Qoutside[s.sigma+3] = 0.0;
  Qoutside[s.sigma+4] = 0.0;
  Qoutside[s.sigma+5] = 0.0;

  //velocities
  Qoutside[s.v+0]     = 0.0;
  Qoutside[s.v+1]     = 0.0;
  Qoutside[s.v+2]     = 0.0;

  //auxiliary variables
  Qoutside[s.rho]     = Qinside[s.rho];
  Qoutside[s.cp]      = Qinside[s.cp];
  Qoutside[s.cs]      = Qinside[s.cs];  

}




  


double ::examples::exahype2::elastic::elastic::maxEigenvalue(
  const double* __restrict__ Q, // Q[9+3]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
){

  return std::max(std::abs(Q[s.cp]), std::abs(Q[s.cs]));

}




void ::examples::exahype2::elastic::elastic::flux(
  const double* __restrict__ Q, // Q[9+3]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[9]
) {

  for(int i=0; i<6; i++){
    F[s.sigma+i] = 0.0;
  }

switch(normal) {
  case 0:
    F[s.v+0] = Q[s.sigma+0]; //sigma_xx
    F[s.v+1] = Q[s.sigma+3]; //sigma_xy
    F[s.v+2] = Q[s.sigma+4]; //sigma_xz
    break;
  case 1:
    F[s.v+0] = Q[s.sigma+3]; //sigma_xy
    F[s.v+1] = Q[s.sigma+1]; //sigma_yy
    F[s.v+2] = Q[s.sigma+5]; //sigma_yz
    break;
  case 2:
    F[s.v+0] = Q[s.sigma+4]; //sigma_xz
    F[s.v+1] = Q[s.sigma+5]; //sigma_yz
    F[s.v+2] = Q[s.sigma+2]; //sigma_zz
}
}




void ::examples::exahype2::elastic::elastic::nonconservativeProduct(
  const double *__restrict__ Q, //Q[9+3]
  const double *__restrict__ deltaQ, //deltaQ[9+3]
  const tarch::la::Vector<Dimensions, double> &volumeX,
  const tarch::la::Vector<Dimensions, double> &volumeH,
  double t,
  double dt,
  int normal,
  double *__restrict__ BgradQ //BgradQ[9]
){

  //Lamé parameters
  double mu = Q[s.rho] * Q[s.cs] * Q[s.cs]; //rho*cs^2
  double lambda = Q[s.rho] * Q[s.cp] * Q[s.cp] - 2 * mu; //rho*cp^2 - 2 mu

  switch(normal){
    case 0:
      BgradQ[s.sigma+0] = (lambda + 2*mu) * deltaQ[s.v+0]; //xx
      BgradQ[s.sigma+1] = lambda * deltaQ[s.v+0];          //yy
      BgradQ[s.sigma+2] = lambda * deltaQ[s.v+0];          //zz
      BgradQ[s.sigma+3] = mu * deltaQ[s.v+1];              //xy
      BgradQ[s.sigma+4] = mu * deltaQ[s.v+2];              //xz
      BgradQ[s.sigma+5] = 0.0;                               //yz
      break;
    case 1:
      BgradQ[s.sigma+0] = lambda * deltaQ[s.v+1];          //xx
      BgradQ[s.sigma+1] = (lambda + 2*mu) * deltaQ[s.v+1]; //yy
      BgradQ[s.sigma+2] = lambda * deltaQ[s.v+1];          //zz
      BgradQ[s.sigma+3] = mu * deltaQ[s.v+0];              //xy
      BgradQ[s.sigma+4] = 0.0;                               //xz
      BgradQ[s.sigma+5] = mu * deltaQ[s.v+2];              //yz
      break;
    case 2:
      BgradQ[s.sigma+0] = lambda * deltaQ[s.v+2];          //xx
      BgradQ[s.sigma+1] = lambda * deltaQ[s.v+2];          //yy
      BgradQ[s.sigma+2] = (lambda + 2*mu) * deltaQ[s.v+2]; //zz
      BgradQ[s.sigma+3] = 0.0;                               //xy
      BgradQ[s.sigma+4] = mu * deltaQ[s.v+0];              //xz
      BgradQ[s.sigma+5] = mu * deltaQ[s.v+1];              //yz
  }

  for(int i=0; i<3; i++){
    BgradQ[s.v+i] = 0.0;
  }

}








