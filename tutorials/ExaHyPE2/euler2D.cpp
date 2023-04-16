#include "euler2D.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   examples::exahype2::tutorial::euler2D::_log( "examples::exahype2::tutorial::euler2D" );




void examples::exahype2::tutorial::euler2D::initialCondition(
  double * __restrict__ Q, //Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {

  Q[0] = 1.0;
  Q[1] = 0.0;
  Q[2] = 0.0;

  tarch::la::Vector<Dimensions,double> circleCentre = {0.5, 0.5};
  double distanceFromCenter=tarch::la::norm2(x-circleCentre);

  Q[3] = (distanceFromCenter<0.2 ? 1.0 : 1.01);

}


void examples::exahype2::tutorial::euler2D::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[4+0]
  double * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
){

  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];

}




  


double ::examples::exahype2::tutorial::euler2D::maxEigenvalue(
  const double* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
){

  const double irho = 1.0/Q[0];

  // based on the assumption that the fluid is an ideal gas, gamma chosen for dry air
  const double gamma = 1.4;
  const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  const double c   = std::sqrt(gamma * p * irho);

  const double u = Q[normal+1]*irho;

  return std::max(std::abs(u-c), std::abs(u+c));

}




void ::examples::exahype2::tutorial::euler2D::flux(
  const double* __restrict__ Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double* __restrict__ F // F[4]
) {

  double irho = 1.0 / Q[0];
  const double gamma = 1.4;
  const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  switch(normal){
    case 0:
      F[0] = Q[1];
      F[1] = Q[1]*Q[1]*irho+p;
      F[2] = Q[1]*Q[2]*irho;
      F[3] = Q[1]*irho*(Q[3]+p);
      break;
    case 1:
      F[0] = Q[2];
      F[1] = Q[1]*Q[2]*irho;
      F[2] = Q[2]*Q[2]*irho+p;
      F[3] = Q[2]*irho*(Q[3]+p);
      break;
  }


}


::exahype2::RefinementCommand examples::exahype2::tutorial::euler2D::refinementCriterion(
  const double * __restrict__ Q,               // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
	const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  return result;
}









