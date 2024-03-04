# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from enum import Enum

from exahype2.solvers.PDETerms import PDETerms
import exahype2

from exahype2.solvers.LagrangeBasisWithDiagonalMassMatrix import GaussLegendreBasis


class SolverVariant(Enum):
    WithVirtualFunctions = 0
    Stateless = 1
    Accelerator = 2


def create_abstract_solver_declarations(
    flux_implementation,
    ncp_implementation,
    eigenvalues_implementation,
    source_term_implementation,
    point_source_implementation,
    pde_terms_without_state,
):
    Template = jinja2.Template(
        """
  public:
    {% if EIGENVALUES_IMPLEMENTATION=="<none>" or EIGENVALUES_IMPLEMENTATION=="<empty>" %}
    #error Eigenvalue implementation cannot be none or empty
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it 
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}

    /**
     * Determine max eigenvalue over Jacobian in a given point with solution values
     * (states) Q. All parameters are in.
     *
     * @return Max eigenvalue. Result has to be positive, so we are actually speaking
     *   about the maximum absolute eigenvalue.
     */
    virtual double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};

    {% if FLUX_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it 
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if FLUX_IMPLEMENTATION=="<user-defined>" %}=0{% else %} final {% endif %};
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static void nonconservativeProduct(
      const double * __restrict__                  Q,    // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__                  deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<none>" %}
    virtual void nonconservativeProduct(
      const double * __restrict__                  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__                  deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__ BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if NCP_IMPLEMENTATION=="<user-defined>" %}=0{% endif %};
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static void sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      double                                       t,
      double                                       dt,
      double * __restrict__ S,
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    virtual void sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      double                                       t,
      double                                       dt,
      double * __restrict__ S
    ) {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};
    {% endif %}

    {% if POINT_SOURCE_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it 
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    void pointSource(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double                                       dt,
      double * __restrict__ S,
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if POINT_SOURCE_IMPLEMENTATION!="<none>" %}
    virtual void pointSource(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double                                       dt,
      double * __restrict__ S
    ) {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};
    {% endif %}
""",
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["FLUX_IMPLEMENTATION"] = flux_implementation
    d["NCP_IMPLEMENTATION"] = ncp_implementation
    d["EIGENVALUES_IMPLEMENTATION"] = eigenvalues_implementation
    d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
    d["POINT_SOURCE_IMPLEMENTATION"] = point_source_implementation
    d["STATELESS_PDE_TERMS"] = pde_terms_without_state
    return Template.render(**d)


def create_abstract_solver_definitions(
    flux_implementation,
    ncp_implementation,
    eigenvalues_implementation,
    source_term_implementation,
    point_source_implementation,
    pde_terms_without_state,
):
    Template = jinja2.Template(
        """
{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal
) {
  {{EIGENVALUES_IMPLEMENTATION}}
}
{% endif %}

{% if FLUX_IMPLEMENTATION!="<none>" and FLUX_IMPLEMENTATION!="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{FLUX_IMPLEMENTATION}}
}
{% endif %}

{% if NCP_IMPLEMENTATION!="<none>" and NCP_IMPLEMENTATION!="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__                  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__                  deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{NCP_IMPLEMENTATION}}
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S
) {
  {% if SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
  {{SOURCE_TERM_IMPLEMENTATION}}
  {% else %}
  std::fill_n(S,{{NUMBER_OF_UNKNOWNS}},0.0);
  {% endif %}
}
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION!="<user-defined>" and POINT_SOURCE_IMPLEMENTATION!="<none>" %}
#error Point sources not yet implemented
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::pointSource(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S
) {
  {% if POINT_SOURCE_IMPLEMENTATION!="<empty>" %}
  {{POINT_SOURCE_IMPLEMENTATION}}
  {% else %}
  std::fill_n(S,{{NUMBER_OF_UNKNOWNS}},0.0);
  {% endif %}
}
{% endif %}

{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  Offloadable
) {
  {{EIGENVALUES_IMPLEMENTATION}};
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif
{% endif %}

{% if FLUX_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__                        F, // F[{{NUMBER_OF_UNKNOWNS}}]
  Offloadable
) {
  {% if FLUX_IMPLEMENTATION=="<none>" %}
  abort();
  {% else %}
  {{FLUX_IMPLEMENTATION}}
  {% endif %}
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif
{% endif %}

{% if NCP_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
      const double * __restrict__                  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__                  deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__                        BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
  Offloadable
) {
  {% if NCP_IMPLEMENTATION=="<none>" %}
  abort();
  {% else %}
  {{NCP_IMPLEMENTATION}}
  {% endif %}
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      double                                       t,
      double                                       dt,
      double * __restrict__ S,
      Offloadable
) {
  {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %}
  abort();
  {% else %}
  {{SOURCE_TERM_IMPLEMENTATION}}
  {% endif %}
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::pointSource(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double                                       dt,
      double * __restrict__ S,
      Offloadable
) {
  assertionMsg( false, "point sources not implemented yet" );
  {% if POINT_SOURCE_IMPLEMENTATION=="<none>" %}
  abort();
  {% else %}
  {{POINT_SOURCE_IMPLEMENTATION}}
  {% endif %}
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif
{% endif %}
""",
        undefined=jinja2.DebugUndefined,
    )

    d = {}
    d["FLUX_IMPLEMENTATION"] = flux_implementation
    d["NCP_IMPLEMENTATION"] = ncp_implementation
    d["EIGENVALUES_IMPLEMENTATION"] = eigenvalues_implementation
    d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
    d["POINT_SOURCE_IMPLEMENTATION"] = point_source_implementation
    d["STATELESS_PDE_TERMS"] = pde_terms_without_state
    return Template.render(**d)


def create_solver_declarations(
    flux_implementation,
    ncp_implementation,
    eigenvalues_implementation,
    source_term_implementation,
    point_source_implementation,
    pde_terms_without_state,
):
    Template = jinja2.Template(
        """
  public:
    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    virtual void sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      double                                       t,
      double                                       dt,
      double * __restrict__ S
    ) override;
    {% endif %}

    {% if POINT_SOURCE_IMPLEMENTATION=="<user-defined>" %}
    #error point sources not defined yet
    virtual void pointSource(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double                                       dt,
      double * __restrict__ S
    ) override;
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
    /**
     * Determine max eigenvalue over Jacobian in a given point with solution values
     * (states) Q. All parameters are in.
     *
     * @return Max eigenvalue. Result has to be positive, so we are actually speaking
     *   about the maximum absolute eigenvalue.
     */
    virtual double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal
    ) override;
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
    virtual void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}

    {% if NCP_IMPLEMENTATION=="<user-defined>" %}
    virtual void nonconservativeProduct(
      const double * __restrict__                  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__                  deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}

    {% if STATELESS_PDE_TERMS  and SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    /**
     * To obtain the best performance, I recommend to man inline command to
     * this signature and to copy the implementation into the header. So it would
     * read
     *
     * static inline void sourceTerm( ... ) {
     *  code here
     * }
     *
     * The GPU offloading requires static functions. As we cannot overload the
     * original (virtual) function with a static alternative, we do the
     * TBB trick and overload by adding an additional enum. It has no semantics
     * but helps the compiler to distinguish the different function variants.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
     static void sourceTerm(
       const double * __restrict__ Q,
       const tarch::la::Vector<Dimensions,double>&  volumeCentre,
       const tarch::la::Vector<Dimensions,double>&  volumeH,
       double                                       t,
       double                                       dt,
       double * __restrict__ S,
       Offloadable
     );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if STATELESS_PDE_TERMS  and POINT_SOURCE_IMPLEMENTATION=="<user-defined>" %}
    /**
     * To obtain the best performance, I recommend to man inline command to
     * this signature and to copy the implementation into the header. So it would
     * read
     *
     * static inline void pointSource( ... ) {
     *  code here
     * }
     *
     * The GPU offloading requires static functions. As we cannot overload the
     * original (virtual) function with a static alternative, we do the
     * TBB trick and overload by adding an additional enum. It has no semantics
     * but helps the compiler to distinguish the different function variants.
     */
     #error point sources not implemented yet
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
     static void pointSource(
       const double * __restrict__ Q,
       const tarch::la::Vector<Dimensions,double>&  volumeCentre,
       const tarch::la::Vector<Dimensions,double>&  volumeH,
       double                                       t,
       double                                       dt,
       double * __restrict__ S,
       Offloadable
     );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * To obtain the best performance, I recommend to man inline command to 
     * this signature and to copy the implementation into the header. So it would
     * read 
     *
     * static inline double maxEigenvalue( ... ) {
     *  code here
     * }
     *
     * The GPU offloading requires static functions. As we cannot overload the
     * original (virtual) function with a static alternative, we do the
     * TBB trick and overload by adding an additional enum. It has no semantics
     * but helps the compiler to distinguish the different function variants.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if STATELESS_PDE_TERMS  and FLUX_IMPLEMENTATION=="<user-defined>" %}
    /**
     * To obtain the best performance, I recommend to man inline command to
     * this signature and to copy the implementation into the header. So it would
     * read
     *
     * static inline void flux( ... ) {
     *  code here
     * }
     *
     * The GPU offloading requires static functions. As we cannot overload the
     * original (virtual) function with a static alternative, we do the
     * TBB trick and overload by adding an additional enum. It has no semantics
     * but helps the compiler to distinguish the different function variants.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if STATELESS_PDE_TERMS  and NCP_IMPLEMENTATION=="<user-defined>" %}
    /**
     * To obtain the best performance, I recommend to man inline command to 
     * this signature and to copy the implementation into the header. So it would
     * read
     *
     * static inline void nonconservativeProduct( ... ) {
     *  code here
     * }
     *
     * The GPU offloading requires static functions. As we cannot overload the
     * original (virtual) function with a static alternative, we do the
     * TBB trick and overload by adding an additional enum. It has no semantics
     * but helps the compiler to distinguish the different function variants.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static void nonconservativeProduct(
      const double * __restrict__                  Q,       // Q[5+0],
      const double * __restrict__                  deltaQ,    // deltaQ[5+0]
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        BTimesDeltaQ,     // BTimesDeltaQ[5]
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}
""",
        undefined=jinja2.DebugUndefined,
    )
    d = {}
    d["FLUX_IMPLEMENTATION"] = flux_implementation
    d["NCP_IMPLEMENTATION"] = ncp_implementation
    d["EIGENVALUES_IMPLEMENTATION"] = eigenvalues_implementation
    d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
    d["POINT_SOURCE_IMPLEMENTATION"] = point_source_implementation
    d["STATELESS_PDE_TERMS"] = pde_terms_without_state
    return Template.render(**d)


def create_solver_definitions(
    flux_implementation,
    ncp_implementation,
    eigenvalues_implementation,
    source_term_implementation,
    point_source_implementation,
    pde_terms_without_state,
):
    Template = jinja2.Template(
        """
{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", x, t, dt, normal );
  // @todo implement
  logTraceOut( "maxEigenvalue(...)" );
}
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, t, dt, normal );
  // @todo implement
  logTraceOut( "flux(...)" );
}
{% endif %}

{% if NCP_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__                  Q,    // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__                  deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__                        BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
)  {
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, t, dt, normal );
  // @todo implement
  logTraceOut( "nonconservativeProduct(...)" );
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__                  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceInWith3Arguments( "sourceTerm(...)", volumeCentre, t, dt );

  // @todo implement and ensure that all entries of S are properly set
  for (int i=0; i<NumberOfUnknowns; i++) {
    S[i] = 0.0;
  }

  logTraceOut( "sourceTerm(...)" );
}
{% endif %}

{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  Offloadable
)  {
  // @todo implement
}
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F,
  Offloadable
)  {
  // @todo implement
}
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
{% endif %}

{% if NCP_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__                  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__                  deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__                        BTimesDeltaQ,
  Offloadable
)  {
  // @todo implement
}
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  double                                       t,
  double                                       dt,
  double * __restrict__ S,
  Offloadable
) {
  // @todo implement
}
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
{% endif %}

{% if POINT_SOURCE_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
#error  point sources not implemented yet
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  double                                       t,
  double                                       dt,
  double * __restrict__ S,
  Offloadable
) {
  // @todo implement
}
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
{% endif %}
""",
        undefined=jinja2.DebugUndefined,
    )
    d = {}
    d["FLUX_IMPLEMENTATION"] = flux_implementation
    d["NCP_IMPLEMENTATION"] = ncp_implementation
    d["EIGENVALUES_IMPLEMENTATION"] = eigenvalues_implementation
    d["SOURCE_TERM_IMPLEMENTATION"] = source_term_implementation
    d["POINT_SOURCE_IMPLEMENTATION"] = point_source_implementation
    d["STATELESS_PDE_TERMS"] = pde_terms_without_state
    return Template.render(**d)


def get_face_merge_implementation(patch_overlap):
    assert False, "I don't think this one is used anymore. @todo Remove"

    d = {
        "OVERLAP": patch_overlap.dim[0]
        / 2,  # assumed to always be two, but present here anyway
        "DOFS_PER_AXIS": patch_overlap.dim[1],
        "UNKNOWNS": patch_overlap.no_of_unknowns,
    }
    template = """
  
  #if Dimensions==2
  constexpr int nodesPerFace = {DOFS_PER_AXIS};
  #else
  constexpr int nodesPerFace = {DOFS_PER_AXIS}*{DOFS_PER_AXIS};
  #endif
  
  constexpr int strideQ = {UNKNOWNS};
    
  const int faceDimension = marker.getSelectedFaceNumber()%Dimensions;
  //if the outer normal is positive, the normal points to the right so the face is on the right
  const int faceLR = (marker.outerNormal()[faceDimension]>0.0 ? 1 : 0);

  for(int i=0; i<nodesPerFace; i++){{
    for(int j=0; j<strideQ ; j++){{
      value[(2*i+faceLR)*strideQ+j] = neighbour.value[(2*i+faceLR)*strideQ+j];
    }}
  }}

"""

    return template.format(**d)


def create_start_time_step_implementation_for_fixed_time_stepping(
    normalised_time_step_size,
):
    """
    We roll over all reduced data after the last time step, and we
    plot status info in the first step.
    """
    return (
        """
  if (
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxCellH>0.0
    and
    isFirstGridSweepOfTimeStep()
  ) {
    logInfo( "step()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "step()", "t       = " << _minTimeStampThisTimeStep );
    logInfo( "step()", "dt      = " << getTimeStepSize() );
    logInfo( "step()", "h_{min} = " << _minCellHThisTimeStep );
    logInfo( "step()", "h_{max} = " << _maxCellHThisTimeStep );
  }
  _timeStepSize  = """
        + str(normalised_time_step_size)
        + """;
"""
    )


def create_volumetric_solver_call(polynomial_basis, solver_variant):
    """
    A set of default volumetric kernel calls. You might want to use different
    solver calls in your code depending on the context.

    ## Difference to Finite Volume (FV) implementation

    In the FV code base, I need the implementation routines as argument: As we work
    with FV, the generic FV class does not know which implementations are around. If
    you work with a domain-specific Rusanov solver, e.g., then there is no such
    thing as an ncp.

    For the DG schemes, things are different: Every DG solver in ExaHyPE 2 in principle
    supports the ncp. So the base Python class already instantiates the corresponding
    dictionary entries, and we can use them straightaway.

    ## Arguments

    solver_variant: SolverVariant
      Picks different implementation variants
    """
    if (
        isinstance(polynomial_basis, GaussLegendreBasis)
        and solver_variant == SolverVariant.WithVirtualFunctions
    ):
        return """cellIntegral_patchwise_in_situ_GaussLegendre_functors(
      cellData,
      {{ORDER}},                                                   
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        F
      )->void {
        {% if FLUX_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,dt,normal,F);
        {% endif %}
      }, //flux
      [&](
          const double * __restrict__                  Q,
          const double * __restrict__                  deltaQ,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__                        F
        ) -> void {
          {% if NCP_IMPLEMENTATION!="<none>" %}
          repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q, deltaQ, faceCentre, t, dt, normal, F);
          {% endif %}
        }, //ncp
      [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        double * __restrict__                        S
      ) -> void {
        {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.sourceTerm(Q,x,t,dt,S);
        {% endif %}
      }, //source
      [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  cellCentre,
        const tarch::la::Vector<Dimensions,double>&  cellH,
        double                                       t,
        double                                       dt
      ) -> std::vector<::exahype2::dg::PointSource> {
        {% if POINT_SOURCES_IMPLEMENTATION!="<none>" %}
        return repositories::{{SOLVER_INSTANCE}}.pointSources(Q,cellCentre,cellH,t,dt);
        {% else %}
        return std::vector<::exahype2::dg::PointSource>();
        {% endif %}
      }, //source
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
      repositories::{{SOLVER_INSTANCE}}.MassMatrixDiagonal1d,
      repositories::{{SOLVER_INSTANCE}}.StiffnessOperator1d,
      repositories::{{SOLVER_INSTANCE}}.DerivativeOperator1d,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }}, //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }}, //useNCP
      {{ "true" if SOURCE_TERM_IMPLEMENTATION!="<none>" else "false" }}, //useSource
      {{ "true" if POINT_SOURCES_IMPLEMENTATION!="<none>" else "false" }} //usePointSource
    );
"""
    elif (
        isinstance(polynomial_basis, GaussLegendreBasis)
        and solver_variant == SolverVariant.Stateless
    ):
        return """cellIntegral_patchwise_in_situ_GaussLegendre<{{SOLVER_NAME}},{{ORDER}},{{NUMBER_OF_UNKNOWNS}},{{NUMBER_OF_AUXILIARY_VARIABLES}}>(
      cellData,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }},          //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }},           //useNCP
      {{ "true" if SOURCE_TERM_IMPLEMENTATION!="<none>" else "false" }},   //useSource
      {{ "true" if POINT_SOURCES_IMPLEMENTATION!="<none>" else "false" }}  //usePointSource
    );
"""
    elif (
        isinstance(polynomial_basis, GaussLegendreBasis)
        and solver_variant == SolverVariant.Accelerator
    ):
        return """cellIntegral_patchwise_in_situ_GaussLegendre<{{SOLVER_NAME}},{{ORDER}},{{NUMBER_OF_UNKNOWNS}},{{NUMBER_OF_AUXILIARY_VARIABLES}}>(
      targetDevice,
      cellData,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }},          //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }},           //useNCP
      {{ "true" if SOURCE_TERM_IMPLEMENTATION!="<none>" else "false" }},   //useSource
      {{ "true" if POINT_SOURCES_IMPLEMENTATION!="<none>" else "false" }}  //usePointSource
    );
"""
    else:
        return "#solver variant not implemented yet"


def create_add_solver_contributions_call(polynomial_basis):
    if isinstance(polynomial_basis, GaussLegendreBasis):
        return """integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
    #if Dimensions==2
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(0).value, //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(2).value, //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(1).value, //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(3).value, //up
    #else
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(0).value, //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(3).value, //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(1).value, //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(4).value, //up
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(2).value, //front
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(5).value, //back
    #endif
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.h(),
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft1d,
      repositories::{{SOLVER_INSTANCE}}.MassMatrixDiagonal1d,
      dQdt
    );
"""
    else:
        return "#error Not supported yet"


def create_multiply_with_inverted_mass_matrix_call(polynomial_basis):
    if isinstance(polynomial_basis, GaussLegendreBasis):
        return """multiplyWithInvertedMassMatrix_GaussLegendre(
      cellData,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.MassMatrixDiagonal1d
    );
"""
    else:
        return "#error Not supported yet"
