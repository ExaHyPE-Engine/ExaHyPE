# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.fv.PDETerms  import PDETerms

import jinja2

from enum import Enum


from exahype2.solvers.rkfd.kernels import SolverVariant    
from exahype2.solvers.rkfd.kernels import KernelVariant    

   
def create_compute_kernel_for_FD4(flux_implementation, 
                                          ncp_implementation, 
                                          source_implementation, 
                                          compute_max_eigenvalue_of_next_time_step, 
                                          solver_variant,
                                          kernel_variant,
                                          KOSigma):
  """
  
  I return only the unqualified function call, i.e. without any namespaces.
  So by setting the right namespace as prefix, you can direct it to particular
  implementations.
  
  solver_variant: SolverVariant
  
  riemann_kernel_variant: KernelVariant
  
  """  
  KernelCalls = {
    KernelVariant.PatchWiseAoSHeap:               "timeStep_patchwise_heap",
    KernelVariant.PatchWiseAoSoAHeap:             "timeStep_patchwise_heap",
    KernelVariant.PatchWiseSoAHeap:               "timeStep_patchwise_heap",
    KernelVariant.BatchedAoSHeap:                 "timeStep_batched_heap",
    KernelVariant.BatchedAoSoAHeap:               "timeStep_batched_heap",
    KernelVariant.BatchedSoAHeap:                 "timeStep_batched_heap",
    KernelVariant.TaskGraphAoSHeap:               "timeStep_batched_heap",
    KernelVariant.TaskGraphAoSoAHeap:             "timeStep_batched_heap",
    KernelVariant.TaskGraphSoAHeap:               "timeStep_batched_heap",
  }
  
  EnumeratorTemplateTypes = {
    KernelVariant.PatchWiseAoSHeap:               "::exahype2::enumerator::AoSLexicographicEnumerator",
    KernelVariant.PatchWiseAoSoAHeap:             "::exahype2::enumerator::AoSoALexicographicEnumerator",
    KernelVariant.PatchWiseSoAHeap:               "::exahype2::enumerator::SoALexicographicEnumerator",
    KernelVariant.BatchedAoSHeap:                 "::exahype2::enumerator::AoSLexicographicEnumerator",
    KernelVariant.BatchedAoSoAHeap:               "::exahype2::enumerator::AoSoALexicographicEnumerator",
    KernelVariant.BatchedSoAHeap:                 "::exahype2::enumerator::SoALexicographicEnumerator",
    KernelVariant.TaskGraphAoSHeap:               "::exahype2::enumerator::AoSLexicographicEnumerator",
    KernelVariant.TaskGraphAoSoAHeap:             "::exahype2::enumerator::AoSoALexicographicEnumerator",
    KernelVariant.TaskGraphSoAHeap:               "::exahype2::enumerator::SoALexicographicEnumerator",
  }

  template  = KernelCalls[kernel_variant]
  
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
  }
);
  """
  if solver_variant == SolverVariant.Stateless and EnumeratorTemplateTypes[kernel_variant]==None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}>(
      patchData,
      {{KOSIGMA}},
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      false     // Runge-Kutta, so no need to copy old data over or to take dt into account
    );
  """
  if solver_variant == SolverVariant.Accelerator and EnumeratorTemplateTypes[kernel_variant]==None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}>(
      targetDevice,
      patchData,
      {{KOSIGMA}},
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      false     // Runge-Kutta, so no need to copy old data over or to take dt into account
    );
  """
  if solver_variant == SolverVariant.Stateless and EnumeratorTemplateTypes[kernel_variant]!=None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, {{TEMP_DATA_ENUMERATOR}}>(
      patchData,
      {{KOSIGMA}},
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      false     // Runge-Kutta, so no need to copy old data over or to take dt into account
    );
  """
  if solver_variant == SolverVariant.Accelerator and EnumeratorTemplateTypes[kernel_variant]!=None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},{{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, {{TEMP_DATA_ENUMERATOR}}>(
      targetDevice,
      patchData,
      {{KOSIGMA}},
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      false     // Runge-Kutta, so no need to copy old data over or to take dt into account
    );
  """
        

  result = jinja2.Template( template, undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION" ]         = flux_implementation
  d[ "NCP_IMPLEMENTATION" ]          = ncp_implementation
  d[ "SOURCE_IMPLEMENTATION" ]       = source_implementation
  d[ "COMPUTE_MAX_EIGENVALUE" ]      = compute_max_eigenvalue_of_next_time_step
  d[ "KOSIGMA" ]                     = KOSigma
  d[ "TEMP_DATA_ENUMERATOR" ]        = EnumeratorTemplateTypes[kernel_variant]
  return result.render(**d)
