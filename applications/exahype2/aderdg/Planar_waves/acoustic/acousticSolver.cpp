#include "acousticSolver.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   exahype2::acoustic::acousticSolver::_log( "exahype2::acoustic::acousticSolver" );

constexpr double rho = 1.0;
constexpr double K_0 = 4.0;
constexpr double inv_rho = 1.0/rho;
constexpr double c = std::sqrt(K_0*inv_rho);

// void exahype2::acoustic::acousticSolver::analyticalSolution(
//   const tarch::la::Vector<Dimensions,double>&   x,
//   const double                                  t,
//   const double                                  dt,
//   double*                                       sol
// ){

//   double k0 = 1.0;
//   double k1 = 1.0;
//   double k2 = 1.0;

//   double w = 6*M_PI/std::sqrt(3.0);

//   double val = cos( w*t - std::numbers::pi*( 
//     k0*x[0]
//     + k1*x[1]
// #if Dimensions==3
//     + k2*x[2]
// #endif
//   ));

//   sol[0] = (k0+k1+k2)*c*val;
//   sol[1] = k0*val;
//   sol[2] = k1*val;
//   sol[3] = k2*val;

// }


template<typename T>
void exahype2::acoustic::acousticSolver::initialCondition(
  T * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {

  T k0 = 1.0;
  T k1 = 1.0;
  T k2 = 1.0;

  T val = cos( -std::numbers::pi*( 
    k0*x[0]
    + k1*x[1]
#if Dimensions==3
    + k2*x[2]
#endif
  ));

  Q[0] = (k0+k1+k2)*c*val;
  Q[1] = k0*val;
  Q[2] = k1*val;
  Q[3] = k2*val;

}

template void exahype2::acoustic::acousticSolver::initialCondition(
  float * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
);
template void exahype2::acoustic::acousticSolver::initialCondition(
  double * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
);


template<typename T>
void exahype2::acoustic::acousticSolver::boundaryConditions(
  const T * __restrict__                  Qinside, // Qinside[4+0]
  T * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
){

  // Do not matter, we have periodic boundary conditions
  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];

}

template void exahype2::acoustic::acousticSolver::boundaryConditions(
  const float * __restrict__                   Qinside, // Qinside[4+0]
  float * __restrict__                         Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
);

template void exahype2::acoustic::acousticSolver::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[4+0]
  double * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
);




  
template<typename T>
double ::exahype2::acoustic::acousticSolver::maxEigenvalue(
  const T* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
) {

  return c;

}


template double ::exahype2::acoustic::acousticSolver::maxEigenvalue(
  const float* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
);

template double ::exahype2::acoustic::acousticSolver::maxEigenvalue(
  const double* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
);


template<typename T>
void ::exahype2::acoustic::acousticSolver::flux(
  const T* __restrict__                        Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  T* __restrict__                              F // F[Dimensions*4]
){

  F[0] = K_0 * Q[normal+1];
  F[1] = 0.0;
  F[2] = 0.0;
  F[3] = 0.0;

  F[normal+1] = inv_rho * Q[0];

}


template void ::exahype2::acoustic::acousticSolver::flux(
  const float* __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  float* __restrict__ F
);

template void ::exahype2::acoustic::acousticSolver::flux(
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F
);








