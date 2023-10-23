#include "eulerSolver.h"
#include "exahype2/RefinementControl.h"
#include "cmath"

tarch::logging::Log   exahype2::euler::eulerSolver::_log( "exahype2::euler::eulerSolver" );
using namespace std;

/*
 * Enables the usage of shortcuts to access variables, e.g. use Q[s.rho] instead of Q[s.rho]
 */
exahype2::euler::VariableShortcuts s;


template<typename T>
void exahype2::euler::eulerSolver::initialCondition(
  T * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {

  Q[s.rho] = 0.02*(1.0+std::exp(-0.5*tarch::la::norm2(x)/0.01));

  Q[s.rho_u+0] = 1.0*Q[s.rho];
  Q[s.rho_u+1] = 1.0*Q[s.rho];
  Q[s.rho_u+2] = 0.0;

  constexpr T gamma = 1.4;
  constexpr T p = 1.0;

  //Should lead to a constant p over the domain given the initial conditions
  Q[s.E] = p/(gamma-1.0) + 0.5*(Q[s.rho_u+0]*Q[s.rho_u+0]+Q[s.rho_u+1]*Q[s.rho_u+1]+Q[s.rho_u+2]*Q[s.rho_u+2]) / Q[s.rho];

}


template<typename T>
void exahype2::euler::eulerSolver::boundaryConditions(
  const T * __restrict__                  Qinside, // Qinside[4+0]
  T * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
){

  assert(false);

}

  
template<typename T>
double ::exahype2::euler::eulerSolver::maxEigenvalue(
  const T* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
) {

  const T irho = 1.0/Q[s.rho];

  // based on the assumption that the fluid is an ideal gas, gamma chosen for dry air
  constexpr T gamma = 1.4;
  const T p = (gamma-1) * (Q[s.E] - 0.5*irho*(Q[s.rho_u+0]*Q[s.rho_u+0]+Q[s.rho_u+1]*Q[s.rho_u+1]+Q[s.rho_u+2]*Q[s.rho_u+2]));

  const T c   = std::sqrt(gamma * p * irho);
  const T u = Q[normal+1]*irho;

  return std::max(std::abs(u-c), std::abs(u+c));

}


template<typename T>
void ::exahype2::euler::eulerSolver::flux(
  const T* __restrict__                        Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  T* __restrict__                              F // F[Dimensions*4]
){

  const T irho = 1.0 / Q[s.rho];
  constexpr T gamma = 1.4;
  const T p = (gamma-1) * (Q[s.E] - 0.5*irho*(Q[s.rho_u+0]*Q[s.rho_u+0]+Q[s.rho_u+1]*Q[s.rho_u+1]+Q[s.rho_u+2]*Q[s.rho_u+2]));

  F[0] = Q[normal+1];
  F[1] = Q[normal+1]*Q[s.rho_u+0]*irho;
  F[2] = Q[normal+1]*Q[s.rho_u+1]*irho;
  F[3] = Q[normal+1]*Q[s.rho_u+2]*irho;
  F[4] = Q[normal+1]*irho*(Q[s.E]+p);

  F[normal+1] += p;

}


template void exahype2::euler::eulerSolver::initialCondition(
  float * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
);
template double ::exahype2::euler::eulerSolver::maxEigenvalue(
  const float* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
);
template void ::exahype2::euler::eulerSolver::flux(
  const float* __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  float* __restrict__ F
);
template void exahype2::euler::eulerSolver::boundaryConditions(
  const float * __restrict__                   Qinside, // Qinside[4+0]
  float * __restrict__                         Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
);


template void exahype2::euler::eulerSolver::initialCondition(
  double * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
);
template double ::exahype2::euler::eulerSolver::maxEigenvalue(
  const double* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
);
template void ::exahype2::euler::eulerSolver::flux(
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F
);
template void exahype2::euler::eulerSolver::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[4+0]
  double * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
);

#ifdef __STDCPP_FLOAT16_T__
template void exahype2::euler::eulerSolver::initialCondition(
  float16_t * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
);
template double ::exahype2::euler::eulerSolver::maxEigenvalue(
  const float16_t* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
);
template void ::exahype2::euler::eulerSolver::flux(
  const float16_t* __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  float16_t* __restrict__ F
);
template void exahype2::euler::eulerSolver::boundaryConditions(
  const float16_t * __restrict__                  Qinside, // Qinside[4+0]
  float16_t * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
);
#endif

#ifdef __STDCPP_BFLOAT16_T__
//bfloat16
template void exahype2::euler::eulerSolver::initialCondition(
  bfloat16_t * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
);
template double ::exahype2::euler::eulerSolver::maxEigenvalue(
  const bfloat16_t* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
);
template void ::exahype2::euler::eulerSolver::flux(
  const bfloat16_t* __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  bfloat16_t* __restrict__ F
);
template void exahype2::euler::eulerSolver::boundaryConditions(
  const bfloat16_t * __restrict__                  Qinside, // Qinside[4+0]
  bfloat16_t * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
);
#endif