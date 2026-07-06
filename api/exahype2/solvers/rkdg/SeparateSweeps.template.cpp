// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"
#include "exahype2/RefinementControl.h"





{% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" %}
::exahype2::RefinementCommand {{NAMESPACE | join("::")}}::{{CLASSNAME}}::refinementCriterion(
  const double * NOALIAS Q,               // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<DIMENSIONS,double>&  volumeX,
  const tarch::la::Vector<DIMENSIONS,double>&  volumeH,
  double                                       t
) {
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  // see comments in header file

  return result;
}
{% endif %}

{% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initialCondition(
  double * NOALIAS Q,
  const tarch::la::Vector<DIMENSIONS,double>&  x,
  const tarch::la::Vector<DIMENSIONS,double>&  h
) {
  // @todo Implement your stuff here, but ensure all entries are
  //   initialised incl all auxiliary variables. The snippet below
  //   sets everything to zero
  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }
}
{% endif %}

{% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::boundaryConditions(
  const double * NOALIAS                  Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  double * NOALIAS                        Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<DIMENSIONS,double>&  x,
  double                                       t,
  int                                          normal
) {
  // @todo implement
}
{% endif %}


{{SOLVER_USER_DEFINITIONS}}
