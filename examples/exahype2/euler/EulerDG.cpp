#include "EulerDG.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::euler::EulerDG::_log( "examples::exahype2::euler::EulerDG" );


void examples::exahype2::euler::EulerDG::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
  bool                                         gridIsConstructred
) {
  logTraceInWith2Arguments( "initialCondition(...)", x, gridIsConstructred );

  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  #if Dimensions==3
  tarch::la::Vector<Dimensions,double> circleCentre = {0.18,0.3,0.6};
  #else
  tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.3};
  #endif

  const double peakDeviation = MaxAdmissibleCellH;
  double distance = tarch::la::norm2( volumeCentre-circleCentre );
  double exponent = - (distance * distance) / 2.0 / peakDeviation / peakDeviation;

  Q[0]            = 0.1;                  // rho
  Q[1+Dimensions] = std::exp( exponent ); // inner energy

  logTraceOut( "initialCondition(...)" );
}


void examples::exahype2::euler::EulerDG::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[4+0]
  double * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
) {
  logTraceInWith3Arguments( "boundaryConditions(...)", x, t, normal );

  nonCriticalAssertion5( Qinside[0]>1e-12, faceCentre, volumeH, t, normal, Qinside[0] );
  assertion5( Qinside[0]>1e-12, faceCentre, volumeH, t, normal, Qinside[0] );

  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }

  logTraceOut( "boundaryConditions(...)" );
}


double ::examples::exahype2::euler::EulerDG::maxEigenvalue(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", x, t, dt, normal );

  assertion(normal>=0);
  assertion(normal<Dimensions);

  if (Q[0]<=0.0 or Q[0]!=Q[0]) {
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "Q[0]>0", "density negative" );
  }

  constexpr double gamma = 1.4;
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

  logTraceOutWith1Argument( "maxEigenvalue(...)", result );
  return result;
}


void ::examples::exahype2::euler::EulerDG::flux(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[4]
)  {
  logTraceInWith4Arguments( "flux(...)", x, t, dt, normal );

  assertion4( normal>=0, x, t, dt, normal );
  assertion4( normal<Dimensions, x, t, dt, normal);

  constexpr double gamma = 1.4;
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

  logTraceOut( "flux(...)" );
}

