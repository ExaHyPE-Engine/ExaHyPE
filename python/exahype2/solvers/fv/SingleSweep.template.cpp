// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

tarch::logging::Log {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log("{{NAMESPACE | join("::")}}::{{CLASSNAME}}");

{% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" %}
::exahype2::RefinementCommand {{NAMESPACE | join("::")}}::{{CLASSNAME}}::refinementCriterion(
  [[maybe_unused]] const {{STORAGE_PRECISION}}* NOALIAS    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t
) {
  logTraceInWith3Arguments( "refinementCriterion()", x, h, t );
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  // see comments in header file

  logTraceOutWith1Argument( "refinementCriterion()", ::toString(result) );
  return result;
}
{% endif %}

{% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initialCondition(
  [[maybe_unused]] {{STORAGE_PRECISION}}* NOALIAS          Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] bool                                         gridIsConstructed
) {
  logTraceInWith3Arguments( "initialCondition()", x, h, gridIsConstructed );

  // @todo Implement your stuff here, but ensure all entries are
  //       initialised incl all auxiliary variables. The snippet below
  //       sets everything to zero.
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  logTraceOut( "initialCondition()" );
}
{% endif %}

{% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::boundaryConditions(
  [[maybe_unused]] const {{STORAGE_PRECISION}}* NOALIAS    Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] {{STORAGE_PRECISION}}* NOALIAS          Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] int                                          normal
) {
  logTraceInWith4Arguments( "boundaryConditions()", x, h, t, normal );
  // @todo Implement
  logTraceOut( "boundaryConditions()" );
}
{% endif %}

{{SOLVER_USER_DEFINITIONS}}
