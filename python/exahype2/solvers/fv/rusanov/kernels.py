# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.fv.PDETerms  import PDETerms

import jinja2


def create_preprocess_reconstructed_patch_throughout_sweep_kernel_for_fixed_time_stepping( time_step_size ):
  return """
  cellTimeStepSize = marker.h()(0) / repositories::{{SOLVER_INSTANCE}}.MaxAdmissibleVolumeH * """ + str(time_step_size) + """;
  cellTimeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
"""


def create_preprocess_reconstructed_patch_throughout_sweep_kernel_for_adaptive_time_stepping():
  return """
  cellTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize();
  cellTimeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
"""


def create_postprocess_updated_patch_for_adaptive_time_stepping():
  return """
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


def create_constructor_implementation_for_adaptive_time_stepping():
  return "_admissibleTimeStepSize = 0.0;"
  

def create_abstract_solver_user_declarations_for_adaptive_time_stepping():
  return """
private:
  double _maxEigenvalue;
  double _admissibleTimeStepSize;
public:
  void setMaxEigenvalue( double eigenvalue );  
  double getAdmissibleTimeStepSize() const;  
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


def create_start_time_step_implementation_for_adaptive_time_stepping():
  return """
  _maxEigenvalue = 0.0;
"""

    
def create_finish_time_step_implementation_for_adaptive_time_stepping(time_step_relaxation):
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

  if ( tarch::la::smaller(_maxEigenvalue, 0.0 ) ) {
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_maxEigenvalue>=0", "invalid max eigenvalue: " + std::to_string(_maxEigenvalue) );
    _admissibleTimeStepSize = _admissibleTimeStepSize;
  }
  else if ( tarch::la::equals(_maxEigenvalue, 0.0 ) ) {
    logWarning( "finishTimeStep(...)", "maximum eigenvalue approaches 0.0. For nonlinear PDEs, this often means the PDE becomes stationary. It could also be a bug however" ); 
    _admissibleTimeStepSize = 0.0;
    _minTimeStamp           = std::numeric_limits<double>::max();
    _maxTimeStamp           = std::numeric_limits<double>::max();
  }
  else {
    _admissibleTimeStepSize = """ + str(time_step_relaxation) + """ * getMinVolumeSize() / _maxEigenvalue / 3.0;
    if ( std::isnan(_admissibleTimeStepSize) or std::isinf(_admissibleTimeStepSize) ) {
      ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSize>0", "invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSize) );
    }
    if (tarch::la::smallerEquals(_admissibleTimeStepSize,0.0,1e-10) ) {
      logWarning( "finishTimeStep(...)", "degenerated time step size of " << std::to_string(_admissibleTimeStepSize) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug" );
    }
  }
   
  _maxTimeStepSize  = _admissibleTimeStepSize; // for plotting reasons
  _minTimeStepSize  = std::min( _minTimeStepSize, _admissibleTimeStepSize );
"""


def create_source_term_kernel_for_Rusanov(source_term_implementation):
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
  return Template.render(**d)
  
    
def create_fused_compute_Riemann_kernel_for_Rusanov(flux_implementation, ncp_implementation, source_term_implementation):
  Template = jinja2.Template( """
#if Dimensions==2
::exahype2::fv::Fusanov_2D
#elif Dimensions==3
::exahype2::fv::Fusanov_3D
#endif
  <{{NUMBER_OF_VOLUMES_PER_AXIS}},{{NUMBER_OF_UNKNOWNS}},{{NUMBER_OF_AUXILIARY_VARIABLES}},{{SOLVER_NAME}},{{SKIP_FLUX}},{{SKIP_NCP}},{{SKIP_SOURCE_TERM}}>
""", undefined=jinja2.DebugUndefined)
  
  d= {}
  if flux_implementation==PDETerms.None_Implementation or flux_implementation==PDETerms.Empty_Implementation:
    d[ "SKIP_FLUX"] = "true"
  else:
    d[ "SKIP_FLUX"] = "false"
  if ncp_implementation==PDETerms.None_Implementation or ncp_implementation==PDETerms.Empty_Implementation:
    d[ "SKIP_NCP"]  = "true"
  else:
    d[ "SKIP_NCP"] = "false"
  if source_term_implementation==PDETerms.None_Implementation or source_term_implementation==PDETerms.Empty_Implementation:
    d[ "SKIP_SOURCE_TERM"]  = "true"
  else:
    d[ "SKIP_SOURCE_TERM"] = "false"
  return Template.render(**d)
    
    
def create_compute_Riemann_kernel_for_Rusanov(flux_implementation, ncp_implementation):
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
  return Template.render(**d)


def create_abstract_solver_declarations(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, use_gpu):
  Template = jinja2.Template( """
  public:
    {% if EIGENVALUES_IMPLEMENTATION=="<none>" %}
    #error eigenvalue implementation cannot be none
    {% endif %}
    
    {% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and USE_GPU %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
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
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal
    ) {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};


    {% if FLUX_IMPLEMENTATION!="<user-defined>" and USE_GPU %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
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
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if FLUX_IMPLEMENTATION=="<user-defined>" %}=0{% else %} final {% endif %};
    {% endif %}


    {% if NCP_IMPLEMENTATION!="<user-defined>" and USE_GPU %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static void nonconservativeProduct(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
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
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__ BgradQ // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if NCP_IMPLEMENTATION=="<user-defined>" %}=0{% endif %};
    {% endif %}
    

    {% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and USE_GPU %}
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
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
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
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
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
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
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
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
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



{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and USE_GPU %}
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
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


{% if FLUX_IMPLEMENTATION!="<user-defined>" and USE_GPU %}
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__                        F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
) {
  {% if FLUX_IMPLEMENTATION=="<none>" %}
  exit(-1);
  {% else %}
  {{FLUX_IMPLEMENTATION}}
  {% endif %}
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif
{% endif %}


{% if NCP_IMPLEMENTATION!="<user-defined>" and USE_GPU %}
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__                        BgradQ, // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
) {
  {% if NCP_IMPLEMENTATION=="<none>" %}
  exit(-1);
  {% else %}
  {{NCP_IMPLEMENTATION}}
  {% endif %}
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif
{% endif %}


{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and USE_GPU %}
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      double * __restrict__ S,
      Offloadable
) {
  {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %}
  exit(-1);
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
    
   
    {% if USE_GPU  and SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
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


    {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and USE_GPU %}
    /**
     * Determine max eigenvalue over Jacobian in a given point with solution values
     * (states) Q. All parameters are in.
     *
     * @return Max eigenvalue. Result has to be positive, so we are actually speaking
     *   about the maximum absolute eigenvalue.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}


    {% if USE_GPU  and FLUX_IMPLEMENTATION=="<user-defined>" %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__                        F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}
     
   
    {% if USE_GPU  and NCP_IMPLEMENTATION=="<user-defined>" %}
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
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
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
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
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
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
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
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
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





{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and USE_GPU %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
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


{% if FLUX_IMPLEMENTATION=="<user-defined>" and USE_GPU %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
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


{% if NCP_IMPLEMENTATION=="<user-defined>" and USE_GPU %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
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



{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and USE_GPU %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
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
  d[ "USE_GPU"]                             = use_gpu  
  return Template.render(**d)


