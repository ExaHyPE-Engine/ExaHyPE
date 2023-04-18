#include "AbstractLOH1OnGPU.h"
#include "Constants.h"

tarch::logging::Log   examples::exahype2::loh1::AbstractLOH1OnGPU::AbstractLOH1OnGPU::_log( "examples::exahype2::loh1::AbstractLOH1OnGPU::AbstractLOH1OnGPU" );

std::bitset<Dimensions> examples::exahype2::loh1::AbstractLOH1OnGPU::AbstractLOH1OnGPU::PeriodicBC = examples::exahype2::loh1::PeriodicBC;


double examples::exahype2::loh1::AbstractLOH1OnGPU::getMinMeshSize() const {
  return _minH;
}


double examples::exahype2::loh1::AbstractLOH1OnGPU::getMaxMeshSize() const {
  return _maxH;
}


double examples::exahype2::loh1::AbstractLOH1OnGPU::getMinTimeStamp() const {
  return _timeStamp;
}


double examples::exahype2::loh1::AbstractLOH1OnGPU::getMaxTimeStamp() const {
  return _timeStamp;
}


double examples::exahype2::loh1::AbstractLOH1OnGPU::getMinTimeStepSize() const {
  return 0.04;
}


double examples::exahype2::loh1::AbstractLOH1OnGPU::getMaxTimeStepSize() const {
  return 0.04;
}


examples::exahype2::loh1::AbstractLOH1OnGPU::SolverState examples::exahype2::loh1::AbstractLOH1OnGPU::getSolverState() const {
  return _solverState;
}




::exahype2::RefinementCommand examples::exahype2::loh1::AbstractLOH1OnGPU::refinementCriterion(
  const double * __restrict__ Q, // Q[9+4],
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  if ( tarch::la::smallerEquals(_maxH,_NumberOfFiniteVolumesPerAxisPerPatch*tarch::la::max(volumeH)) ) {
    result = ::exahype2::RefinementCommand::Refine;
  }

  return result;
  
}








#include <algorithm>


examples::exahype2::loh1::AbstractLOH1OnGPU::AbstractLOH1OnGPU():
  _NumberOfFiniteVolumesPerAxisPerPatch( 8 ),
  _timeStamp(0.0),
  _solverState(SolverState::GridConstruction),
  _maxH(10.0),
  _minH(10.0)
  {
}


void examples::exahype2::loh1::AbstractLOH1OnGPU::startGridConstructionStep() {
  assertion( _solverState == SolverState::GridConstruction );
}


void examples::exahype2::loh1::AbstractLOH1OnGPU::finishGridConstructionStep() {
}


void examples::exahype2::loh1::AbstractLOH1OnGPU::startGridInitialisationStep() {
  assertion( _solverState == SolverState::GridConstruction );
  _solverState = SolverState::GridInitialisation;
  logDebug( "startGridInitialisationStep(...)", "new state is " << toString(_solverState) );
}


void examples::exahype2::loh1::AbstractLOH1OnGPU::finishGridInitialisationStep() {
}


void examples::exahype2::loh1::AbstractLOH1OnGPU::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  if (
    _solverState == SolverState::GridInitialisation
  ) {
    _solverState = SolverState::PrimaryAfterGridInitialisation;
  }
  else if (
    _solverState == SolverState::Primary
    or
    _solverState == SolverState::PrimaryAfterGridInitialisation
  ) {
    _solverState = SolverState::Secondary;
  }
  else {
    _solverState = SolverState::Primary;
  }

  logDebug( "startTimeStep(...)", "new state is " << toString(_solverState) );
}


void examples::exahype2::loh1::AbstractLOH1OnGPU::finishTimeStep() {
  if ( _solverState == SolverState::Secondary ) {
    _timeStamp += 0.04;
  }
}


void examples::exahype2::loh1::AbstractLOH1OnGPU::startPlottingStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  if ( _solverState == SolverState::GridInitialisation ) {
    _solverState = SolverState::PlottingInitialCondition;
  }
  else {
    _solverState = SolverState::Plotting;
  }
}


void examples::exahype2::loh1::AbstractLOH1OnGPU::finishPlottingStep() {
}


std::string examples::exahype2::loh1::AbstractLOH1OnGPU::toString(SolverState state) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::Primary:
      return "primary";
    case SolverState::Secondary:
      return "secondary";
    case SolverState::PlottingInitialCondition:
      return "plotting-initial-condition";
    case SolverState::PrimaryAfterGridInitialisation:
      return "primary-after-grid-initialisation";
    case SolverState::Plotting:
      return "plotting";
  }
  return "<undef>";
}



void examples::exahype2::loh1::AbstractLOH1OnGPU::sourceTerm(
  const double * __restrict__                  Q, // Q[9+4]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S // S[9]
) {
  
  std::fill_n(S,9,0.0);
  
}









