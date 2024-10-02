# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2


class AbstractSolverDefinitions:
    def __init__(
        self,
        flux_implementation,
        ncp_implementation,
        eigenvalues_implementation,
        source_term_implementation,
        material_parameters_implementation,
        point_source_implementation,
        is_linear,
        computation_precisions,
        pde_terms_without_state,
    ):
        self._flux_implementation = flux_implementation
        self._ncp_implementation = ncp_implementation
        self._eigenvalues_implementation = eigenvalues_implementation
        self._source_term_implementation = source_term_implementation
        self._material_parameters_implementation = material_parameters_implementation
        self._point_source_implementation = point_source_implementation
        self._is_linear = is_linear
        self._computation_precisions = computation_precisions
        self._pde_terms_without_state = pde_terms_without_state

    def create_abstract_solver_user_definitions(self):
        Template = jinja2.Template(
            """
{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}

{% if EIGENVALUES_IMPLEMENTATION!="<empty>" and EIGENVALUES_IMPLEMENTATION!="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  h,
  [[maybe_unused]] double                                                        t,
  [[maybe_unused]] double                                                        dt,
  [[maybe_unused]] int                                                           normal
) {
  {{EIGENVALUES_IMPLEMENTATION}}
}
{% endif %}

{% if FLUX_IMPLEMENTATION!="<empty>" and FLUX_IMPLEMENTATION!="<user-defined>" and FLUX_IMPLEMENTATION!="<none>"%}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__  Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                   x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                   h,
  [[maybe_unused]] double                                                         t,
  [[maybe_unused]] double                                                         dt,
  [[maybe_unused]] int                                                            normal,
  {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__                         F
) {
  {{FLUX_IMPLEMENTATION}}
}
{% endif %}

{% if NCP_IMPLEMENTATION!="<empty>" and NCP_IMPLEMENTATION!="<user-defined>" and NCP_IMPLEMENTATION!="<none>"%}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*__restrict__ Q,
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*__restrict__ deltaQ,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                 x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                 h,
  [[maybe_unused]] double                                                       t,
  [[maybe_unused]] double                                                       dt,
  [[maybe_unused]] int                                                          normal,
  {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*__restrict__                        BTimesDeltaQ
) {
  {{NCP_IMPLEMENTATION}}
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION!="<empty>" and SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>"%}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::algebraicSource(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const       Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                 x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                 h,
  [[maybe_unused]] double                                                       t,
  [[maybe_unused]] double                                                       dt,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*                   S
) {
  {{SOURCE_TERM_IMPLEMENTATION}}
}
{% endif %}

{% if MATERIAL_PARAM_IMPLEMENTATION!="<empty>" and MATERIAL_PARAM_IMPLEMENTATION!="<user-defined>" and MATERIAL_PARAM_IMPLEMENTATION!="<none>"%}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::multiplyMaterialParameterMatrix(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const       Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                 x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                 h,
  [[maybe_unused]] double                                                       t,
  [[maybe_unused]] double                                                       dt,
  [[maybe_unused]] int                                                          normal,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const             rhs
) {
  {{MATERIAL_PARAM_IMPLEMENTATION}}
}
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION!="<empty>" and POINT_SOURCE_IMPLEMENTATION!="<user-defined>" and POINT_SOURCE_IMPLEMENTATION!="<none>"%}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::pointSource(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const       Q,
  [[maybe_unused]] const double* const                                          x,
  [[maybe_unused]] const double                                                 t,
  [[maybe_unused]] const double                                                 dt,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const             forceVector,
  [[maybe_unused]] int                                                          n
) {
  {{POINT_SOURCE_IMPLEMENTATION}}
}
{% endif %}

{% endfor %}
""",
            undefined=jinja2.DebugUndefined,
        )

        d = {}
        d["FLUX_IMPLEMENTATION"] = self._flux_implementation
        d["NCP_IMPLEMENTATION"] = self._ncp_implementation
        d["EIGENVALUES_IMPLEMENTATION"] = self._eigenvalues_implementation
        d["SOURCE_TERM_IMPLEMENTATION"] = self._source_term_implementation
        d["MATERIAL_PARAM_IMPLEMENTATION"] = self._material_parameters_implementation
        d["POINT_SOURCE_IMPLEMENTATION"] = self._point_source_implementation
        d["IS_LINEAR"] = self._is_linear
        d["COMPUTATION_PRECISIONS"] = self._computation_precisions
        d["STATELESS_PDE_TERMS"] = self._pde_terms_without_state
        return Template.render(**d)
