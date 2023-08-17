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

  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  return result;

}





void examples::exahype2::elastic::elastic::initialCondition(
  double * __restrict__ Q, //Q[9+3]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {

  //Lamé parameters
  double mu = 1.0;
  double lambda = 2.0;
  double rho = 1.0;
  double cp = 2.0;
  double cs = 1.0;

  double k0 = 1.0;
  double k1 = 1.0;
  double k2 = 1.0;

  double val = cos( -std::numbers::pi*(
    k0*x[0]
    + k1*x[1]
#if Dimensions==3
    + k2*x[2]
#endif
  ));

  //stresses
  // Q[s.sigma+0] = val*(k0*(lambda+2*mu)+k1*lambda+k2*lambda); //xx
  // Q[s.sigma+1] = val*(k0*lambda+k1*(lambda+2*mu)+k2*lambda); //yy
  // Q[s.sigma+2] = val*(k0*lambda+k1*lambda+k2*(lambda+2*mu)); //zz
  // Q[s.sigma+3] = 0.0; //xy
  // Q[s.sigma+4] = 0.0; //xz
  // Q[s.sigma+5] = 0.0; //yz

  // //velocities
  // Q[s.v+0] = cp*k0*val; //vx
  // Q[s.v+1] = cp*k1*val; //vy
  // Q[s.v+2] = cp*k2*val; //vz


  //stresses
  Q[s.sigma+0] = 0.0; //xx
  Q[s.sigma+1] = 0.0; //yy
  Q[s.sigma+2] = 0.0; //zz
  Q[s.sigma+3] = k0*val*mu; //xy
  Q[s.sigma+4] = k2*val*mu; //xz
  Q[s.sigma+5] = k1*val*mu; //yz

  //velocities
  Q[s.v+0] = k2*cs*val; //vx
  Q[s.v+1] = k0*cs*val; //vy
  Q[s.v+2] = k1*cs*val; //vz


//Dumbser 2006, doi: 10.1111/j.1365-246X.2006.03120.x 3d isotropic case
//   double val = sin( 
//     2*std::numbers::pi*k0*x[0]/100.0
//     + 2*std::numbers::pi*k1*x[1]/100.0
// #if Dimensions==3
//     + 2*std::numbers::pi*k2*x[2]/100.0
// #endif
//   );

//   //stresses
//   Q[s.sigma+0] = val*(lambda+2*mu); //xx
//   Q[s.sigma+1] = val*lambda; //yy
//   Q[s.sigma+2] = val*lambda; //zz
//   Q[s.sigma+3] = val*mu; //xy
//   Q[s.sigma+4] = 0.0; //xz
//   Q[s.sigma+5] = 0.0; //yz

//   //velocities
//   Q[s.v+0] = val*cp; //vx
//   Q[s.v+1] = -val*cs; //vy
//   Q[s.v+2] = 0.0; //vz




  //auxiliary variables
  Q[s.rho]  = 1.0; //rho
  Q[s.cp]   = 2.0; //cp
  Q[s.cs]   = 1.0; //cs

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

  //Lamé parameters
  double mu = Q[s.rho] * Q[s.cs] * Q[s.cs]; //rho*cs^2
  double lambda = Q[s.rho] * Q[s.cp] * Q[s.cp] - 2.0 * mu; //rho*cp^2 - 2 mu
  double neg_irho = -1.0/Q[s.rho];

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











