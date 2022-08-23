# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.new_ader.PDETerms  import PDETerms

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
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
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
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
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
      const tarch::la::Vector<Dimensions,double>&  x,
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


def create_abstract_solver_user_declarations_for_fixed_time_stepping():
  return """
private:
  double _timeStepSize;
public:
  double getTimeStepSize() const;  
    """  


def create_abstract_solver_user_definitions_for_fixed_time_stepping():
  return """
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getTimeStepSize() const {
  return _timeStepSize;
}
    """  


def create_compute_time_step_size_for_fixed_time_stepping(_time_step_size):
  return """
  const double timeStepSize = """ +str(_time_step_size)

def create_start_time_step_implementation_for_fixed_time_stepping(use_enclave_tasking):
  """
  
  The outcome is used before we actually roll over the accumulation variables
  and other stuff.
  
  """
  predicate = """
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxCellH>0.0
  """
  
  if use_enclave_tasking:
    predicate += """and (_solverState == SolverState::Primary or _solverState == SolverState::PrimaryAfterGridInitialisation) """
      
  return """
  if (""" + predicate + """) {
    logInfo( "step()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "step()", "t       = " << _minTimeStampThisTimeStep );
    logInfo( "step()", "dt      = " << getTimeStepSize() );
    logInfo( "step()", "h_{min} = " << _minCellHThisTimeStep << " (volume size)");
    logInfo( "step()", "h_{max} = " << _maxCellHThisTimeStep<< " (volume size)" );
  }
"""

def create_finish_time_step_implementation_for_fixed_time_stepping(normalised_time_step_size):
  return """
  assertion( _minCellH >= 0.0 );
  assertion( MaxAdmissibleCellH > 0.0 );
  if (_minCellH <= MaxAdmissibleCellH) {
    _timeStepSize  = """ + str(normalised_time_step_size) + """ * _minCellH / MaxAdmissibleCellH;
  }
  else {
    _timeStepSize = 0.0;
  }
"""


def create_abstract_solver_user_declarations_for_adaptive_time_stepping():
  return """
private:
  double _maxEigenvalue;
  double _admissibleTimeStepSize;
public:
  void setMaxEigenvalue( double eigenvalue );  
  /**
   * @return Admissible time step size for the current sweep, i.e. 
   *         return _admissibleTimeStepSize. This value always refers
   *         to the minimum mesh volume size. If you use subcycling,
   *         you have to scale it for cells that are not on the finest
   *         mesh resolution. 
   */
  virtual double getAdmissibleTimeStepSize() const;  
"""


def create_abstract_solver_user_definitions_for_adaptive_time_stepping():
  return """
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::setMaxEigenvalue( double eigenvalue ) {
  if ( tarch::la::greater( eigenvalue, 0.0 ) ) {
    tarch::multicore::Lock lock(_semaphore);
    _maxEigenvalue = std::max(_maxEigenvalue,eigenvalue);
  }
}    


double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getAdmissibleTimeStepSize() const {
  return _admissibleTimeStepSize;
}
"""

def create_compute_time_step_size_for_adaptive_time_stepping():
  return """
  double timeStepSize = repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize();
"""

def create_compute_new_time_step_size_for_adaptive_time_stepping():
  return """
  
    double maxEigenvalue = kernels::aderdg::generic::c::maxScaledEigenvalue<
      {{SOLVER_NAME}},
      false //useViscousFlux
      >(
        repositories::{{SOLVER_INSTANCE}},
        luh,
        marker.x()
      );

    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue( maxEigenvalue );  
    
    // This is a value set for stats reasons. We'll ignore it later as we 
    // ask for a new valid time step size from getAdmissibleTimeStepSize().
    //const double newTimeStepSize = 0.0;
    const double newTimeStepSize =  repositories::{{SOLVER_INSTANCE}}.getMinCellSize(false)>0.0 ?
      repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize() * marker.h()(0) / repositories::{{SOLVER_INSTANCE}}.getMinCellSize(false) :
      0.0;

"""

def create_start_time_step_implementation_for_adaptive_time_stepping():
  predicate = """
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxCellH>0.0
  """
        
  statistics = """
  if (""" + predicate + """) {
    logInfo( "startTimeStep()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "startTimeStep()", "t            = " << _minTimeStampThisTimeStep );
    logInfo( "startTimeStep()", "dt           = " << getAdmissibleTimeStepSize() );
    logInfo( "startTimeStep()", "h_{min}      = " << _minCellHThisTimeStep << " (volume size)");
    logInfo( "startTimeStep()", "h_{max}      = " << _maxCellHThisTimeStep << " (volume size)" );
    logInfo( "startTimeStep()", "lambda_{max} = " << _maxEigenvalue );
  }
"""
    
  clear_max_eigenvalue = """if (_solverState == SolverState::Correction) {
  _maxEigenvalue = 0.0;
}"""

  return statistics + clear_max_eigenvalue

def create_finish_time_step_implementation_for_adaptive_time_stepping():
  """
  
  This routine is inserted after we have reduced all global quantities. These
  are the quantities with the postfix ThisTimeStep.
  
  """ 
  return """
  #ifdef Parallel
  double newMaxEigenvalue = _maxEigenvalue;
  tarch::mpi::Rank::getInstance().allReduce(
        &newMaxEigenvalue,
        &_maxEigenvalue,
        1,
        MPI_DOUBLE,
        MPI_MAX, 
        [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
        );
  #endif

  if (_solverState == SolverState::Correction) {  
    if ( tarch::la::smaller(_maxEigenvalue, 0.0 ) ) {
      ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_maxEigenvalue>=0", "invalid max eigenvalue: " + std::to_string(_maxEigenvalue) );
      // keep time step size invariant
      // _admissibleTimeStepSize = _admissibleTimeStepSize;
    }
    else if ( tarch::la::equals(_maxEigenvalue, 0.0 ) ) {
      logWarning( "finishTimeStep(...)", "maximum eigenvalue approaches 0.0. For nonlinear PDEs, this often means the PDE becomes stationary. It could also be a bug however" ); 
      _admissibleTimeStepSize = 0.0;
    }
    else {
      const double minVolumeSize = _minCellHThisTimeStep;
      _admissibleTimeStepSize = CFL * PNPM / _maxEigenvalue; //the max eigenvalue here is already scaled with the volume size
      if ( std::isnan(_admissibleTimeStepSize) or std::isinf(_admissibleTimeStepSize) ) {
        ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSize>0", "invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSize) );
      }
      if (tarch::la::smallerEquals(_admissibleTimeStepSize,0.0,1e-10) ) {
        logWarning( "finishTimeStep(...)", "degenerated time step size of " << std::to_string(_admissibleTimeStepSize) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug (h_volume=" << minVolumeSize << ")" );
      }
    }
  }
"""

def create_constructor_implementation_for_adaptive_time_stepping():
  return "_admissibleTimeStepSize = 0.0;"
