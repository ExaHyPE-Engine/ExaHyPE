# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2


class AbstractSolverDeclarations:
    def __init__(
        self,
        flux_implementation,
        ncp_implementation,
        eigenvalues_implementation,
        max_eigenvalue_implementation,
        riemann_solver_implementation,
        source_term_implementation,
        adjust_solution_implementation,
        pde_terms_without_state,
    ):
        self._flux_implementation = flux_implementation
        self._ncp_implementation = ncp_implementation
        self._eigenvalues_implementation = eigenvalues_implementation
        self._max_eigenvalue_implementation = max_eigenvalue_implementation
        self._riemann_solver_implementation = riemann_solver_implementation
        self._source_term_implementation = source_term_implementation
        self._adjust_solution_implementation = adjust_solution_implementation
        self._pde_terms_without_state = pde_terms_without_state

    def create_abstract_solver_user_declarations(self):
        Template = jinja2.Template(
            """
  public:
    {% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    void eigenvalues(
      [[maybe_unused]] const double* const __restrict__             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] double* const __restrict__                   L,
      Offloadable
    ) InlineMethod {
      {% if EIGENVALUES_IMPLEMENTATION=="<none>" %}
      tarch::gpuAbort();
      {% else %}
      {{EIGENVALUES_IMPLEMENTATION}}
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION!="<none>" %}
    virtual void eigenvalues(
      const double* const __restrict__                              Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      double* const __restrict__                                    L
    ) {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final{% endif %};
    {% endif %}

    {% if MAX_EIGENVALUE_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    double maxEigenvalue(
      [[maybe_unused]] const double* const __restrict__             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      Offloadable
    ) InlineMethod {
      {% if MAX_EIGENVALUE_IMPLEMENTATION=="<none>" %}
      tarch::gpuAbort();
      return 0.0;
      {% else %}
      {{MAX_EIGENVALUE_IMPLEMENTATION}}
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
    virtual double maxEigenvalue(
      const double* const __restrict__                              Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal
    ) {% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final{% endif %};
    {% endif %}

    {% if FLUX_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    void flux(
      [[maybe_unused]] const double* const __restrict__             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] double* const __restrict__                   F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    ) InlineMethod {
      {% if FLUX_IMPLEMENTATION=="<none>" %}
      tarch::gpuAbort();
      {% else %}
      {{FLUX_IMPLEMENTATION}}
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      const double* const __restrict__                              Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      double* const __restrict__                                    F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if FLUX_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final {% endif %};
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    void nonconservativeProduct(
      [[maybe_unused]] const double* const __restrict__             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const double* const __restrict__             deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] double* const __restrict__                   BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    ) InlineMethod {
      {% if NCP_IMPLEMENTATION=="<none>" %}
      tarch::gpuAbort();
      {% else %}
      {{NCP_IMPLEMENTATION}}
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<none>" %}
    virtual void nonconservativeProduct(
      const double* const __restrict__                              Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double* const __restrict__                              deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      double* const __restrict__                                    BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if NCP_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final {% endif %};
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    void sourceTerm(
      [[maybe_unused]] const double* const __restrict__             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] double* const __restrict__                   S, // S[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    ) InlineMethod {
      {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %}
      tarch::gpuAbort();
      {% else %}
      {{SOURCE_TERM_IMPLEMENTATION}}
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    virtual void sourceTerm(
      const double* const __restrict__                              Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      double* const __restrict__                                    S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final {% endif %};
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    double solveRiemannProblem(
      [[maybe_unused]] const double* const __restrict__             QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const double* const __restrict__             QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] double* const __restrict__                   FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] double* const __restrict__                   FR, // FR[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    ) InlineMethod {
      {% if RIEMANN_SOLVER_IMPLEMENTATION=="<none>" %}
      tarch::gpuAbort();
      return 0.0;
      {% else %}
      {{RIEMANN_SOLVER_IMPLEMENTATION}}
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    virtual double solveRiemannProblem(
      const double* const __restrict__                              QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double* const __restrict__                              QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      double* const __restrict__                                    FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
      double* const __restrict__                                    FR  // FR[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final {% endif %};
    {% endif %}

    {% if ADJUST_SOLUTION_IMPLEMENTATION!="<none>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static inline GPUCallableMethod
    void adjustSolution(
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] double* const __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      Offloadable
    ) InlineMethod {
      {% if ADJUST_SOLUTION_IMPLEMENTATION=="<none>" %}
      tarch::gpuAbort();
      {% else %}
      {{ADJUST_SOLUTION_IMPLEMENTATION}}
      {% endif %}
    }
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if ADJUST_SOLUTION_IMPLEMENTATION!="<none>" %}
    virtual void adjustSolution(
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      double* const __restrict__                                    Q // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    ) {% if ADJUST_SOLUTION_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final {% endif %};
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
        d["STATELESS_PDE_TERMS"] = self._pde_terms_without_state
        return Template.render(**d)
