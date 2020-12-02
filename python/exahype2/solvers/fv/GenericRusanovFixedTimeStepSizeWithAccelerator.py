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
    const int gpuTaskId = tarch::multicore::reserveTaskNumber();
    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( gpuTaskId );
     
    // @todo Holger: The two functions have to go to the GPU - later as indepenent tasks 
    
    #if Dimensions==2
    const int destinationPatchSize = {{NUMBER_OF_VOLUMES_PER_AXIS}}*{{NUMBER_OF_VOLUMES_PER_AXIS}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    #elif Dimensions==3
    const int destinationPatchSize = {{NUMBER_OF_VOLUMES_PER_AXIS}}*{{NUMBER_OF_VOLUMES_PER_AXIS}}*{{NUMBER_OF_VOLUMES_PER_AXIS}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    #endif
    double* destinationPatchOnGPU = ::tarch::multicore::allocateMemory(destinationPatchSize, ::tarch::multicore::MemoryLocation::Accelerator);
    double* destinationPatchOnCPU = ::tarch::multicore::allocateMemory(destinationPatchSize, ::tarch::multicore::MemoryLocation::Heap);
    
    ::exahype2::fv::copyPatch(
      reconstructedPatch,
      destinationPatchOnGPU,
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
        destinationPatchOnGPU
    );
    
    
    // get stuff explicitly back from GPU, as it will be stored
    // locally for a while
    std::copy_n(destinationPatchOnGPU,destinationPatchSize,destinationPatchOnCPU);
    ::tarch::multicore::freeMemory(reconstructedPatch,    ::tarch::multicore::MemoryLocation::Accelerator);
    ::tarch::multicore::freeMemory(destinationPatchOnGPU, ::tarch::multicore::MemoryLocation::Accelerator);
    ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(gpuTaskId,destinationPatchSize,destinationPatchOnCPU);
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

  
  def get_user_includes(self):
    return GenericRusanovFixedTimeStepSizeWithEnclaves.get_user_includes(self) + """
#include <algorithm>
"""    
    