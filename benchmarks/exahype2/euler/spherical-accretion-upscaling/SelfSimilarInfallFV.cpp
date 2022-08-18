#include "exahype2/RefinementControl.h"
#include "EulerKernels.h"
#include "SelfSimilarInfallFV.h"
#include "spherical-accretion/GravityModel.h"

tarch::logging::Log   benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFV::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFV" );



benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFV::SelfSimilarInfallFV() {
//  _accumulator.addMass(AdditionalMass,0.0);
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFV::addDensity(
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       density
) {
  double overDensity = density - BaseDensity;
  double radius = tarch::la::norm2( volumeCentre );

  double mass = overDensity * tarch::la::volume( volumeH );

  _accumulator.addMass(mass,radius);
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFV::initialCondition(
  double * __restrict__                        Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments( "initialCondition(...)", volumeX, volumeH, gridIsConstructred );

  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  applications::exahype2::euler::sphericalaccretion::initialiseOverdensity_topHat(
    Q,
    volumeX,
    InitialTopHatRadius,
    AdditionalMass,
    BaseDensity,
    pInitial,
    Gamma
  );

/*
  applications::exahype2::euler::sphericalaccretion::initialiseHomogeneousDensity(
    Q,
    BaseDensity,
    pInitial,
    Gamma
  );
*/

  logTraceOut( "initialCondition(...)" );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFV::boundaryConditions(
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


double ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFV::maxEigenvalue(
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


void ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFV::flux(
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


void ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFV::sourceTerm(
  const double * __restrict__                  Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[5
) {
  logTraceInWith4Arguments( "sourceTerm(...)", volumeX, volumeH, t, dt );

  for (int i=0; i<NumberOfUnknowns; i++) {
    S[i] = 0.0;
  }

  double massInterpolated = _accumulator.getMass_linearInterpolation( tarch::la::norm2( volumeX ) );

  applications::exahype2::euler::sphericalaccretion::addGravitationalSource_AlphaCDM(
    S,
    volumeX,
    Q,
    massInterpolated,
    aInitial,
    t
  );

  logTraceOut( "sourceTerm(...)" );
}


void ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFV::finishTimeStep() {
  AbstractSelfSimilarInfallFV::finishTimeStep();
  if (isLastGridSweepOfTimeStep()) {
    _accumulator.finishAccumulation();
  }
}


    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
GPUCallableMethod double ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFV::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  Offloadable
)  {
  // @todo implement
}
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif




    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
GPUCallableMethod void ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFV::flux(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F,
  Offloadable
)  {
  // @todo implement
}
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif


    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
GPUCallableMethod void ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFV::sourceTerm(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S,
  Offloadable
) {
  // @todo implement but ensure that all entries of S are properly set
  for (int i=0; i<NumberOfUnknowns; i++) {
    S[i] = 0.0;
  }
}
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
