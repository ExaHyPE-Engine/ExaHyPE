#include "SelfSimilarInfallFD4.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFD4::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFD4" );


double benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFD4::TotalMassInPreviousTimeStep = 0.0;


benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFD4::SelfSimilarInfallFD4() {
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFD4::addDensity(
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       density
) {
  double overDensity = density - BaseDensity;
  double radius = tarch::la::norm2( volumeCentre );

  double mass = overDensity * tarch::la::volume( volumeH );

  logDebug( "touchCellFirstTime(...)", "dump data of " << volumeCentre << ": " << density << " x " << volumeH << "->" << radius << " x " << mass );

  _accumulator.addMass(mass,radius);
}


::exahype2::RefinementCommand benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFD4::refinementCriterion(
  const double * __restrict__ Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  logTraceInWith3Arguments( "refinementCriterion(...)", volumeX, volumeH, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  // see comments in header file

  logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
  return result;
}




void benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFD4::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  logTraceInWith3Arguments( "initialCondition(...)", volumeCentre, volumeH, gridIsConstructred );

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

  logTraceOut( "initialCondition(...)" );
}





void benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFD4::boundaryConditions(
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





double ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFD4::maxEigenvalue(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  gridCellH,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, gridCellH, t, normal );

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

  logTraceOut( "maxEigenvalue(...)" );

  return result;
}




void ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFD4::flux(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  gridCellH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[5]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, gridCellH, t, normal );

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







void ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFD4::sourceTerm(
  const double * __restrict__                  Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  gridCellX,
  const tarch::la::Vector<Dimensions,double>&  gridCellH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[5]
) {
  logTraceInWith4Arguments( "sourceTerm(...)", gridCellX, gridCellH, t, dt );

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


void ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallFD4::finishTimeStep() {
  AbstractSelfSimilarInfallFV::finishTimeStep();
  if (isLastGridSweepOfTimeStep()) {
    _accumulator.finishAccumulation();
    TotalMassInPreviousTimeStep = _accumulator.getTotalMass();
  }
}
