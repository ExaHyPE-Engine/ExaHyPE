# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms  import PDETerms

import jinja2


def create_abstract_solver_declarations(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, point_source_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
  public:
    {% if EIGENVALUES_IMPLEMENTATION=="<none>" %}
    #error eigenvalue implementation cannot be none
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
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
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
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
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
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
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
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
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
      const double * __restrict__                  dQdx, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      int                                          normal,
      double * __restrict__                        BgradQ, // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}
    
    {% if NCP_IMPLEMENTATION!="<none>" %}
    virtual void nonconservativeProduct(
      const double * __restrict__                  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__                  dQdx, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      int                                          normal,
      double * __restrict__ BgradQ // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
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
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
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
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
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
    #error point source not implemented yet
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
    #error point source not implemented yet
    virtual void pointSource(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double                                       dt,
      double * __restrict__ S
    ) {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};
    {% endif %}
""", undefined=jinja2.DebugUndefined)

  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "POINT_SOURCE_IMPLEMENTATION"]         = point_source_implementation
  d[ "STATELESS_PDE_TERMS"]                 = pde_terms_without_state
  return Template.render(**d)


def create_abstract_solver_definitions(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, point_source_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
) {
  {{EIGENVALUES_IMPLEMENTATION}}
}
{% endif %}


{% if FLUX_IMPLEMENTATION!="<none>" and FLUX_IMPLEMENTATION!="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
 const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
 const tarch::la::Vector<Dimensions,double>&  x,
 double                                       t,
 int                                          normal,
 double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{FLUX_IMPLEMENTATION}}
}
{% endif %}


{% if NCP_IMPLEMENTATION!="<none>" and NCP_IMPLEMENTATION!="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{NCP_IMPLEMENTATION}}
}
{% endif %}


{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  x,
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
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  x,
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


{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
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
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
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
      const double * __restrict__                  deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      int                                          normal,
      double * __restrict__                        BgradQ, // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
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
      const tarch::la::Vector<Dimensions,double>&  x,
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
#error point sources not yet implemented
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  x,
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
""", undefined=jinja2.DebugUndefined)

  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "POINT_SOURCE_IMPLEMENTATION"]         = point_source_implementation
  d[ "STATELESS_PDE_TERMS"]                             = pde_terms_without_state  
  return Template.render(**d)


def create_solver_declarations(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, point_source_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
  public:
    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    void sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      double                                       dt,
      double * __restrict__ S
    ) override;
    {% endif %}

    {% if POINT_SOURCE_IMPLEMENTATION=="<user-defined>" %}
    #error point sources not defined yet
    void sourceTerm(
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
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      int                                          normal
    ) override;
    {% endif %}


    {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
    virtual void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      int                                          normal,
      double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}
     
     
    {% if NCP_IMPLEMENTATION=="<user-defined>" %}
    virtual void nonconservativeProduct(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      int                                          normal,
      double * __restrict__ BgradQ // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
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
     * static inline void sourceTerm( ... ) {
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
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
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
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
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
      const double * __restrict__                  Q,         // Q[5+0],
      const double * __restrict__                  deltaQ,    // [5+0]
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       t,
      int                                          normal,
      double * __restrict__                        BgradQ,     // BgradQ[5]
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}    
""", undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "POINT_SOURCE_IMPLEMENTATION"]         = point_source_implementation
  d[ "STATELESS_PDE_TERMS"]                 = pde_terms_without_state  
  return Template.render(**d)


def create_solver_definitions(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, point_source_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
)  {
  logTraceInWith3Arguments( "maxEigenvalue(...)", x, t, normal );
  // @todo implement
  logTraceOut( "maxEigenvalue(...)" );
}
{% endif %}


{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal,
  double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
)  {
  logTraceInWith3Arguments( "flux(...)", x, t, normal );
  // @todo implement
  logTraceOut( "flux(...)" );
}
{% endif %}


{% if NCP_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
)  {
  logTraceInWith3Arguments( "nonconservativeProduct(...)", x, t, normal );
  // @todo implement
  logTraceOut( "nonconservativeProduct(...)" );
}
{% endif %}


{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__                  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[{{NUMBER_OF_UNKNOWNS}}
) {
  logTraceInWith3Arguments( "sourceTerm(...)", x, t, dt );
  // @todo implement
  logTraceOut( "sourceTerm(...)" );
}
{% endif %}





{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
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
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
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
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ,
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
  const tarch::la::Vector<Dimensions,double>&  x,
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
  const tarch::la::Vector<Dimensions,double>&  x,
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


""", undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "POINT_SOURCE_IMPLEMENTATION"]         = point_source_implementation
  d[ "STATELESS_PDE_TERMS"]                 = pde_terms_without_state  
  return Template.render(**d)

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

def create_finish_time_step_implementation_for_fixed_time_stepping(normalised_time_step_size):
  return """
  assertion( _minCellH >= 0.0 );
  assertion( MaxAdmissibleCellH > 0.0 );
  if (_minCellH <= MaxAdmissibleCellH) {
    _timeStepSize  = """ + str(normalised_time_step_size) + """ * _minCellH;
  }
  else {
    _timeStepSize = 0.0;
  }
"""

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
    logInfo( "step()", "t       = " << _minTimeStampThisTimeStep );
    logInfo( "step()", "dt      = " << getTimeStepSize() );
    logInfo( "step()", "h_{min} = " << _minCellHThisTimeStep );
    logInfo( "step()", "h_{max} = " << _maxCellHThisTimeStep );
  }
"""

def create_abstract_solver_user_declarations_for_fixed_time_stepping(timeStep):
  return """
private:
  double _timeStepSize =""" + str(timeStep) + """;
public:
  double getTimeStepSize() const;  
    """  


def create_abstract_solver_user_definitions_for_fixed_time_stepping():
  return """
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getTimeStepSize() const {
  return _timeStepSize;
}
    """
