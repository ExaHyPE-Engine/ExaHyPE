#include "SelfSimilarInfallDG.h"
#include "exahype2/RefinementControl.h"
#include "exahype2/dg/DGUtils.h"
#include "EulerKernels.h"
#include "spherical-accretion/GravityModel.h"


tarch::logging::Log   benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallDG::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallDG" );



double benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallDG::TotalMassInPreviousTimeStep = 0.0;


benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallDG::SelfSimilarInfallDG() {
//  _accumulator.addMass(AdditionalMass,0.0);
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallDG::addDensity(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<3,double>&           cellSize,
  const tarch::la::Vector<3,int>&              index,
  double                                       density
) {
  double overDensity = density - BaseDensity;
  double radius = tarch::la::norm2( x );

  assertion3(
    ::exahype2::dg::getQuadratureWeight(cellSize,index,QuadratureWeights1d)>0.0,
    cellSize, index, QuadratureWeights1d[0]
  );
  double mass = overDensity * ::exahype2::dg::getQuadratureWeight(cellSize,index,QuadratureWeights1d);

  _accumulator.addMass(mass,radius);
}


::exahype2::RefinementCommand benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallDG::refinementCriterion(
  const double * __restrict__                  Q,    // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {
  if (
    tarch::la::equals(t,0.0)
    and
    tarch::la::norm2(x) <= InitialTopHatRadius + h(0)
  ) {
    return ::exahype2::RefinementCommand::Refine;
  }
  else if (
    DynamicAMR
    and
    ( Q[4]>1.1*BaseDensity )
  ) {
    return ::exahype2::RefinementCommand::Refine;
  }
  else if (
    DynamicAMR
  ) {
    return ::exahype2::RefinementCommand::Erase;
  }
  else {
    return ::exahype2::RefinementCommand::Keep;
  }
}



void benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallDG::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
  bool                                         gridIsConstructed
) {
  logTraceInWith2Arguments( "initialCondition(...)", x, gridIsConstructed );

  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

/*
  applications::exahype2::euler::sphericalaccretion::initialiseOverdensity_topHat(
    Q,
    x,
    InitialTopHatRadius,
    AdditionalMass,
    BaseDensity,
    pInitial,
    Gamma
  );
*/

//  applications::exahype2::euler::sphericalaccretion::initialiseOverdensity_hyperbolicSecant(


  applications::exahype2::euler::sphericalaccretion::initialiseOverdensity_Gaussian(
    Q,
    x,
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


void benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallDG::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[5+0]
  double * __restrict__                        Qoutside, // Qoutside[5+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
) {
  logTraceInWith3Arguments( "boundaryConditions(...)", x, t, normal );

  ::applications::exahype2::euler::NeumannBoundaryConditions(
    Qinside,
    Qoutside,
    x,
    0.0, // no h in classical sense
    t,
    normal,
    NumberOfUnknowns,            // defined in superclass
    NumberOfAuxiliaryVariables   // defined in superclass
  );

  logTraceOut( "boundaryConditions(...)" );
}


double ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallDG::maxEigenvalue(
  const double * __restrict__                  Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", x, t, dt, normal );

  double result = ::applications::exahype2::euler::maxEigenvalue(
      Q,
      x,
      0.0,  // no volume, as point-wise
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


void ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallDG::flux(
  const double * __restrict__ Q, // Q[5+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[5]
)  {
  logTraceInWith4Arguments( "flux(...)", x, t, dt, normal );

  ::applications::exahype2::euler::flux(
    Q,
    x,
    0.0, // no volume, as point-wise
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


void ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallDG::sourceTerm(
  const double * __restrict__                  Q, // Q[5+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[5
) {
  logTraceInWith3Arguments( "sourceTerm(...)", x, t, dt );

  for (int i=0; i<NumberOfUnknowns; i++) {
    S[i] = 0.0;
  }

  double massInterpolated = _accumulator.getMass_linearInterpolation( tarch::la::norm2( x ) );

  applications::exahype2::euler::sphericalaccretion::addGravitationalSource_AlphaCDM(
    S,
    x,
    Q,
    massInterpolated,
    aInitial,
    t
  );

  logTraceOut( "sourceTerm(...)" );
}


void ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallDG::finishTimeStep() {
  AbstractSelfSimilarInfallDG::finishTimeStep();
  if (isLastGridSweepOfTimeStep()) {
    _accumulator.finishAccumulation();
    TotalMassInPreviousTimeStep = _accumulator.getTotalMass();
  }
}


bool ::benchmarks::exahype2::euler::sphericalaccretionupscaling::SelfSimilarInfallDG::cellCanUseStatelessPDETerms(
  const tarch::la::Vector<Dimensions,double>&  cellCentre,
  const tarch::la::Vector<Dimensions,double>&  cellH,
  double                                       t,
  double                                       dt
) const {
  const double radius = tarch::la::norm2( cellCentre );
  return radius > _accumulator.getMaxRelevantRadius() + std::sqrt(Dimensions) * cellH(0) * cellH(0);

}
