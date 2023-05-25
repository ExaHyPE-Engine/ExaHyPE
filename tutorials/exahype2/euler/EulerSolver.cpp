#include "EulerSolver.h"

tarch::logging::Log tutorials::exahype2::euler::EulerSolver::_log( "tutorials::exahype2::euler::EulerSolver" );

void tutorials::exahype2::euler::EulerSolver::initialCondition(
  double * __restrict__                         Q, // Q[Dimensions+2+0]
  const tarch::la::Vector<Dimensions,double>&   volumeCentre,
  const tarch::la::Vector<Dimensions,double>&   volumeH,
  bool                                          gridIsConstructed
) {
  Q[0] = 1.0;
  Q[1] = 0.0;
  Q[2] = 0.0;
#if Dimensions == 3
  Q[3] = 0.0;
#endif

#if Dimensions == 2
  tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.5};
  const double distanceFromCenter = tarch::la::norm2(volumeCentre-circleCentre);
  Q[3] = (distanceFromCenter < 0.2 ? 1.0 : 1.01);
#else
  tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.5, 0.5};
  const double distanceFromCenter = tarch::la::norm2(volumeCentre-circleCentre);
  Q[4] = (distanceFromCenter < 0.2 ? 1.0 : 1.01);
#endif
}

void tutorials::exahype2::euler::EulerSolver::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[Dimensions+2+0]
  double * __restrict__                        Qoutside, // Qoutside[Dimensions+2+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  for(int i = 0; i < NumberOfUnknowns+NumberOfAuxiliaryVariables; i++){
    Qoutside[i] = Qinside[i];
  }
}

double tutorials::exahype2::euler::EulerSolver::maxEigenvalue(
  const double* __restrict__                    Q, // Q[Dimensions+2+0]
  const tarch::la::Vector<Dimensions,double>&   faceCentre,
  const tarch::la::Vector<Dimensions,double>&   volumeSize,
  double                                        t,
  double                                        dt,
  int                                           normal
) {
  assertion(Q[0] > 0);
  const double irho = 1.0 / Q[0];

#if Dimensions == 2
  const double p = (Gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
#else
  const double p = (Gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
#endif

  assertion(p > 0);
  assertion(irho > 0);
  const double c = std::sqrt(Gamma * p * irho);
  const double u = Q[normal+1]*irho;
  return std::max(std::abs(u-c), std::abs(u+c));
}

void tutorials::exahype2::euler::EulerSolver::flux(
  const double* __restrict__                    Q, // Q[Dimensions+2+0]
  const tarch::la::Vector<Dimensions,double>&   faceCentre,
  const tarch::la::Vector<Dimensions,double>&   volumeH,
  double                                        t,
  double                                        dt,
  int                                           normal,
  double* __restrict__                          F // F[Dimensions+2]
) {
  assertion(Q[0] > 0);
  const double irho = 1.0 / Q[0];

#if Dimensions == 2
  const double p = (Gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  switch (normal) {
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
#else
  const double p = (Gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  F[0] = Q[normal+1];
  F[1] = Q[normal+1]*Q[1]*irho;
  F[2] = Q[normal+1]*Q[2]*irho;
  F[3] = Q[normal+1]*Q[3]*irho;
  F[4] = Q[normal+1]*irho*(Q[4]+p);
  F[normal+1] += p;
#endif
}

::exahype2::RefinementCommand tutorials::exahype2::euler::EulerSolver::refinementCriterion(
  const double * __restrict__                   Q, // Q[Dimensions+2+0]
  const tarch::la::Vector<Dimensions,double>&   volumeX,
  const tarch::la::Vector<Dimensions,double>&   volumeH,
  double                                        t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  return result;
}
