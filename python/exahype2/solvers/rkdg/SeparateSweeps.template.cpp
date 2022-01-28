#include "{{CLASSNAME}}.h"
#include "exahype2/RefinementControl.h"


tarch::logging::Log   {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );



{% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" %}
::exahype2::RefinementCommand {{NAMESPACE | join("::")}}::{{CLASSNAME}}::refinementCriterion(
  const double * __restrict__ Q,               // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const double * __restrict__ dQdx,            // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t
) {
  logTraceInWith2Arguments( "refinementCriterion(...)", x, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  // see comments in header file

  logTraceOutWith1Argument( "refinementCriterion(...)", ::toString(result) );
  return result;
}
{% endif %}


{% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  bool                                         gridIsConstructred
) {
  logTraceInWith2Arguments( "initialCondition(...)", x, gridIsConstructred );

  // @todo Implement your stuff here

  logTraceOut( "initialCondition(...)" );
}
{% endif %}



{% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::boundaryConditions(
  const double * __restrict__                  Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  double * __restrict__                        Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
) {
  logTraceInWith3Arguments( "boundaryConditions(...)", x, t, normal );
  // @todo implement
  logTraceOut( "boundaryConditions(...)" );
}
{% endif %}


{{SOLVER_USER_DEFINITIONS}}
