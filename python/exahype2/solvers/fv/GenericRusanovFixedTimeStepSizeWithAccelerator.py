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
    const int sourcePatchSize      = ({{NUMBER_OF_VOLUMES_PER_AXIS}}+2)*({{NUMBER_OF_VOLUMES_PER_AXIS}}+2)*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    #elif Dimensions==3
    const int destinationPatchSize = {{NUMBER_OF_VOLUMES_PER_AXIS}}*{{NUMBER_OF_VOLUMES_PER_AXIS}}*{{NUMBER_OF_VOLUMES_PER_AXIS}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    const int sourcePatchSize      = ({{NUMBER_OF_VOLUMES_PER_AXIS}}+2)*({{NUMBER_OF_VOLUMES_PER_AXIS}}+2)*({{NUMBER_OF_VOLUMES_PER_AXIS}}+2)*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    #endif
    double* destinationPatchOnGPU = ::tarch::multicore::allocateMemory(destinationPatchSize, ::tarch::multicore::MemoryLocation::Accelerator);

    const double timeStamp = {{SOLVER_INSTANCE}}.getMinTimeStamp();
    
    //#if defined(GPUOffloading)
    #pragma omp target map(from:destinationPatchOnGPU[0:destinationPatchSize]) map(to:reconstructedPatch[0:sourcePatchSize])
    {
    //#endif
    ::exahype2::fv::copyPatch(
      reconstructedPatch,
      destinationPatchOnGPU,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      1 // halo size
    );
}
    
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
        timeStamp,
        {{TIME_STEP_SIZE}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        reconstructedPatch,
        destinationPatchOnGPU
    );
//    #if defined(GPUOffloading)
//    }
//    #endif
    
    
    // get stuff explicitly back from GPU, as it will be stored
    // locally for a while
    double* destinationPatchOnCPU = ::tarch::multicore::allocateMemory(destinationPatchSize, ::tarch::multicore::MemoryLocation::Heap);
    std::copy_n(destinationPatchOnGPU,destinationPatchSize,destinationPatchOnCPU);
    ::tarch::multicore::freeMemory(reconstructedPatch,    ::tarch::multicore::MemoryLocation::Accelerator);
    ::tarch::multicore::freeMemory(destinationPatchOnGPU, ::tarch::multicore::MemoryLocation::Accelerator);
    ::exahype2::EnclaveBookkeeping::getInstance().finishedTask(gpuTaskId,destinationPatchSize,destinationPatchOnCPU);
  }
  """      


  RusanovCallWithFlux = """
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
          }
"""


  """
  
    Flux is empty here.
    
  """
  RusanovCallWithNCP = """
          [] (
           double * __restrict__ Q,
           const tarch::la::Vector<Dimensions,double>&  faceCentre,
           const tarch::la::Vector<Dimensions,double>&  volumeH,
           double                                       t,
           double                                       dt,
           int                                          normal,
           double * __restrict__                        F
          ) -> void {
            for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) F[i] = 0.0;
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
            {{SOLVER_NAME}}::nonconservativeProduct( Q, gradQ, faceCentre, volumeH, t, normal, BgradQ );
          }
"""


  """
  
    Combination of the two previous ones.
    
  """
  RusanovCallWithFluxAndNCP = """
          [] (
           double * __restrict__ Q,
           const tarch::la::Vector<Dimensions,double>&  faceCentre,
           const tarch::la::Vector<Dimensions,double>&  volumeH,
           double                                       t,
           double                                       dt,
           int                                          normal,
           double * __restrict__                        F
          ) -> void {
            {{SOLVER_NAME}}::flux( Q, faceCentre, volumeH, t, normal, F );
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
            {{SOLVER_NAME}}::nonconservativeProduct( Q, gradQ, faceCentre, volumeH, t, normal, BgradQ );
          }
"""



  EigenvaluesCall = """
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal
          ) -> double {
            return {{SOLVER_NAME}}::maxEigenvalue( Q, faceCentre, volumeH, t, normal);
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
    

  #def set_implementation(self,flux=None,ncp=None,eigenvalues=None,boundary_conditions=None,refinement_criterion=None,initial_conditions=None):
  #  """
  #    Redefinition to ensure that we use our Riemann solver ingredient calls
  #  """
  #  GenericRusanovFixedTimeStepSizeWithEnclaves.set_implementation(self,flux,ncp,eigenvalues,boundary_conditions,refinement_criterion,initial_conditions)

  #  if self._flux_implementation!=PDETerms.None_Implementation and self._ncp_implementation==PDETerms.None_Implementation:
  #    self._rusanov_call = self.RusanovCallWithFlux
  #  elif self._flux_implementation==PDETerms.None_Implementation and self._ncp_implementation!=PDETerms.None_Implementation:
  #    self._rusanov_call = self.RusanovCallWithNCP
  #  elif self._flux_implementation!=PDETerms.None_Implementation and self._ncp_implementation!=PDETerms.None_Implementation:
  #    self._rusanov_call = self.RusanovCallWithFluxAndNCP
  #  else:
  #    raise Exception("ERROR: Combination of fluxes/operators not supported. flux: {} ncp: {}".format(flux, ncp))

  #  self._construct_template_update_cell()


  def _construct_template_update_cell(self):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    d[ "LOOP_OVER_PATCH_FUNCTION_CALL" ] = self._kernel_implementation
    d[ "TIME_STAMP" ]                   = "{{SOLVER_INSTANCE}}.getMinTimeStamp()"
    d[ "EIGENVALUES"]                   = self.EigenvaluesCall

    if self._flux_implementation!=PDETerms.None_Implementation and self._ncp_implementation==PDETerms.None_Implementation:
      d[ "RUSANOV_ON_FACE"] = self.RusanovCallWithFlux
    elif self._flux_implementation==PDETerms.None_Implementation and self._ncp_implementation!=PDETerms.None_Implementation:
      d[ "RUSANOV_ON_FACE"] = self.RusanovCallWithNCP
    elif self._flux_implementation!=PDETerms.None_Implementation and self._ncp_implementation!=PDETerms.None_Implementation:
      d[ "RUSANOV_ON_FACE"] = self.RusanovCallWithFluxAndNCP
      
    temp = jinja2.Template( self.TemplateUpdateCell ).render(d);
    self._template_update_cell      = jinja2.Template( temp ); 


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
    
