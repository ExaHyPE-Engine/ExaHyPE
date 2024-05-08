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
      const tarch::la::Vector<Dimensions,double>&  volumeX,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) = 0;
{% else %}
    virtual double maxEigenvalue(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeX,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal
    );
{% endif %}

{% if FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeX,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
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
      const tarch::la::Vector<Dimensions, double> &faceCentre,
      const tarch::la::Vector<Dimensions, double> &volumeH,
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
      const tarch::la::Vector<Dimensions, double> &volumeX,
      const tarch::la::Vector<Dimensions, double> &volumeH,
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
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
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
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
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
  const tarch::la::Vector<Dimensions, double> &faceCentre,
  const tarch::la::Vector<Dimensions, double> &volumeH,
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
  const tarch::la::Vector<Dimensions, double> &volumeX,
  const tarch::la::Vector<Dimensions, double> &volumeH,
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


def create_solver_declarations(
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
    TemplateSinglePrecisions = jinja2.Template(
        """

{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
  virtual double maxEigenvalue(
    const {{COMPUTATION_PRECISIONS[0]}}* __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  volumeX,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) override;
{% endif %}
{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
    void flux(
      const {{COMPUTATION_PRECISIONS[0]}} * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  volumeX,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      {{COMPUTATION_PRECISIONS[0]}} * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
{% endif %}
{% if NCP_IMPLEMENTATION=="<user-defined>" %}
    void nonconservativeProduct(
      const {{COMPUTATION_PRECISIONS[0]}}*__restrict__ Q,
      const {{COMPUTATION_PRECISIONS[0]}}*__restrict__ deltaQ,
      const tarch::la::Vector<Dimensions, double> &faceCentre,
      const tarch::la::Vector<Dimensions, double> &volumeH,
      double t,
      double dt,
      int normal,
      {{COMPUTATION_PRECISIONS[0]}}*__restrict__ BTimesDeltaQ) override;
{% endif %}
{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    void algebraicSource(const tarch::la::Vector<Dimensions, double>& x, double t, const {{COMPUTATION_PRECISIONS[0]}} *const Q, {{COMPUTATION_PRECISIONS[0]}} *S) override;
{% endif %}
{% if MATERIAL_PARAM_IMPLEMENTATION=="<user-defined>" %}
    void multiplyMaterialParameterMatrix(
      const {{COMPUTATION_PRECISIONS[0]}}* const Q, 
      const tarch::la::Vector<Dimensions, double> &volumeX,
      const tarch::la::Vector<Dimensions, double> &volumeH,
      double t,
      double dt,
      int normal,
      {{COMPUTATION_PRECISIONS[0]}}* const rhs) override;
{% endif %}
{% if POINT_SOURCE_IMPLEMENTATION=="<user-defined>" %}
    void pointSource(const {{COMPUTATION_PRECISIONS[0]}}* const Q, const double* const x, const double t, const double dt, {{COMPUTATION_PRECISIONS[0]}}* const forceVector, int n) override;
{% endif %}
{% if POINT_SOURCE_IMPLEMENTATION!="<none>" %}
    void initPointSourceLocations(double sourceLocation[NumberOfPointSources][Dimensions]) override;
{% endif %}
  """,
        undefined=jinja2.DebugUndefined,
    )

    TemplateMultiplePrecisions = jinja2.Template(
        """
  public:
{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
    template<typename T>
    double maxEigenvalue(
      const T* __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeX,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal
    );
{% endif %}
{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
    template<typename T>
    void flux(
      const T* __restrict__                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  volumeX,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      T* __restrict__                              F // F[{{NUMBER_OF_UNKNOWNS}}]
    );
{% endif %}
{% if NCP_IMPLEMENTATION=="<user-defined>" %}
    template<typename T>
    void nonconservativeProduct(
      const T*__restrict__ Q,
      const T*__restrict__ deltaQ,
      const tarch::la::Vector<Dimensions, double> &volumeX,
      const tarch::la::Vector<Dimensions, double> &volumeH,
      double t,
      double dt,
      int normal,
      T*__restrict__ BTimesDeltaQ);

{% endif %}
{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    template<typename T>
    void algebraicSource(const tarch::la::Vector<Dimensions, double>& x, double t, const T *const Q, T *S);
{% endif %}
{% if MATERIAL_PARAM_IMPLEMENTATION=="<user-defined>" %}
    template<typename T>
    void multiplyMaterialParameterMatrix(
      const T* const Q, 
      const tarch::la::Vector<Dimensions, double> &volumeX,
      const tarch::la::Vector<Dimensions, double> &volumeH,
      double t,
      double dt,
      int normal,
      T* const rhs);
{% endif %}
{% if POINT_SOURCE_IMPLEMENTATION=="<user-defined>" %}
    template<typename T>
    void pointSource(const T* const Q, const double* const x, const double t, const double dt, T* const forceVector, int n);
{% endif %}
{% if POINT_SOURCE_IMPLEMENTATION!="<none>" %}
    void initPointSourceLocations(double sourceLocation[NumberOfPointSources][Dimensions]) override;
{% endif %}
  
  
{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}

{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}

    double maxEigenvalue(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeX,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal
    ){
      return maxEigenvalue<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>(Q, volumeX, volumeH, t, dt, normal);
    }
{% endif %}
{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
    void flux(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeX,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ F
    ){
      flux<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>(Q, volumeX, volumeH, t, dt, normal, F);
    }
{% endif %}
{% if NCP_IMPLEMENTATION=="<user-defined>" %}
    void nonconservativeProduct(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *__restrict__ Q,
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *__restrict__ deltaQ,
      const tarch::la::Vector<Dimensions, double> &volumeX,
      const tarch::la::Vector<Dimensions, double> &volumeH,
      double t,
      double dt,
      int normal,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *__restrict__ BTimesDeltaQ) override 
    {
      
      nonconservativeProduct<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>(Q, deltaQ, volumeX, volumeH, t, dt, normal, BTimesDeltaQ);

    }
{% endif %}
{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    void algebraicSource(const tarch::la::Vector<Dimensions, double>& x, double t, const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *const Q, {{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *S) override {
      algebraicSource<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>(x, t, Q, S);
    }
{% endif %}
{% if MATERIAL_PARAM_IMPLEMENTATION=="<user-defined>" %}
    void multiplyMaterialParameterMatrix(
      const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const Q, 
      const tarch::la::Vector<Dimensions, double> &volumeX,
      const tarch::la::Vector<Dimensions, double> &volumeH,
      double t,
      double dt,
      int normal,
      {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const rhs) override {
      
      multiplyMaterialParameterMatrix<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>(Q, volumeX, volumeH, t, dt, normal, rhs);

    }
{% endif %}
{% if POINT_SOURCE_IMPLEMENTATION=="<user-defined>" %}
    void pointSource(const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const Q, const double* const x, const double t, const double dt, {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const forceVector, int n) override {
      pointSource<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>(Q, x, t, dt, forceVector, n);
    }
{% endif %}
  
{% endfor %}
""",
        undefined=jinja2.DebugUndefined,
    )
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

    if len(computation_precisions) > 1:
        return TemplateMultiplePrecisions.render(**d)
    else:
        return TemplateSinglePrecisions.render(**d)


def create_solver_definitions(
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
    TemplateSinglePrecisions = jinja2.Template(
        """
  
{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}

double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const {{COMPUTATION_PRECISIONS[0]}}* __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
){
  logTraceInWith3Arguments( "eigenvalue(...)", volumeCentre, t, normal );
  // @todo implement
  double maxEigenvalue = 1.0;
  return maxEigenvalue;
  logTraceOut( "eigenvalue(...)" );
}
{% endif %}


{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const {{COMPUTATION_PRECISIONS[0]}}* __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  {{COMPUTATION_PRECISIONS[0]}}* __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceIn( "flux(...)");
  // @todo implement
  logTraceOut( "flux(...)" );
}
{% endif %}


{% if NCP_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const {{COMPUTATION_PRECISIONS[0]}} *__restrict__ Q, //Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const {{COMPUTATION_PRECISIONS[0]}} *__restrict__ deltaQ, //deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions, double> &volumeX,
  const tarch::la::Vector<Dimensions, double> &volumeH,
  double t,
  double dt,
  int normal,
  {{COMPUTATION_PRECISIONS[0]}} *__restrict__ BTimesDeltaQ //BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
){
  logTraceInWith3Arguments( "nonconservativeProduct(...)", volumeX, t, normal );
  // @todo implement
  logTraceOut( "nonconservativeProduct(...)" );
}
{% endif %}


{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::algebraicSource(
  const tarch::la::Vector<Dimensions, double>& x,
  double t,
  const {{COMPUTATION_PRECISIONS[0]}} *const Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  {{COMPUTATION_PRECISIONS[0]}} *S
) {
  logTraceInWith3Arguments( "algebraicSource(...)", x, t, dt );
  // @todo implement
  logTraceOut( "algebraicSource(...)" );
}
{% endif %}

{% if MATERIAL_PARAM_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::multiplyMaterialParameterMatrix(
  const {{COMPUTATION_PRECISIONS[0]}}* const Q,
  const tarch::la::Vector<Dimensions, double> &volumeX,
  const tarch::la::Vector<Dimensions, double> &volumeH,
  double t,
  double dt,
  int normal,
  {{COMPUTATION_PRECISIONS[0]}}* const rhs
) {
  
    // @todo implement
  
}
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::initPointSourceLocations(double sourceLocation[NumberOfPointSources][Dimensions]){
//    sourceLocation[NumberOfPointSources][Dimensions] = 0.0;

    // @todo implement
}
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::pointSource(
  const {{COMPUTATION_PRECISIONS[0]}}* const Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const double* const x, 
  const double t, 
  const double dt, 
  {{COMPUTATION_PRECISIONS[0]}}* const forceVector, // Q[{{NUMBER_OF_UNKNOWNS}}
  int n) {
  // @todo implement
}
{% endif %}

""",
        undefined=jinja2.DebugUndefined,
    )

    TemplateMultiplePrecisions = jinja2.Template(
        """
  
{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
template<typename T>
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const T* __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
) {
  logTraceInWith3Arguments( "eigenvalue(...)", x, t, normal );
  // @todo implement
  logTraceOut( "eigenvalue(...)" );
}

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeSize,
  double                                       t,
  double                                       dt,
  int                                          normal
);
{% endfor%}
{% endif %}


{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
template<typename T>
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const T* __restrict__                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  T* __restrict__                              F // F[Dimensions*{{NUMBER_OF_UNKNOWNS}}]
){
  logTraceIn( "flux(...)");
  // @todo implement
  logTraceOut( "flux(...)" );
}

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ F
);
{% endfor%}
{% endif %}


{% if NCP_IMPLEMENTATION=="<user-defined>" %}
template<typename T>
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const T*__restrict__ Q, //Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const T*__restrict__ deltaQ, //deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions, double> &volumeX,
  const tarch::la::Vector<Dimensions, double> &volumeH,
  double t,
  double dt,
  int normal,
  T*__restrict__ BTimesDeltaQ //BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
){
  logTraceInWith3Arguments( "nonconservativeProduct(...)", volumeX, t, normal );
  // @todo implement
  logTraceOut( "nonconservativeProduct(...)" );
}

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*__restrict__ Q,
  const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*__restrict__ deltaQ,
  const tarch::la::Vector<Dimensions, double> &volumeX,
  const tarch::la::Vector<Dimensions, double> &volumeH,
  double t,
  double dt,
  int normal,
  {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*__restrict__ BTimesDeltaQ
);
{% endfor%}

{% endif %}


{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
template<typename T>
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::algebraicSource(
  const tarch::la::Vector<Dimensions, double>& x,
  double t,
  const T *const Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  T *S
) {
  logTraceInWith3Arguments( "algebraicSource(...)", x, t, dt );
  // @todo implement
  logTraceOut( "algebraicSource(...)" );
}

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::algebraicSource(const tarch::la::Vector<Dimensions, double>& x, double t, const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *const Q, {{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *S);
{% endfor%}
{% endif %}

{% if MATERIAL_PARAM_IMPLEMENTATION=="<user-defined>" %}
template<typename T>
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::multiplyMaterialParameterMatrix(
  const T* const Q,
  const tarch::la::Vector<Dimensions, double> &volumeX,
  const tarch::la::Vector<Dimensions, double> &volumeH,
  double t,
  double dt,
  int normal,
  T* const rhs
) {
  
    // @todo implement
  
}

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::multiplyMaterialParameterMatrix(const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const Q, const tarch::la::Vector<Dimensions, double> &volumeX, const tarch::la::Vector<Dimensions, double> &volumeH, double t, double dt, int normal, {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const rhs);
{% endfor%}
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::initPointSourceLocations(double sourceLocation[NumberOfPointSources][Dimensions]){
//    sourceLocation[NumberOfPointSources][Dimensions] = 0.0;

    // @todo implement
}
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION=="<user-defined>" %}
template<typename T>
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::pointSource(
  const T* const Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const double* const x, 
  const double t, 
  const double dt, 
  T* const forceVector, // Q[{{NUMBER_OF_UNKNOWNS}}
  int n) {
  // @todo implement
}

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::pointSource(const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const Q, const double* const x, const double t, const double dt, {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const forceVector, int n);
{% endfor%}
{% endif %}


""",
        undefined=jinja2.DebugUndefined,
    )
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

    if len(computation_precisions) > 1:
        return TemplateMultiplePrecisions.render(**d)
    else:
        return TemplateSinglePrecisions.render(**d)


def create_compute_time_step_size_for_fixed_time_stepping():
    """

    If you work with fixed time stepping, the computation of the next
    time step size per cell or patch is trivial: You simply read out
    the solver. Therefore, this computation can trivially be a generic
    one which is the same for all solver flavours.

    """
    return """
    const double timeStepSize = repositories::{{SOLVER_INSTANCE}}.getTimeStepSize();
"""


def create_finish_time_step_implementation_for_fixed_time_stepping(
    normalised_time_step_size,
):
    return (
        """
  assertion( _minCellH >= 0.0 );
  assertion( MaxAdmissibleCellH > 0.0 );
  if (_minCellH <= MaxAdmissibleCellH) {
    _timeStepSize  = """
        + str(normalised_time_step_size)
        + """ * _minCellH;
  }
  else {
    _timeStepSize = 0.0;
  }
"""
    )


def create_start_time_step_implementation_for_fixed_time_stepping():
    """

    The outcome is used before we actually roll over the accumulation variables
    and other stuff.

    """
    return """
  if (
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxCellH>0.0
    and
    isLastGridSweepOfTimeStep()
  ) {
    logInfo( "step()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "step()", "t       = " << globalMinTimeStamp );
    logInfo( "step()", "dt      = " << getTimeStepSize() );
    logInfo( "step()", "h_{min} = " << _minCellHThisTimeStep );
    logInfo( "step()", "h_{max} = " << _maxCellHThisTimeStep );
  }
"""


def create_abstract_solver_user_declarations_for_fixed_time_stepping(timeStep):
    return (
        """
private:
  double _timeStepSize ="""
        + str(timeStep)
        + """;
public:
  double getTimeStepSize() const;  
    """
    )


def create_abstract_solver_user_definitions_for_fixed_time_stepping():
    return """
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getTimeStepSize() const {
  return _timeStepSize;
}
    """


def get_face_overlap_merge_implementation(patch_overlap):
    d = {
        "OVERLAP": patch_overlap.dim[0]
        / 2,  # assumed to always be two, but present here anyway
        "DOFS_PER_AXIS": patch_overlap.dim[1],
        "UNKNOWNS": patch_overlap.no_of_unknowns,
    }
    template = """
  
  #if Dimensions==2
  constexpr int spaceFaceSize = {DOFS_PER_AXIS}*{UNKNOWNS}; //Order+1
  #else
  constexpr int spaceFaceSize = {DOFS_PER_AXIS}*{DOFS_PER_AXIS}*{UNKNOWNS}; //(Order+1)^2
  #endif
  
  const int faceDimension = marker.getSelectedFaceNumber()%Dimensions;
  //if the outer normal is positive, the normal points to the right so the face is on the right
  const int faceLR = (marker.outerNormal()[faceDimension]>0.0 ? 0 : 1);

  //  std::copy_n(from, how_many, to);
  std::copy_n(
      &neighbour.value[(1-faceLR)*spaceFaceSize],
      spaceFaceSize,
      &value[(1-faceLR)*spaceFaceSize]
     );
  
"""

    return template.format(**d)


# function which fills an array of one type with elements of a different array, converting them via explicit conversion
def convert_array_from_type(from_name, to_name, type_to, array_size):
    template = (
        """  
      for(int i=0; i<"""
        + array_size
        + """; i++){
      """
        + to_name
        + """[i] = ( """
        + type_to
        + """ ) """
        + from_name
        + """[i];
      }  
  """
    )
    return template


# creates an array toName with array_size elements of type typeTo, then fills then with values from array fromName
def create_array_and_convert_from_type(from_name, to_name, type_to, array_size):
    template = (
        """
      """
        + type_to
        + " "
        + to_name
        + "["
        + array_size
        + "];"
        + """
      for(int i=0; i<"""
        + array_size
        + """; i++){
        """
        + to_name
        + """[i] = ( """
        + type_to
        + """ ) """
        + from_name
        + """[i];
      }  
  """
    )
    return template