#include "Poisson.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   examples::exahype2::selfgravitation::Poisson::_log( "examples::exahype2::selfgravitation::Poisson" );







void examples::exahype2::selfgravitation::Poisson::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments( "initialCondition(...)", volumeCentre, volumeH, gridIsConstructred );

  // Trivial initial conditions
  Q[0] = 0.0;
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = 0.0;

  logTraceOut( "initialCondition(...)" );
}





void examples::exahype2::selfgravitation::Poisson::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[4+0]
  double * __restrict__                        Qoutside, // Qoutside[4+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );

  // Neumann conditions
  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];

  logTraceOut( "boundaryConditions(...)" );
}





double ::examples::exahype2::selfgravitation::Poisson::maxEigenvalue(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );

  // @todo needs cross-checking
  const double result = 4.0 * tarch::la::PI;

  logTraceOutWith1Argument( "maxEigenvalue(...)", result );
  return result;
}




void ::examples::exahype2::selfgravitation::Poisson::flux(
  const double * __restrict__ Q, // Q[4+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[4]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );

  const double T_tau = 1.0 / 4.0 / tarch::la::PI/tarch::la::PI;
  F[0] = Q[normal+1];
  F[1] = normal==0 ? -Q[0]/T_tau : 0.0;
  F[2] = normal==1 ? -Q[0]/T_tau : 0.0;
  F[3] = normal==2 ? -Q[0]/T_tau : 0.0;

  logTraceOut( "flux(...)" );
}







void ::examples::exahype2::selfgravitation::Poisson::sourceTerm(
  const double * __restrict__                  Q, // Q[4+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[4
) {
  logTraceInWith4Arguments( "sourceTerm(...)", volumeX, volumeH, t, dt );

  const double T_tau = 1.0 / 4.0 / tarch::la::PI/tarch::la::PI;
  const double G     = 6.67e-11;
  const double rho   = 0.1; // @todo coupling term. This one comes 1:1 from the other solver, whereas I need the gradient (vector) for the source term of the other solver.

  S[0] = - tarch::la::PI * G / rho;
  S[1] = -Q[1]/T_tau;
  S[1] = -Q[2]/T_tau;
  S[1] = -Q[3]/T_tau;

  logTraceOut( "sourceTerm(...)" );
}
















