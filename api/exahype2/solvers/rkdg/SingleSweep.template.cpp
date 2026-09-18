// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"
#include "exahype2/RefinementControl.h"

{% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" %}
/**
 * @param Q Whole nodal patch in AoS layout, or nullptr during grid construction.
 * @param x Patch centre.
 * @param h Patch size.
 * @param t Time.
 */
::exahype2::RefinementCommand {{NAMESPACE | join("::")}}::{{CLASSNAME}}::refinementCriterion(
  [[maybe_unused]] const double* const NOALIAS Q,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& h,
  [[maybe_unused]] const double t
) {
  // @todo Implement the refinement criterion here.
  return ::exahype2::RefinementCommand::Keep;
}
{% endif %}

{% if REFINEMENT_CRITERION_IMPLEMENTATION!="<user-defined>" %}
::exahype2::RefinementCommand {{NAMESPACE | join("::")}}::{{CLASSNAME}}::refinementCriterion(
  [[maybe_unused]] const double* const NOALIAS Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& h,
  [[maybe_unused]] const double t
) {
  {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<empty>" %}
  return ::exahype2::RefinementCommand::Keep;
  {% else %}
  {{REFINEMENT_CRITERION_IMPLEMENTATION}}
  {% endif %}
}
{% endif %}

{% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initialCondition(
  [[maybe_unused]] double* const NOALIAS Q,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& h
) {
  // @todo Implement the initial condition here and initialise every entry, the
  //   auxiliary variables included.
  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }
}
{% endif %}

{% if INITIAL_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initialCondition(
  [[maybe_unused]] double* const NOALIAS Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& h
) {
  {{INITIAL_CONDITIONS_IMPLEMENTATION}}
}
{% endif %}

{% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::boundaryConditions(
  [[maybe_unused]] const double* const NOALIAS Qinside,
  [[maybe_unused]] double* const NOALIAS Qoutside,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& h,
  [[maybe_unused]] const double t,
  [[maybe_unused]] const int normal
) {
  // @todo Implement the boundary conditions here and write every outside
  //   entry, the auxiliary variables included.
  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
}
{% endif %}

{{SOLVER_USER_DEFINITIONS}}
