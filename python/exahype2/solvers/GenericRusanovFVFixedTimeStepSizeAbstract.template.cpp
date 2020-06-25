#include "{{CLASSNAME}}.h"


{% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::{{CLASSNAME}}():
  _NumberOfFiniteVolumesPerAxisPerPatch( {{NUMBER_OF_VOLUMES_PER_AXIS}} ),
  _timeStamp(0.0) {
  {% if ENCLAVE %}
  _traversalState = TraversalState::Primary;
  {% endif %}
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


void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
}


void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::finishTimeStep() {
  {% if ENCLAVE %}
  if ( _traversalState == TraversalState::Primary ) {
    _traversalState = TraversalState::Secondary;
  }
  else {
    _traversalState = TraversalState::Primary;
    _timeStamp += {{TIME_STEP_SIZE}};
  }
  {% else %}
  _timeStamp += {{TIME_STEP_SIZE}};
  {% endif %}
}


{% if REFINEMENT_CRITERION_IMPLEMENTATION!="<user-defined>" %}
::exahype2::RefinementCommand {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::refinementCriterion(
  double                                       Q[{{NUMBER_OF_UNKNOWNS}}],
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  {{REFINEMENT_CRITERION_IMPLEMENTATION}}
}
{% endif %}


{% if INITIAL_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::adjustSolution(
  double                                       Q[{{NUMBER_OF_UNKNOWNS}}],
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  {{INITIAL_CONDITIONS_IMPLEMENTATION}}
}
}
{% endif %}


{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" %}
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::eigenvalues(
  double                                       Q[{{NUMBER_OF_UNKNOWNS}}],
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
  double                                       Qinside[{{NUMBER_OF_UNKNOWNS}}],
  double                                       Qoutside[{{NUMBER_OF_UNKNOWNS}}],
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
 double                                       Q[{{NUMBER_OF_UNKNOWNS}}],
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
  double                                       Q[{{NUMBER_OF_UNKNOWNS}}],
  double                                       gradQ[{{NUMBER_OF_UNKNOWNS}}][Dimensions],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double                                       F[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{NCP_IMPLEMENTATION}}
}
{% endif %}
{% endif %}
