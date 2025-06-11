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
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const bool                                   gridIsConstructed
    ) InlineMethod {
      {% if INITIAL_CONDITIONS_IMPLEMENTATION!="<empty>" %}
      {{INITIAL_CONDITIONS_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    void boundaryConditions(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const int                                    normal
    ) InlineMethod {
      {% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<empty>" %}
      {{BOUNDARY_CONDITIONS_IMPLEMENTATION}}
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if REFINEMENT_CRITERION_IMPLEMENTATION!="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    ::exahype2::RefinementCommand refinementCriterion(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t
    ) InlineMethod {
      {% if REFINEMENT_CRITERION_IMPLEMENTATION!="<empty>" %}
      {{REFINEMENT_CRITERION_IMPLEMENTATION}}
      {% else %}
      auto result = ::exahype2::RefinementCommand::Keep;
      return result;
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    void eigenvalues(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         L
    ) InlineMethod {
      {% if EIGENVALUES_IMPLEMENTATION!="<empty>" %}
      {{EIGENVALUES_IMPLEMENTATION}}
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if MAX_EIGENVALUE_IMPLEMENTATION!="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    double maxEigenvalue(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal
    ) InlineMethod {
      {% if MAX_EIGENVALUE_IMPLEMENTATION!="<empty>" %}
      {{MAX_EIGENVALUE_IMPLEMENTATION}}
      {% else %}
      return 0.0;
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if FLUX_IMPLEMENTATION!="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    void flux(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) InlineMethod {
      {% if FLUX_IMPLEMENTATION!="<empty>" %}
      {{FLUX_IMPLEMENTATION}}
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<user-defined>" and NCP_IMPLEMENTATION!="<none>" %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    void nonconservativeProduct(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) InlineMethod {
      {% if NCP_IMPLEMENTATION!="<empty>" %}
      {{NCP_IMPLEMENTATION}}
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    void sourceTerm(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) InlineMethod {
      {% if SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
      {{SOURCE_TERM_IMPLEMENTATION}}
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION!="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    double solveRiemannProblem(
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const {{COMPUTE_PRECISION}}* const NOALIAS   QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         FR // FR[{{NUMBER_OF_UNKNOWNS}}]
    ) InlineMethod {
      {% if RIEMANN_SOLVER_IMPLEMENTATION!="<empty>" %}
      {{RIEMANN_SOLVER_IMPLEMENTATION}}
      {% else %}
      return 0.0;
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if ADJUST_SOLUTION_IMPLEMENTATION!="<user-defined>" and ADJUST_SOLUTION_IMPLEMENTATION!="<none>" %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    void adjustSolution(
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] {{COMPUTE_PRECISION}}* const NOALIAS         Q // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    ) InlineMethod {
      {% if ADJUST_SOLUTION_IMPLEMENTATION!="<empty>" %}
      {{ADJUST_SOLUTION_IMPLEMENTATION}}
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if INITIAL_CONDITIONS_IMPLEMENTATION=="<user-defined>" %}
    void initialCondition(
      {{COMPUTE_PRECISION}}* const NOALIAS                          Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const bool                                                    gridIsConstructed
    ) override final;
    {% endif %}

    {% if BOUNDARY_CONDITIONS_IMPLEMENTATION=="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
    void boundaryConditions(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      {{COMPUTE_PRECISION}}* const NOALIAS                          Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const int                                                     normal
    ) override final;
    {% endif %}

    {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<user-defined>" and REFINEMENT_CRITERION_IMPLEMENTATION!="<none>" %}
    ::exahype2::RefinementCommand refinementCriterion(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t
    ) override final;
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
    void eigenvalues(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      {{COMPUTE_PRECISION}}* const NOALIAS                          L
    ) override final;
    {% endif %}

    {% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
    double maxEigenvalue(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
    ) override final;
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
    void flux(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
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
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      {{COMPUTE_PRECISION}}* const NOALIAS                          BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    void sourceTerm(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      {{COMPUTE_PRECISION}}* const NOALIAS                          S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    double solveRiemannProblem(
      const {{COMPUTE_PRECISION}}* const NOALIAS                    QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{COMPUTE_PRECISION}}* const NOALIAS                    QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      {{COMPUTE_PRECISION}}* const NOALIAS                          FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
      {{COMPUTE_PRECISION}}* const NOALIAS                          FR  // FR[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if ADJUST_SOLUTION_IMPLEMENTATION=="<user-defined>" and ADJUST_SOLUTION_IMPLEMENTATION!="<none>" %}
    void adjustSolution(
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      {{COMPUTE_PRECISION}}* const NOALIAS                          Q // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    ) override final;
    {% endif %}
""",
            undefined=jinja2.DebugUndefined,
        )

        return Template.render(**self.d)
