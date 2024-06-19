# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms import PDETerms

import jinja2

def create_abstract_solver_definitions(
    flux_implementation,
    ncp_implementation,
    eigenvalues_implementation,
    source_term_implementation,
    material_param_implementation,
    point_source_implementation,
    is_linear,
    computation_precisions,
    pde_terms_without_state,
):
    Template = jinja2.Template(
        """
{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}

{% if EIGENVALUES_IMPLEMENTATION!="<empty>" and EIGENVALUES_IMPLEMENTATION!="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t,
  double                                       dt,
  int                                          normal
){
  {{EIGENVALUES_IMPLEMENTATION}}
}
{% endif %}

{% if FLUX_IMPLEMENTATION!="<empty>" and FLUX_IMPLEMENTATION!="<user-defined>" and FLUX_IMPLEMENTATION!="<none>"%}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q, // Q[3+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t,
  double                                       dt,
  int                                          normal,
  {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ F // F[3]
){
  {{FLUX_IMPLEMENTATION}}
}
{% endif %}

{% if NCP_IMPLEMENTATION!="<empty>" and NCP_IMPLEMENTATION!="<user-defined>" and NCP_IMPLEMENTATION!="<none>"%}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*__restrict__ Q,
  const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*__restrict__ deltaQ,
  const tarch::la::Vector<Dimensions, double> &x,
  const tarch::la::Vector<Dimensions, double> &h,
  double t,
  double dt,
  int normal,
  {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*__restrict__ BTimesDeltaQ)
{
    {{NCP_IMPLEMENTATION}}
}
{% endif %}


{% if SOURCE_TERM_IMPLEMENTATION!="<empty>" and SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>"%}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::algebraicSource(const tarch::la::Vector<Dimensions, double>& volumeCentre, double t, const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *const Q, {{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *S) {
  {{SOURCE_TERM_IMPLEMENTATION}}
}
{% endif %}

{% if MATERIAL_PARAM_IMPLEMENTATION!="<empty>" and MATERIAL_PARAM_IMPLEMENTATION!="<user-defined>" and MATERIAL_PARAM_IMPLEMENTATION!="<none>"%}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::multiplyMaterialParameterMatrix(
  const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const Q,
  const tarch::la::Vector<Dimensions, double> &x,
  const tarch::la::Vector<Dimensions, double> &h,
  double t,
  double dt,
  int normal,
  {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const rhs){
  {{MATERIAL_PARAM_IMPLEMENTATION}}
}
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION!="<empty>" and POINT_SOURCE_IMPLEMENTATION!="<user-defined>" and POINT_SOURCE_IMPLEMENTATION!="<none>"%}
    void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::pointSource(const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const Q, const double* const x, const double t, const double dt, {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const forceVector,int n) {
    {{POINT_SOURCE_IMPLEMENTATION}}
}
{% endif %}

{% endfor %}
""",
        undefined=jinja2.DebugUndefined,
    )

    Exception("finish")
    d = {}
    d["FLUX_IMPLEMENTATION"] = flux_implementation
    d["NCP_IMPLEMENTATION"] = ncp_implementation
    d["EIGENVALUES_IMPLEMENTATION"] = eigenvalues_implementation
    d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
    d["MATERIAL_PARAM_IMPLEMENTATION"] = material_param_implementation
    d["POINT_SOURCE_IMPLEMENTATION"] = point_source_implementation
    d["IS_LINEAR"] = is_linear
    d["COMPUTATION_PRECISIONS"] = computation_precisions
    d["STATELESS_PDE_TERMS"] = pde_terms_without_state
    return Template.render(**d)
