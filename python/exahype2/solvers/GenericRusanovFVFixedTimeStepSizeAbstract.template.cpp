#include "{{CLASSNAME}}.h"



{% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::{{CLASSNAME}}():
  _NumberOfFiniteVolumesPerAxisPerPatch( {{NUMBER_OF_VOLUMES_PER_AXIS}} ),
  _timeStamp(0.0),
  _solverState(SolverState::GridConstruction),
  _maxH({{MAX_H}}),
  _minH({{MIN_H}})
  {
}


double {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::getMinMeshSize() const {
  return _minH;
}


double {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::getMaxMeshSize() const {
  return _maxH;
}


double {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::getMinTimeStamp() const {
  return _timeStamp;
}


double {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::getMaxTimeStamp() const {
  return _timeStamp;
}


double {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::getMinTimeStepSize() const {
  return {{TIME_STEP_SIZE}};
}


double {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::getMaxTimeStepSize() const {
  return {{TIME_STEP_SIZE}};
}


void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::startGridConstructionStep() {
}


void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::finishGridConstructionStep() {
}


void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::startGridInitialisationStep() {
  _solverState = SolverState::GridInitialisation;
}


void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::finishGridInitialisationStep() {
}


void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  _solverState = SolverState::TimeStep;
}


void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::finishTimeStep() {
  _timeStamp += {{TIME_STEP_SIZE}};
}


void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::startPlottingStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  _solverState = SolverState::Plotting;
}


void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::finishPlottingStep() {
}


std::string {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::toString(SolverState state) {
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


{% if REFINEMENT_CRITERION_IMPLEMENTATION!="<user-defined>" %}
::exahype2::RefinementCommand {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::refinementCriterion(
  double                                       Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
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
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::adjustSolution(
  double                                       Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  if (tarch::la::equals(t,0.0) ) {
    {{INITIAL_CONDITIONS_IMPLEMENTATION}}
  }
}
{% endif %}


{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" %}
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::eigenvalues(
  double                                       Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double                                       lambda[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{EIGENVALUES_IMPLEMENTATION}}
}
{% endif %}


{% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::boundaryConditions(
  double                                       Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  double                                       Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  {{BOUNDARY_CONDITIONS_IMPLEMENTATION}}
}
{% endif %}


{% if FLUX_IMPLEMENTATION!="<none>" %}
{% if FLUX_IMPLEMENTATION!="<user-defined>" %}
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::flux(
 double                                       Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
 const tarch::la::Vector<Dimensions,double>&  faceCentre,
 const tarch::la::Vector<Dimensions,double>&  volumeH,
 double                                       t,
 int                                          normal,
 double                                       F[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{FLUX_IMPLEMENTATION}}
}
{% endif %}
{% endif %}


{% if NCP_IMPLEMENTATION!="<none>" %}
{% if NCP_IMPLEMENTATION!="<user-defined>" %}
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::nonconservativeProduct(
  double                                       Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  double                                       gradQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}][Dimensions],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double                                       BgradQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{NCP_IMPLEMENTATION}}
}
{% endif %}
{% endif %}
