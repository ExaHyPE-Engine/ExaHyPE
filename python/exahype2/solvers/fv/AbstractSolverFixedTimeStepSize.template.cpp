#include "{{CLASSNAME}}.h"


tarch::logging::Log   {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::{{CLASSNAME}}::_log( "{% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::{{CLASSNAME}}" );


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


{% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::SolverState {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::getSolverState() const {
  return _solverState;
}



{% if REFINEMENT_CRITERION_IMPLEMENTATION!="<user-defined>" %}
::exahype2::RefinementCommand {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::refinementCriterion(
  double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
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
  double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  if (tarch::la::equals(t,0.0) ) {
    {{INITIAL_CONDITIONS_IMPLEMENTATION}}
  }
}
{% endif %}



{% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::boundaryConditions(
  double * __restrict__ Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  double * __restrict__ Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  {{BOUNDARY_CONDITIONS_IMPLEMENTATION}}
}
{% endif %}

