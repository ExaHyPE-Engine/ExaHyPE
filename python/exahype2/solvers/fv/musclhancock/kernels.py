# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms  import PDETerms

import jinja2
from enum import Enum


class SolverVariant(Enum):
  WithVirtualFunctions = 0
  Stateless            = 1
  Accelerator          = 2


class RiemannKernelVariant(Enum):
  PatchWiseAoSStack                = 0
  PatchWiseAoSoAStack              = 1
  PatchWiseSoAStack                = 2
  BatchedAoSStack                  = 10
  BatchedAoSoAStack                = 11
  BatchedSoAStack                  = 12
  PatchWiseSplitTermsAoSStack      = 20
  PatchWiseSplitTermsAoSoAStack    = 21
  PatchWiseAoSHeap                 = 30
  PatchWiseAoSoAHeap               = 31
  PatchWiseSoAHeap                 = 32
  BatchedAoSHeap                   = 40
  BatchedAoSoAHeap                 = 41
  BatchedSoAHeap                   = 42
  PatchWiseSplitTermsAoSHeap       = 50
  PatchWiseSplitTermsAoSoAHeap     = 51
  TaskGraphAoSHeap                 = 60
  TaskGraphAoSoAHeap               = 61
  TaskGraphSoAHeap                 = 62
  PatchWiseInSitu                  = 90
  BatchedInSitu                    = 91
    

def create_compute_Riemann_kernel_for_MusclHancock(flux_implementation, 
                                              ncp_implementation, 
                                              source_implementation, 
                                              compute_max_eigenvalue_of_next_time_step, 
                                              solver_variant,
                                              riemann_kernel_variant):
  """
  
  I return only the unqualified function call, i.e. without any namespaces.
  So by setting the right namespace as prefix, you can direct it to particular
  implementations.
  
  solver_variant: SolverVariant
  
  riemann_kernel_variant: RiemannKernelVariant
  
  """  
  RiemannKernelCalls = {
    RiemannKernelVariant.PatchWiseAoSStack:              "timeStepWithMusclHancock_patchwise_call_stack",
    RiemannKernelVariant.PatchWiseAoSoAStack:            "timeStepWithMusclHancock_patchwise_call_stack",
    RiemannKernelVariant.PatchWiseSoAStack:              "timeStepWithMusclHancock_patchwise_call_stack",
    RiemannKernelVariant.BatchedAoSStack:                "timeStepWithMusclHancock_batched_call_stack",
    RiemannKernelVariant.BatchedAoSoAStack:              "timeStepWithMusclHancock_batched_call_stack",
    RiemannKernelVariant.BatchedSoAStack:                "timeStepWithMusclHancock_batched_call_stack",
    RiemannKernelVariant.PatchWiseSplitTermsAoSStack:    "timeStepWithMusclHancock_patchwise_split_terms_call_stack",
    RiemannKernelVariant.PatchWiseSplitTermsAoSoAStack:  "timeStepWithMusclHancock_patchwise_split_terms_call_stack",
    RiemannKernelVariant.PatchWiseAoSHeap:               "timeStepWithMusclHancock_patchwise_heap",
    RiemannKernelVariant.PatchWiseAoSoAHeap:             "timeStepWithMusclHancock_patchwise_heap",
    RiemannKernelVariant.PatchWiseSoAHeap:               "timeStepWithMusclHancock_patchwise_heap",
    RiemannKernelVariant.BatchedAoSHeap:                 "timeStepWithMusclHancock_batched_heap",
    RiemannKernelVariant.BatchedAoSoAHeap:               "timeStepWithMusclHancock_batched_heap",
    RiemannKernelVariant.BatchedSoAHeap:                 "timeStepWithMusclHancock_batched_heap",
    RiemannKernelVariant.TaskGraphAoSHeap:               "timeStepWithMusclHancock_batched_heap",
    RiemannKernelVariant.TaskGraphAoSoAHeap:             "timeStepWithMusclHancock_batched_heap",
    RiemannKernelVariant.TaskGraphSoAHeap:               "timeStepWithMusclHancock_batched_heap",
    RiemannKernelVariant.PatchWiseSplitTermsAoSHeap:     "timeStepWithMusclHancock_patchwise_split_terms_heap",
    RiemannKernelVariant.PatchWiseSplitTermsAoSoAHeap:   "timeStepWithMusclHancock_patchwise_split_terms_heap",
    RiemannKernelVariant.PatchWiseInSitu:                "timeStepWithMusclHancock_patchwise_in_situ",
    RiemannKernelVariant.BatchedInSitu:                  "timeStepWithMusclHancock_patchwise_in_situ"
  }
  
  EnumeratorTemplateTypes = {
    RiemannKernelVariant.PatchWiseAoSStack:              "::exahype2::enumerator::AoSLexicographicEnumerator",
    RiemannKernelVariant.PatchWiseAoSoAStack:            "::exahype2::enumerator::AoSoALexicographicEnumerator",
    RiemannKernelVariant.PatchWiseSoAStack:              "::exahype2::enumerator::SoALexicographicEnumerator",
    RiemannKernelVariant.BatchedAoSStack:                "::exahype2::enumerator::AoSLexicographicEnumerator",
    RiemannKernelVariant.BatchedAoSoAStack:              "::exahype2::enumerator::AoSoALexicographicEnumerator",
    RiemannKernelVariant.BatchedSoAStack:                "::exahype2::enumerator::SoALexicographicEnumerator",
    RiemannKernelVariant.PatchWiseSplitTermsAoSStack:    "::exahype2::enumerator::AoSLexicographicEnumerator",
    RiemannKernelVariant.PatchWiseSplitTermsAoSoAStack:  "::exahype2::enumerator::AoSoALexicographicEnumerator",
    RiemannKernelVariant.PatchWiseAoSHeap:               "::exahype2::enumerator::AoSLexicographicEnumerator",
    RiemannKernelVariant.PatchWiseAoSoAHeap:             "::exahype2::enumerator::AoSoALexicographicEnumerator",
    RiemannKernelVariant.PatchWiseSoAHeap:               "::exahype2::enumerator::SoALexicographicEnumerator",
    RiemannKernelVariant.BatchedAoSHeap:                 "::exahype2::enumerator::AoSLexicographicEnumerator",
    RiemannKernelVariant.BatchedAoSoAHeap:               "::exahype2::enumerator::AoSoALexicographicEnumerator",
    RiemannKernelVariant.BatchedSoAHeap:                 "::exahype2::enumerator::SoALexicographicEnumerator",
    RiemannKernelVariant.TaskGraphAoSHeap:               "::exahype2::enumerator::AoSLexicographicEnumerator",
    RiemannKernelVariant.TaskGraphAoSoAHeap:             "::exahype2::enumerator::AoSoALexicographicEnumerator",
    RiemannKernelVariant.TaskGraphSoAHeap:               "::exahype2::enumerator::SoALexicographicEnumerator",
    RiemannKernelVariant.PatchWiseSplitTermsAoSHeap:     "::exahype2::enumerator::AoSLexicographicEnumerator",
    RiemannKernelVariant.PatchWiseSplitTermsAoSoAHeap:   "::exahype2::enumerator::AoSoALexicographicEnumerator",
    RiemannKernelVariant.PatchWiseInSitu:           None,
    RiemannKernelVariant.BatchedInSitu:             None
  }

  template  = RiemannKernelCalls[riemann_kernel_variant]
  
  if solver_variant == SolverVariant.WithVirtualFunctions:
    template += """_functors(
  patchData,
  {{NUMBER_OF_VOLUMES_PER_AXIS}},
  {{HALO_SIZE}},
  {{NUMBER_OF_UNKNOWNS}},
  {{NUMBER_OF_AUXILIARY_VARIABLES}},
  {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
  {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
  {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
  {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %},
  [&](
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double * __restrict__ F
  )->void {
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, dt, normal, F );
    {% endif %}
  },
  [&](
    const double * __restrict__                  Q,
    const double * __restrict__     deltaQ,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double * __restrict__ BTimesDeltaQ
  )->void {
    {% if NCP_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, deltaQ, faceCentre, volumeH, t, dt, normal, BTimesDeltaQ );
    {% endif %}
  },
  [&](
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  volumeX,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    double * __restrict__ S
  )->void {
    {% if SOURCE_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.sourceTerm( Q, volumeX, volumeH, t, dt, S );
    {% endif %}
  },
  [&](
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal
  )->double {
    return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, dt, normal);
  }
);
  """
  if solver_variant == SolverVariant.Stateless and EnumeratorTemplateTypes[riemann_kernel_variant]==None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_VOLUMES_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}>(
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    );
  """
  if solver_variant == SolverVariant.Accelerator and EnumeratorTemplateTypes[riemann_kernel_variant]==None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_VOLUMES_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}>(
      targetDevice,
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    );
  """
  if solver_variant == SolverVariant.Stateless and EnumeratorTemplateTypes[riemann_kernel_variant]!=None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_VOLUMES_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, {{TEMP_DATA_ENUMERATOR}}>(
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    );
  """
  if solver_variant == SolverVariant.Accelerator and EnumeratorTemplateTypes[riemann_kernel_variant]!=None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_VOLUMES_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, {{TEMP_DATA_ENUMERATOR}}>(
      targetDevice,
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    );
  """
        

  result = jinja2.Template( template, undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION" ]         = flux_implementation
  d[ "NCP_IMPLEMENTATION" ]          = ncp_implementation
  d[ "SOURCE_IMPLEMENTATION" ]       = source_implementation
  d[ "COMPUTE_MAX_EIGENVALUE" ]      = compute_max_eigenvalue_of_next_time_step
  #if EnumeratorTemplateTypes[riemann_kernel_variant]==None:
  #  d[ "TEMP_DATA_ENUMERATOR" ]      = "::exahype2::fv::enumerator::AoSoALexicographicEnumerator"
  #else:
  d[ "TEMP_DATA_ENUMERATOR" ]      = EnumeratorTemplateTypes[riemann_kernel_variant]
  return result.render(**d)


def create_abstract_solver_declarations(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, pde_terms_without_state):
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
     *
     * ## SYCL
     *
     * At the moment, SYCL seems to struggle with ipo, even if a function is 
     * never called. So I embed the (empty) implementation directly into the
     * header.
     */
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      Offloadable
    )
    #if defined(GPUOffloadingSYCL)
    {}
    #else
    ;
    #endif
    #if defined(GPUOffloadingOMP)
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
      double                                       dt,
      int                                          normal
    ) {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};


    {% if FLUX_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it 
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     *
     * ## SYCL
     *
     * At the moment, SYCL seems to struggle with ipo, even if a function is 
     * never called. So I embed the (empty) implementation directly into the
     * header.
     */
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    )
    #if defined(GPUOffloadingSYCL)
    {}
    #else
    ;
    #endif
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}
    
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
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
     *
     * ## SYCL
     *
     * At the moment, SYCL seems to struggle with ipo, even if a function is 
     * never called. So I embed the (empty) implementation directly into the
     * header.
     */
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void nonconservativeProduct(
      const double * __restrict__                  Q,       // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__                  deltaQ,  // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    )
    #if defined(GPUOffloadingSYCL)
    {}
    #else
    ;
    #endif
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}
    
    {% if NCP_IMPLEMENTATION!="<none>" %}
    virtual void nonconservativeProduct(
      const double * __restrict__                  Q,      // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__                  deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if NCP_IMPLEMENTATION=="<user-defined>" %}=0{% endif %};
    {% endif %}
    

    {% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it 
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     *
     * ## SYCL
     *
     * At the moment, SYCL seems to struggle with ipo, even if a function is 
     * never called. So I embed the (empty) implementation directly into the
     * header.
     */
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      double * __restrict__ S,
      Offloadable
    )
    #if defined(GPUOffloadingSYCL)
    {}
    #else
    ;
    #endif
    #if defined(GPUOffloadingOMP)
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
  d[ "STATELESS_PDE_TERMS"]                 = pde_terms_without_state
  return Template.render(**d)


def create_abstract_solver_definitions(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
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
 const tarch::la::Vector<Dimensions,double>&  volumeH,
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
  const double * __restrict__                  Q,      // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__                  deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__                        BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{NCP_IMPLEMENTATION}}
}
{% endif %}


{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
#if !defined(GPUOffloadingSYCL)
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
#endif
{% endif %}



{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if !defined(GPUOffloadingSYCL)
GPUCallableMethod double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      Offloadable
) {
  {{EIGENVALUES_IMPLEMENTATION}};
}
#endif
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}


{% if FLUX_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if !defined(GPUOffloadingSYCL)
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
) {
  {% if FLUX_IMPLEMENTATION=="<none>" %}
  tarch::gpuAbort();
  {% else %}
  {{FLUX_IMPLEMENTATION}}
  {% endif %}
}
#endif
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}


{% if NCP_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if !defined(GPUOffloadingSYCL)
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
      const double * __restrict__                  Q,        // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__                  deltaQ,   // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
) {
  {% if NCP_IMPLEMENTATION=="<none>" %}
  tarch::gpuAbort();
  {% else %}
  {{NCP_IMPLEMENTATION}}
  {% endif %}
}
#endif
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}


{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
#if !defined(GPUOffloadingSYCL)
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      double * __restrict__ S,
      Offloadable
) {
  {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %}
  tarch::gpuAbort();
  {% else %}
  {{SOURCE_TERM_IMPLEMENTATION}}
  {% endif %}
}
#endif
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}
""", undefined=jinja2.DebugUndefined)

  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "STATELESS_PDE_TERMS"]                             = pde_terms_without_state  
  return Template.render(**d)


def create_solver_declarations(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
  public:
    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    virtual void sourceTerm(
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
      double                                       dt,
      int                                          normal
    ) override;
    {% endif %}


    {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
    virtual void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}
     
     
    {% if NCP_IMPLEMENTATION=="<user-defined>" %}
    virtual void nonconservativeProduct(
      const double * __restrict__                  Q,        // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__                  deltaQ,   // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
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
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void sourceTerm(
       const double * __restrict__ Q,
       const tarch::la::Vector<Dimensions,double>&  volumeCentre,
       const tarch::la::Vector<Dimensions,double>&  volumeH,
       double                                       t,
       double                                       dt,
       double * __restrict__ S,
       Offloadable
     );
    #if defined(GPUOffloadingOMP)
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
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
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
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
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
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void nonconservativeProduct(
      const double * __restrict__                  Q,         
      const double * __restrict__                  deltaQ,    
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        BTimesDeltaQ,
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}    
""", undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "STATELESS_PDE_TERMS"]                             = pde_terms_without_state  
  return Template.render(**d)


def create_solver_definitions(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
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
  double                                       dt,
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
  const double * __restrict__                  Q,      // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__                  deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__                        BTimesDeltaQ  // BTimesDeltaQQ[{{NUMBER_OF_UNKNOWNS}}]
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





{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
GPUCallableMethod double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  Offloadable
)  {
  // @todo implement
}
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
{% endif %}


{% if FLUX_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F,
  Offloadable
)  {
  // @todo implement
}
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
{% endif %}


{% if NCP_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__                  Q,        // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__                  deltaQ,   // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__                        BTimesDeltaQ,
  Offloadable
)  {
  // @todo implement
}
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
{% endif %}



{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
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
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
{% endif %}


""", undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "STATELESS_PDE_TERMS"]                             = pde_terms_without_state  
  return Template.render(**d)


