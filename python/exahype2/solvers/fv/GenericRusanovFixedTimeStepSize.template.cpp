#include "{{CLASSNAME}}.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::_log( "{% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}" );



{% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" %}
::exahype2::RefinementCommand {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::refinementCriterion(
  double Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  logTraceInWith3Arguments( "refinementCriterion(...)", volumeX, volumeH, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  if ( tarch::la::smallerEquals(_maxH,_NumberOfFiniteVolumesPerAxisPerPatch*tarch::la::max(volumeH)) ) {
    result = ::exahype2::RefinementCommand::Refine;
  }

  logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
  return result;
}
{% endif %}


{% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::adjustSolution(
  double                                       Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  logTraceInWith3Arguments( "adjustSolution(...)", volumeX, volumeH, t );
  if (tarch::la::equals(t,0.0) ) {
    // initial conditions
  }
  else {
    // other stuff
  }
  logTraceOut( "adjustSolution(...)" );
}
{% endif %}


{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
double {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::maxEigenvalue(
  double                                       Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       lambda[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceInWith4Arguments( "eigenvalues(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "eigenvalues(...)" );
}
{% endif %}


{% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
void {% for item in NAMESPACE -%}{{ item }}::{%- endfor %}{{CLASSNAME}}::boundaryConditions(
  double                                       Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  double                                       Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "boundaryConditions(...)" );
}
{% endif %}

