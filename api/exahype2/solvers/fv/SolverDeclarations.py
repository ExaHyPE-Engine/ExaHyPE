# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


class SolverDeclarations:
    def create_solver_user_declarations(self):
        template = """
    {% if INITIAL_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}
    static INLINE_METHOD
    void initialCondition(
      [[maybe_unused]] {{STORAGE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h
    ) {
      {% if INITIAL_CONDITIONS_IMPLEMENTATION!="<empty>" %}
      {{INITIAL_CONDITIONS_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
    static INLINE_METHOD GPU_CALLABLE_METHOD
    void boundaryConditions(
      [[maybe_unused]] const {{STORAGE_PRECISION}}* const NOALIAS Qinside, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] {{STORAGE_PRECISION}}* const NOALIAS Qoutside, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double t,
      [[maybe_unused]] const int normal
    ) {
      {% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<empty>" %}
      {{BOUNDARY_CONDITIONS_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if REFINEMENT_CRITERION_IMPLEMENTATION!="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
    static INLINE_METHOD GPU_CALLABLE_METHOD
    ::exahype2::RefinementCommand refinementCriterion(
      [[maybe_unused]] const {{STORAGE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_VOLUMES_PER_AXIS}}^DIMENSIONS * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}})]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double t
    ) {
      {% if REFINEMENT_CRITERION_IMPLEMENTATION!="<empty>" %}
      {{REFINEMENT_CRITERION_IMPLEMENTATION}}
      {% else %}
      return ::exahype2::RefinementCommand::Keep;
      {% endif %}
    }
    {% endif %}

    {% if MAX_EIGENVALUE_IMPLEMENTATION!="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
    static INLINE_METHOD GPU_CALLABLE_METHOD
    double maxEigenvalue(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double t,
      [[maybe_unused]] const double dt,
      [[maybe_unused]] const int normal
    ) {
      {% if MAX_EIGENVALUE_IMPLEMENTATION!="<empty>" %}
      {{MAX_EIGENVALUE_IMPLEMENTATION}}
      {% else %}
      return 0.0;
      {% endif %}
    }
    {% endif %}

    {% if FLUX_IMPLEMENTATION!="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
    static INLINE_METHOD GPU_CALLABLE_METHOD
    void flux(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double t,
      [[maybe_unused]] const double dt,
      [[maybe_unused]] const int normal,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS F // [{{NUMBER_OF_UNKNOWNS}}]
    ) {
      {% if FLUX_IMPLEMENTATION!="<empty>" %}
      {{FLUX_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<user-defined>" and NCP_IMPLEMENTATION!="<none>" %}
    static INLINE_METHOD GPU_CALLABLE_METHOD
    void nonconservativeProduct(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS deltaQ, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double t,
      [[maybe_unused]] const double dt,
      [[maybe_unused]] const int normal,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS BTimesDeltaQ // [{{NUMBER_OF_UNKNOWNS}}]
    ) {
      {% if NCP_IMPLEMENTATION!="<empty>" %}
      {{NCP_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    static INLINE_METHOD GPU_CALLABLE_METHOD
    void sourceTerm(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double t,
      [[maybe_unused]] const double dt,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS S // [{{NUMBER_OF_UNKNOWNS}}]
    ) {
      {% if SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
      {{SOURCE_TERM_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    static INLINE_METHOD GPU_CALLABLE_METHOD
    void diffusiveSourceTerm(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS deltaQ, // [{{DIMENSIONS}} * {{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double t,
      [[maybe_unused]] const double dt,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS S // [{{NUMBER_OF_UNKNOWNS}}]
    ) {
      {% if DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
      {{DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION!="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    static INLINE_METHOD GPU_CALLABLE_METHOD
    double solveRiemannProblem(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS QL, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS QR, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double t,
      [[maybe_unused]] const double dt,
      [[maybe_unused]] const int normal,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS FL, // [{{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS FR // [{{NUMBER_OF_UNKNOWNS}}]
    ) {
      {% if RIEMANN_SOLVER_IMPLEMENTATION!="<empty>" %}
      {{RIEMANN_SOLVER_IMPLEMENTATION}}
      {% else %}
      return 0.0;
      {% endif %}
    }
    {% endif %}

    {% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
    void initialCondition(
      {{STORAGE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h
    ) override final;
    {% endif %}

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
    void boundaryConditions(
      const {{STORAGE_PRECISION}}* const NOALIAS Qinside, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      {{STORAGE_PRECISION}}* const NOALIAS Qoutside, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const double t,
      const int normal
    ) override final;
    {% endif %}

    {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
    ::exahype2::RefinementCommand refinementCriterion(
      const {{STORAGE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_VOLUMES_PER_AXIS}}^DIMENSIONS * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}})]
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const double t
    ) override final;
    {% endif %}

    {% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
    static double maxEigenvalue(
      const {{COMPUTE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const double t,
      const double dt,
      const int normal
    );
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
    static void flux(
      const {{COMPUTE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const double t,
      const double dt,
      const int normal,
      {{COMPUTE_PRECISION}}* const NOALIAS F // [{{NUMBER_OF_UNKNOWNS}}]
    );
    {% endif %}

    {% if NCP_IMPLEMENTATION=="<user-defined>" and NCP_IMPLEMENTATION!="<none>" %}
    static void nonconservativeProduct(
      const {{COMPUTE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{COMPUTE_PRECISION}}* const NOALIAS deltaQ, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const double t,
      const double dt,
      const int normal,
      {{COMPUTE_PRECISION}}* const NOALIAS BTimesDeltaQ // [{{NUMBER_OF_UNKNOWNS}}]
    );
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    static void sourceTerm(
      const {{COMPUTE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const double t,
      const double dt,
      {{COMPUTE_PRECISION}}* const NOALIAS S // [{{NUMBER_OF_UNKNOWNS}}]
    );
    {% endif %}

    {% if DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    static void diffusiveSourceTerm(
      const {{COMPUTE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{COMPUTE_PRECISION}}* const NOALIAS deltaQ, // [{{DIMENSIONS}} * {{NUMBER_OF_UNKNOWNS}}]
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const double t,
      const double dt,
      {{COMPUTE_PRECISION}}* const NOALIAS S // [{{NUMBER_OF_UNKNOWNS}}]
    );
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    static double solveRiemannProblem(
      const {{COMPUTE_PRECISION}}* const NOALIAS QL, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{COMPUTE_PRECISION}}* const NOALIAS QR, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h,
      const double t,
      const double dt,
      const int normal,
      {{COMPUTE_PRECISION}}* const NOALIAS FL, // [{{NUMBER_OF_UNKNOWNS}}]
      {{COMPUTE_PRECISION}}* const NOALIAS FR // [{{NUMBER_OF_UNKNOWNS}}]
    );
    {% endif %}
"""

        return template
