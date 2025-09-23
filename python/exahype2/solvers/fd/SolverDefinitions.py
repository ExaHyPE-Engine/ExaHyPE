# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2
from exahype2.solvers.SolverCommons import *


class SolverDefinitions:
    def create_solver_user_definitions(self):
        template = (
            """
{% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::initialCondition(
  [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const bool                                   gridIsConstructed
) {
  // @todo Implement
}
{% endif %}

{% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::boundaryConditions(
  [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  {% for i in range(ORDER) -%}
      {% if loop.first -%}
  // closest node outside
      {%- elif loop.last %}
  // furthest node outside
      {%- endif %}
  [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         Qoutside{{i}}, // Qoutside{{i}}[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      {%- endfor %}
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const int                                    normal
) {
  // @todo Implement
}
{% endif %}

{% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
::exahype2::RefinementCommand {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::refinementCriterion(
  [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
  [[maybe_unused]] const double                                 t
) {
  auto result = ::exahype2::RefinementCommand::Keep;
  // @todo Implement
  return result;
}
{% endif %}

{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::eigenvalues(
  """
            + eigenvalues_signature
            + """
) {
  // @todo Implement
}
{% endif %}

{% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  """
            + max_eigenvalue_signature
            + """
) {
  logTraceInWith4Arguments("maxEigenvalue(...)", x, h, t, normal);
  // @todo Compute max eigenvalue
  logTraceOut("maxEigenvalue(...)");
  // @todo Return max eigenvalue (make sure to call logTraceOut before the return)
  return 0.0;
}
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  """
            + flux_signature
            + """
) {
  // @todo Implement
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  """
            + source_term_signature
            + """
) {
  // @todo Implement
}
{% endif %}

{%- if DIMENSIONS == 2 -%}
{%- set DIMENSION_DESCRIPTOR = ["Left", "Right", "Down", "Top"] -%}
{%- elif DIMENSIONS == 3 -%}
{%- set DIMENSION_DESCRIPTOR =  ["Left", "Right", "Down", "Top", "Back", "Front"]  -%}
{%- endif -%}

{% if SOLVER_IMPLEMENTATION=="<user-defined>" and SOLVER_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::solve(
  const {{COMPUTE_PRECISION}}* const NOALIAS                    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  {# Comment for new line #}
  {% for D in DIMENSION_DESCRIPTOR -%}
  {% for i in range(ORDER) -%}
  {%- if loop.first -%}
  // closest to node
  {%- elif loop.last -%}
  {# Comment for new line #}
  // furthest from node
  {%- endif -%} {# comments #}
  const {{COMPUTE_PRECISION}}* const NOALIAS                    Q{{D}}{{i}}, // Q{{D}}{{i}}[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  {%- endfor -%} {# order range #}
  {# Comment for new line #}
  {% endfor %} {# for D #}
  const tarch::la::Vector<DIMENSIONS, double>&                  x,
  const tarch::la::Vector<DIMENSIONS, double>&                  h,
  const double                                                  t,
  const double                                                  dt,
  {{COMPUTE_PRECISION}}* const NOALIAS                          QNew // QNew[{{NUMBER_OF_UNKNOWNS + NUMBER_OF_AUXILIARY_VARIABLES}}]
) {
  // @todo Implement
}
{% endif %}
"""
        )
        return template
