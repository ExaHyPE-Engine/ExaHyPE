#include "eulerSolver.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   exahype2::euler::eulerSolver::_log( "exahype2::euler::eulerSolver" );


/*
 * Enables the usage of shortcuts to access variables, e.g. use Q[s.rho] instead of Q[0]
 */
exahype2::euler::VariableShortcuts s;

constexpr double original_shock_position = 0.111;


void exahype2::euler::eulerSolver::initialCondition(
  double * __restrict__ Q, //Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {


  Q[s.rho] = (x[0]<-original_shock_position && x[1]<-original_shock_position) ? 0.125 : 1.0;

  Q[s.rho_u+0] = 0.0;
  Q[s.rho_u+1] = 0.0;
  Q[s.rho_u+2] = 0.0;

  constexpr double gamma = 1.4;
  const double p = (x[0]<-original_shock_position && x[1]<-original_shock_position) ? 0.1 : 1.0;

  Q[s.E] = p/(gamma-1.0);

}


void exahype2::euler::eulerSolver::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[5+0]
  double * __restrict__                        Qoutside, // Qoutside[5+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
){


  Qoutside[s.rho] = (faceCentre[0]<-original_shock_position && faceCentre[1]<-original_shock_position) ? 0.125 : 1.0;
  // Qoutside[s.rho] = 1.0;
  Qoutside[s.rho_u+0] = 0.0;  // Qinside[s.rho+0];
  Qoutside[s.rho_u+1] = 0.0;  // Qinside[s.rho+1];
  Qoutside[s.rho_u+2] = 0.0;  // Qinside[s.rho+2];
  // Qoutside[s.E] = 1.0;

  constexpr double gamma = 1.4;
  const double p = (faceCentre[0]<-original_shock_position && faceCentre[1]<-original_shock_position) ? 0.1 : 1.0;
  Qoutside[s.E] = p/(gamma-1.0);    // Qinside[s.E];

}


double ::exahype2::euler::eulerSolver::maxEigenvalue(
  const double* __restrict__ Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
){

  const double irho = 1.0/Q[s.rho];

  // based on the assumption that the fluid is an ideal gas, gamma chosen for dry air
  constexpr double gamma = 1.4;
  const double p = (gamma-1) * (Q[s.E] - 0.5*irho*(Q[s.rho_u+0]*Q[s.rho_u+0]+Q[s.rho_u+1]*Q[s.rho_u+1]+Q[s.rho_u+2]*Q[s.rho_u+2]));

  const double c   = std::sqrt(gamma * p * irho);
  // std::cout << "normal is " << normal << "\n";
  const double u = Q[normal+1]*irho;

  return std::max(std::abs(u-c), std::abs(u+c));

}


void ::exahype2::euler::eulerSolver::flux(
  const double* __restrict__ Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[5]
) { 

  const double irho = 1.0 / Q[s.rho];
  constexpr double gamma = 1.4;
  const double p = (gamma-1) * (Q[s.E] - 0.5*irho*(Q[s.rho_u+0]*Q[s.rho_u+0]+Q[s.rho_u+1]*Q[s.rho_u+1]+Q[s.rho_u+2]*Q[s.rho_u+2]));

  F[0] = Q[normal+1];
  F[1] = Q[normal+1]*Q[s.rho_u+0]*irho;
  F[2] = Q[normal+1]*Q[s.rho_u+1]*irho;
  F[3] = Q[normal+1]*Q[s.rho_u+2]*irho;
  F[4] = Q[normal+1]*irho*(Q[s.E]+p);

  F[normal+1] += p;

}

bool ::exahype2::euler::eulerSolver::isPhysicallyAdmissible(
  const double* const                         solution,
  const tarch::la::Vector<Dimensions,double>& x,
  const double timeStamp){

  // if(x[0]>0.0){
  //   return false;
  // }
  // else{
  //   return true;
  // }
  if(solution[0]<=0.0 || solution[4]<=0.0){ return false; };
  return true;

  // return true;
  // return false;

}









