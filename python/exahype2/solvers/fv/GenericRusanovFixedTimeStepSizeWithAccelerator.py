# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV                       import FV
 
from .PDETerms import PDETerms

import peano4
import exahype2

import jinja2

from .GenericRusanovFixedTimeStepSizeWithEnclaves import GenericRusanovFixedTimeStepSizeWithEnclaves 


class GenericRusanovFixedTimeStepSizeWithAccelerator( GenericRusanovFixedTimeStepSizeWithEnclaves ):
  TemplateUpdateCell = """
  if (marker.isSkeletonCell()) {
    {{LOOP_OVER_PATH_FUNCTION_CALL}}(
      [&](
        double                                       QL[],
        double                                       QR[],
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       dx,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       FL[],
        double                                       FR[]
      ) -> void {
        ::exahype2::fv::splitRusanov1d(
          {{RUSANOV_ON_FACE}},
          {{EIGENVALUES}},
          QL, QR, x, dx, t, dt, normal,
          {{NUMBER_OF_UNKNOWNS}},
          {{NUMBER_OF_AUXILIARY_VARIABLES}},
          FL,FR
        );
      },
      marker.x(),
      marker.h(),
      {{TIME_STAMP}},
      {{TIME_STEP_SIZE}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      reconstructedPatch,
      originalPatch
    );
    
    {{FREE_SKELETON_MEMORY}}
  }
  else { // is an enclave cell
    #if defined(GPUOffloading)
    #pragma omp declare target
    #endif
    auto perCellFunctor = [&](double* reconstructedPatch, double* originalPatch, const ::peano4::datamanagement::CellMarker& marker) -> void {
      ::exahype2::fv::copyPatch(
        reconstructedPatch,
        originalPatch,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        1 // halo size
      );
      {{LOOP_OVER_PATH_FUNCTION_CALL}}(
        [&](
          double                                       QL[],
          double                                       QR[],
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       dx,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double                                       FL[],
          double                                       FR[]
        ) -> void {
          ::exahype2::fv::splitRusanov1d(
            {{RUSANOV_ON_FACE}},
            {{EIGENVALUES}},
            QL, QR, x, dx, t, dt, normal,
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            FL,FR
          );
        },
        marker.x(),
        marker.h(),
        {{TIME_STAMP}},
        {{TIME_STEP_SIZE}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        reconstructedPatch,
        originalPatch
      );
    };
    #if defined(GPUOffloading)
    #pragma omp end declare target
    #endif

    ::exahype2::EnclaveOpenMPGPUTask* newEnclaveTask = new ::exahype2::EnclaveOpenMPGPUTask(
      marker,
      reconstructedPatch,
      #if Dimensions==2
      {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}},
      #else
      {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}},
      #endif
      perCellFunctor
    );
      
    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( newEnclaveTask->getTaskId() );
    peano4::parallel::Tasks spawn( 
      newEnclaveTask,
      peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
      peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovFixedTimeStepSizeWithEnclaves" )
    );      
  }
  """      

  """
  
   This is a specialisation of the enclave tasking that works with accelerators
   (GPUs). 
  
  """
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size, flux=PDETerms.User_Defined_Implementation, ncp=None, plot_grid_properties=False):
    GenericRusanovFixedTimeStepSizeWithEnclaves.__init__(self, 
      name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size, 
      flux, ncp, 
      plot_grid_properties,
      FV.CellUpdateImplementation_SplitLoop, #, kernel_implementation
      peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete
    )
    


  def set_update_cell_implementation(self,
    kernel_implementation   = FV.CellUpdateImplementation_SplitLoop,
    memory_location         = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete
  ):
    if kernel_implementation != FV.CellUpdateImplementation_SplitLoop:
      raise Exception( "kernel implementation cannot be reconfigured for GPU/Accelerator FV solver" )
    if memory_location != peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
      raise Exception( "memory selector cannot be reset to non-accelerator memory" )

    GenericRusanovFixedTimeStepSizeWithEnclaves.set_update_cell_implementation(self,
      FV.CellUpdateImplementation_SplitLoop,
      peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete)

  
  #def _wrap_update_cell_template(self, update_cell_template):
  #  """
  #  
  #  I use the same memory management for both the skeletons and the enclave
  #  cells. Consequently, I also have to use the corresponding frees.
  #  
  #  """
  #  free_memory_call_for_skeleton_cells = ""
  #  if self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete:
  #    free_memory_call_for_skeleton_cells = "tarch::multicore::freeMemory(reconstructedPatch,::tarch::multicore::MemoryLocation::Heap);"
  #  if self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
  #    free_memory_call_for_skeleton_cells = "tarch::multicore::freeMemory(reconstructedPatch,::tarch::multicore::MemoryLocation::Accelerator);"##

  #  result = """
  #  double minTimeStamp =  {{SOLVER_INSTANCE}}.getMinTimeStamp();#

  #  if (marker.isSkeletonCell()) {
  #""" + update_cell_template + """
  #    """ + free_memory_call_for_skeleton_cells + """
  #  }
  #  else {
  #    #if defined(GPUOffloading)
  #    #pragma omp declare target
  #    #endif
  #    auto perCellFunctor = 
  #      [&](double* reconstructedPatch, double* originalPatch, const ::peano4::datamanagement::CellMarker& marker) -> void {
  #         ::exahype2::fv::copyPatch(
  #           reconstructedPatch,
  #           originalPatch,
  #           {{NUMBER_OF_UNKNOWNS}},
  #           {{NUMBER_OF_AUXILIARY_VARIABLES}},
  #           {{NUMBER_OF_VOLUMES_PER_AXIS}},
  #           {{HALO_SIZE}}
  #         );
  #         """ + update_cell_template + """
  #      };
  #    #if defined(GPUOffloading)
  #    #pragma omp end declare target
  #    #endif##

  #    {{ENCLAVE_TASK_TYPE}}* newEnclaveTask = new {{ENCLAVE_TASK_TYPE}}(
  #      marker,
  #      reconstructedPatch,
  #      #if Dimensions==2
  #      {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}},
  #      #else
  #      {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}},
  #      #endif
  #      perCellFunctor
  #    );
      
  #    fineGridCell""" + exahype2.grid.EnclaveLabels.get_attribute_name(self._name) + """.setSemaphoreNumber( newEnclaveTask->getTaskId() );
  #    peano4::parallel::Tasks spawn( 
  #      newEnclaveTask,
  #      peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
  #      peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovFV" )
  #    );      
  #  }
  #"""
  #  return result
    