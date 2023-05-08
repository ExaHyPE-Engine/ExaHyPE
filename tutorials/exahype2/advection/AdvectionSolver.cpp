#include "AdvectionSolver.h"

tarch::logging::Log tutorials::exahype2::advection::AdvectionSolver::_log( "tutorials::exahype2::advection::AdvectionSolver" );

void tutorials::exahype2::advection::AdvectionSolver::initialCondition(
  double * __restrict__                         Q,
  const tarch::la::Vector<Dimensions,double>&   volumeCentre,
  const tarch::la::Vector<Dimensions,double>&   volumeH,
  bool                                          gridIsConstructred
) {
  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = volumeCentre[i];
  }
}

void tutorials::exahype2::advection::AdvectionSolver::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[3+0]
  double * __restrict__                        Qoutside, // Qoutside[3+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}

double tutorials::exahype2::advection::AdvectionSolver::maxEigenvalue(
  const double * __restrict__                   Q, // Q[3+0],
  const tarch::la::Vector<Dimensions,double>&   faceCentre,
  const tarch::la::Vector<Dimensions,double>&   volumeH,
  double                                        t,
  double                                        dt,
  int                                           normal
) {
  return 1.0;
}

void tutorials::exahype2::advection::AdvectionSolver::flux(
  const double * __restrict__                   Q, // Q[3+0],
  const tarch::la::Vector<Dimensions,double>&   faceCentre,
  const tarch::la::Vector<Dimensions,double>&   volumeH,
  double                                        t,
  double                                        dt,
  int                                           normal,
  double * __restrict__                         F // F[3]
) {
  if (normal == 0) {
    F[0] = Q[0];
    F[1] = 0.0;
    F[2] = 0.0;
  } else if (normal == 1) {
    F[0] = 0.0;
    F[1] = Q[1];
    F[2] = 0.0;
  } else {
    F[0] = 0.0;
    F[1] = 0.0;
    F[2] = Q[2];
  }
}

::exahype2::RefinementCommand tutorials::exahype2::advection::AdvectionSolver::refinementCriterion(
  const double * __restrict__                   Q, // Q[3+0]
  const tarch::la::Vector<Dimensions,double>&   volumeX,
  const tarch::la::Vector<Dimensions,double>&   volumeH,
  double                                        t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  return result;
}
