#include "Euler.h"
#include "exahype2/RefinementControl.h"

#include "Constants.h"
#include "exahype2/NonCriticalAssertions.h"


tarch::logging::Log   examples::exahype2::euler::Euler::_log( "examples::exahype2::euler::Euler" );



::exahype2::RefinementCommand examples::exahype2::euler::Euler::refinementCriterion(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;
  if ( Q[4]>0.4 ) {
    result = ::exahype2::RefinementCommand::Refine;
  }
  if ( Q[4]<0.2 ) {
    result = ::exahype2::RefinementCommand::Coarsen;
  }
  return result;
}

void examples::exahype2::euler::Euler::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructed
) {
  logDebug( "initialCondition(...)", "init volume at " << volumeCentre << "x" << volumeH << " (grid constructed=" << gridIsConstructred << ")" );

  // Manual offset to make the wave originate slightly to the left of the center --- helps
  // to detect if wave is moving to the left or right
  #if Dimensions==2
  tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.3};
  #else
  tarch::la::Vector<Dimensions,double> circleCentre = {0.18,0.3,0.6};
  #endif

  // initial conditions
  bool isInTheCentre = ( tarch::la::norm2( volumeX-circleCentre ) < 0.05 );
  Q[0] = 0.1;  // rho
  Q[1] = 0;    // velocities
  Q[2] = 0;
  Q[3] = 0;
  Q[4] = isInTheCentre ? 1.0 : 0.0; // inner energy
}

double examples::exahype2::euler::Euler::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
   return examples::exahype2::euler::Euler::maxEigenvalue(Q, faceCentre, volumeH, t, normal, Offloadable::yes);
}


#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
double examples::exahype2::euler::Euler::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  Offloadable
) {

#if ! defined(OpenMPGPUOffloading)
  // We should have a GPU assertion which is automatically removed
  assertion(normal>=0);
  assertion(normal<Dimensions);
  assertion( Q[0]>0.0 );
#endif

  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  const double u_n = Q[normal + 1] * irho;
  const double c   = std::sqrt(gamma * p * irho);

  double result = std::max( std::abs(u_n - c), std::abs(u_n + c) );
#if ! defined(OpenMPGPUOffloading)
  assertion10( result>=0.0, result, p, u_n, irho, c, Q[0], Q[1], Q[2], Q[3], Q[4] );
#endif
  return result;
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif


void examples::exahype2::euler::Euler::flux(
 const double * __restrict__ Q, // Q[5+0],
 const tarch::la::Vector<Dimensions,double>&  faceCentre,
 const tarch::la::Vector<Dimensions,double>&  volumeH,
 double                                       t,
 int                                          normal,
 double * __restrict__ F // F[5]
)
{
 examples::exahype2::euler::Euler::flux( Q, faceCentre,  volumeH, t, normal, F, Offloadable::yes);
}

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void examples::exahype2::euler::Euler::flux(
 const double * __restrict__ Q, // Q[5+0],
 const tarch::la::Vector<Dimensions,double>&  faceCentre,
 const tarch::la::Vector<Dimensions,double>&  volumeH,
 double                                       t,
 int                                          normal,
 double * __restrict__ F, // F[5],
 Offloadable
)
{

#if ! defined(OpenMPGPUOffloading)
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
  assertion4( normal>=0, faceCentre, volumeH, t, normal );
  assertion4( normal<Dimensions, faceCentre, volumeH, t, normal);
  nonCriticalAssertion9( Q[0]==Q[0], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  nonCriticalAssertion9( Q[1]==Q[1], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  nonCriticalAssertion9( Q[2]==Q[2], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  nonCriticalAssertion9( Q[3]==Q[3], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  nonCriticalAssertion9( Q[4]==Q[4], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  nonCriticalAssertion9( Q[0]>1e-12, Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
#endif


  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

#if ! defined(OpenMPGPUOffloading)
  nonCriticalAssertion( F[0]==F[0] );
  nonCriticalAssertion( F[1]==F[1] );
  nonCriticalAssertion( F[2]==F[2] );
  nonCriticalAssertion( F[3]==F[3] );
  nonCriticalAssertion( F[4]==F[4] );
#endif


  const double coeff = irho*Q[normal+1];
  F[0] = coeff*Q[0];
  F[1] = coeff*Q[1];
  F[2] = coeff*Q[2];
  F[3] = coeff*Q[3];
  F[4] = coeff*Q[4];
  F[normal+1] += p;
  F[4]        += coeff*p;

#if ! defined(OpenMPGPUOffloading)
  logTraceOutWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
#endif
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif


void examples::exahype2::euler::Euler::boundaryConditions(
  const double * __restrict__                  Qinside,    // Qinside[5+0]
  double * __restrict__                        Qoutside,   // Qoutside[5+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
)
{

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
