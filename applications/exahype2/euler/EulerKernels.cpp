#include "EulerKernels.h"
#include "exahype2/NonCriticalAssertions.h"


void applications::exahype2::euler::NeumannBoundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[5+0]
  double * __restrict__                        Qoutside, // Qoutside[5+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  int                                          numberOfUnknowns,
  int                                          numberOfAuxiliaryVariables
) {
  nonCriticalAssertion5( Qinside[0]>1e-12, faceCentre, volumeH, t, normal, Qinside[0] );

  for (int i=0; i<numberOfUnknowns+numberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}


double ::applications::exahype2::euler::maxEigenvalue(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  int                                          numberOfUnknowns,
  int                                          numberOfAuxiliaryVariables,
  double                                       gamma
)  {
  assertion(normal>=0);
  assertion(normal<Dimensions);

  if (Q[0]<=0.0 or Q[0]!=Q[0]) {
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "Q[0]>0", "density negative (Q[0]=" + std::to_string( Q[0] ) + ") at x=" + toString(faceCentre) );
    assertion(false);
  }

  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  nonCriticalAssertion9( p>=0.0, Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  const double c   = std::sqrt(gamma * p * irho);

  const double u_n = Q[normal + 1] * irho;
  double result = std::max( std::abs(u_n - c), std::abs(u_n + c) );
  nonCriticalAssertion9( result>=0.0, result, p, u_n, irho, c, faceCentre, volumeH, t, normal );

  return result;
}


void ::applications::exahype2::euler::flux(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  int                                          numberOfUnknowns,
  int                                          numberOfAuxiliaryVariables,
  double * __restrict__                        F,
  double                                       gamma
)  {
  assertion4( normal>=0, faceCentre, volumeH, t, normal );
  assertion4( normal<Dimensions, faceCentre, volumeH, t, normal);

  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[3] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  const double coeff = irho*Q[normal+1];
  F[0] = coeff*Q[0];
  F[1] = coeff*Q[1];
  F[2] = coeff*Q[2];
  F[normal+1] += p;

  #if Dimensions==3
  F[3] = coeff*Q[3];
  F[4] = coeff*Q[4] + coeff*p;
  #else
  F[3] = coeff*Q[3] + coeff*p;
  #endif
}

