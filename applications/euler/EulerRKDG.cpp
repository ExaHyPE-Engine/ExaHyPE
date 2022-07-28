#include "EulerRKDG.h"
#include "exahype2/RefinementControl.h"
#include "exahype2/NonCriticalAssertions.h"


tarch::logging::Log   examples::exahype2::euler::EulerRKDG::_log( "examples::exahype2::euler::EulerRKDG" );



/**
 * With plain DG, Rusanov and without an additional limiter, we cannot solve
 * discontinuous initial condition. At the moment, most of the scenarios
 * below thus are physically meaningless. The only setup that should work
 * out-of-the-box is the Gaussian, as it is infinitely smooth. Having said
 * this, the Gaussian still can run into problems if it is too steep: If the
 * polynomial order and mesh resolution are too low, the Gaussian continues
 * to look like a discontinuous initial condition and thus introduces
 * problems. I therefore scale its diameter with the maximum cell width. In
 * a real setup, I should also take the polynomial order into account, but
 * I kind of keep it simple here.
 */
enum class Scenario {
  ConstantSolution,
  PointExplosion,
  PointExplosionWithDynamicAMR,
  GaussianExplosion,
  GaussianExplosionWithDynamicAMR,
  BreakingDam,
  BreakingDamWithDynamicAMR,
  BreakingDamResolutionStudies
};


Scenario scenario = Scenario::ConstantSolution;
//Scenario scenario = Scenario::GaussianExplosion;
//Scenario scenario = Scenario::PointExplosion;
//Scenario scenario = Scenario::PointExplosionWithDynamicAMR;
//Scenario scenario = Scenario::BreakingDam;
//Scenario scenario = Scenario::BreakingDamResolutionStudies;
//Scenario scenario = Scenario::BreakingDamWithDynamicAMR;



::exahype2::RefinementCommand examples::exahype2::euler::EulerRKDG::refinementCriterion(
  const double * __restrict__ Q,               // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  #if Dimensions==3
  tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.3,0.5};
  #else
  tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.3};
  #endif

  switch (scenario) {
    case Scenario::ConstantSolution:
    case Scenario::PointExplosion:
    case Scenario::GaussianExplosion:
      {
        // This is an example how to implement a priori refinement. If
        // you only have this thing, then you work with static AMR, as
        // you never invoke erase.
        if ( tarch::la::equals(t,0.0) ) {
          bool isInTheCentre = ( tarch::la::norm2( x-circleCentre ) < 0.05 );
          if (isInTheCentre)
            result = ::exahype2::RefinementCommand::Refine;
        }
      }
      break;
    case Scenario::PointExplosionWithDynamicAMR:
    case Scenario::GaussianExplosionWithDynamicAMR:
      {
        // Same as for static AMR. This is the initial pattern
        if ( tarch::la::equals(t,0.0) ) {
          bool isInTheCentre = ( tarch::la::norm2( x-circleCentre ) < 0.05 );
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
        if ( x(0)<0.5 ) {
          result = ::exahype2::RefinementCommand::Refine;
        }
      }
      break;
    case Scenario::BreakingDamWithDynamicAMR:
      {
        if ( tarch::la::equals(t,0.0) and tarch::la::equals(x(0),1.0/3.0) ) {
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
          x(1)>0.5
          and
          x(1)<0.5
        ) {
          result = ::exahype2::RefinementCommand::Refine;
        }
      }
      break;
  }

  return result;
}




void examples::exahype2::euler::EulerRKDG::initialCondition(
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

  switch (scenario) {
    case Scenario::ConstantSolution:
      {
        Q[0] = 0.1;  // rho
        Q[1] = 0;    // velocities
        Q[2] = 0;
        Q[3] = 0;
        Q[4] = 1.0;
      }
      break;
    case Scenario::GaussianExplosion:
    case Scenario::GaussianExplosionWithDynamicAMR:
      {
        logDebug( "initialCondition(...)", "set point explosion initial condition" );
        // Manual offset to make the wave originate slightly to the left of the center --- helps
        // to detect if wave is moving to the left or right
        #if Dimensions==2
        tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.3};
        #else
        tarch::la::Vector<Dimensions,double> circleCentre = {0.18,0.3,0.6};
        #endif

        // see comment on Gaussian initial condition above
        const double peakDeviation = MaxAdmissibleCellH;
        double distance = tarch::la::norm2( x-circleCentre );
        double exponent = - (distance * distance) / 2.0 / peakDeviation / peakDeviation;

        Q[0] = 0.1;  // rho
        Q[1] = 0;    // velocities
        Q[2] = 0;
        Q[3] = 0;
        Q[4] = std::exp( exponent );
      }
      break;
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
        bool isInTheCentre = ( tarch::la::norm2( x-circleCentre ) < 0.05 );
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
        Q[4] = x(0)<0.5 ? 1.0 : 0.0; // inner energy
      }
      break;
  }

  logTraceOut( "initialCondition(...)" );
}





void examples::exahype2::euler::EulerRKDG::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[5+0]
  double * __restrict__                        Qoutside, // Qoutside[5+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
) {
  logTraceInWith3Arguments( "boundaryConditions(...)", x, t, normal );

  nonCriticalAssertion3( Qinside[0]==Qinside[0], x, t, normal );
  nonCriticalAssertion3( Qinside[1]==Qinside[1], x, t, normal );
  nonCriticalAssertion3( Qinside[2]==Qinside[2], x, t, normal );
  nonCriticalAssertion3( Qinside[3]==Qinside[3], x, t, normal );
  nonCriticalAssertion3( Qinside[4]==Qinside[4], x, t, normal );

  nonCriticalAssertion4( Qinside[0]>1e-12, x, t, normal, Qinside[0] );
  assertion4( Qinside[0]>1e-12, x, t, normal, Qinside[0] );

  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }

  logTraceOut( "boundaryConditions(...)" );
}





double ::examples::exahype2::euler::EulerRKDG::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  assertion(normal>=0);
  assertion(normal<Dimensions);
  //assertion( Q[0]>0.0 );

  if (Q[0]<=0.0 or Q[0]!=Q[0]) {
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "Q[0]>0", "density negative" );
    assertion(false);
  }

  constexpr double gamma = 1.4;
  const double irho = 1./Q[0];
  #if Dimensions==3
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3]));
  #else
  const double p = (gamma-1) * (Q[4] - 0.5*irho*(Q[1]*Q[1]+Q[2]*Q[2]));
  #endif

  nonCriticalAssertion9( tarch::la::greaterEquals(p,0.0), Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal, p );
  const double c   = std::sqrt( std::max(0.0,gamma * p * irho) );

  const double u_n = Q[normal + 1] * irho;
  double result = std::max( std::abs(u_n - c), std::abs(u_n + c) );
  nonCriticalAssertion13( result>=0.0, result, p, u_n, irho, c, Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal );
  return result;
}




void ::examples::exahype2::euler::EulerRKDG::flux(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[5]
)  {
  logTraceInWith3Arguments( "flux(...)", x, t, normal );
  assertion3( normal>=0, x, t, normal );
  assertion3( normal<Dimensions, x, t, normal);
  nonCriticalAssertion8( Q[0]==Q[0], Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal );
  nonCriticalAssertion8( Q[1]==Q[1], Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal );
  nonCriticalAssertion8( Q[2]==Q[2], Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal );
  nonCriticalAssertion8( Q[3]==Q[3], Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal );
  nonCriticalAssertion8( Q[4]==Q[4], Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal );

  if (Q[0]<=1e-12  or Q[0]!=Q[0]) {
    std::ostringstream msg;
    msg << "density is negative"
        << ". x=" << x
        << ",normal=" << normal
        << ",Q[0]=" << Q[0];
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "Q[0]>0", msg.str() );
    assertion(false);
    exit(-1);
  }

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

  nonCriticalAssertion7( F[0]==F[0], x, normal, Q[0], Q[1], Q[2], Q[3], Q[4] );
  nonCriticalAssertion7( F[1]==F[1], x, normal, Q[0], Q[1], Q[2], Q[3], Q[4] );
  nonCriticalAssertion7( F[2]==F[2], x, normal, Q[0], Q[1], Q[2], Q[3], Q[4] );
  nonCriticalAssertion7( F[3]==F[3], x, normal, Q[0], Q[1], Q[2], Q[3], Q[4] );
  nonCriticalAssertion7( F[4]==F[4], x, normal, Q[0], Q[1], Q[2], Q[3], Q[4] );

  logTraceOutWith3Arguments( "flux(...)", x, t, normal );
}

