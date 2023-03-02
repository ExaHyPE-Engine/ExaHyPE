#include "SWE.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   examples::exahype2::heat::SWE::_log( "examples::exahype2::heat::SWE" );




::exahype2::RefinementCommand examples::exahype2::heat::SWE::refinementCriterion(
  const double * __restrict__ Q,               // Q[3+0]
  const tarch::la::Vector<Dimensions,double>&  x,
	const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {

  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  return result;

}




void examples::exahype2::heat::SWE::initialCondition(
  double * __restrict__ Q, //Q[3+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {

  double x_0 = x[0]-0.5;
  x_0 = x_0*x_0;
  double y_0 = x[1]-0.5;
  y_0 = y_0*y_0;

  double distanceFromCenter=std::sqrt(x_0+y_0);

  Q[0] = (distanceFromCenter<0.1 ? 1.0 : 1.01);
  Q[1] = 0.0;
  Q[2] = 0.0;

}





void examples::exahype2::heat::SWE::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[3+0]
  double * __restrict__                        Qoutside, // Qoutside[3+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
){

  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];

}




  


double ::examples::exahype2::heat::SWE::maxEigenvalue(
  const double* __restrict__ Q, // Q[3+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
){

  double u = Q[normal+1]/Q[0];
  double c = std::sqrt(Q[0]);

  return std::max(std::abs(u-c), std::abs(u+c));
}




void ::examples::exahype2::heat::SWE::flux(
  const double* __restrict__ Q, // Q[3+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[3]
) {

  double ih = 1.0 / Q[0];

  F[0] = Q[normal+1];
  F[1] = Q[normal+1]*Q[1]*ih;
  F[2] = Q[normal+1]*Q[2]*ih;
  F[normal+1] += 0.5*Q[0]*Q[0];

}











