# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2


class SolverDefinitions:
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

    def create_solver_user_definitions(self):
        Template = jinja2.Template(
            """
{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::eigenvalues(
  [[maybe_unused]] const {{PR}}* const noalias             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] {{PR}}* const noalias                   L
) {
  // @todo Implement
}
{% endif %}

{% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" and MAX_EIGENVALUE_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  [[maybe_unused]] const {{PR}}* const noalias             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal
) {
  // @todo Implement
  return 0.0;
}
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" and FLUX_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const {{PR}}* const noalias             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] {{PR}}* const noalias                   F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  // @todo Implement
}
{% endif %}

{% if NCP_IMPLEMENTATION=="<user-defined>" and NCP_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const {{PR}}* const noalias             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const {{PR}}* const noalias             deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] {{PR}}* const noalias                   BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  // @todo Implement
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  [[maybe_unused]] const {{PR}}* const noalias             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] {{PR}}* const noalias                   S // S[{{NUMBER_OF_UNKNOWNS}}]
) {
  // @todo Implement
}
{% endif %}

{% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::solveRiemannProblem(
  [[maybe_unused]] const {{PR}}* const noalias             QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const {{PR}}* const noalias             QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] {{PR}}* const noalias                   FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] {{PR}}* const noalias                   FR  // FR[{{NUMBER_OF_UNKNOWNS}}]
) {
  // @todo Implement
  return 0.0;
}
{% endif %}

{% if ADJUST_SOLUTION_IMPLEMENTATION=="<user-defined>" and ADJUST_SOLUTION_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::adjustSolution(
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] double* const noalias                   Q // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
) {
  // @todo Implement
}
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
