{% include "AbstractSolverAdaptiveTimeStepSize.template.cpp" %}


#include "exahype2/NonCriticalAssertions.h"


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _NumberOfFiniteVolumesPerAxisPerPatch( {{NUMBER_OF_VOLUMES_PER_AXIS}} ),
  _timeStamp(0.0),
  _timeStepSize(0.0),
  _admissibleTimeStepSize(std::numeric_limits<double>::max()),
  _solverState(SolverState::GridConstruction),
  _maxH({{MAX_H}}),
  _minH({{MIN_H}})
  {
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridConstructionStep() {
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridConstructionStep() {
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridInitialisationStep() {
  _solverState = SolverState::GridInitialisation;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridInitialisationStep() {
}



void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  _solverState = SolverState::TimeStep;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {
  _timeStamp += getMaxTimeStepSize();
  
  double newTimeStepSize = _admissibleTimeStepSize;
  #ifdef Parallel
  MPI_Allreduce(&newTimeStepSize, &_timeStepSize, 1, MPI_DOUBLE, MPI_MIN, tarch::mpi::Rank::getInstance().getCommunicator() );
  #else
  _timeStepSize = newTimeStepSize;
  #endif

  if ( std::isnan(_timeStepSize) or std::isinf(_timeStepSize) ) {
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_timeStepSize>0", "invalid (NaN of inf) time step size: " + std::to_string(_timeStepSize) );
  }
  if (tarch::la::smallerEquals(_timeStepSize,0.0,1e-12) ) {
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_timeStepSize>0", "degenerated time step size of " + std::to_string(_timeStepSize) );
  }

  _admissibleTimeStepSize = std::numeric_limits<double>::max();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startPlottingStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  _solverState = SolverState::Plotting;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishPlottingStep() {
}


std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString(SolverState state) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::TimeStep:
      return "time-step";
    case SolverState::Plotting:
      return "plotting";
  }
  return "<undef>";
}


{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S
) {
  {% if SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
  {{SOURCE_TERM_IMPLEMENTATION}}
  {% else %}
  std::fill_n(S,{{NUMBER_OF_UNKNOWNS}},0.0);
  {% endif %}
}
{% endif %}

