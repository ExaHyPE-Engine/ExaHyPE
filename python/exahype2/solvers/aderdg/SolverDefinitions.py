# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2


class SolverDefinitions:
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

    def create_solver_user_definitions(self):
        Template_SinglePrecisions = jinja2.Template(
            """
{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[0]}}* __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&      x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&      h,
  [[maybe_unused]] double                                            t,
  [[maybe_unused]] double                                            dt,
  [[maybe_unused]] int                                               normal
) {
  logTraceInWith3Arguments("maxEigenvalue(...)", x, t, normal);
  // @todo implement
  double maxEigenvalue = 1.0;
  return maxEigenvalue;
  logTraceOut("maxEigenvalue(...)");
}
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[0]}}* __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&      x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&      h,
  [[maybe_unused]] double                                            t,
  [[maybe_unused]] double                                            dt,
  [[maybe_unused]] int                                               normal,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[0]}}* __restrict__       F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceIn("flux(...)");
  // @todo implement
  logTraceOut("flux(...)");
}
{% endif %}

{% if NCP_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[0]}}* __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[0]}}* __restrict__ deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&      x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&      h,
  [[maybe_unused]] double                                            t,
  [[maybe_unused]] double                                            dt,
  [[maybe_unused]] int                                               normal,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[0]}}* __restrict__       BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceInWith3Arguments("nonconservativeProduct(...)", x, t, normal);
  // @todo implement
  logTraceOut("nonconservativeProduct(...)");
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::algebraicSource(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[0]}}* const        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&      x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&      h,
  [[maybe_unused]] double                                            t,
  [[maybe_unused]] double                                            dt,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[0]}}*                    S
) {
  logTraceInWith6Arguments("algebraicSource(...)", Q, x, h, t, dt, S);
  // @todo implement
  logTraceOut("algebraicSource(...)");
}
{% endif %}

{% if MATERIAL_PARAM_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::multiplyMaterialParameterMatrix(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[0]}}* const        Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&      x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&      h,
  [[maybe_unused]] double                                            t,
  [[maybe_unused]] double                                            dt,
  [[maybe_unused]] int                                               normal,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[0]}}* const              rhs
) {
  // @todo implement
}
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::initPointSourceLocations(
  [[maybe_unused]] double sourceLocation[NumberOfPointSources][Dimensions]
) {
  // @todo implement
}
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::pointSource(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[0]}}* const        Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* const                               x,
  [[maybe_unused]] const double                                      t,
  [[maybe_unused]] const double                                      dt,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[0]}}* const              forceVector, // forceVector[{{NUMBER_OF_UNKNOWNS}}
  [[maybe_unused]] int                                               n
) {
  // @todo implement
}
{% endif %}
""",
            undefined=jinja2.DebugUndefined,
        )

        Template_MultiplePrecisions = jinja2.Template(
            """
{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
template <typename T>
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  [[maybe_unused]] const T* __restrict__                                         Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  h,
  [[maybe_unused]] double                                                        t,
  [[maybe_unused]] double                                                        dt,
  [[maybe_unused]] int                                                           normal
) {
  logTraceInWith3Arguments("maxEigenvalue(...)", x, t, normal);
  // @todo implement
  logTraceOut("maxEigenvalue(...)");
}

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  h,
  [[maybe_unused]] double                                                        t,
  [[maybe_unused]] double                                                        dt,
  [[maybe_unused]] int                                                           normal
);
{% endfor%}
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
template <typename T>
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const T* __restrict__                                         Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  h,
  [[maybe_unused]] double                                                        t,
  [[maybe_unused]] double                                                        dt,
  [[maybe_unused]] int                                                           normal,
  [[maybe_unused]] T* __restrict__                                               F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceIn( "flux(...)");
  // @todo implement
  logTraceOut( "flux(...)" );
}

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  h,
  [[maybe_unused]] double                                                        t,
  [[maybe_unused]] double                                                        dt,
  [[maybe_unused]] int                                                           normal,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__       F
);
{% endfor%}
{% endif %}

{% if NCP_IMPLEMENTATION=="<user-defined>" %}
template <typename T>
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const T* __restrict__                                         Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const T* __restrict__                                         deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  h,
  [[maybe_unused]] double                                                        t,
  [[maybe_unused]] double                                                        dt,
  [[maybe_unused]] int                                                           normal,
  [[maybe_unused]] T* __restrict__                                               BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceInWith3Arguments("nonconservativeProduct(...)", x, t, normal);
  // @todo implement
  logTraceOut("nonconservativeProduct(...)");
}

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ Q,
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__ deltaQ,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  h,
  [[maybe_unused]] double                                                        t,
  [[maybe_unused]] double                                                        dt,
  [[maybe_unused]] int                                                           normal,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* __restrict__       BTimesDeltaQ
);
{% endfor%}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
template <typename T>
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::algebraicSource(
  [[maybe_unused]] const T* const                                                Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  h,
  [[maybe_unused]] double                                                        t,
  [[maybe_unused]] double                                                        dt,
  [[maybe_unused]] T*                                                            S
) {
  logTraceInWith6Arguments("algebraicSource(...)", Q, x, h, t, dt, S);
  // @todo implement
  logTraceOut("algebraicSource(...)");
}

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::algebraicSource(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const        Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  h,
  [[maybe_unused]] double                                                        t,
  [[maybe_unused]] double                                                        dt,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}*                    S
);
{% endfor%}
{% endif %}

{% if MATERIAL_PARAM_IMPLEMENTATION=="<user-defined>" %}
template <typename T>
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::multiplyMaterialParameterMatrix(
  [[maybe_unused]] const T* const                                                Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  h,
  [[maybe_unused]] double                                                        t,
  [[maybe_unused]] double                                                        dt,
  [[maybe_unused]] int                                                           normal,
  [[maybe_unused]] T* const                                                      rhs
) {
  // @todo implement
}

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::multiplyMaterialParameterMatrix(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const        Q,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>&                  h,
  [[maybe_unused]] double                                                        t,
  [[maybe_unused]] double                                                        dt,
  [[maybe_unused]] int                                                           normal,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const              rhs
);
{% endfor%}
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::initPointSourceLocations(
  [[maybe_unused]] double sourceLocation[NumberOfPointSources][Dimensions]
) {
  // @todo implement
}
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION=="<user-defined>" %}
template <typename T>
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::pointSource(
  [[maybe_unused]] const T* const                                                Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* const                                           x,
  [[maybe_unused]] const double                                                  t,
  [[maybe_unused]] const double                                                  dt,
  [[maybe_unused]] T* const                                                      forceVector, // forceVector[{{NUMBER_OF_UNKNOWNS}}
  [[maybe_unused]] int                                                           n
) {
  // @todo implement
}

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
template void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::pointSource(
  [[maybe_unused]] const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const        Q,
  [[maybe_unused]] const double* const                                           x,
  [[maybe_unused]] const double                                                  t,
  [[maybe_unused]] const double                                                  dt,
  [[maybe_unused]] {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const              forceVector,
  [[maybe_unused]] int                                                           n
);
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
            return Template_MultiplePrecisions.render(**d)
        else:
            return Template_SinglePrecisions.render(**d)
