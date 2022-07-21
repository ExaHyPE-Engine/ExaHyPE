# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.fv.PDETerms  import PDETerms

import jinja2

from enum import Enum


from exahype2.solvers.rkfd.kernels import SolverVariant    
from exahype2.solvers.rkfd.kernels import RiemannKernelVariant    

   
def create_compute_Riemann_kernel_for_FD4(flux_implementation, 
                                              ncp_implementation, 
                                              source_implementation, 
                                              compute_max_eigenvalue_of_next_time_step, 
                                              solver_variant,
                                              riemann_kernel_variant,
                                              KOSigma):
  """
  
  I return only the unqualified function call, i.e. without any namespaces.
  So by setting the right namespace as prefix, you can direct it to particular
  implementations.
  
  solver_variant: SolverVariant
  
  riemann_kernel_variant: RiemannKernelVariant
  
  """  
  RiemannKernelCalls = {
    RiemannKernelVariant.PatchWiseAoSStack:              "timeStep_patchwise_call_stack",
    RiemannKernelVariant.PatchWiseAoSoAStack:            "timeStep_patchwise_call_stack",
    RiemannKernelVariant.PatchWiseSoAStack:              "timeStep_patchwise_call_stack",
    RiemannKernelVariant.BatchedAoSStack:                "timeStep_batched_call_stack",
    RiemannKernelVariant.BatchedAoSoAStack:              "timeStep_batched_call_stack",
    RiemannKernelVariant.BatchedSoAStack:                "timeStep_batched_call_stack",
    RiemannKernelVariant.PatchWiseSplitTermsAoSStack:    "timeStep_patchwise_split_terms_call_stack",
    RiemannKernelVariant.PatchWiseSplitTermsAoSoAStack:  "timeStep_patchwise_split_terms_call_stack",
    RiemannKernelVariant.PatchWiseAoSHeap:               "timeStep_patchwise_heap",
    RiemannKernelVariant.PatchWiseAoSoAHeap:             "timeStep_patchwise_heap",
    RiemannKernelVariant.PatchWiseSoAHeap:               "timeStep_patchwise_heap",
    RiemannKernelVariant.BatchedAoSHeap:                 "timeStep_batched_heap",
    RiemannKernelVariant.BatchedAoSoAHeap:               "timeStep_batched_heap",
    RiemannKernelVariant.BatchedSoAHeap:                 "timeStep_batched_heap",
    RiemannKernelVariant.TaskGraphAoSHeap:               "timeStep_batched_heap",
    RiemannKernelVariant.TaskGraphAoSoAHeap:             "timeStep_batched_heap",
    RiemannKernelVariant.TaskGraphSoAHeap:               "timeStep_batched_heap",
    RiemannKernelVariant.PatchWiseSplitTermsAoSHeap:     "timeStep_patchwise_split_terms_heap",
    RiemannKernelVariant.PatchWiseSplitTermsAoSoAHeap:   "timeStep_patchwise_split_terms_heap",
    RiemannKernelVariant.PatchWiseInSitu:                "timeStep_patchwise_in_situ",
    RiemannKernelVariant.BatchedInSitu:                  "timeStep_patchwise_in_situ"
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
  {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
  {{HALO_SIZE}},
  {{NUMBER_OF_UNKNOWNS}},
  {{NUMBER_OF_AUXILIARY_VARIABLES}},
  {{KOSIGMA}},
  {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
  {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
  {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
  {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %},
  false, // Runge-Kutta, so no need to copy old data over or to take dt into account
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
    {% if COMPUTE_MAX_EIGENVALUE==False %} 
    return 0.0; // dummy value; should not be called anyway
    {% else %}
    return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, dt, normal);
    {% endif %}
  }
);
  """
  if solver_variant == SolverVariant.Stateless and EnumeratorTemplateTypes[riemann_kernel_variant]==None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}>(
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
    );
  """
  if solver_variant == SolverVariant.Accelerator and EnumeratorTemplateTypes[riemann_kernel_variant]==None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}>(
      targetDevice,
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
    );
  """
  if solver_variant == SolverVariant.Stateless and EnumeratorTemplateTypes[riemann_kernel_variant]!=None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, {{TEMP_DATA_ENUMERATOR}}>(
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
    );
  """
  if solver_variant == SolverVariant.Accelerator and EnumeratorTemplateTypes[riemann_kernel_variant]!=None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, {{TEMP_DATA_ENUMERATOR}}>(
      targetDevice,
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
    );
  """
        

  result = jinja2.Template( template, undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION" ]         = flux_implementation
  d[ "NCP_IMPLEMENTATION" ]          = ncp_implementation
  d[ "SOURCE_IMPLEMENTATION" ]       = source_implementation
  d[ "COMPUTE_MAX_EIGENVALUE" ]      = compute_max_eigenvalue_of_next_time_step
  d[ "KOSIGMA" ]                     = KOSigma
  #if EnumeratorTemplateTypes[riemann_kernel_variant]==None:
  #  d[ "TEMP_DATA_ENUMERATOR" ]      = "::exahype2::fv::enumerator::AoSoALexicographicEnumerator"
  #else:
  d[ "TEMP_DATA_ENUMERATOR" ]      = EnumeratorTemplateTypes[riemann_kernel_variant]
  return result.render(**d)
