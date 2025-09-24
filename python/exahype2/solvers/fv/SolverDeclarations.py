# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2


class SolverDeclarations:
    def __init__(self, dictionary):
        self.d = dictionary

    def create_solver_user_declarations(self):
        Template = jinja2.Template(
            """
    {% if INITIAL_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}
    static inline
    void initialCondition(
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const bool                                   gridIsConstructed
    ) {
      {% if INITIAL_CONDITIONS_IMPLEMENTATION!="<empty>" %}
      {{INITIAL_CONDITIONS_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void boundaryConditions(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const int                                    normal
    ) {
      {% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<empty>" %}
      {{BOUNDARY_CONDITIONS_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if REFINEMENT_CRITERION_IMPLEMENTATION!="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    ::exahype2::RefinementCommand refinementCriterion(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t
    ) {
      {% if REFINEMENT_CRITERION_IMPLEMENTATION!="<empty>" %}
      {{REFINEMENT_CRITERION_IMPLEMENTATION}}
      {% else %}
      auto result = ::exahype2::RefinementCommand::Keep;
      return result;
      {% endif %}
    }
    {% endif %}

    {% if MAX_EIGENVALUE_IMPLEMENTATION!="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    double maxEigenvalue(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal
    ) {
      {% if MAX_EIGENVALUE_IMPLEMENTATION!="<empty>" %}
      {{MAX_EIGENVALUE_IMPLEMENTATION}}
      {% else %}
      return 0.0;
      {% endif %}
    }
    {% endif %}

    {% if FLUX_IMPLEMENTATION!="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void flux(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) {
      {% if FLUX_IMPLEMENTATION!="<empty>" %}
      {{FLUX_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<user-defined>" and NCP_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void nonconservativeProduct(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) {
      {% if NCP_IMPLEMENTATION!="<empty>" %}
      {{NCP_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void sourceTerm(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) {
      {% if SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
      {{SOURCE_TERM_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void diffusiveSourceTerm(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   deltaQ, // deltaQ[{{DIMENSIONS}} * ({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) {
      {% if DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
      {{DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION!="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    double solveRiemannProblem(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         FR // FR[{{NUMBER_OF_UNKNOWNS}}]
    ) {
      {% if RIEMANN_SOLVER_IMPLEMENTATION!="<empty>" %}
      {{RIEMANN_SOLVER_IMPLEMENTATION}}
      {% else %}
      return 0.0;
      {% endif %}
    }
    {% endif %}

    {% if ADJUST_SOLUTION_IMPLEMENTATION!="<user-defined>" and ADJUST_SOLUTION_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    void adjustSolution(
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         Q // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    ) {
      {% if ADJUST_SOLUTION_IMPLEMENTATION!="<empty>" %}
      {{ADJUST_SOLUTION_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if LIMITER_IMPLEMENTATION!="<user-defined>" and LIMITER_IMPLEMENTATION!="<none>" %}
    static inline GPU_CALLABLE_METHOD
    {{COMPUTE_PRECISION}} limit(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QL,
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q,
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QR,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] const int                                    unknown
    ) {
      {% if LIMITER_IMPLEMENTATION!="<empty>" %}
      {{LIMITER_IMPLEMENTATION}}
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

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
    void boundaryConditions(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      {{COMPUTE_PRECISION}}* const NOALIAS                          Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      const tarch::la::Vector<DIMENSIONS, double>&                  h,
      const double                                                  t,
      const int                                                     normal
    ) override final;
    {% endif %}

    {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
    ::exahype2::RefinementCommand refinementCriterion(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      const tarch::la::Vector<DIMENSIONS, double>&                  h,
      const double                                                  t
    ) override final;
    {% endif %}

    {% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
    double maxEigenvalue(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      const tarch::la::Vector<DIMENSIONS, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal
    ) override final;
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
    void flux(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      const tarch::la::Vector<DIMENSIONS, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      {{COMPUTE_PRECISION}}* const NOALIAS                          F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if NCP_IMPLEMENTATION=="<user-defined>" and NCP_IMPLEMENTATION!="<none>" %}
    void nonconservativeProduct(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{COMPUTE_PRECISION}}* const NOALIAS                    deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      const tarch::la::Vector<DIMENSIONS, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      {{COMPUTE_PRECISION}}* const NOALIAS                          BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    void sourceTerm(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      const tarch::la::Vector<DIMENSIONS, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      {{COMPUTE_PRECISION}}* const NOALIAS                          S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    void diffusiveSourceTerm(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{COMPUTE_PRECISION}}* const NOALIAS                    deltaQ, // deltaQ[{{DIMENSIONS}} * ({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})]
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      const tarch::la::Vector<DIMENSIONS, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      {{COMPUTE_PRECISION}}* const NOALIAS                          S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    double solveRiemannProblem(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{COMPUTE_PRECISION}}* const NOALIAS                    QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      const tarch::la::Vector<DIMENSIONS, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      {{COMPUTE_PRECISION}}* const NOALIAS                          FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
      {{COMPUTE_PRECISION}}* const NOALIAS                          FR  // FR[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if ADJUST_SOLUTION_IMPLEMENTATION=="<user-defined>" and ADJUST_SOLUTION_IMPLEMENTATION!="<none>" %}
    void adjustSolution(
      const tarch::la::Vector<DIMENSIONS, double>&                  x,
      const tarch::la::Vector<DIMENSIONS, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      {{COMPUTE_PRECISION}}* const NOALIAS                          Q // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    ) override final;
    {% endif %}

    {% if LIMITER_IMPLEMENTATION=="<user-defined>" and LIMITER_IMPLEMENTATION!="<none>" %}
    {{COMPUTE_PRECISION}} limit(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QL,
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q,
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QR,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] const int                                    unknown
    ) override final;
    {% endif %}
""",
            undefined=jinja2.DebugUndefined,
        )

        return Template.render(**self.d)
