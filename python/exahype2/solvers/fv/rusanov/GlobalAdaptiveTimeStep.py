# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .SingleSweep import SingleSweep

from .PDETerms import PDETerms

import jinja2


def create_source_term_kernel_for_Rusanov(source_term_implementation, use_gpu):
  Template = jinja2.Template( """
  {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
  repositories::{{SOLVER_INSTANCE}}.sourceTerm(
    Q,
    x, dx, t, dt,
    S
  );
  {% else %}
  std::fill_n(S,{{NUMBER_OF_UNKNOWNS}},0.0);
  {% endif %}
""", undefined=jinja2.DebugUndefined)
  d= {}
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "USE_GPU"]                             = use_gpu
  return Template.render(**d)
  
    

def create_compute_Riemann_kernel_for_Rusanov_fixed_timestepping(flux_implementation, ncp_implementation, eigenvalues_implementation, use_gpu):
  Template = jinja2.Template( """
        ::exahype2::fv::splitRusanov1d(
          [] (
            const double * __restrict__                  Q,
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double                                       F[]
          ) -> void {
            {% if FLUX_IMPLEMENTATION=="<none>" %}
            std::fill_n(F,{{NUMBER_OF_UNKNOWNS}},0.0);
            {% else %}
            repositories::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, normal, F );
            {% endif %}
          },
          [] (
            const double * __restrict__                  Q,
            const double * __restrict__                  deltaQ,
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double                                       BgradQ[]
          ) -> void {
            {% if NCP_IMPLEMENTATION!="<none>" %}
            repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, deltaQ, faceCentre, volumeH, t, normal, BgradQ );
            {% else %}
            std::fill_n(BgradQ,{{NUMBER_OF_UNKNOWNS}},0.0);
            {% endif %}
          },
          [] (
            const double * __restrict__                  Q,
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal
          ) -> double {
            return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
          },
          QL, QR, x, dx, t, dt, normal,
          {{NUMBER_OF_UNKNOWNS}},
          {{NUMBER_OF_AUXILIARY_VARIABLES}},
          FL,FR,
          {% if FLUX_IMPLEMENTATION=="<none>" %}
          true,
          {% else %}
          false,
          {% endif %}
          {% if NCP_IMPLEMENTATION=="<none>" %}
          true
          {% else %}
          false
          {% endif %}
        );
""", undefined=jinja2.DebugUndefined)
  
  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "USE_GPU"]                             = use_gpu
  return Template.render(**d)


def create_abstract_solver_declarations(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, use_gpu):
  Template = jinja2.Template( """
  public:
    {% if EIGENVALUES_IMPLEMENTATION!="<none>" and EIGENVALUES_IMPLEMENTATION!="<user-defined>" and USE_GPU %}
    {% endif %}
    {% if EIGENVALUES_IMPLEMENTATION!="<none>" %}
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
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal
    ) {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};
    {% endif %}


    {% if FLUX_IMPLEMENTATION!="<none>" and FLUX_IMPLEMENTATION!="<user-defined>" and USE_GPU %}
    {% endif %}
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if FLUX_IMPLEMENTATION=="<user-defined>" %}=0{% else %} final {% endif %};
    {% endif %}
     
     
    {% if NCP_IMPLEMENTATION!="<none>" and NCP_IMPLEMENTATION!="<user-defined>" and USE_GPU %}
    {% endif %}
    {% if NCP_IMPLEMENTATION!="<none>" %}
    virtual void nonconservativeProduct(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__ BgradQ // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if NCP_IMPLEMENTATION=="<user-defined>" %}=0{% endif %};
    {% endif %}
    

    {% if SOURCE_TERM_IMPLEMENTATION!="<none>" and SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and USE_GPU %}
    {% endif %}
    {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    virtual void sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      double * __restrict__ S
    ) {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};
    {% endif %}

   
    {% if USE_GPU  and SOURCE_TERM_IMPLEMENTATION!="<user-defined>" %}
    // The GPU offloading requires static functions, we do the
    // TBB trick of overloading static functions with an enum
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
   
    {% if USE_GPU  and NCP_IMPLEMENTATION!="<user-defined>" %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static void nonconservativeProduct(
      const double * __restrict__                  Q,         // Q[5+0],
      const double * __restrict__                  deltaQ,    // [5+0]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
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
  d[ "USE_GPU"]                             = use_gpu
  return Template.render(**d)


def create_abstract_solver_definitions(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, use_gpu):
  Template = jinja2.Template( """
{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  {{EIGENVALUES_IMPLEMENTATION}}
}
{% endif %}


{% if FLUX_IMPLEMENTATION!="<none>" %}
{% if FLUX_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::flux(
 const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
 const tarch::la::Vector<Dimensions,double>&  faceCentre,
 const tarch::la::Vector<Dimensions,double>&  volumeH,
 double                                       t,
 int                                          normal,
 double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{FLUX_IMPLEMENTATION}}
}
{% endif %}
{% endif %}


{% if NCP_IMPLEMENTATION!="<none>" %}
{% if NCP_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{NCP_IMPLEMENTATION}}
}
{% endif %}
{% endif %}


{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
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

{% if USE_GPU %}

#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
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
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif

// Fusanov needs an ncp function defined and compiled, so this does
// nothing, just produces the right symbol
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__                  Q,         // Q[5+0],
  const double * __restrict__                  deltaQ,    // [5+0]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__                        BgradQ,     // BgradQ[5]
  Offloadable
) {

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
  d[ "USE_GPU"]                             = use_gpu  
  return Template.render(**d)


def create_solver_declarations(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, use_gpu):
  Template = jinja2.Template( """
  public:
    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    void sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
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
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal
    ) override;
    {% endif %}


    {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
    virtual void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}
     
     
    {% if NCP_IMPLEMENTATION=="<user-defined>" %}
    virtual void nonconservativeProduct(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__ BgradQ // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}
    
   
    {% if USE_GPU  and SOURCE_TERM_IMPLEMENTATION!="<user-defined>" %}
    // The GPU offloading requires static functions, we do the
    // TBB trick of overloading static functions with an enum
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
   
    {% if USE_GPU  and NCP_IMPLEMENTATION!="<user-defined>" %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static void nonconservativeProduct(
      const double * __restrict__                  Q,         // Q[5+0],
      const double * __restrict__                  deltaQ,    // [5+0]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
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
  d[ "USE_GPU"]                             = use_gpu  
  return Template.render(**d)


def create_solver_definitions(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, use_gpu):
  Template = jinja2.Template( """

{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "maxEigenvalue(...)" );
}
{% endif %}


{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::flux(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "flux(...)" );
}
{% endif %}


{% if NCP_IMPLEMENTATION=="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
)  {
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "nonconservativeProduct(...)" );
}
{% endif %}


{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__                  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[{{NUMBER_OF_UNKNOWNS}}
) {
  logTraceInWith4Arguments( "sourceTerm(...)", volumeX, volumeH, t, dt );
  // @todo implement
  logTraceOut( "sourceTerm(...)" );
}
{% endif %}


{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__ S
) {
  logTraceInWith4Arguments( "sourceTerm(...)", volumeCentre, volumeH, t, dt );
  // @todo implement
  logTraceOut( "sourceTerm(...)" );
}
{% endif %}
""", undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "USE_GPU"]                             = use_gpu  
  return Template.render(**d)


class Rusanov_GlobalFixedTimeStep( SingleSweep ):
  def __init__(self, 
    name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size,
    flux=PDETerms.User_Defined_Implementation, 
    ncp=None, 
    eigenvalues=PDETerms.User_Defined_Implementation, 
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    plot_grid_properties=False
  ):
    super(Rusanov_GlobalFixedTimeStep,self).__init__(name, patch_size, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties) 
    
    self._time_step_size = time_step_size

    self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.None_Implementation
    self._source_term_implementation          = PDETerms.None_Implementation
    
    self._preprocess_reconstructed_patch      = """
  cellTimeStepSize = marker.h()(0) / repositories::{{SOLVER_INSTANCE}}.getMaxMeshSize() * """ + str(time_step_size) + """;
  cellTimeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
"""
    self._postprocess_updated_patch      = ""

    self.set_implementation(flux=flux, 
      ncp=ncp, 
      eigenvalues=eigenvalues, 
      boundary_conditions=boundary_conditions, 
      refinement_criterion=refinement_criterion, 
      initial_conditions=initial_conditions, 
      source_term=source_term )


  def set_implementation(self,
    flux=None,ncp=None,
    eigenvalues=None,
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    memory_location         = None,
    use_split_loop          = False
  ):
    """
      If you pass in User_Defined, then the generator will create C++ stubs
      that you have to befill manually. If you pass in None_Implementation, it
      will create nop, i.e. no implementation or defaults. Any other string
      is copied 1:1 into the implementation. If you pass in None, then the
      set value so far won't be overwritten.

      Please note that not all options are supported by all solvers. You
      cannot set ncp and fluxes for the ClawPack Riemann solvers, e.g.

      This routine should be the very last invoked by the constructor.
    """
    if flux                 is not None:  self._flux_implementation                       = flux
    if ncp                  is not None:  self._ncp_implementation                        = ncp
    if eigenvalues          is not None:  self._eigenvalues_implementation                = eigenvalues
    if source_term          is not None:  self._source_term_implementation                = source_term

    self._source_term_call    = create_source_term_kernel_for_Rusanov(self._source_term_implementation, False)
    self._Riemann_solver_call = create_compute_Riemann_kernel_for_Rusanov_fixed_timestepping(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, False)

    self._abstract_solver_user_declarations = create_abstract_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    self._abstract_solver_user_definitions  = create_abstract_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    self._solver_user_declarations          = create_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    self._solver_user_definitions           = create_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
      
    SingleSweep.set_implementation(self, boundary_conditions, refinement_criterion, initial_conditions, memory_location, use_split_loop)




class Rusanov_GlobalAdaptiveTimeStep( SingleSweep ):
  def __init__(self, 
    name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_relaxation,
    flux=PDETerms.User_Defined_Implementation, 
    ncp=None, 
    eigenvalues=PDETerms.User_Defined_Implementation, 
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    plot_grid_properties=False
  ):
    super(Rusanov_GlobalAdaptiveTimeStep,self).__init__(name, patch_size, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties) 
    
    self._time_step_relaxation = time_step_relaxation

    self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.None_Implementation
    self._source_term_implementation          = PDETerms.None_Implementation
    
    self._preprocess_reconstructed_patch      = """
  cellTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize();
  cellTimeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
"""
    self._postprocess_updated_patch      = """
    const double maxEigenvalue = ::exahype2::fv::maxEigenvalue_AoS(
      [] (
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) -> double {
        return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
      },
      marker.x(),
      marker.h(),
      cellTimeStamp,
      cellTimeStepSize,
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      targetPatch
    );
    
    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue( maxEigenvalue );  
"""

    self.set_implementation(flux=flux, 
      ncp=ncp, 
      eigenvalues=eigenvalues, 
      boundary_conditions=boundary_conditions, 
      refinement_criterion=refinement_criterion, 
      initial_conditions=initial_conditions, 
      source_term=source_term )


  def set_implementation(self,
    flux=None,ncp=None,
    eigenvalues=None,
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    memory_location         = None,
    use_split_loop          = False
  ):
    """
      If you pass in User_Defined, then the generator will create C++ stubs
      that you have to befill manually. If you pass in None_Implementation, it
      will create nop, i.e. no implementation or defaults. Any other string
      is copied 1:1 into the implementation. If you pass in None, then the
      set value so far won't be overwritten.

      Please note that not all options are supported by all solvers. You
      cannot set ncp and fluxes for the ClawPack Riemann solvers, e.g.

      This routine should be the very last invoked by the constructor.
    """
    if flux                 is not None:  self._flux_implementation                       = flux
    if ncp                  is not None:  self._ncp_implementation                        = ncp
    if eigenvalues          is not None:  self._eigenvalues_implementation                = eigenvalues
    if source_term          is not None:  self._source_term_implementation                = source_term

    self._source_term_call    = create_source_term_kernel_for_Rusanov(self._source_term_implementation, False)
    self._Riemann_solver_call = create_compute_Riemann_kernel_for_Rusanov_fixed_timestepping(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, False)

    self._abstract_solver_user_declarations = create_abstract_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    self._abstract_solver_user_definitions  = create_abstract_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    self._solver_user_declarations          = create_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    self._solver_user_definitions           = create_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)

    self._constructor_implementation = "_admissibleTimeStepSize = 0.0;"
    
    self._abstract_solver_user_declarations += """
private:
  double _maxEigenvalue;
  double _admissibleTimeStepSize;
public:
  void setMaxEigenvalue( double eigenvalue );  
  double getAdmissibleTimeStepSize() const;  
    """  

    self._abstract_solver_user_definitions += """
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
    
    self._start_time_step_implementation          = """
  _maxEigenvalue = 0.0;
"""

    self._finish_time_step_implementation          = """
  #ifdef Parallel
  double newMaxEigenvalue = _maxEigenvalue;
  tarch::mpi::Rank::getInstance().allReduce(
        &_maxEigenvalue,
        &newMaxEigenvalue,
        1,
        MPI_DOUBLE,
        MPI_MAX, 
        [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
        );
  #endif

  _admissibleTimeStepSize = """ + str(self._time_step_relaxation) + """ * _minH / _maxEigenvalue / 3.0;
  if ( std::isnan(_admissibleTimeStepSize) or std::isinf(_admissibleTimeStepSize) ) {
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSize>0", "invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSize) );
  }
  if (tarch::la::smallerEquals(_admissibleTimeStepSize,0.0,1e-10) ) {
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSize>0", "degenerated time step size of " + std::to_string(_admissibleTimeStepSize) );
  }
  
  _maxTimeStepSize  = _admissibleTimeStepSize; // for plotting reasons
  _minTimeStepSize  = std::min( _minTimeStepSize, _admissibleTimeStepSize );
"""
    
    SingleSweep.set_implementation(self, boundary_conditions, refinement_criterion, initial_conditions, memory_location, use_split_loop)

