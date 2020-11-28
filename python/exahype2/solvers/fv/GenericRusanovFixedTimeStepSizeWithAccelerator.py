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
    // @todo Holger: Das bleibt auf der CPU. Also eigentlich kein Grund, es zu 
    //  aendern. Wenn Du allerdings den GPU Task generierst, kannste das in der
    //  Theorie auch generien. Wie gesagt - glaube das braucht es nicht.
    {{LOOP_OVER_PATCH_FUNCTION_CALL}}(
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
    
    ::tarch::multicore::freeMemory(reconstructedPatch, ::tarch::multicore::MemoryLocation::Accelerator);
  }
  else { // is an enclave cell
    // @todo Holger: Your part
    /*
    auto perCellFunctor = [&](double* reconstructedPatch, double* originalPatch, const ::peano4::datamanagement::CellMarker& marker) -> void {
      ::exahype2::fv::copyPatch(
        reconstructedPatch,
        originalPatch,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        1 // halo size
      );
      {{LOOP_OVER_PATCH_FUNCTION_CALL}}(
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
    */

    ::exahype2::EnclaveOpenMPGPUTask* newEnclaveTask = new ::exahype2::EnclaveOpenMPGPUTask(
      marker,
      reconstructedPatch,
      #if Dimensions==2
      {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}}
      #else
      {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}}
      #endif
    );
      
    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( newEnclaveTask->getTaskId() );
    peano4::parallel::Tasks spawn( 
      newEnclaveTask,
      peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
      peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovFixedTimeStepSizeWithAccelerator" )
    );      
  }
  """      


  RusanovCallWithFluxAndEigenvalues = """
        ::exahype2::fv::splitRusanov1d(
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double                                       F[]
          ) -> void {
            {{SOLVER_NAME}}::flux( Q, faceCentre, volumeH, t, normal, F );
          },
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal
          ) -> double {
            return {{SOLVER_NAME}}::maxEigenvalue( Q, faceCentre, volumeH, t, normal );
          }, 
"""


  RusanovCallWithNCPAndEigenvalues = """
        ::exahype2::fv::splitRusanov1d(
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double                                       F[]
          ) -> void {
          },
          [] (
            double                                       Q[],
            double                                       gradQ[][Dimensions],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double                                       BgradQ[]
          ) -> void {
            {{SOLVER_INSTANCE}}::nonconservativeProduct( Q, gradQ, faceCentre, volumeH, t, normal, BgradQ );
          },
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal
          ) -> double {
            return {{SOLVER_NAME}}::maxEigenvalue( Q, faceCentre, volumeH, t, normal );
          }, 
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
    