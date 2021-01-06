# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV                       import *
 
from .PDETerms import PDETerms

import peano4
import exahype2

import jinja2

from .GenericRusanovFixedTimeStepSizeWithEnclaves import GenericRusanovFixedTimeStepSizeWithEnclaves 

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor


class UpdateCellWithEnclavesOnAccelerator(ReconstructPatchAndApplyFunctor):
  TemplateUpdateCell = """
    ::exahype2::fv::validatePatch(
      reconstructedPatch,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      1, // halo
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
    ); // previous time step has to be valid
  
  if (marker.isSkeletonCell()) {
    tasks::{{GPU_ENCLAVE_TASK_NAME}}::runComputeKernelsOnSkeletonCell( reconstructedPatch, marker, fineGridCell{{UNKNOWN_IDENTIFIER}}.value );
  }
  else { // is an enclave cell
    tasks::{{GPU_ENCLAVE_TASK_NAME}}* newEnclaveTask = new tasks::{{GPU_ENCLAVE_TASK_NAME}}(
      marker,
      reconstructedPatch
    );
    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( newEnclaveTask->getTaskId() );
    peano4::parallel::Tasks spawn( 
      newEnclaveTask,
      peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
      peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovFixedTimeStepSizeWithAccelerator" )
    );
  }
  """      
  
  
  def __init__(self,solver,use_split_loop=False):
    d = {}
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)
    d["USE_SPLIT_LOOP"] = use_split_loop      
    d["GPU_ENCLAVE_TASK_NAME"] = solver._GPU_enclave_task_name()
    
    self._solver = solver
    
    ReconstructPatchAndApplyFunctor.__init__(self,
      solver._patch,
      solver._patch_overlap,
      jinja2.Template( self.TemplateUpdateCell ).render(**d),
      solver._reconstructed_array_memory_location,
      "not marker.isRefined() and (" + \
      "observers::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::Primary or " + \
      "observers::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::PrimaryAfterGridInitialisation" + \
      ")"
    )
    self.label_name = exahype2.grid.EnclaveLabels.get_attribute_name(solver._name)


  def get_includes(self):
    return ReconstructPatchAndApplyFunctor.get_includes(self) + """
#include "tasks/""" + self._solver._GPU_enclave_task_name() + """.h"    
"""


class GenericRusanovFixedTimeStepSizeWithAccelerator( GenericRusanovFixedTimeStepSizeWithEnclaves ):
  """
  
   This is a specialisation of the enclave tasking that works with accelerators
   (GPUs). 
  
  """
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size, flux=PDETerms.User_Defined_Implementation, ncp=None, plot_grid_properties=False):
    GenericRusanovFixedTimeStepSizeWithEnclaves.__init__(self, 
      name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size, 
      flux, ncp, 
      plot_grid_properties
    )


  def set_implementation(self,
    flux=None,ncp=None,eigenvalues=None,boundary_conditions=None,refinement_criterion=None,initial_conditions=None,
    memory_location         = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete,
    use_split_loop          = True
  ):
    """
     Call implementation configuration of master class, but exchange the actual compute kernel
    """
    GenericRusanovFixedTimeStepSizeWithEnclaves.set_implementation(self,
      flux, ncp, eigenvalues, boundary_conditions, refinement_criterion, initial_conditions,
      memory_location, use_split_loop)

    self._action_set_update_cell = UpdateCellWithEnclavesOnAccelerator(self, use_split_loop=True)
    
    
  def _GPU_enclave_task_name(self):
    return self._name + "GPUEnclaveTask"


  def add_implementation_files_to_project(self,namespace,output):
    """
    
      Add the enclave task for the GPU  
      
    """
    GenericRusanovFixedTimeStepSizeWithEnclaves.add_implementation_files_to_project(self,namespace,output)

    templatefile_prefix = os.path.dirname( os.path.realpath(__file__) ) + "/" + self.__class__.__name__

    implementationDictionary = {}
    self._init_dictionary_with_default_parameters(implementationDictionary)
    self.add_entries_to_text_replacement_dictionary(implementationDictionary)

    task_name = self._GPU_enclave_task_name()
    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".GPUEnclaveTask.template.h",
      templatefile_prefix + ".GPUEnclaveTask.template.cpp",
      task_name, 
      namespace + [ "tasks" ],
      "tasks", 
      implementationDictionary,
      True)

    output.add( generated_solver_files )
    output.makefile.add_cpp_file( "tasks/" + task_name + ".cpp" )

