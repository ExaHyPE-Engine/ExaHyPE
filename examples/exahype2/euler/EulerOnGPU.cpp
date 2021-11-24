#include "EulerOnGPU.h"
#include "exahype2/RefinementControl.h"
#include "Constants.h"
#include "exahype2/NonCriticalAssertions.h"


tarch::logging::Log   examples::exahype2::euler::EulerOnGPU::_log( "examples::exahype2::euler::EulerOnGPU" );



enum class Scenario {
  PointExplosion,
  PointExplosionWithDynamicAMR,
  BreakingDam,
  BreakingDamWithDynamicAMR,
  BreakingDamResolutionStudies
};


//Scenario scenario = Scenario::BreakingDamResolutionStudies;
Scenario scenario = Scenario::BreakingDamWithDynamicAMR;

//Scenario scenario = Scenario::BreakingDam;


::exahype2::RefinementCommand examples::exahype2::euler::EulerOnGPU::refinementCriterion(
  const double * __restrict__ Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  switch (scenario) {
    case Scenario::PointExplosion:
      {
        // This is an example how to implement a priori refinement. If
        // you only have this thing, then you work with static AMR, as
        // you never invoke erase.
        if ( tarch::la::equals(t,0.0) ) {
          tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.3};
          bool isInTheCentre = ( tarch::la::norm2( volumeX-circleCentre ) < 0.05 );
          if (isInTheCentre)
            result = ::exahype2::RefinementCommand::Refine;
        }
      }
      break;
    case Scenario::PointExplosionWithDynamicAMR:
      {
        // Same as for static AMR. This is the initial pattern
        if ( tarch::la::equals(t,0.0) ) {
          tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.3};
          bool isInTheCentre = ( tarch::la::norm2( volumeX-circleCentre ) < 0.05 );
          if (isInTheCentre)
            result = ::exahype2::RefinementCommand::Refine;
        }

        // This is an example how to implement dynamic AMR, as the
        // AMR instruction depends on the actual solution (which is
        // not directly available throughout the grid construction).
        // If you remove this part, you get static AMR.
        if ( tarch::la::greater(t,0.0) and t<0.0001 ) {
          if ( Q[4]>0.4 ) {
            result = ::exahype2::RefinementCommand::Refine;
          }
          if ( Q[4]<0.2 ) {
            result = ::exahype2::RefinementCommand::Erase;
          }
        }
      }
      break;
    case Scenario::BreakingDam:
      {
        if ( volumeX(0)<0.5 ) {
          result = ::exahype2::RefinementCommand::Refine;
        }
      }
      break;
    case Scenario::BreakingDamWithDynamicAMR:
      {
        if ( tarch::la::equals(t,0.0) and tarch::la::equals(volumeX(0),1.0/3.0) ) {
          result = ::exahype2::RefinementCommand::Refine;
        }
        else if (t>0.0 and Q[4]>0.4) {
          result = ::exahype2::RefinementCommand::Refine;
        }
        else if (t>0.0 and Q[4]<0.2) {
          result = ::exahype2::RefinementCommand::Erase;
        }
      }
      break;
    case Scenario::BreakingDamResolutionStudies:
      {
        if (
          volumeX(1)>0.5-0.5*volumeH(1)*NumberOfFiniteVolumesPerAxisPerPatch
          and
          volumeX(1)<0.5+0.5*volumeH(1)*NumberOfFiniteVolumesPerAxisPerPatch
        ) {
          result = ::exahype2::RefinementCommand::Refine;
        }
      }
      break;
  }

  return result;
}


void examples::exahype2::euler::EulerOnGPU::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  switch (scenario) {
    case Scenario::PointExplosion:
    case Scenario::PointExplosionWithDynamicAMR:
      {
        logDebug( "initialCondition(...)", "set point explosion initial condition" );
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
      break;
    case Scenario::BreakingDam:
    case Scenario::BreakingDamWithDynamicAMR:
    case Scenario::BreakingDamResolutionStudies:
      {
        logDebug( "initialCondition(...)", "set breaking dam initial condition" );
        Q[0] = 0.1;  // rho
        Q[1] = 0;    // velocities
        Q[2] = 0;
        Q[3] = 0;
        Q[4] = volumeX(0)<0.5 ? 1.0 : 0.0; // inner energy
      }
      break;
  }
}


void examples::exahype2::euler::EulerOnGPU::boundaryConditions(
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

  nonCriticalAssertion5( Qinside[0]>1e-12, faceCentre, volumeH, t, normal, Qinside[0] );
  assertion5( Qinside[0]>1e-12, faceCentre, volumeH, t, normal, Qinside[0] );

  Qoutside[0] = Qinside[0];
  Qoutside[1] = Qinside[1];
  Qoutside[2] = Qinside[2];
  Qoutside[3] = Qinside[3];
  Qoutside[4] = Qinside[4];

  logTraceOut( "boundaryConditions(...)" );
}


double ::examples::exahype2::euler::EulerOnGPU::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );
  double result = maxEigenvalue( Q, faceCentre, volumeH, t, normal, Offloadable::Yes );
  logTraceOutWith1Argument( "maxEigenvalue(...)", result );
  return result;
}


void ::examples::exahype2::euler::EulerOnGPU::flux(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ F // F[5]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
  flux( Q, faceCentre, volumeH, t, normal, F, Offloadable::Yes );
  logTraceOut( "flux(...)" );
}



    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
/**
 * The big difference between the normal Euler and the GPU version is
 * that the PDE terms are stateless (static). So we have always two
 * versions of the PDE terms: one with state and one without.
 *
 * To avoid code duplication, we let the non-static versions of the terms
 * call the static ones. In our case, they do the same. But in principle,
 * you can have different ones if something "weird"/special happens along
 * resolution boundaries, e.g., or in certain subdomains.
 *
 * Now, our static flux function and eigenvalue function does not differ
 * from the non-static one of the standard Euler. It only lacks some
 * logging and assertions, as these features are not available on the GPU.
 */
double ::examples::exahype2::euler::EulerOnGPU::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  Offloadable
)  {
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
  return result;
}
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif




    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
void ::examples::exahype2::euler::EulerOnGPU::flux(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ F,
  Offloadable
)  {
  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  nonCriticalAssertion( F[0]==F[0] );
  nonCriticalAssertion( F[1]==F[1] );
  nonCriticalAssertion( F[2]==F[2] );
  nonCriticalAssertion( F[3]==F[3] );
  nonCriticalAssertion( F[4]==F[4] );

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







