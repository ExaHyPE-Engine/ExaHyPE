#include "EulerRKFD.h"
#include "exahype2/RefinementControl.h"
#include "exahype2/NonCriticalAssertions.h"


tarch::logging::Log   examples::exahype2::euler::EulerRKFD::_log( "examples::exahype2::euler::EulerRKFD" );



enum class Scenario {
  PointExplosion,
  PointExplosionWithDynamicAMR,
  GaussianExplosion,
  GaussianExplosionWithDynamicAMR
};



Scenario scenario = Scenario::GaussianExplosion;



::exahype2::RefinementCommand examples::exahype2::euler::EulerRKFD::refinementCriterion(
  const double * __restrict__ Q, // Q[5+1]
  const tarch::la::Vector<Dimensions,double>&  gridCellX,
  const tarch::la::Vector<Dimensions,double>&  gridCellH,
  double                                       t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  #if Dimensions==3
  tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.3,0.5};
  #else
  tarch::la::Vector<Dimensions,double> circleCentre = {0.5,0.3};
  #endif

  switch (scenario) {
    case Scenario::GaussianExplosion:
    case Scenario::PointExplosion:
      {
        // This is an example how to implement a priori refinement. If
        // you only have this thing, then you work with static AMR, as
        // you never invoke erase.
        if ( tarch::la::equals(t,0.0) ) {
          bool isInTheCentre = ( tarch::la::norm2( gridCellX-circleCentre ) < 0.05 );
          if (isInTheCentre)
            result = ::exahype2::RefinementCommand::Refine;
        }
      }
      break;
    case Scenario::GaussianExplosionWithDynamicAMR:
    case Scenario::PointExplosionWithDynamicAMR:
      {
        // Same as for static AMR. This is the initial pattern
        if ( tarch::la::equals(t,0.0) ) {
          bool isInTheCentre = ( tarch::la::norm2( gridCellX-circleCentre ) < 0.05 );
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
  }

  return result;
}




void examples::exahype2::euler::EulerRKFD::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  gridCellCentre,
  const tarch::la::Vector<Dimensions,double>&  gridCellH,
  bool                                         gridIsConstructred
) {
  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

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
        bool isInTheCentre = ( tarch::la::norm2( gridCellCentre-circleCentre ) < 0.05 );
        Q[0] = 0.1;  // rho
        Q[1] = 0;    // velocities
        Q[2] = 0;
        Q[3] = 0;
        Q[4] = isInTheCentre ? 1.0 : 0.05; // inner energy
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
        double distance = tarch::la::norm2( gridCellCentre-circleCentre );
        double exponent = - (distance * distance) / 2.0 / peakDeviation / peakDeviation;

        Q[0] = 0.1;  // rho
        Q[1] = 0;    // velocities
        Q[2] = 0;
        Q[3] = 0;
        Q[4] = std::exp( exponent );
      }
      break;
  }
}





void examples::exahype2::euler::EulerRKFD::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[5+1]
  double * __restrict__                        Qoutside, // Qoutside[5+1]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  gridCellH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, gridCellH, t, normal );
  nonCriticalAssertion4( Qinside[0]==Qinside[0], faceCentre, gridCellH, t, normal );
  nonCriticalAssertion4( Qinside[1]==Qinside[1], faceCentre, gridCellH, t, normal );
  nonCriticalAssertion4( Qinside[2]==Qinside[2], faceCentre, gridCellH, t, normal );
  nonCriticalAssertion4( Qinside[3]==Qinside[3], faceCentre, gridCellH, t, normal );
  nonCriticalAssertion4( Qinside[4]==Qinside[4], faceCentre, gridCellH, t, normal );

  nonCriticalAssertion5( Qinside[0]>1e-12, faceCentre, gridCellH, t, normal, Qinside[0] );
  assertion5( Qinside[0]>1e-12, faceCentre, gridCellH, t, normal, Qinside[0] );

  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }

  logTraceOut( "boundaryConditions(...)" );
}





double ::examples::exahype2::euler::EulerRKFD::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+1],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );

  assertion(false);

  logTraceOut( "maxEigenvalue(...)" );

  return 0.0;
}




void ::examples::exahype2::euler::EulerRKFD::flux(
  const double * __restrict__ Q, // Q[5+1],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[5]
)  {
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
        << ",Q[0]=" << Q[0]
        << ",Q[1]=" << Q[1]
        << ",Q[2]=" << Q[2]
        << ",Q[3]=" << Q[3]
        << ",Q[4]=" << Q[4];
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

  nonCriticalAssertion8( F[0]==F[0], faceCentre, volumeH, normal, Q[0], Q[1], Q[2], Q[3], Q[4] );
  nonCriticalAssertion8( F[1]==F[1], faceCentre, volumeH, normal, Q[0], Q[1], Q[2], Q[3], Q[4] );
  nonCriticalAssertion8( F[2]==F[2], faceCentre, volumeH, normal, Q[0], Q[1], Q[2], Q[3], Q[4] );
  nonCriticalAssertion8( F[3]==F[3], faceCentre, volumeH, normal, Q[0], Q[1], Q[2], Q[3], Q[4] );
  nonCriticalAssertion8( F[4]==F[4], faceCentre, volumeH, normal, Q[0], Q[1], Q[2], Q[3], Q[4] );

  logTraceOutWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
}
