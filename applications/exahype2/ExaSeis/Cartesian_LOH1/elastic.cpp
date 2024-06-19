#include "elastic.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   examples::exahype2::elastic::elastic::_log( "examples::exahype2::elastic::elastic" );


/*
 * Enables the usage of shortcuts to access variables, e.g. use Q[s.v] instead of Q[0]
 */
examples::exahype2::elastic::VariableShortcuts s;



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
    Q[s.sigma+2] = 0.0; //zz
    Q[s.sigma+3] = 0.0; //xy
    Q[s.sigma+4] = 0.0; //xz
    Q[s.sigma+5] = 0.0; //yz


    // LOH1
    bool layerWidth = 1.0;
    bool upperLayer = x(2) <= layerWidth;

    // HHS
    // bool upperLayer = false;

    Q[ s.v + 0 ] = 0.0;
    Q[ s.v + 1 ] = Q[ s.v + 0 ];
    Q[ s.v + 2 ] = Q[ s.v + 1 ];

    //auxiliary variables
    Q[s.rho]  = upperLayer ? 2.6 : 2.7;
    Q[s.cp]   = upperLayer ? 4.0 : 6.0;
    Q[s.cs]   = upperLayer ? 2.0 : 3.464;

}





void examples::exahype2::elastic::elastic::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[9+3]
  double * __restrict__                        Qoutside, // Qoutside[9+3]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
){

    switch(normal){
      case 0:
        Qoutside[s.sigma+0] = 0.; //xx
        Qoutside[s.sigma+3] = 0.; //xy
        Qoutside[s.sigma+4] = 0.; //xz
        Qoutside[s.v+0] = 0.;
        Qoutside[s.v+1] = 0.;
        Qoutside[s.v+2] = 0.;
        break;
      case 1:
        Qoutside[s.sigma+1] = 0.; //yy
        Qoutside[s.sigma+3] = 0.; //xy
        Qoutside[s.sigma+5] = 0.; //yz
        Qoutside[s.v+0] = 0.;
        Qoutside[s.v+1] = 0.;
        Qoutside[s.v+2] = 0.;
        break;
      case 2:
        // free surface boundary condition
        Qoutside[s.sigma+2] = -Qinside[s.sigma+2]; //zz
        Qoutside[s.sigma+4] = -Qinside[s.sigma+4]; //xz
        Qoutside[s.sigma+5] = -Qinside[s.sigma+5]; //yz
        Qoutside[s.v+0] = Qinside[s.v+0];
        Qoutside[s.v+1] = Qinside[s.v+1];
        Qoutside[s.v+2] = Qinside[s.v+2];
    }

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

  //Lamé parameters
  double mu(Q[s.rho] * Q[s.cs] * Q[s.cs]); //rho*cs^2
  double lambda(Q[s.rho] * Q[s.cp] * Q[s.cp] - 2.0 * mu); //rho*cp^2 - 2 mu
  double neg_irho(-1.0/Q[s.rho]);

  switch(normal) {
    case 0:
      F[s.v+0] = neg_irho*Q[s.sigma+0]; //sigma_xx
      F[s.v+1] = neg_irho*Q[s.sigma+3]; //sigma_xy
      F[s.v+2] = neg_irho*Q[s.sigma+4]; //sigma_xz
      F[s.sigma+0] = -(lambda + 2*mu) * Q[s.v+0]; //xx
      F[s.sigma+1] = -lambda * Q[s.v+0];          //yy
      F[s.sigma+2] = -lambda * Q[s.v+0];          //zz
      F[s.sigma+3] = -mu * Q[s.v+1];              //xy
      F[s.sigma+4] = -mu * Q[s.v+2];              //xz
      F[s.sigma+5] =  0.0;                        //yz
      break;
    case 1:
      F[s.v+0] = neg_irho*Q[s.sigma+3]; //sigma_xy
      F[s.v+1] = neg_irho*Q[s.sigma+1]; //sigma_yy
      F[s.v+2] = neg_irho*Q[s.sigma+5]; //sigma_yz
      F[s.sigma+0] = -lambda * Q[s.v+1];          //xx
      F[s.sigma+1] = -(lambda + 2*mu) * Q[s.v+1]; //yy
      F[s.sigma+2] = -  lambda * Q[s.v+1];          //zz
      F[s.sigma+3] = -mu * Q[s.v+0];              //xy
      F[s.sigma+4] =  0.0;                        //xz
      F[s.sigma+5] = -mu * Q[s.v+2];              //yz
      break;
    case 2:
      F[s.v+0] = neg_irho*Q[s.sigma+4]; //sigma_xz
      F[s.v+1] = neg_irho*Q[s.sigma+5]; //sigma_yz
      F[s.v+2] = neg_irho*Q[s.sigma+2]; //sigma_zz
      F[s.sigma+0] = -lambda * Q[s.v+2];          //xx
      F[s.sigma+1] = -lambda * Q[s.v+2];          //yy
      F[s.sigma+2] = -(lambda + 2*mu) * Q[s.v+2]; //zz
      F[s.sigma+3] =  0.0;                        //xy
      F[s.sigma+4] = -mu * Q[s.v+0];              //xz
      F[s.sigma+5] = -mu * Q[s.v+1];              //yz
  }

}











void ::examples::exahype2::elastic::elastic::initPointSourceLocations(
  double sourceLocation[NumberOfPointSources][Dimensions]
){

  sourceLocation[0][0] = 0.0;
  sourceLocation[0][1] = 0.0;
#if Dimensions == 3
  //HHS1:
  // sourceLocation[0][2] = 0.693;
  //LOH1:
  sourceLocation[0][2] = 2.0;
#endif

}



void ::examples::exahype2::elastic::elastic::pointSource(
  const double* const Q, // Q[9+3]
  const double* const x, 
  const double t, 
  const double dt, 
  double* const forceVector, // Q[9
  int n) {

  for (int i = 0; i < s.NumberOfUnknowns; i++) {
    forceVector[i] = 0.0;
  }

  constexpr double t0 = 0.1;
  constexpr double M0 = 10.;//1000.0;
  double           f  = M0 * t / (t0 * t0) * std::exp(-t / t0);

  forceVector[s.sigma+3] = f; //sigma_xy

}