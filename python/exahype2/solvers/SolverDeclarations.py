# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2


class SolverDeclarations:
    def __init__(
        self,
        flux_implementation,
        ncp_implementation,
        eigenvalues_implementation,
        max_eigenvalue_implementation,
        riemann_solver_implementation,
        source_term_implementation,
        adjust_solution_implementation,
        precision = "double"
    ):
        self._flux_implementation = flux_implementation
        self._ncp_implementation = ncp_implementation
        self._eigenvalues_implementation = eigenvalues_implementation
        self._max_eigenvalue_implementation = max_eigenvalue_implementation
        self._riemann_solver_implementation = riemann_solver_implementation
        self._source_term_implementation = source_term_implementation
        self._adjust_solution_implementation = adjust_solution_implementation
        self._precision = precision

    def create_solver_user_declarations(self):
        Template = jinja2.Template(
            """
  public:
    {% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    void eigenvalues(
      [[maybe_unused]] const {{PR}}* const noalias             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] {{PR}}* const noalias                   L
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
      [[maybe_unused]] const {{PR}}* const noalias             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
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
      [[maybe_unused]] const {{PR}}* const noalias             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] {{PR}}* const noalias                   F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) InlineMethod {
      {% if MAX_EIGENVALUE_IMPLEMENTATION!="<empty>" %}
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
      [[maybe_unused]] const {{PR}}* const noalias             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const {{PR}}* const noalias             deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] {{PR}}* const noalias                   BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) InlineMethod {
      {% if MAX_EIGENVALUE_IMPLEMENTATION!="<empty>" %}
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
      [[maybe_unused]] const {{PR}}* const noalias             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] {{PR}}* const noalias                   S // S[{{NUMBER_OF_UNKNOWNS}}]
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
      [[maybe_unused]] const {{PR}}* const noalias             QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const {{PR}}* const noalias             QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] {{PR}}* const noalias                   FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] {{PR}}* const noalias                   FR // FR[{{NUMBER_OF_UNKNOWNS}}]
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
      [[maybe_unused]] double* const noalias                   Q // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    ) InlineMethod {
      {% if ADJUST_SOLUTION_IMPLEMENTATION!="<empty>" %}
      {{ADJUST_SOLUTION_IMPLEMENTATION}}
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
    void eigenvalues(
      const {{PR}}* const noalias                              Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      {{PR}}* const noalias                                    L
    ) override final;
    {% endif %}

    {% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
    double maxEigenvalue(
      const {{PR}}* const noalias                              Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
    ) override final;
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
    void flux(
      const {{PR}}* const noalias                              Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      {{PR}}* const noalias                                    F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if NCP_IMPLEMENTATION=="<user-defined>" and NCP_IMPLEMENTATION!="<none>" %}
    void nonconservativeProduct(
      const {{PR}}* const noalias                              Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{PR}}* const noalias                              deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      {{PR}}* const noalias                                    BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    void sourceTerm(
      const {{PR}}* const noalias                              Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      {{PR}}* const noalias                                    S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    double solveRiemannProblem(
      const {{PR}}* const noalias                              QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{PR}}* const noalias                              QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      {{PR}}* const noalias                                    FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
      {{PR}}* const noalias                                    FR  // FR[{{NUMBER_OF_UNKNOWNS}}]
    ) override final;
    {% endif %}

    {% if ADJUST_SOLUTION_IMPLEMENTATION=="<user-defined>" and ADJUST_SOLUTION_IMPLEMENTATION!="<none>" %}
    void adjustSolution(
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      double* const noalias                                    Q // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    ) override final;
    {% endif %}
""",
            undefined=jinja2.DebugUndefined,
        )

        d = {}
        d["FLUX_IMPLEMENTATION"] = self._flux_implementation
        d["NCP_IMPLEMENTATION"] = self._ncp_implementation
        d["EIGENVALUES_IMPLEMENTATION"] = self._eigenvalues_implementation
        d["MAX_EIGENVALUE_IMPLEMENTATION"] = self._max_eigenvalue_implementation
        d["RIEMANN_SOLVER_IMPLEMENTATION"] = self._riemann_solver_implementation
        d["SOURCE_TERM_IMPLEMENTATION"] = self._source_term_implementation
        d["ADJUST_SOLUTION_IMPLEMENTATION"] = self._adjust_solution_implementation
        d["PR"] = self._precision
        return Template.render(**d)
