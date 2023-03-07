#include "acousticSolver.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   exahype2::acoustic::acousticSolver::_log( "exahype2::acoustic::acousticSolver" );




::exahype2::RefinementCommand exahype2::acoustic::acousticSolver::refinementCriterion(
  const double * __restrict__ Q,               // Q[4+0]
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




void exahype2::acoustic::acousticSolver::initialCondition(
  double * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {

  Q[0] = 0.5*(sqrt(x[0])+sqrt(x[1]));
  Q[1] = 0.005*( sqrt(x[0]) + sqrt(x[1]) ); //vx
  Q[2] = Q[1]; //vy
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

  Qoutside[0] = 0.5*( sqrt(faceCentre[0]) + sqrt(faceCentre[1]) ) -10*t;
  Qoutside[1] = 0.005*(faceCentre[0]+faceCentre[1]) - 0.05*sqrt(2)*t;
  Qoutside[2] = Qoutside[1];
  Qoutside[3] = 0.0;

}




  


double ::exahype2::acoustic::acousticSolver::maxEigenvalue(
  const double* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
){

  return 100;

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

  double rho = 10.0;
//  double c = 100.0;
  double K_0 = 1000; //rho*c*c;

  F[0] = K_0 * Q[normal+1];
  F[1] = 0.0;
  F[2] = 0.0;
  F[3] = 0.0;

  F[normal+1] = (1/rho) * Q[0];

}











