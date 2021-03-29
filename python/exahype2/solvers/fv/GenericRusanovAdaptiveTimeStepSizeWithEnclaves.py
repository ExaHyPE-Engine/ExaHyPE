# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV                       import *
 
from .PDETerms import PDETerms

import peano4
import exahype2

import jinja2

from .GenericRusanovFixedTimeStepSize import GenericRusanovFixedTimeStepSize 
from .GenericRusanovFixedTimeStepSize import UpdateCell 
from .EnclaveTasking                  import MergeEnclaveTaskOutcome
from .EnclaveTasking                  import EnclaveTaskingFV

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor


import peano4.output.Jinja2TemplatedHeaderImplementationFilePair


class UpdateCellWithEnclaves(ReconstructPatchAndApplyFunctor):
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
    tasks::{{SOLVER_NAME}}EnclaveTask::applyKernelToCell(
      marker,
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
      reconstructedPatch,
      originalPatch
    );
  
  }
  else {
    assertion( marker.isEnclaveCell() );
    ::exahype2::EnclaveTask* newEnclaveTask = new tasks::{{SOLVER_NAME}}EnclaveTask(
      marker,
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
      reconstructedPatch
    );
          
    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( newEnclaveTask->getTaskId() );

    peano4::parallel::Tasks spawn( 
      newEnclaveTask,
      peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
      peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovFixedTimeStepSizeWithEnclaves" )
    );   
  }
  """      
    
    
  def __init__(self,solver,use_split_loop=False):
    d = {}
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)
    d["USE_SPLIT_LOOP"] = use_split_loop      
    
    ReconstructPatchAndApplyFunctor.__init__(self,
      solver._patch,
      solver._patch_overlap,
      jinja2.Template( self.TemplateUpdateCell ).render(**d),
      solver._reconstructed_array_memory_location,
      "not marker.isRefined() and (" + \
      "repositories::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::Primary or " + \
      "repositories::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::PrimaryAfterGridInitialisation" + \
      ")", 
      add_assertions_to_halo_exchange = True
    )
    self.label_name = exahype2.grid.EnclaveLabels.get_attribute_name(solver._name)

    self._solver    = solver



  def get_includes(self):
    return ReconstructPatchAndApplyFunctor.get_includes(self) + """
#include "peano4/parallel/Tasks.h"
#include "repositories/SolverRepository.h"
#include "tasks/""" + self._solver._name + """EnclaveTask.h"
""" + self._solver._get_default_includes() + self._solver.get_user_includes() 


class GenericRusanovAdaptiveTimeStepSizeWithEnclaves( EnclaveTaskingFV ):
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, flux=PDETerms.User_Defined_Implementation, ncp=None, plot_grid_properties=False, time_step_relaxation=0.1):
    """
    
    An adaptive time stepping scheme with enclave tasking
    
    This is a simple implementation of a FV scheme using a generic 
    Rusanov solver. It applies the concept of enclave tasking and 
    thus exhibits a higher concurrency level than a plain FV 
    counterpart. The code also adds adaptive time stepping, i.e. the
    solver analyses the eigenvalues and sets the time step size 
    accordingly.
    
    !! Adding your own includes and source code 
    
    To add further includes to the generated task, add them to your
    solver via a statement similar to 
    <pre>
    
self.additional_includes += " ""
 #include "../CCZ4Kernels.h"
" ""
    
    </pre>
    
    The actual task will be dumped into a subdirectory task, so the .. 
    ensures that the relative path starts at the project's root.
    
    
    """
    self._time_step_relaxation                = time_step_relaxation

    EnclaveTaskingFV.__init__(self, name, patch_size, 1, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties)
   
    self.set_implementation(flux=flux,ncp=ncp)

  
  def add_entries_to_text_replacement_dictionary(self,d):
    """
     
     d: Dictionary of string to string
        in/out argument
    
    """
    EnclaveTaskingFV.add_entries_to_text_replacement_dictionary(self,d)
    d[ "TIME_STEP_SIZE" ]                     = "repositories::"+d[ "SOLVER_INSTANCE" ] + ".getMinTimeStepSize()"
    d[ "TIME_STAMP" ]                         = "repositories::"+d[ "SOLVER_INSTANCE" ] + ".getMinTimeStamp()"
    d[ "TIME_STEP_RELAXATION" ]               = self._time_step_relaxation
   

  def create_action_sets(self):
    EnclaveTaskingFV.create_action_sets(self)

    self._action_set_update_cell = UpdateCellWithEnclaves(self)


  def _enclave_task_name(self):
    return self._name + "EnclaveTask"


  def add_implementation_files_to_project(self,namespace,output):
    """

      Add the enclave task for the GPU

    """
    EnclaveTaskingFV.add_implementation_files_to_project(self,namespace,output)

    # TODO this is not working if we have inheritance
    # templatefile_prefix = os.path.dirname( os.path.realpath(__file__) ) + "/" + self.__class__.__name__
    templatefile_prefix = os.path.join(os.path.dirname(os.path.realpath(__file__)), "GenericRusanovAdaptiveTimeStepSizeWithEnclaves")


    implementationDictionary = {}
    self._init_dictionary_with_default_parameters(implementationDictionary)
    self.add_entries_to_text_replacement_dictionary(implementationDictionary)

    task_name = self._enclave_task_name()

    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".EnclaveTask.template.h",
      templatefile_prefix + ".EnclaveTask.template.cpp",
      task_name,
      namespace + [ "tasks" ],
      "tasks",
      implementationDictionary,
      True)

    output.add( generated_solver_files )
    output.makefile.add_cpp_file( "tasks/" + task_name + ".cpp" )
