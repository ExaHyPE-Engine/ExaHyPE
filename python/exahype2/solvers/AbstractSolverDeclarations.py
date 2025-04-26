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

    def create_abstract_solver_user_declarations(self):
        Template = jinja2.Template(
            """
  public:
    {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
    virtual void eigenvalues(
      const {{PR}}* const NOALIAS                              Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      {{PR}}* const NOALIAS                                    L
    ) = 0;
    {% endif %}

    {% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
    virtual double maxEigenvalue(
      const {{PR}}* const NOALIAS                              Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal
    ) = 0;
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      const {{PR}}* const NOALIAS                              Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      {{PR}}* const NOALIAS                                    F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) = 0;
    {% endif %}

    {% if NCP_IMPLEMENTATION=="<user-defined>" and NCP_IMPLEMENTATION!="<none>" %}
    virtual void nonconservativeProduct(
      const {{PR}}* const NOALIAS                              Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{PR}}* const NOALIAS                              deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      {{PR}}* const NOALIAS                                    BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) = 0;
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    virtual void sourceTerm(
      const {{PR}}* const NOALIAS                              Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      {{PR}}* const NOALIAS                                    S // S[{{NUMBER_OF_UNKNOWNS}}]
    ) = 0;
    {% endif %}

    {% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    virtual double solveRiemannProblem(
      const {{PR}}* const NOALIAS                              QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{PR}}* const NOALIAS                              QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      const int                                                     normal,
      {{PR}}* const NOALIAS                                    FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
      {{PR}}* const NOALIAS                                    FR  // FR[{{NUMBER_OF_UNKNOWNS}}]
    ) = 0;
    {% endif %}

    {% if ADJUST_SOLUTION_IMPLEMENTATION=="<user-defined>" and ADJUST_SOLUTION_IMPLEMENTATION!="<none>" %}
    virtual void adjustSolution(
      const tarch::la::Vector<Dimensions, double>&                  x,
      const tarch::la::Vector<Dimensions, double>&                  h,
      const double                                                  t,
      const double                                                  dt,
      double* const NOALIAS                                    Q // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    ) = 0;
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
