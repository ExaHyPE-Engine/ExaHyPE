#include "elastic.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   examples::exahype2::elastic::elastic::elastic::_log( "examples::exahype2::elastic::elastic::elastic" );

/*
 * Enables the usage of shortcuts to access variables, e.g. use Q[s.sigma] instead of Q[0]
 */
examples::exahype2::elastic::VariableShortcuts s;

using namespace std;

template<typename T>
void examples::exahype2::elastic::elastic::initialCondition(
  T * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {

  //Lamé parameters
  T mu(1.0);
  T lambda(2.0);
  T rho(1.0);
  T cp(2.0);
  T cs(1.0);

  T k0(1.0);
  T k1(1.0);
  T k2(1.0);

  T val(cos( -std::numbers::pi*(
    k0*x[0]
    + k1*x[1]
#if Dimensions==3
    + k2*x[2]
#endif
  )));

  // roots of the plane-wave operator are: sqrt(lambda+4*mu), -sqrt(lambda+4*mu), 0.0, 
  // characteristic equation is A^2 - B^2 = 0 with
  // A = alpha^4 - 2*alpha^2*(lambda+4mu) + (lambda+4*mu)^2 - (lambda+mu)^2
  // B = -alpha^2 * (lambda+mu) + (lambda+mu)*(lambda+4*mu) - (lambda+mu)^2

  //stresses
  // Q[s.sigma+0] = val*(k0*(lambda+2*mu)+k1*lambda+k2*lambda); //xx
  // Q[s.sigma+1] = val*(k0*lambda+k1*(lambda+2*mu)+k2*lambda); //yy
  // Q[s.sigma+2] = val*(k0*lambda+k1*lambda+k2*(lambda+2*mu)); //zz
  // Q[s.sigma+3] = 0.0; //xy
  // Q[s.sigma+4] = 0.0; //xz
  // Q[s.sigma+5] = 0.0; //yz

  // //velocities
  Q[s.v+0] = cs*k0*val; //vx
  Q[s.v+1] = cs*k1*val; //vy
  Q[s.v+2] = cs*k2*val; //vz

  Q[s.sigma+0] = 0.0; //xx
  Q[s.sigma+1] = 0.0; //yy
  Q[s.sigma+2] = 0.0; //zz
  Q[s.sigma+3] = k0*val*mu; //xy
  Q[s.sigma+4] = k2*val*mu; //xz
  Q[s.sigma+5] = k1*val*mu; //yz

//Dumbser 2006, doi: 10.1111/j.1365-246X.2006.03120.x 3d isotropic case
//   T val = sin( 
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


void examples::exahype2::elastic::elastic::analyticalSolution(
  const tarch::la::Vector<Dimensions,double>&   x,
  const double                                  t,
  const double                                  dt,
  double*                                       sol
) {

  //Lamé parameters
  double mu(1.0);
  double lambda(2.0);
  double cp(2.0);
  double cs(1.0);

  double k0(1.0);
  double k1(1.0);
  double k2(1.0);

  double w(6*M_PI/std::sqrt(3.0));

  double val(cos( w*t - std::numbers::pi*(
    k0*x[0]
    + k1*x[1]
#if Dimensions==3
    + k2*x[2]
#endif
  )));

  //stresses
  sol[s.sigma+0] = 0.0; //xx
  sol[s.sigma+1] = 0.0; //yy
  sol[s.sigma+2] = 0.0; //zz
  sol[s.sigma+3] = k0*val*mu; //xy
  sol[s.sigma+4] = k2*val*mu; //xz
  sol[s.sigma+5] = k1*val*mu; //yz

  //velocities
  sol[s.v+0] = k2*cs*val; //vx
  sol[s.v+1] = k0*cs*val; //vy
  sol[s.v+2] = k1*cs*val; //vz

}


template<typename T>
void examples::exahype2::elastic::elastic::boundaryConditions(
  const T * __restrict__                  Qinside, // Qinside[4+0]
  T * __restrict__                        Qoutside, // Qoutside[4+0]
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


template<typename T>
double ::examples::exahype2::elastic::elastic::maxEigenvalue(
  const T* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
) {

  return std::max(std::abs(Q[s.cp]), std::abs(Q[s.cs]));

}


template<typename T>
void ::examples::exahype2::elastic::elastic::flux(
  const T* __restrict__                        Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  T* __restrict__                              F // F[Dimensions*4]
){

  //Lamé parameters
  T mu(Q[s.rho] * Q[s.cs] * Q[s.cs]); //rho*cs^2
  T lambda(Q[s.rho] * Q[s.cp] * Q[s.cp] - 2.0 * mu); //rho*cp^2 - 2 mu
  T neg_irho(-1.0/Q[s.rho]);

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




template void examples::exahype2::elastic::elastic::initialCondition(
  float * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
);
template void examples::exahype2::elastic::elastic::boundaryConditions(
  const float * __restrict__                   Qinside, // Qinside[4+0]
  float * __restrict__                         Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
);template double ::examples::exahype2::elastic::elastic::maxEigenvalue(
  const float* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
);template void ::examples::exahype2::elastic::elastic::flux(
  const float* __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  float* __restrict__ F
);


template void examples::exahype2::elastic::elastic::initialCondition(
  double * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
);
template void examples::exahype2::elastic::elastic::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[4+0]
  double * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
);
template double ::examples::exahype2::elastic::elastic::maxEigenvalue(
  const double* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
);
template void ::examples::exahype2::elastic::elastic::flux(
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F
);


#ifdef __STDCPP_FLOAT16_T__
template void examples::exahype2::elastic::elastic::initialCondition(
  float16_t * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
);
template double ::examples::exahype2::elastic::elastic::maxEigenvalue(
  const float16_t* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
);
template void ::examples::exahype2::elastic::elastic::flux(
  const float16_t* __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  float16_t* __restrict__ F
);
template void examples::exahype2::elastic::elastic::boundaryConditions(
  const float16_t * __restrict__                  Qinside, // Qinside[4+0]
  float16_t * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
);
#endif

#ifdef __STDCPP_BFLOAT16_T__
template void examples::exahype2::elastic::elastic::initialCondition(
  bfloat16_t * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
);
template double ::examples::exahype2::elastic::elastic::maxEigenvalue(
  const bfloat16_t* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
);
template void ::examples::exahype2::elastic::elastic::flux(
  const bfloat16_t* __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  bfloat16_t* __restrict__ F
);
template void examples::exahype2::elastic::elastic::boundaryConditions(
  const bfloat16_t * __restrict__                  Qinside, // Qinside[4+0]
  bfloat16_t * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
);
#endif