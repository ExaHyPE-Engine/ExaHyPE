#include "{{CLASSNAME}}.h"
#include "exahype2/RefinementControl.h"

tarch::logging::Log   {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );

{% if USE_VARIABLE_SHORTCUT is sameas True %}
/*
 * Enables the usage of shortcuts to access variables, e.g. use Q[s.{{VARIABLE_NAMES[0]}}] instead of Q[0]
 */
{{NAMESPACE | join("::")}}::VariableShortcuts s;
{% endif %}

{% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" %}
::exahype2::RefinementCommand {{NAMESPACE | join("::")}}::{{CLASSNAME}}::refinementCriterion(
  const double * __restrict__ Q,               // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  x,
	const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {
	logTraceInWith3Arguments( "refinementCriterion(...)", x, h, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  // see comments in header file

  logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
  return result;
}
{% endif %}


{% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initialCondition(
  double * __restrict__ Q, //Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     point,
	bool                                         gridIsConstructed
) {
  logTraceInWith2Arguments( "initialCondition(...)", x, gridIsConstructed );

  // @todo Implement your stuff here

  logTraceOut( "initialCondition(...)" );
}
{% endif %}



{% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::boundaryValues(
    const double* const x,
    const double        t,
    const double        dt,
    const int           faceIndex,
    const int           direction,
    const double* const fluxIn,
    const double* const stateIn,
    const double* const gradStateIn,
    double* const       fluxOut,
    double* const       stateOut
) {
  logTraceInWith3Arguments( "boundaryConditions(...)", x, t, direction );
  // @todo implement
  logTraceOut( "boundaryConditions(...)" );
}
{% endif %}


{{SOLVER_USER_DEFINITIONS}}
