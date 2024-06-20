# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

class SolverDefinitions():
  def __init__(self,
              flux_implementation,
              ncp_implementation,
              eigenvalues_implementation,
              source_term_implementation,
              material_parameters_implementation,
              point_source_implementation,
              is_linear,
              computation_precisions,
              pde_terms_without_state):
     self._flux_implementation = flux_implementation
     self._ncp_implementation = ncp_implementation
     self._eigenvalues_implementation = eigenvalues_implementation
     self._source_term_implementation = source_term_implementation
     self._material_parameters_implementation = material_parameters_implementation
     self._point_source_implementation = point_source_implementation
     self._is_linear = is_linear
     self._computation_precisions = computation_precisions
     self._pde_terms_without_state = pde_terms_without_state

  def create_solver_user_definitions(self):
      TemplateSinglePrecisions = jinja2.Template(
          """
  {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}

  double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
    const {{COMPUTATION_PRECISIONS[0]}}* __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    const tarch::la::Vector<Dimensions,double>&  x,
    const tarch::la::Vector<Dimensions,double>&  h,
    double                                       t,
    double                                       dt,
    int                                          normal
  ){
    logTraceInWith3Arguments( "eigenvalue(...)", x, t, normal );
    // @todo implement
    double maxEigenvalue = 1.0;
    return maxEigenvalue;
    logTraceOut( "eigenvalue(...)" );
  }
  {% endif %}


  {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
  void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
    const {{COMPUTATION_PRECISIONS[0]}}* __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    const tarch::la::Vector<Dimensions,double>&  x,
    const tarch::la::Vector<Dimensions,double>&  h,
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
    const tarch::la::Vector<Dimensions, double> &x,
    const tarch::la::Vector<Dimensions, double> &h,
    double t,
    double dt,
    int normal,
    {{COMPUTATION_PRECISIONS[0]}} *__restrict__ BTimesDeltaQ //BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
  ){
    logTraceInWith3Arguments( "nonconservativeProduct(...)", x, t, normal );
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
    const tarch::la::Vector<Dimensions, double> &x,
    const tarch::la::Vector<Dimensions, double> &h,
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
    const tarch::la::Vector<Dimensions,double>&  x,
    const tarch::la::Vector<Dimensions,double>&  h,
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
    const tarch::la::Vector<Dimensions,double>&  x,
    const tarch::la::Vector<Dimensions,double>&  h,
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
    const tarch::la::Vector<Dimensions,double>&  x,
    const tarch::la::Vector<Dimensions,double>&  h,
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
    const tarch::la::Vector<Dimensions,double>&  x,
    const tarch::la::Vector<Dimensions,double>&  h,
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
    const tarch::la::Vector<Dimensions, double> &x,
    const tarch::la::Vector<Dimensions, double> &h,
    double t,
    double dt,
    int normal,
    T*__restrict__ BTimesDeltaQ //BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
  ){
    logTraceInWith3Arguments( "nonconservativeProduct(...)", x, t, normal );
    // @todo implement
    logTraceOut( "nonconservativeProduct(...)" );
  }

  {% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
  template void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
    const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*__restrict__ Q,
    const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*__restrict__ deltaQ,
    const tarch::la::Vector<Dimensions, double> &x,
    const tarch::la::Vector<Dimensions, double> &h,
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
    const tarch::la::Vector<Dimensions, double> &x,
    const tarch::la::Vector<Dimensions, double> &h,
    double t,
    double dt,
    int normal,
    T* const rhs
  ) {
      // @todo implement
  }

  {% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
  template void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::multiplyMaterialParameterMatrix(const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const Q, const tarch::la::Vector<Dimensions, double> &x, const tarch::la::Vector<Dimensions, double> &h, double t, double dt, int normal, {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const rhs);
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
      d["FLUX_IMPLEMENTATION"] = self._flux_implementation
      d["NCP_IMPLEMENTATION"] = self._ncp_implementation
      d["EIGENVALUES_IMPLEMENTATION"] = self._eigenvalues_implementation
      d["SOURCE_TERM_IMPLEMENTATION"] = self._source_term_implementation
      d["MATERIAL_PARAM_IMPLEMENTATION"] = self._material_parameters_implementation
      d["POINT_SOURCE_IMPLEMENTATION"] = self._point_source_implementation
      d["IS_LINEAR"] = self._is_linear
      d["COMPUTATION_PRECISIONS"] = self._computation_precisions
      d["STATELESS_PDE_TERMS"] = self._pde_terms_without_state

      if len(self._computation_precisions) > 1:
          return TemplateMultiplePrecisions.render(**d)
      else:
          return TemplateSinglePrecisions.render(**d)
