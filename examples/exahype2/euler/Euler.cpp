#include "Euler.h"
#include "exahype2/RefinementControl.h"

#include "Constants.h"
#include "exahype2/NonCriticalAssertions.h"


tarch::logging::Log   examples::exahype2::euler::Euler::_log( "examples::exahype2::euler::Euler" );



enum class Scenario {
  PointExplosion,
  PointExplosionWithDynamicAMR,
  BreakingDam,
  BreakingDamWithDynamicAMR,
  BreakingDamResolutionStudies
};


//Scenario scenario = Scenario::BreakingDamResolutionStudies;
Scenario scenario = Scenario::BreakingDamWithDynamicAMR;

// Scenario scenario = Scenario::BreakingDam;



::exahype2::RefinementCommand examples::exahype2::euler::Euler::refinementCriterion(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
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
          bool isInTheCentre = ( tarch::la::norm2( volumeCentre-circleCentre ) < 0.05 );
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
          bool isInTheCentre = ( tarch::la::norm2( volumeCentre-circleCentre ) < 0.05 );
          if (isInTheCentre)
            result = ::exahype2::RefinementCommand::Refine;
        }

        // This is an example how to implement dynamic AMR, as the
        // AMR instruction depends on the actual solution (which is
        // not directly available throughout the grid construction).
        // If you remove this part, you get static AMR.
        if ( tarch::la::greater(t,0.0) ) {
          if ( Q[4]>0.4 ) {
            result = ::exahype2::RefinementCommand::Refine;
          }
          if ( Q[4]<0.2 ) {
            result = ::exahype2::RefinementCommand::Coarsen;
          }
        }
      }
      break;
    case Scenario::BreakingDam:
      {
        if ( volumeCentre(0)<0.5 ) {
          result = ::exahype2::RefinementCommand::Refine;
        }
      }
      break;
    case Scenario::BreakingDamWithDynamicAMR:
      {
        if ( tarch::la::equals(t,0.0) and tarch::la::equals(volumeCentre(0),1.0/3.0) ) {
          result = ::exahype2::RefinementCommand::Refine;
        }
        else if (t>0.0 and Q[4]>0.4) {
          result = ::exahype2::RefinementCommand::Refine;
        }
        else if (t>0.0 and Q[4]<0.2) {
          result = ::exahype2::RefinementCommand::Coarsen;
        }
      }
      break;
    case Scenario::BreakingDamResolutionStudies:
      {
        if (
          volumeCentre(1)>0.5-0.5*volumeH(1)*NumberOfFiniteVolumesPerAxisPerPatch
          and
          volumeCentre(1)<0.5+0.5*volumeH(1)*NumberOfFiniteVolumesPerAxisPerPatch
        ) {
          result = ::exahype2::RefinementCommand::Refine;
        }
      }
      break;
  }

  return result;
}

void examples::exahype2::euler::Euler::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructed
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
        bool isInTheCentre = ( tarch::la::norm2( volumeCentre-circleCentre ) < 0.05 );
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
        Q[4] = volumeCentre(0)<0.5 ? 1.0 : 0.0; // inner energy
      }
      break;
  }
}


double examples::exahype2::euler::Euler::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  assertion(normal>=0);
  assertion(normal<Dimensions);
  //assertion( Q[0]>0.0 );

  if (Q[0]<=0.0 or Q[0]!=Q[0])
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "Q[0]>0", "density negative" );

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


void examples::exahype2::euler::Euler::flux(
 const double * __restrict__ Q, // Q[5+0],
 const tarch::la::Vector<Dimensions,double>&  faceCentre,
 const tarch::la::Vector<Dimensions,double>&  volumeH,
 double                                       t,
 int                                          normal,
 double * __restrict__ F // F[5]
)
{
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
  assertion4( normal>=0, faceCentre, volumeH, t, normal );
  assertion4( normal<Dimensions, faceCentre, volumeH, t, normal);
  nonCriticalAssertion9( Q[0]==Q[0], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  nonCriticalAssertion9( Q[1]==Q[1], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  nonCriticalAssertion9( Q[2]==Q[2], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  nonCriticalAssertion9( Q[3]==Q[3], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );
  nonCriticalAssertion9( Q[4]==Q[4], Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, volumeH, t, normal );

  if (Q[0]<=1e-12  or Q[0]!=Q[0]) {
    std::ostringstream msg;
    msg << "density is negative"
        << ".faceCentre=" << faceCentre
        << ",volumeH=" << volumeH
        << ",normal=" << normal
        << ",Q[0]=" << Q[0];
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "Q[0]>0", msg.str() );
    assertion(false);
  }

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

  logTraceOutWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
}




void examples::exahype2::euler::Euler::boundaryConditions(
  const double * __restrict__                  Qinside,    // Qinside[5+0]
  double * __restrict__                        Qoutside,   // Qoutside[5+0]
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




#if defined(OpenMPGPUOffloading)
/**
 * Please ignore everything below if you don't have GPUs
 *
 *
 * This is a lot of code duplication here. Once we enable GPUs, we need a
 * version of the flux and eigenvalue which is a classic member function
 * of the class (as we have it without the GPU support) and we need a
 * version of these routines that does not have a state, i.e. is static.
 *
 * Some explanations for this are given in the autogenerated abstract
 * solver once you switch on GPUs.
 *
 * Now, our static flux function and eigenvalue function does not differ
 * from the non-static one. It thus would be convenient to write the
 * static flavour once and then to make the standard, non-static version
 * call the static one. Indeed, I'd do so in a real application.
 *
 * This however is our standard introductory example where we try to make
 * concepts to users as clear as possible, and it is not clear immediately
 * why you need different versions of one routine. So I prefer to replicate
 * code here. As pointed out: You likely would not do this in a proper app.
 */
#pragma omp declare target
double examples::exahype2::euler::Euler::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  Offloadable
) {
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
#pragma omp end declare target


#pragma omp declare target
void examples::exahype2::euler::Euler::flux(
 const double * __restrict__ Q, // Q[5+0],
 const tarch::la::Vector<Dimensions,double>&  faceCentre,
 const tarch::la::Vector<Dimensions,double>&  volumeH,
 double                                       t,
 int                                          normal,
 double * __restrict__ F, // F[5],
 Offloadable
) {
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
#pragma omp end declare target

#endif
