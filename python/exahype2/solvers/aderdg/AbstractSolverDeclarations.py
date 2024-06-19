# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms import PDETerms

import jinja2

def create_abstract_solver_declarations(
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
public:

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}

{% if EIGENVALUES_IMPLEMENTATION=="<none>" or EIGENVALUES_IMPLEMENTATION=="<empty>" %}
  #error eigenvalue implementation cannot be none or empty
{% elif EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
    virtual double maxEigenvalue(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) = 0;
{% else %}
    virtual double maxEigenvalue(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h,
      double                                       t,
      double                                       dt,
      int                                          normal
    );
{% endif %}

{% if FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h,
      double                                       t,
      double                                       dt,
      int                                          normal,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ F
    )  {% if FLUX_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final {% endif %};
{% endif %}

{% if NCP_IMPLEMENTATION!="<none>" %}
    virtual void nonconservativeProduct(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*__restrict__ Q,
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*__restrict__ deltaQ,
      const tarch::la::Vector<Dimensions, double> &x,
      const tarch::la::Vector<Dimensions, double> &h,
      double t,
      double dt,
      int normal,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*__restrict__ BTimesDeltaQ
    ) {% if NCP_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final {% endif %};
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    virtual void algebraicSource(
      const tarch::la::Vector<Dimensions, double>& x,
      double t,
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *const Q,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *S
      ) {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final {% endif %};
{% endif %}

{% if MATERIAL_PARAMETER_IMPLEMENTATION!="<none>" %}
    virtual void multiplyMaterialParameterMatrix(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const Q,
      const tarch::la::Vector<Dimensions, double> &x,
      const tarch::la::Vector<Dimensions, double> &h,
      double t,
      double dt,
      int normal,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const rhs
      ) {% if MATERIAL_PARAMETER_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final {% endif %};
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION!="<none>" %}
    virtual void pointSource(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const Q,
      const double* const x,
      const double t,
      const double dt,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const forceVector,
      int n
    ) {% if POINT_SOURCE_IMPLEMENTATION=="<user-defined>" %} = 0{% else %} final {% endif %};
{% endif %}

{% endfor %}

{% if POINT_SOURCE_IMPLEMENTATION!="<none>" %}
    virtual void initPointSourceLocations(
      double sourceLocation[NumberOfPointSources][Dimensions]
    ) = 0;
{% endif %}
""",
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["FLUX_IMPLEMENTATION"] = flux_implementation
    d["NCP_IMPLEMENTATION"] = ncp_implementation
    d["EIGENVALUES_IMPLEMENTATION"] = eigenvalues_implementation
    d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
    d["MATERIAL_PARAMETER_IMPLEMENTATION"] = material_param_implementation
    d["POINT_SOURCE_IMPLEMENTATION"] = point_source_implementation
    d["IS_LINEAR"] = is_linear
    d["COMPUTATION_PRECISIONS"] = computation_precisions
    d["STATELESS_PDE_TERMS"] = pde_terms_without_state
    return Template.render(**d)
