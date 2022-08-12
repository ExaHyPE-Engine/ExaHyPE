#include "SelfSimilarInfall.h"
#include "exahype2/RefinementControl.h"
#include "EulerKernels.h"


tarch::logging::Log   examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall::_log( "examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall" );



void examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall::addDensity(
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       density
) {
  double overDensity = density - BaseDensity;
  double radius = tarch::la::norm2( volumeCentre );

  double mass = overDensity * tarch::la::volume( volumeH );

  _accumulator.addMass(mass,radius);
}


void examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall::initialCondition(
  double * __restrict__                        Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments( "initialCondition(...)", volumeCentre, volumeH, gridIsConstructred );

  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }


  double distanceFromCentre = tarch::la::norm2( volumeX );
  double additionalMass = (distanceFromCentre<InitialTopHatRadius + 0.5*std::sqrt(3.0)*volumeH(0))
                        ? InitialDeltaM
                        : 0.0;

  const double recalibratedDistanceFromCentre = std::max( distanceFromCentre, InitialTopHatRadius ) / InitialTopHatRadius;

  Q[0] = BaseDensity + additionalMass / tarch::la::volume(volumeH);

  double normalisedInitialVelocity = 1.5 * aInitial * InitialDeltaM / std::pow(recalibratedDistanceFromCentre,3.0) * Q[0];

  if ( tarch::la::greater(distanceFromCentre,0.0) ) {
    Q[1] = -normalisedInitialVelocity * volumeX(0)/distanceFromCentre;    // velocities
    Q[2] = -normalisedInitialVelocity * volumeX(1)/distanceFromCentre;    // velocities
    Q[3] = -normalisedInitialVelocity * volumeX(2)/distanceFromCentre;    // velocities
  }
  else {
    Q[1] = 0.0;
    Q[2] = 0.0;
    Q[3] = 0.0;
  }

  Q[4] = 0.5*(Q[1]*Q[1]+Q[2]*Q[2]+Q[3]*Q[3])/Q[0]+pInitial/(Gamma-1); // inner energy

  logTraceOut( "initialCondition(...)" );
}


void examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[5+0]
  double * __restrict__                        Qoutside, // Qoutside[5+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );

  ::applications::exahype2::euler::NeumannBoundaryConditions(
    Qinside,
    Qoutside,
    faceCentre,
    volumeH,
    t,
    normal,
    NumberOfUnknowns,            // defined in superclass
    NumberOfAuxiliaryVariables   // defined in superclass
  );

  logTraceOut( "boundaryConditions(...)" );
}


double ::examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall::maxEigenvalue(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );

  double result = ::applications::exahype2::euler::maxEigenvalue(
      Q,
      faceCentre,
      volumeH,
      t,
      dt,
      normal,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      Gamma
    );

  logTraceOutWith1Argument( "maxEigenvalue(...)", result );

  return result;
}


void ::examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall::flux(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__                        F // F[5]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );

  ::applications::exahype2::euler::flux(
    Q,
    faceCentre,
    volumeH,
    t,
    dt,
    normal,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    F,
    Gamma
  );

  logTraceOut( "flux(...)" );
}


void ::examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall::sourceTerm(
  const double * __restrict__                  Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[5
) {
  logTraceInWith4Arguments( "sourceTerm(...)", volumeX, volumeH, t, dt );

  double distance = tarch::la::norm2( volumeX );

  double a   = 1e-4 * 0.0287 * std::pow((-t/11.8+0.1694*std::pow(aInitial,-0.5)),-2); //when code time ~ 2*(a_i^(-0.5)-1), a~1

  double massInterpolated = _accumulator.getMass( distance );

  S[0] = 0.0;  // rho
  if (tarch::la::greater(distance,0.0)) {
    double normalisedDistance = std::max(1.0,distance);

    double forceDensity = a * Q[0] * massInterpolated / std::pow( normalisedDistance, 3.0 );

    S[1] = -forceDensity * volumeX(0)/distance;    // velocities
    S[2] = -forceDensity * volumeX(1)/distance;    // velocities
    S[3] = -forceDensity * volumeX(2)/distance;    // velocities
    S[4] = -forceDensity * (Q[1]*volumeX(0)+Q[2]*volumeX(1)+Q[3]*volumeX(2)) /distance / Q[0];
  }
  else {
    S[1] = 0.0;
    S[2] = 0.0;
    S[3] = 0.0;
    S[4] = 0.0;
  }


  logTraceOut( "sourceTerm(...)" );
}


void ::examples::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfall::finishTimeStep() {
  AbstractSelfSimilarInfall::finishTimeStep();
  if (isLastGridSweepOfTimeStep()) {
    _accumulator.finishAccumulation();
  }
}

