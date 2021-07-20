#include "EulerOnGPU.h"
#include "exahype2/RefinementControl.h"

#include "Constants.h"
#include "exahype2/NonCriticalAssertions.h"


tarch::logging::Log   examples::exahype2::euler::EulerOnGPU::_log( "examples::exahype2::euler::EulerOnGPU" );



::exahype2::RefinementCommand examples::exahype2::euler::EulerOnGPU::refinementCriterion(
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

void examples::exahype2::euler::EulerOnGPU::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructed
) {
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


#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
double examples::exahype2::euler::EulerOnGPU::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {

  // We should have a GPU assertion which is automatically removed

  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  const double u_n = Q[normal + 1] * irho;
  const double c   = std::sqrt(gamma * p * irho);

   //printf("THIS IS HAPPENING %f %f\n",std::max( std::abs(u_n - c), std::abs(u_n + c) ), Q[0]);
  return std::max( std::abs(u_n - c), std::abs(u_n + c) );
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif



#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void examples::exahype2::euler::EulerOnGPU::flux(
 const double * __restrict__ Q, // Q[5+0],
 const tarch::la::Vector<Dimensions,double>&  faceCentre,
 const tarch::la::Vector<Dimensions,double>&  volumeH,
 double                                       t,
 int                                          normal,
 double * __restrict__ F // F[5]
)
{
  constexpr double gamma = 1.4;
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

}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif


void examples::exahype2::euler::EulerOnGPU::boundaryConditions(
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
