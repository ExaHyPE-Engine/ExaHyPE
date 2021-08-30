#include <algorithm>


#include "{{CLASSNAME}}.h"
#include "Constants.h"


#include "tarch/multicore/Lock.h"


tarch::logging::Log   {{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}" );

std::bitset<Dimensions> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}::PeriodicBC = {{NAMESPACE | join("::")}}::PeriodicBC;


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinMeshSize() const {
  return _minH;
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxMeshSize() const {
  return _maxH;
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStamp() const {
  return _minTimeStamp;
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStamp() const {
  return _maxTimeStamp;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::setTimeStepSizeAndTimeStamp(double timeStepSize, double timeStamp) {
  tarch::multicore::Lock lock(_semaphore);

  assertion1(timeStepSize<std::numeric_limits<double>::max()/10.0,timeStepSize);
  assertion1(timeStepSize>=0.0,timeStepSize);
  _minTimeStepSize = std::min(timeStepSize,_minTimeStepSize);
  _maxTimeStepSize = std::max(timeStepSize,_maxTimeStepSize);
  assertion2(_minTimeStepSize<=_maxTimeStepSize, _minTimeStepSize, _maxTimeStepSize );

  assertion1(timeStamp<std::numeric_limits<double>::max()/10.0,timeStamp);
  assertion1(timeStamp>=0.0,timeStamp);
  _maxTimeStamp = std::max(timeStamp,_maxTimeStamp);
  _minTimeStamp = std::min(timeStamp,_minTimeStamp);
  assertion2(_minTimeStamp<=_maxTimeStamp, _minTimeStamp, _maxTimeStamp );
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStepSize() const {
  return _minTimeStepSize;
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStepSize() const {
  return _maxTimeStepSize;
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::SolverState {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getSolverState() const {
  return _solverState;
}


{% if REFINEMENT_CRITERION_IMPLEMENTATION!="<user-defined>" %}
::exahype2::RefinementCommand {{NAMESPACE | join("::")}}::{{CLASSNAME}}::refinementCriterion(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<empty>" %}
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  if ( tarch::la::smallerEquals(_maxH,_NumberOfFiniteVolumesPerAxisPerPatch*tarch::la::max(volumeH)) ) {
    result = ::exahype2::RefinementCommand::Refine;
  }

  return result;
  {% else %}
  {{REFINEMENT_CRITERION_IMPLEMENTATION}}
  {% endif %}
}
{% endif %}


{% if INITIAL_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  {{INITIAL_CONDITIONS_IMPLEMENTATION}}
}
{% endif %}


{% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::boundaryConditions(
  const double * __restrict__                  Qinside,   // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  double * __restrict__                        Qoutside,  // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  {{BOUNDARY_CONDITIONS_IMPLEMENTATION}}
}
{% endif %}



{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _NumberOfFiniteVolumesPerAxisPerPatch( {{NUMBER_OF_VOLUMES_PER_AXIS}} ),
  _minTimeStamp(0.0),
  _maxTimeStamp(0.0),
  _solverState(SolverState::GridConstruction),
  _maxH({{MAX_H}}),
  _minH({{MIN_H}}),
  _minTimeStepSize(0.0),
  _maxTimeStepSize(0.0) {
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
  _minTimeStamp += _maxTimeStepSize;
  _minTimeStepSize = std::numeric_limits<double>::max();
  _maxTimeStepSize = std::numeric_limits<double>::min();
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {
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




{{ABSTRACT_SOLVER_USER_DEFINITIONS}}
