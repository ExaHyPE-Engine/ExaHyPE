# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from exahype2.solvers.fv.kernels import SolverVariant
from exahype2.solvers.fv.kernels import KernelVariant

def create_compute_Riemann_kernel_for_Rusanov(flux_implementation,
                                              ncp_implementation,
                                              source_implementation,
                                              compute_max_eigenvalue_of_next_time_step,
                                              solver_variant: SolverVariant,
                                              kernel_variant: KernelVariant): 
  """
  I return only the unqualified function call, i.e. without any namespaces.
  So by setting the right namespace as prefix, you can direct it to particular
  implementations.
  """
  KernelCalls = {
    KernelVariant.PatchWiseAoSStack:               "timeStepWithRusanov_patchwise_call_stack",
    KernelVariant.PatchWiseAoSoAStack:             "timeStepWithRusanov_patchwise_call_stack",
    KernelVariant.PatchWiseSoAStack:               "timeStepWithRusanov_patchwise_call_stack",
    KernelVariant.PatchWiseAoSHeap:               "timeStepWithRusanov_patchwise_heap",
    KernelVariant.PatchWiseAoSoAHeap:             "timeStepWithRusanov_patchwise_heap",
    KernelVariant.PatchWiseSoAHeap:               "timeStepWithRusanov_patchwise_heap",
    KernelVariant.BatchedAoSStack:                 "timeStepWithRusanov_batched_call_stack",
    KernelVariant.BatchedAoSoAStack:               "timeStepWithRusanov_batched_call_stack",
    KernelVariant.BatchedSoAStack:                 "timeStepWithRusanov_batched_call_stack",
    KernelVariant.BatchedAoSHeap:                 "timeStepWithRusanov_batched_heap",
    KernelVariant.BatchedAoSoAHeap:               "timeStepWithRusanov_batched_heap",
    KernelVariant.BatchedSoAHeap:                 "timeStepWithRusanov_batched_heap",
    KernelVariant.TaskGraphAoSHeap:               "timeStepWithRusanov_taskgraph_heap",
    KernelVariant.TaskGraphAoSoAHeap:             "timeStepWithRusanov_taskgraph_heap",
    KernelVariant.TaskGraphSoAHeap:               "timeStepWithRusanov_taskgraph_heap",
    KernelVariant.GenericAoS:                     "timeStepWithRusanov_generic",
    KernelVariant.GenericAoSoA:                   "timeStepWithRusanov_generic",
    KernelVariant.GenericSoA:                     "timeStepWithRusanov_generic",
  }
  
  EnumeratorTemplateTypes = {
    KernelVariant.PatchWiseAoSStack:               "::exahype2::enumerator::AoSLexicographicEnumerator",
    KernelVariant.PatchWiseAoSoAStack:             "::exahype2::enumerator::AoSoALexicographicEnumerator",
    KernelVariant.PatchWiseSoAStack:               "::exahype2::enumerator::SoALexicographicEnumerator",
    KernelVariant.PatchWiseAoSHeap:               "::exahype2::enumerator::AoSLexicographicEnumerator",
    KernelVariant.PatchWiseAoSoAHeap:             "::exahype2::enumerator::AoSoALexicographicEnumerator",
    KernelVariant.PatchWiseSoAHeap:               "::exahype2::enumerator::SoALexicographicEnumerator",
    KernelVariant.BatchedAoSStack:                 "::exahype2::enumerator::AoSLexicographicEnumerator",
    KernelVariant.BatchedAoSoAStack:               "::exahype2::enumerator::AoSoALexicographicEnumerator",
    KernelVariant.BatchedSoAStack:                 "::exahype2::enumerator::SoALexicographicEnumerator",
    KernelVariant.BatchedAoSHeap:                 "::exahype2::enumerator::AoSLexicographicEnumerator",
    KernelVariant.BatchedAoSoAHeap:               "::exahype2::enumerator::AoSoALexicographicEnumerator",
    KernelVariant.BatchedSoAHeap:                 "::exahype2::enumerator::SoALexicographicEnumerator",
    KernelVariant.TaskGraphAoSHeap:               "::exahype2::enumerator::AoSLexicographicEnumerator",
    KernelVariant.TaskGraphAoSoAHeap:             "::exahype2::enumerator::AoSoALexicographicEnumerator",
    KernelVariant.TaskGraphSoAHeap:               "::exahype2::enumerator::SoALexicographicEnumerator",
    KernelVariant.GenericAoS:                     "::exahype2::enumerator::AoSLexicographicEnumerator",
    KernelVariant.GenericAoSoA:                   "::exahype2::enumerator::AoSoALexicographicEnumerator",
    KernelVariant.GenericSoA:                     "::exahype2::enumerator::SoALexicographicEnumerator",
  }

  template  = KernelCalls[kernel_variant]

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
    [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
    [[maybe_unused]] double                                       t,
    [[maybe_unused]] double                                       dt,
    [[maybe_unused]] int                                          normal,
    [[maybe_unused]] double* __restrict__                         F // F[{{NUMBER_OF_UNKNOWNS}}]
  )->void {
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, dt, normal, F );
    {% endif %}
  },
  [&](
    [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
    [[maybe_unused]] double                                       t,
    [[maybe_unused]] double                                       dt,
    [[maybe_unused]] int                                          normal,
    [[maybe_unused]] double* __restrict__                         BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
  )->void {
    {% if NCP_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, deltaQ, faceCentre, volumeH, t, dt, normal, BTimesDeltaQ );
    {% endif %}
  },
  [&](
    [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeX,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
    [[maybe_unused]] double                                       t,
    [[maybe_unused]] double                                       dt,
    [[maybe_unused]] double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
  )->void {
    {% if SOURCE_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.sourceTerm( Q, volumeX, volumeH, t, dt, S );
    {% endif %}
  },
  [&](
    [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
    [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
    [[maybe_unused]] double                                       t,
    [[maybe_unused]] double                                       dt,
    [[maybe_unused]] int                                          normal
  )->double {
    return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, dt, normal);
  }
);
  """
  elif solver_variant == SolverVariant.Stateless:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_VOLUMES_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, ::exahype2::enumerator::AoSLexicographicEnumerator>(
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    );
  """
  elif solver_variant == SolverVariant.Multicore:
    template += """_multicore_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_VOLUMES_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, ::exahype2::enumerator::AoSLexicographicEnumerator>(
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    );
  """
  elif solver_variant == SolverVariant.AcceleratorWithExplicitCopy:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_VOLUMES_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, ::exahype2::enumerator::AoSLexicographicEnumerator>(
      targetDevice,
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    );
  """
  else:
    assert False, "not supported combination: {} x {}".format( solver_variant, kernel_variant )

  result = jinja2.Template( template, undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION" ]         = flux_implementation
  d[ "NCP_IMPLEMENTATION" ]          = ncp_implementation
  d[ "SOURCE_IMPLEMENTATION" ]       = source_implementation
  d[ "COMPUTE_MAX_EIGENVALUE" ]      = compute_max_eigenvalue_of_next_time_step
  d[ "TEMP_DATA_ENUMERATOR" ]        = EnumeratorTemplateTypes[kernel_variant]
  return result.render(**d)

def create_abstract_solver_declarations(flux_implementation,
                                        ncp_implementation,
                                        eigenvalues_implementation,
                                        source_term_implementation,
                                        pde_terms_without_state):
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
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      Offloadable
    )
    //#if defined(GPUOffloadingSYCL)
    //{}
    //#else
    ;
    //#endif
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
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal
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
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}
    
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         F // F[{{NUMBER_OF_UNKNOWNS}}]
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
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if NCP_IMPLEMENTATION!="<none>" %}
    virtual void nonconservativeProduct(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
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
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] double* __restrict__                         S, // S[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    virtual void sourceTerm(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
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
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  {{EIGENVALUES_IMPLEMENTATION}}
}
{% endif %}

{% if FLUX_IMPLEMENTATION!="<none>" and FLUX_IMPLEMENTATION!="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{FLUX_IMPLEMENTATION}}
}
{% endif %}

{% if NCP_IMPLEMENTATION!="<none>" and NCP_IMPLEMENTATION!="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{NCP_IMPLEMENTATION}}
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
//#if !defined(GPUOffloadingSYCL)
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
) {
  {% if SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
  {{SOURCE_TERM_IMPLEMENTATION}}
  {% else %}
  std::fill_n(S,{{NUMBER_OF_UNKNOWNS}},0.0);
  {% endif %}
}
//#endif
{% endif %}

{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
//#if !defined(GPUOffloadingSYCL)
GPUCallableMethod double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  Offloadable
) {
  {{EIGENVALUES_IMPLEMENTATION}};
}
//#endif
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if FLUX_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
//#if !defined(GPUOffloadingSYCL)
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         F, // F[{{NUMBER_OF_UNKNOWNS}}]
  Offloadable
) {
  {% if FLUX_IMPLEMENTATION=="<none>" %}
  tarch::gpuAbort();
  {% else %}
  {{FLUX_IMPLEMENTATION}}
  {% endif %}
}
//#endif
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if NCP_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] Offloadable
) {
  {% if NCP_IMPLEMENTATION=="<none>" %}
  tarch::gpuAbort();
  {% else %}
  {{NCP_IMPLEMENTATION}}
  {% endif %}
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double* __restrict__                         S, // S[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] Offloadable
) {
  {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %}
  tarch::gpuAbort();
  {% else %}
  {{SOURCE_TERM_IMPLEMENTATION}}
  {% endif %}
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
  d[ "STATELESS_PDE_TERMS"]                 = pde_terms_without_state
  return Template.render(**d)

def create_solver_declarations(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
  public:
    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    virtual void sourceTerm(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
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
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal
    ) override;
    {% endif %}

    {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
    virtual void flux(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}

    {% if NCP_IMPLEMENTATION=="<user-defined>" %}
    virtual void nonconservativeProduct(
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         BTimesDeltaQ // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
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
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] double* __restrict__                         S, // S[{{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] Offloadable
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
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] Offloadable
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
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         F, // F[{{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}

    {% if STATELESS_PDE_TERMS and NCP_IMPLEMENTATION=="<user-defined>" %}
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
      [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
      [[maybe_unused]] double                                       t,
      [[maybe_unused]] double                                       dt,
      [[maybe_unused]] int                                          normal,
      [[maybe_unused]] double* __restrict__                         BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
      [[maybe_unused]] Offloadable
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
  d[ "STATELESS_PDE_TERMS"]                 = pde_terms_without_state
  return Template.render(**d)

def create_solver_definitions(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal
) {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "maxEigenvalue(...)" );
}
{% endif %}

{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "flux(...)" );
}
{% endif %}

{% if NCP_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         BTimesDeltaQ // BTimesDeltaQQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "nonconservativeProduct(...)" );
}
{% endif %}

{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeX,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double* __restrict__                         S // S[{{NUMBER_OF_UNKNOWNS}}]
) {
  logTraceInWith4Arguments( "sourceTerm(...)", volumeX, volumeH, t, dt );

  // @todo implement and ensure that all entries of S are properly set
  for (int i=0; i<NumberOfUnknowns; i++) {
    S[i] = 0.0;
  }

  logTraceOut( "sourceTerm(...)" );
}
{% endif %}

{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  Offloadable
) {
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
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         F, // F[{{NUMBER_OF_UNKNOWNS}}]
  Offloadable
) {
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
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const double* __restrict__                   deltaQ, // deltaQ[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& faceCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] int                                          normal,
  [[maybe_unused]] double* __restrict__                         BTimesDeltaQ, // BTimesDeltaQ[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] Offloadable
) {
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
  [[maybe_unused]] const double* __restrict__                   Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeCentre,
  [[maybe_unused]] const tarch::la::Vector<Dimensions, double>& volumeH,
  [[maybe_unused]] double                                       t,
  [[maybe_unused]] double                                       dt,
  [[maybe_unused]] double* __restrict__                         S, // S[{{NUMBER_OF_UNKNOWNS}}]
  [[maybe_unused]] Offloadable
) {
  // @todo implement but ensure that all entries of S are properly set
  for (int i=0; i<NumberOfUnknowns; i++) {
    S[i] = 0.0;
  }
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
  d[ "STATELESS_PDE_TERMS"]                 = pde_terms_without_state
  return Template.render(**d)
