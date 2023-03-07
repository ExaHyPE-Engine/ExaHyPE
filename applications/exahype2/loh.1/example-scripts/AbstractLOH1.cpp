#include "AbstractLOH1.h"


tarch::logging::Log   examples::exahype2::loh1::AbstractLOH1::AbstractLOH1::_log( "examples::exahype2::loh1::AbstractLOH1::AbstractLOH1" );


examples::exahype2::loh1::AbstractLOH1::AbstractLOH1():
  _NumberOfFiniteVolumesPerAxisPerPatch( 5 ),
  _timeStamp(0.0),
  _solverState(SolverState::GridConstruction),
  _maxH(3.0),
  _minH(3.0)
  {
}


double examples::exahype2::loh1::AbstractLOH1::getMinMeshSize() const {
  return _minH;
}


double examples::exahype2::loh1::AbstractLOH1::getMaxMeshSize() const {
  return _maxH;
}


double examples::exahype2::loh1::AbstractLOH1::getMinTimeStamp() const {
  return _timeStamp;
}


double examples::exahype2::loh1::AbstractLOH1::getMaxTimeStamp() const {
  return _timeStamp;
}


double examples::exahype2::loh1::AbstractLOH1::getMinTimeStepSize() const {
  return 0.01;
}


double examples::exahype2::loh1::AbstractLOH1::getMaxTimeStepSize() const {
  return 0.01;
}


void examples::exahype2::loh1::AbstractLOH1::startGridConstructionStep() {
}


void examples::exahype2::loh1::AbstractLOH1::finishGridConstructionStep() {
}


void examples::exahype2::loh1::AbstractLOH1::startGridInitialisationStep() {
  _solverState = SolverState::GridInitialisation;
}


void examples::exahype2::loh1::AbstractLOH1::finishGridInitialisationStep() {
}


void examples::exahype2::loh1::AbstractLOH1::startTimeStep(
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


void examples::exahype2::loh1::AbstractLOH1::finishTimeStep() {
  if ( _solverState == SolverState::Secondary ) {
    _timeStamp += 0.01;
  }
}


void examples::exahype2::loh1::AbstractLOH1::startPlottingStep(
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


void examples::exahype2::loh1::AbstractLOH1::finishPlottingStep() {
}


std::string examples::exahype2::loh1::AbstractLOH1::toString(SolverState state) {
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


examples::exahype2::loh1::AbstractLOH1::SolverState examples::exahype2::loh1::AbstractLOH1::getSolverState() const {
  return _solverState;
}




















