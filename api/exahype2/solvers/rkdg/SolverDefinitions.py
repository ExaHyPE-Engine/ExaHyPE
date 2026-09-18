# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2


class SolverDefinitions:
    def __init__(self, dictionary):
        self.d = dictionary

    def create_solver_user_definitions(self):
        template = jinja2.Template(
            r"""
{% if MAX_EIGENVALUE_IMPLEMENTATION=="<user-defined>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  [[maybe_unused]] const double* const NOALIAS Q,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& h,
  [[maybe_unused]] const double t,
  [[maybe_unused]] const double dt,
  [[maybe_unused]] const int normal
) {
  // @todo Implement
  return 0.0;
}
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
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
  // @todo Implement
}
{% endif %}

{% if NCP_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const double* const NOALIAS Q,
  [[maybe_unused]] const double* const NOALIAS deltaQ,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& h,
  [[maybe_unused]] const double t,
  [[maybe_unused]] const double dt,
  [[maybe_unused]] const int normal,
  [[maybe_unused]] double* const NOALIAS BTimesDeltaQ
) {
  // @todo Implement
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  [[maybe_unused]] const double* const NOALIAS Q,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& x,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS,double>& h,
  [[maybe_unused]] const double t,
  [[maybe_unused]] const double dt,
  [[maybe_unused]] double* const NOALIAS S
) {
  // @todo Implement
}
{% endif %}

{% if RIEMANN_SOLVER_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::riemannSolver(
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
  // @todo Implement
}
{% endif %}
""",
            undefined=jinja2.DebugUndefined,
        )
        return template.render(**self.d)
