#include "advection.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   exahype2::advection::advection::_log( "exahype2::advection::advection" );




::exahype2::RefinementCommand exahype2::advection::advection::refinementCriterion(
  const double * __restrict__ Q,               // Q[3+0]
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




void exahype2::advection::advection::initialCondition(
  double * __restrict__ Q, //Q[3+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {

  Q[0] = x[0];
  Q[1] = x[1];
  Q[2] = x[2];

}





void exahype2::advection::advection::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[3+0]
  double * __restrict__                        Qoutside, // Qoutside[3+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
){

  Qoutside[0] = 0.0;
  Qoutside[1] = 0.0;
  Qoutside[2] = 0.0;

}




  


double ::exahype2::advection::advection::maxEigenvalue(
  const double* __restrict__ Q, // Q[3+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
){

  return 1.0;

}




void ::exahype2::advection::advection::flux(
  const double* __restrict__ Q, // Q[3+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[3]
) {

  F[0] = 0.0;
  F[1] = 0.0;
  F[2] = 0.0;

  F[normal] = Q[normal];

}











