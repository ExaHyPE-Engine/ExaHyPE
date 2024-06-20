# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

class SolverDeclarations():
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

  def create_solver_user_declarations(self):
      TemplateSinglePrecisions = jinja2.Template(
          """

  {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
    virtual double maxEigenvalue(
      const {{COMPUTATION_PRECISIONS[0]}}* __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) override;
  {% endif %}
  {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
      void flux(
        const {{COMPUTATION_PRECISIONS[0]}} * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
        const tarch::la::Vector<Dimensions,double>&  x,
        const tarch::la::Vector<Dimensions,double>&  h,
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
        const tarch::la::Vector<Dimensions, double> &x,
        const tarch::la::Vector<Dimensions, double> &h,
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
        const tarch::la::Vector<Dimensions, double> &x,
        const tarch::la::Vector<Dimensions, double> &h,
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
        const tarch::la::Vector<Dimensions,double>&  x,
        const tarch::la::Vector<Dimensions,double>&  h,
        double                                       t,
        double                                       dt,
        int                                          normal
      );
  {% endif %}
  {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
      template<typename T>
      void flux(
        const T* __restrict__                        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
        const tarch::la::Vector<Dimensions,double>&  x,
        const tarch::la::Vector<Dimensions,double>&  h,
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
        const tarch::la::Vector<Dimensions, double> &x,
        const tarch::la::Vector<Dimensions, double> &h,
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
        const tarch::la::Vector<Dimensions, double> &x,
        const tarch::la::Vector<Dimensions, double> &h,
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
        const tarch::la::Vector<Dimensions,double>&  x,
        const tarch::la::Vector<Dimensions,double>&  h,
        double                                       t,
        double                                       dt,
        int                                          normal
      ){
        return maxEigenvalue<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>(Q, x, h, t, dt, normal);
      }
  {% endif %}
  {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
      void flux(
        const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        const tarch::la::Vector<Dimensions,double>&  h,
        double                                       t,
        double                                       dt,
        int                                          normal,
        {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ F
      ){
        flux<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>(Q, x, h, t, dt, normal, F);
      }
  {% endif %}
  {% if NCP_IMPLEMENTATION=="<user-defined>" %}
      void nonconservativeProduct(
        const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *__restrict__ Q,
        const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *__restrict__ deltaQ,
        const tarch::la::Vector<Dimensions, double> &x,
        const tarch::la::Vector<Dimensions, double> &h,
        double t,
        double dt,
        int normal,
        {{COMPUTATION_PRECISIONS[PRECISION_NUM]}} *__restrict__ BTimesDeltaQ) override
      {
        nonconservativeProduct<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);
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
        const tarch::la::Vector<Dimensions, double> &x,
        const tarch::la::Vector<Dimensions, double> &h,
        double t,
        double dt,
        int normal,
        {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const rhs) override {
        multiplyMaterialParameterMatrix<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>(Q, x, h, t, dt, normal, rhs);
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
