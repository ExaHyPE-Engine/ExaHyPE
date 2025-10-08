# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.SolverSignatures import *


class AbstractSolverDeclarations:
    def create_abstract_solver_user_declarations(self):
        template = (
            """
    {% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
    virtual void initialCondition(
      {{COMPUTE_PRECISION}}* const NOALIAS          Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&  x,
      const tarch::la::Vector<DIMENSIONS, double>&  h,
      const bool                                    gridIsConstructed
    ) = 0;
    {% endif %}

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
    /**
     * Apply boundary conditions. You can overwrite both the inside and
     * outside values though most BCs only modify the outside ones. Please
     * note that the boundary conditions you set here are after that subject
     * to the Riemann solver, i.e. flux and eigenvalues.
     */
    virtual void boundaryConditions(
      const {{COMPUTE_PRECISION}}* const NOALIAS    Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      {% for i in range(ORDER) -%}
      {% if loop.first -%}
      // closest node outside
      {%- elif loop.last %}
      // furthest node outside
      {%- endif %}
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         Qoutside{{i}}, // Qoutside{{i}}[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      {%- endfor %}
      const tarch::la::Vector<DIMENSIONS, double>&  x,
      const tarch::la::Vector<DIMENSIONS, double>&  h,
      const double                                  t,
      const int                                     normal
    ) = 0;
    {% endif %}

    {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
    /**
     * Refinement criterion
     *
     * ExaHyPE 2 is guided by a maximum and minimum mesh (patch) size.
     * All (dynamic) AMR is constrained by these values, i.e. if your
     * mesh is coarser than the maximum mesh size, ExaHyPE 2 will
     * automatically refine. If you try to refine further than the
     * minimum mesh size, ExaHyPE 2 will ignore any refinement.
     *
     * Consequently, you are fine if you work with a regular mesh:
     * You set the maximum mesh size, and you leave everything else
     * to Peano 4/ExaHyPE 2. If you want to have an adaptive mesh,
     * use this routine to implement the refinement pattern.
     *
     * @param Q This is the (current) solution. The data is not set
     *  to a valid value throughout grid construction. In this case,
     *  it is nullptr.
     */
    virtual ::exahype2::RefinementCommand refinementCriterion(
      const {{COMPUTE_PRECISION}}* const NOALIAS    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&  x,
      const tarch::la::Vector<DIMENSIONS, double>&  h,
      const double                                  t
    ) = 0;
    {% endif %}

    {% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
    virtual double maxEigenvalue(
      """
            + max_eigenvalue_signature
            + """
    ) = 0;
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      """
            + flux_signature
            + """
    ) = 0;
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    virtual void sourceTerm(
      """
            + source_term_signature
            + """
    ) = 0;
    {% endif %}

    {%- if DIMENSIONS == 2 -%}
    {%- set DIMENSION_DESCRIPTOR = ["Left", "Right", "Down", "Top"] -%}
    {%- elif DIMENSIONS == 3 -%}
    {%- set DIMENSION_DESCRIPTOR =  ["Left", "Right", "Down", "Top", "Back", "Front"]  -%}
    {%- endif -%}

    {% if SOLVER_IMPLEMENTATION=="<user-defined>" and SOLVER_IMPLEMENTATION!="<none>" %}
    virtual void solve(
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
    ) = 0;
    {% endif %}
"""
        )

        return template
