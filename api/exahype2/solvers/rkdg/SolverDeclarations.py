# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2


class SolverDeclarations:
    def __init__(self, dictionary):
        self.d = dictionary

    def create_solver_user_declarations(self):
        template = jinja2.Template(
            r"""
    {% if MAX_EIGENVALUE_IMPLEMENTATION!="<user-defined>" %}
    static INLINE_METHOD GPU_CALLABLE_METHOD
    double maxEigenvalue(
      [[maybe_unused]] const double* const NOALIAS Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& h,
      [[maybe_unused]] const double t,
      [[maybe_unused]] const double dt,
      [[maybe_unused]] const int normal
    ) {
      {{MAX_EIGENVALUE_IMPLEMENTATION}}
    }
    {% endif %}

    {% if FLUX_IMPLEMENTATION!="<none>" and FLUX_IMPLEMENTATION!="<user-defined>" %}
    static INLINE_METHOD GPU_CALLABLE_METHOD
    void flux(
      [[maybe_unused]] const double* const NOALIAS Q,
      {% if USE_DIFFUSIVE_FLUX %}
      [[maybe_unused]] const double* const NOALIAS deltaQ[DIMENSIONS],
      {% endif %}
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& h,
      [[maybe_unused]] const double t,
      [[maybe_unused]] const double dt,
      [[maybe_unused]] const int normal,
      [[maybe_unused]] double* const NOALIAS F
    ) {
      {% if FLUX_IMPLEMENTATION=="<empty>" %}
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        F[unknown] = 0.0;
      }
      {% else %}
      {{FLUX_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<none>" and NCP_IMPLEMENTATION!="<user-defined>" %}
    static INLINE_METHOD GPU_CALLABLE_METHOD
    void nonconservativeProduct(
      [[maybe_unused]] const double* const NOALIAS Q,
      [[maybe_unused]] const double* const NOALIAS deltaQ,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& h,
      [[maybe_unused]] const double t,
      [[maybe_unused]] const double dt,
      [[maybe_unused]] const int normal,
      [[maybe_unused]] double* const NOALIAS BTimesDeltaQ
    ) {
      {% if NCP_IMPLEMENTATION=="<empty>" %}
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        BTimesDeltaQ[unknown] = 0.0;
      }
      {% else %}
      {{NCP_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<none>" and SOURCE_TERM_IMPLEMENTATION!="<user-defined>" %}
    static INLINE_METHOD GPU_CALLABLE_METHOD
    void sourceTerm(
      [[maybe_unused]] const double* const NOALIAS Q,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& h,
      [[maybe_unused]] const double t,
      [[maybe_unused]] const double dt,
      [[maybe_unused]] double* const NOALIAS S
    ) {
      {% if SOURCE_TERM_IMPLEMENTATION=="<empty>" %}
      for (int i = 0; i < NumberOfUnknowns; i++) {
        S[i] = 0.0;
      }
      {% else %}
      {{SOURCE_TERM_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" and RIEMANN_SOLVER_IMPLEMENTATION!="<user-defined>" %}
    static INLINE_METHOD GPU_CALLABLE_METHOD
    void riemannSolver(
      [[maybe_unused]] double* const NOALIAS FL,
      [[maybe_unused]] double* const NOALIAS FR,
      [[maybe_unused]] const double* const NOALIAS QL,
      [[maybe_unused]] const double* const NOALIAS QR,
      [[maybe_unused]] const double t,
      [[maybe_unused]] const double dt,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& h,
      [[maybe_unused]] const int direction,
      [[maybe_unused]] const bool isBoundaryFace,
      [[maybe_unused]] const int faceIndex
    ) {
      {% if RIEMANN_SOLVER_IMPLEMENTATION!="<empty>" %}
      {{RIEMANN_SOLVER_IMPLEMENTATION}}
      {% endif %}
    }
    {% endif %}

    {% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" %}
    static double maxEigenvalue(
      const double* NOALIAS Q,
      const tarch::la::Vector<DIMENSIONS,double>& x,
      const tarch::la::Vector<DIMENSIONS,double>& h,
      double t,
      double dt,
      int normal
    );
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
    static void flux(
      const double* NOALIAS Q,
      {% if USE_DIFFUSIVE_FLUX %}
      const double* const NOALIAS deltaQ[DIMENSIONS],
      {% endif %}
      const tarch::la::Vector<DIMENSIONS,double>& x,
      const tarch::la::Vector<DIMENSIONS,double>& h,
      double t,
      double dt,
      int normal,
      double* NOALIAS F
    );
    {% endif %}

    {% if NCP_IMPLEMENTATION=="<user-defined>" %}
    static void nonconservativeProduct(
      const double* NOALIAS Q,
      const double* NOALIAS deltaQ,
      const tarch::la::Vector<DIMENSIONS,double>& x,
      const tarch::la::Vector<DIMENSIONS,double>& h,
      double t,
      double dt,
      int normal,
      double* NOALIAS BTimesDeltaQ
    );
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    static void sourceTerm(
      const double* NOALIAS Q,
      const tarch::la::Vector<DIMENSIONS,double>& x,
      const tarch::la::Vector<DIMENSIONS,double>& h,
      double t,
      double dt,
      double* NOALIAS S
    );
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" %}
    static void riemannSolver(
      double* NOALIAS FL,
      double* NOALIAS FR,
      const double* NOALIAS QL,
      const double* NOALIAS QR,
      double t,
      double dt,
      const tarch::la::Vector<DIMENSIONS,double>& x,
      const tarch::la::Vector<DIMENSIONS,double>& h,
      int direction,
      bool isBoundaryFace,
      int faceIndex
    );
    {% endif %}
""",
            undefined=jinja2.DebugUndefined,
        )
        return template.render(**self.d)
