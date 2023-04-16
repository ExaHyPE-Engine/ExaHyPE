#include "advection.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::tutorial::advection::_log( "examples::exahype2::tutorial::advection" );




void examples::exahype2::tutorial::advection::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {

  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = volumeCentre[i];
  }

}


double ::examples::exahype2::tutorial::advection::maxEigenvalue(
  const double * __restrict__ Q, // Q[3+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {

  return 1.0;

}


void ::examples::exahype2::tutorial::advection::flux(
  const double * __restrict__ Q, // Q[3+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[3]
)  {

  if(normal==0){
    F[0] = Q[0];
    F[1] = 0.0;
    F[2] = 0.0;
  }
  else if(normal==1){
    F[0] = 0.0;
    F[1] = Q[1];
    F[2] = 0.0;
  }
  else{
    F[0] = 0.0;
    F[1] = 0.0;
    F[2] = Q[2];
  }

}


void examples::exahype2::tutorial::advection::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[3+0]
  double * __restrict__                        Qoutside, // Qoutside[3+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {

  for(int i=0; i<3; i++){
    Qoutside[i] = Qinside[i];
  }

}


::exahype2::RefinementCommand examples::exahype2::tutorial::advection::refinementCriterion(
  const double * __restrict__ Q, // Q[3+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  return result;
}



















