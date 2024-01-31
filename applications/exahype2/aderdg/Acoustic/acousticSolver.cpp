#include "acousticSolver.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   exahype2::acoustic::acousticSolver::_log( "exahype2::acoustic::acousticSolver" );


constexpr double rho = 1.0;
constexpr double K_0 = 10.0;
constexpr double inv_rho = 1.0/rho;
constexpr double c = std::sqrt(K_0*inv_rho);


void exahype2::acoustic::acousticSolver::initialCondition(
  double * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {

  Q[0] = 1.0+0.1*tarch::la::norm2(x);
  Q[1] = 0.0; //vx
  Q[2] = 0.0; //vy
  Q[3] = 0.0; //vz

}





void exahype2::acoustic::acousticSolver::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[4+0]
  double * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
){

  Qoutside[0] = Qinside[0];
  Qoutside[1] = -Qoutside[1];
  Qoutside[2] = -Qoutside[2];
  Qoutside[3] = -Qoutside[3];

}




  


double ::exahype2::acoustic::acousticSolver::maxEigenvalue(
  const double* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
){

  return c;

}




void ::exahype2::acoustic::acousticSolver::flux(
  const double* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[4]
) {

  F[0] = K_0 * Q[normal+1];
  F[1] = 0.0;
  F[2] = 0.0;
  F[3] = 0.0;

  F[normal+1] = inv_rho * Q[0];

}











