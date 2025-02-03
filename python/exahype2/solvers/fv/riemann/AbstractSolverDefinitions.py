# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2


class AbstractSolverDefinitions:
    def __init__(
        self,
        flux_implementation,
        ncp_implementation,
        eigenvalues_implementation,
        riemann_solver_implementation,
        source_term_implementation,
        pde_terms_without_state,
    ):
        self._flux_implementation = flux_implementation
        self._ncp_implementation = ncp_implementation
        self._eigenvalues_implementation = eigenvalues_implementation
        self._riemann_solver_implementation = riemann_solver_implementation
        self._source_term_implementation = source_term_implementation
        self._pde_terms_without_state = pde_terms_without_state

    def create_abstract_solver_user_definitions(self):
        Template = jinja2.Template(
            """
{% if EIGENVALUES_IMPLEMENTATION!="<empty>" and EIGENVALUES_IMPLEMENTATION!="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::eigenvalues(
  [[maybe_unused]] const double* const __restrict__             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] double* const __restrict__                   L
) {
  {{EIGENVALUES_IMPLEMENTATION}}
}
{% endif %}

{% if FLUX_IMPLEMENTATION!="<empty>" and FLUX_IMPLEMENTATION!="<user-defined>" and FLUX_IMPLEMENTATION!="<none>"%}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const double* const __restrict__             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] double* const __restrict__                   F, // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{FLUX_IMPLEMENTATION}}
}
{% endif %}

{% if NCP_IMPLEMENTATION!="<empty>" and NCP_IMPLEMENTATION!="<user-defined>" and NCP_IMPLEMENTATION!="<none>"%}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const double* const __restrict__             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* const __restrict__             deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] double* const __restrict__                   BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{NCP_IMPLEMENTATION}}
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION!="<empty>" and SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>"%}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  [[maybe_unused]] const double* const __restrict__             Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] double* const __restrict__                   S // S[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{SOURCE_TERM_IMPLEMENTATION}}
}
{% endif %}

{% if RIEMANN_SOLVER_IMPLEMENTATION!="<empty>" and RIEMANN_SOLVER_IMPLEMENTATION!="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>"%}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::solveRiemannProblem(
  [[maybe_unused]] const double* const __restrict__             QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* const __restrict__             QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& h,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  [[maybe_unused]] const int                                    normal,
  [[maybe_unused]] double* const __restrict__                   FL, // FL[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] double* const __restrict__                   FR  // FR[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{RIEMANN_SOLVER_IMPLEMENTATION}}
}
{% endif %}
""",
            undefined=jinja2.DebugUndefined,
        )

        d = {}
        d["FLUX_IMPLEMENTATION"] = self._flux_implementation
        d["NCP_IMPLEMENTATION"] = self._ncp_implementation
        d["EIGENVALUES_IMPLEMENTATION"] = self._eigenvalues_implementation
        d["RIEMANN_SOLVER_IMPLEMENTATION"] = self._riemann_solver_implementation
        d["SOURCE_TERM_IMPLEMENTATION"] = self._source_term_implementation
        d["STATELESS_PDE_TERMS"] = self._pde_terms_without_state
        return Template.render(**d)
