# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.SolverSignatures import *


class SolverDeclarations:
    def create_solver_user_declarations(self):
        template = (
            """

    {%- if DIMENSIONS == 2 -%}
    {%- set DIMENSION_DESCRIPTOR = ["Left", "Right", "Down", "Top"] -%}
    {%- elif DIMENSIONS == 3 -%}
    {%- set DIMENSION_DESCRIPTOR =  ["Left", "Right", "Down", "Top", "Back", "Front"]  -%}
    {%- endif -%}

    {% if INITIAL_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}
    static inline
    void initialCondition(
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const bool                                   gridIsConstructed
    )  {
      {% if INITIAL_CONDITIONS_IMPLEMENTATION!="<empty>" %}
      {{INITIAL_CONDITIONS_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
    void initialCondition(
      {{COMPUTE_PRECISION}}* const NOALIAS                          Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      const tarch::la::Vector<DIMENSIONS, double>&                  h,
      const bool                                                    gridIsConstructed
    ) override final;
    {% endif %}

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void boundaryConditions(
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
    )  {
      {% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<empty>" %}
      {{BOUNDARY_CONDITIONS_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
    void boundaryConditions(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      {% for i in range(ORDER) -%}
      {% if loop.first -%}
      // closest node outside
      {%- elif loop.last %}
      // furthest node outside
      {%- endif %}
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         Qoutside{{i}}, // Qoutside{{i}}[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      {%- endfor %}
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      const tarch::la::Vector<DIMENSIONS, double>&                  h,
      const double                                                  t,
      const int                                                     normal
    ) override final;
    {% endif %}

    {% if REFINEMENT_CRITERION_IMPLEMENTATION!="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    ::exahype2::RefinementCommand refinementCriterion(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t
    )  {
      {% if REFINEMENT_CRITERION_IMPLEMENTATION!="<empty>" %}
      {{REFINEMENT_CRITERION_IMPLEMENTATION}}
      {% else %}
      auto result = ::exahype2::RefinementCommand::Keep;
      return result;
      {% endif %}
    }
    {% endif %}

    {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
    ::exahype2::RefinementCommand refinementCriterion(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      const tarch::la::Vector<DIMENSIONS, double>&                  h,
      const double                                                  t
    ) override final;
    {% endif %}

    {% if MAX_EIGENVALUE_IMPLEMENTATION!="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    double maxEigenvalue(
      """
            + max_eigenvalue_signature
            + """
    )  {
      {% if MAX_EIGENVALUE_IMPLEMENTATION!="<empty>" %}
      {{MAX_EIGENVALUE_IMPLEMENTATION}}
      {% else %}
      return 0.0;
      {% endif %}
    }
    {% endif %}

    {% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
    double maxEigenvalue(
      """
            + max_eigenvalue_signature
            + """
    ) override final;
    {% endif %}

    {% if FLUX_IMPLEMENTATION!="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void flux(
      """
            + flux_signature
            + """
    )  {
      {% if FLUX_IMPLEMENTATION!="<empty>" %}
      {{FLUX_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" and FLUX_IMPLEMENTATION!= "<none>" %}
    void flux(
      """
            + flux_signature
            + """
    ) override final;
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void sourceTerm(
      """
            + source_term_signature
            + """
    )  {
      {% if SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
      {{SOURCE_TERM_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    void sourceTerm(
      """
            + source_term_signature
            + """
    ) override final;
    {% endif %}

    {% if SOLVER_IMPLEMENTATION!="<user-defined>" and SOLVER_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void solve(
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
    )  {
      {% if SOLVER_IMPLEMENTATION!="<empty>" %}
      {{SOLVER_IMPLEMENTATION}}
      {% else %}
      return 0.0;
      {% endif %}
    }
    {% endif %}

    {% if SOLVER_IMPLEMENTATION=="<user-defined>" and SOLVER_IMPLEMENTATION!="<none>" %}
    void solve(
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
      {{COMPUTE_PRECISION}}* const NOALIAS                          QNew // QNew[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}
"""
        )

        return template
