#include "SSInfall.h"
#include "exahype2/RefinementControl.h"

#include "Constants.h"
#include "exahype2/NonCriticalAssertions.h"

tarch::logging::Log   examples::exahype2::SSInfall::SSInfall::_log( "examples::exahype2::SSInfall::SSInfall" );




::exahype2::RefinementCommand examples::exahype2::SSInfall::SSInfall::refinementCriterion(
  const double * __restrict__ Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {

  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  return result;
}



void examples::exahype2::SSInfall::SSInfall::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  logDebug( "initialCondition(...)", "init volume at " << volumeCentre << "x" << volumeH << " (grid constructed=" << gridIsConstructred << ")" );

  // Manual offset to make the wave originate slightly to the left of the center --- helps
  // to detect if wave is moving to the left or right
  #if Dimensions==2
  tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.3};
  #else
  tarch::la::Vector<Dimensions,double> circleCentre = {0,0,0};
  #endif

  // initial conditions
  bool isInTheCentre = ( tarch::la::norm2( volumeCentre-circleCentre ) < 0.05 );
  Q[0] = 0.1;  // rho
  Q[1] = 0;    // velocities
  Q[2] = 0;
  Q[3] = 0;
  Q[4] = isInTheCentre ? 1.0 : 0.0; // inner energy
  //Q[4] = 0.5; // inner energy
}


void examples::exahype2::SSInfall::SSInfall::sourceTerm(
  const double * __restrict__                  Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[5
) {
  logTraceInWith4Arguments( "sourceTerm(...)", volumeX, volumeH, t, dt );
  // @todo implement
  logTraceOut( "sourceTerm(...)" );
  S[0] = 0;  // rho
  S[1] = 0;    // velocities
  S[2] = 0;
  S[3] = 0;
  S[4] = 0;
}


void examples::exahype2::SSInfall::SSInfall::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[5+0]
  double * __restrict__                        Qoutside, // Qoutside[5+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );
  nonCriticalAssertion4( Qinside[0]==Qinside[0], faceCentre, volumeH, t, normal );
  nonCriticalAssertion4( Qinside[1]==Qinside[1], faceCentre, volumeH, t, normal );
  nonCriticalAssertion4( Qinside[2]==Qinside[2], faceCentre, volumeH, t, normal );
  nonCriticalAssertion4( Qinside[3]==Qinside[3], faceCentre, volumeH, t, normal );
  nonCriticalAssertion4( Qinside[4]==Qinside[4], faceCentre, volumeH, t, normal );

  //nonCriticalAssertion4( Qinside[0]>1e-12, faceCentre, volumeH, t, normal );
  nonCriticalAssertion5( Qinside[0]>1e-12, faceCentre, volumeH, t, normal, Qinside[0] );


  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
  Qoutside[4] = Qinside[4];

  logTraceOut( "boundaryConditions(...)" );
}


double examples::exahype2::SSInfall::SSInfall::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
)  {
  assertion(normal>=0);
  assertion(normal<Dimensions);
  assertion( Q[0]>0.0 );

  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  nonCriticalAssertion9( p>=0.0, Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  const double c   = std::sqrt(gamma * p * irho);

  const double u_n = Q[normal + 1] * irho;
  double result = std::max( std::abs(u_n - c), std::abs(u_n + c) );
  nonCriticalAssertion14( result>=0.0, result, p, u_n, irho, c, Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  return result;
}



void examples::exahype2::SSInfall::SSInfall::flux(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ F // F[5]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );

  constexpr double gamma = 5.0/3.0;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  const double coeff = irho*Q[normal+1];
  F[0] = coeff*Q[0];
  F[1] = coeff*Q[1];
  F[2] = coeff*Q[2];
  F[3] = coeff*Q[3];
  F[4] = coeff*Q[4];
  F[normal+1] += p;
  F[4]        += coeff*p;

  logTraceOutWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
}

void examples::exahype2::SSInfall::SSInfall::add_mass(
  const double r_coor,
  const double rho,
  const double size
) {
  double m=rho*pow(size,3);

  if (r_coor<r_s[0]){
    m_tot+=m;
    //std::cout << m_tot << std::endl;
  }


}


















