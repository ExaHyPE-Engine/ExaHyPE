#include "AbstractEuler.h"


tarch::logging::Log   examples::exahype2::euler::AbstractEuler::AbstractEuler::_log( "examples::exahype2::euler::AbstractEuler::AbstractEuler" );


double examples::exahype2::euler::AbstractEuler::getMinMeshSize() const {
  return _minH;
}


double examples::exahype2::euler::AbstractEuler::getMaxMeshSize() const {
  return _maxH;
}


double examples::exahype2::euler::AbstractEuler::getMinTimeStamp() const {
  return _timeStamp;
}


double examples::exahype2::euler::AbstractEuler::getMaxTimeStamp() const {
  return _timeStamp;
}


double examples::exahype2::euler::AbstractEuler::getMinTimeStepSize() const {
  return 1e-06;
}


double examples::exahype2::euler::AbstractEuler::getMaxTimeStepSize() const {
  return 1e-06;
}


examples::exahype2::euler::AbstractEuler::SolverState examples::exahype2::euler::AbstractEuler::getSolverState() const {
  return _solverState;
}















examples::exahype2::euler::AbstractEuler::AbstractEuler():
  _NumberOfFiniteVolumesPerAxisPerPatch( 7 ),
  _timeStamp(0.0),
  _solverState(SolverState::GridConstruction),
  _maxH(0.004),
  _minH(0.004)
  {
}


void examples::exahype2::euler::AbstractEuler::startGridConstructionStep() {
  assertion( _solverState == SolverState::GridConstruction );
}


void examples::exahype2::euler::AbstractEuler::finishGridConstructionStep() {
}


void examples::exahype2::euler::AbstractEuler::startGridInitialisationStep() {
  assertion( _solverState == SolverState::GridConstruction );
  _solverState = SolverState::GridInitialisation;
  logDebug( "startGridInitialisationStep(...)", "new state is " << toString(_solverState) );
}


void examples::exahype2::euler::AbstractEuler::finishGridInitialisationStep() {
}


void examples::exahype2::euler::AbstractEuler::startTimeStep(
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


void examples::exahype2::euler::AbstractEuler::finishTimeStep() {
  if ( _solverState == SolverState::Secondary ) {
    _timeStamp += 1e-06;
  }
}


void examples::exahype2::euler::AbstractEuler::startPlottingStep(
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


void examples::exahype2::euler::AbstractEuler::finishPlottingStep() {
}


std::string examples::exahype2::euler::AbstractEuler::toString(SolverState state) {
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










