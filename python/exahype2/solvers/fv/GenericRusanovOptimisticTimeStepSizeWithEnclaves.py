# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV                       import *
 
from .PDETerms import PDETerms

import peano4
import exahype2

import jinja2

from .EnclaveTasking                  import MergeEnclaveTaskOutcome
from .EnclaveTasking                  import EnclaveTaskingFV

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor


import peano4.output.Jinja2TemplatedHeaderImplementationFilePair


class UpdateCellInPrimarySweep(ReconstructPatchAndApplyFunctor):  
  """
  
  Do the skeleton tasks and kick out the enclave tasks, too. Finally, 
  drop enclave tasks if this is required as we have overestimated the 
  time step size.
  
  """
  
  
  TemplateUpdateCell = """
  ::exahype2::fv::validatePatch(
      reconstructedPatch,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      1, // halo
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  ); // previous time step has to be valid
    
  if ( marker.isSkeletonCell() ) {
    logDebug( "touchCellFirstTime()", "compute skeleton task for " << marker.toString() );

    tasks::{{SOLVER_NAME}}EnclaveTask::applyKernelToCell(
      marker,
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
      reconstructedPatch,
      targetPatch
    );
  }
  else if ( 
    marker.isEnclaveCell() 
    and 
    repositories::{{SOLVER_INSTANCE}}.spawnEnclaveTaskInPrimaryTraversal() 
    and 
    repositories::{{SOLVER_INSTANCE}}.dropOptimisticTaskInPrimaryTraversal() 
  ) {
    int taskToDrop = fineGridCell{{SEMAPHORE_LABEL}}.getSemaphoreNumber();
    assertion( taskToDrop!=::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
    logDebug( "touchCellFirstTime()", "drop task " << taskToDrop << " associated with " << marker.toString() );
    ::exahype2::EnclaveBookkeeping::getInstance().cancelTask(taskToDrop);

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
    logDebug( "touchCellFirstTime()", "issue new enclave task " << newEnclaveTask->getTaskId() << " associated with " << marker.toString() );
  }
  else if ( marker.isEnclaveCell() and repositories::{{SOLVER_INSTANCE}}.spawnEnclaveTaskInPrimaryTraversal() ) {
    ::exahype2::EnclaveTask* newEnclaveTask = new tasks::{{SOLVER_NAME}}EnclaveTask(
      marker,
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
      reconstructedPatch
    );

    logDebug( "touchCellFirstTime()", "spawn enclave task for " << marker.toString() << ", task number=" << newEnclaveTask->getTaskId() );
          
    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( newEnclaveTask->getTaskId() );

    peano4::parallel::Tasks spawn( 
      newEnclaveTask,
      peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
      peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovFixedTimeStepSizeWithEnclaves" )
    );   
  }
  else if ( 
    marker.isEnclaveCell() 
    and 
    repositories::{{SOLVER_INSTANCE}}.mergeOptimisticTaskOutcomeInSecondaryTraversal() 
  ) {
    logDebug( "touchCellFirstTime()", "optimistic task for " << marker.toString() << " is already spawned. Update boundary only" );

    tasks::{{SOLVER_NAME}}OptimisticTask::applyKernelToCellBoundary(
      marker,
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
      reconstructedPatch,
      targetPatch
    );
    
    tarch::freeMemory( reconstructedPatch, tarch::MemoryLocation::Heap );
  }
  """      
  
    
  def __init__(self,solver):
    d = {}
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)
    
    ReconstructPatchAndApplyFunctor.__init__(self,
      solver._patch,
      solver._patch_overlap,
      jinja2.Template( self.TemplateUpdateCell ).render(**d),
      solver._reconstructed_array_memory_location,
      """
not marker.isRefined() 
and 
    (repositories::""" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::Primary or repositories::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + """::SolverState::PrimaryAfterGridInitialisation)
and
 (
    marker.isSkeletonCell() 
    or
    marker.isEnclaveCell() and  repositories::""" + solver.get_name_of_global_instance() + """.spawnEnclaveTaskInPrimaryTraversal()   
    or
    marker.isEnclaveCell() and  repositories::""" + solver.get_name_of_global_instance() + """.mergeOptimisticTaskOutcomeInSecondaryTraversal() 
 )   
    """,
    )
    self.label_name = exahype2.grid.EnclaveLabels.get_attribute_name(solver._name)
    self._solver    = solver


  def get_includes(self):
    return ReconstructPatchAndApplyFunctor.get_includes(self) + """
#include "peano4/parallel/Tasks.h"
#include "repositories/SolverRepository.h"
#include "tasks/""" + self._solver._name + """EnclaveTask.h"
#include "tasks/""" + self._solver._name + """OptimisticTask.h"
""" + self._solver._get_default_includes() + self._solver.get_user_includes() 


class MergeInEnclaveTaskOutcomeInSecondarySweep(AbstractFVActionSet):
  """
  
  This is the plain enclave implementation. The difference to the normal
  implementation is that we switch it off if optimistic time stepping is
  enabled.
  
  """  
  Template = """
  if ( 
    marker.isEnclaveCell() 
    and 
    not marker.isRefined() 
    and 
    repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::Secondary 
    and 
    repositories::{{SOLVER_INSTANCE}}.mergeEnclaveTaskOutcomeInSecondaryTraversal()
  ) {
    const int taskNumber = fineGridCell{{LABEL_NAME}}.getSemaphoreNumber();

    logDebug( "touchCellFirstTime()", "work in enclave data from task " << taskNumber << " into " << marker.toString() );

    assertion( taskNumber!=::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
    ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndCopyResultOver( taskNumber, fineGridCell{{UNKNOWN_IDENTIFIER}}.value );
    fineGridCell{{LABEL_NAME}}.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
      
    ::exahype2::fv::validatePatch(
      fineGridCell{{UNKNOWN_IDENTIFIER}}.value,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      0,
      std::string(__FILE__) + ": " + std::to_string(__LINE__) + "; marker=" + marker.toString()
    );
  }
  

  if ( 
    marker.isEnclaveCell() 
    and 
    not marker.isRefined() 
    and 
    repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::Secondary 
    and 
    repositories::{{SOLVER_INSTANCE}}.mergeOptimisticTaskOutcomeInSecondaryTraversal() 
  ) {
    const int taskNumber = fineGridCell{{LABEL_NAME}}.getSemaphoreNumber();

    logDebug( "touchCellFirstTime()", "work in optimistic data from task " << taskNumber << " into " << marker.toString() );

    assertion( taskNumber!=::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
    
    tasks::{{SOLVER_NAME}}OptimisticTask::mergeTaskOutcomeIntoPatch(
      taskNumber,
      fineGridCell{{UNKNOWN_IDENTIFIER}}.value
    );

    double* targetPatch = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;

    {{POSTPROCESS_UPDATED_PATCH}}

    fineGridCell{{LABEL_NAME}}.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
      
    ::exahype2::fv::validatePatch(
      fineGridCell{{UNKNOWN_IDENTIFIER}}.value,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      0,
      std::string(__FILE__) + ": " + std::to_string(__LINE__) + "; marker=" + marker.toString()
    );
  }

  if ( 
    marker.isEnclaveCell() 
    and 
    not marker.isRefined() 
    and 
    repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::Secondary 
    and 
    repositories::{{SOLVER_INSTANCE}}.spawnOptimisticTaskInSecondaryTraversal() 
  ) {
    logDebug( "touchCellFirstTime()", "spawn new optimistic task for " << marker.toString() );
    assertion( tarch::la::greater( repositories::{{SOLVER_INSTANCE}}.getPredictedTimeStepSize(), 0.0 ) );
  
    ::exahype2::EnclaveTask* newOptimisticTask = new tasks::{{SOLVER_NAME}}OptimisticTask(
      marker,
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
      repositories::{{SOLVER_INSTANCE}}.getPredictedTimeStepSize(),
      fineGridCell{{UNKNOWN_IDENTIFIER}}.value      
    );
          
    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( newOptimisticTask->getTaskId() );

    logDebug( "touchCellFirstTime()", "optimistic task number is " << newOptimisticTask->getTaskId() );

    peano4::parallel::Tasks spawn( 
      newOptimisticTask,
      peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
      peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovOptimisticTimeStepSizeWithEnclaves" )
    );   
  }

"""


  def __init__(self,solver):
    AbstractFVActionSet.__init__(self,solver)
    self.label_name = exahype2.grid.EnclaveLabels.get_attribute_name(solver._name)


  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "LABEL_NAME" ] = self.label_name      
      result = jinja2.Template(self.Template).render(**d)
      pass 
    return result


  def get_includes(self):
    return AbstractFVActionSet.get_includes(self) + """
#include "peano4/parallel/Tasks.h"
#include "repositories/SolverRepository.h"
#include "tasks/""" + self._solver._name + """EnclaveTask.h"
#include "tasks/""" + self._solver._name + """OptimisticTask.h"
""" + self._solver._get_default_includes() + self._solver.get_user_includes() 


class GenericRusanovOptimisticTimeStepSizeWithEnclaves( EnclaveTaskingFV ):
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, flux=PDETerms.User_Defined_Implementation, ncp=None, plot_grid_properties=False, time_step_relaxation=0.1):
    """
    
    
    
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
    d[ "TIME_STEP_RELAXATION" ]               = self._time_step_relaxation

  
  def create_action_sets(self):
    EnclaveTaskingFV.create_action_sets(self)
    self._action_set_update_cell           = UpdateCellInPrimarySweep(self)
    self._merge_enclave_task_outcome       = MergeInEnclaveTaskOutcomeInSecondarySweep(self)
    

  def _enclave_task_name(self):
    return self._name + "EnclaveTask"

  
  def add_implementation_files_to_project(self,namespace,output):
    """

      Add the enclave task for the GPU

    """
    EnclaveTaskingFV.add_implementation_files_to_project(self,namespace,output)

    implementationDictionary = {}
    self._init_dictionary_with_default_parameters(implementationDictionary)
    self.add_entries_to_text_replacement_dictionary(implementationDictionary)

    templatefile_prefix = os.path.join(os.path.dirname(os.path.realpath(__file__)), "GenericRusanovAdaptiveTimeStepSizeWithEnclaves")
    task_name = self._enclave_task_name()
    
    implementationDictionary[ "NUMBER_OF_INNER_DOUBLE_VALUES_IN_PATCH_2D" ] = (implementationDictionary["NUMBER_OF_VOLUMES_PER_AXIS"]-2) * (implementationDictionary["NUMBER_OF_VOLUMES_PER_AXIS"]-2) * (implementationDictionary["NUMBER_OF_UNKNOWNS"] + implementationDictionary["NUMBER_OF_AUXILIARY_VARIABLES"])
    implementationDictionary[ "NUMBER_OF_INNER_DOUBLE_VALUES_IN_PATCH_3D" ] = (implementationDictionary["NUMBER_OF_VOLUMES_PER_AXIS"]-2) * (implementationDictionary["NUMBER_OF_VOLUMES_PER_AXIS"]-2) * (implementationDictionary["NUMBER_OF_VOLUMES_PER_AXIS"]-2) * (implementationDictionary["NUMBER_OF_UNKNOWNS"] + implementationDictionary["NUMBER_OF_AUXILIARY_VARIABLES"])

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

    templatefile_prefix = os.path.join(os.path.dirname(os.path.realpath(__file__)), "GenericRusanovOptimisticTimeStepSizeWithEnclaves")
    task_name = self._name + "OptimisticTask"

    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".OptimisticTask.template.h",
      templatefile_prefix + ".OptimisticTask.template.cpp",
      task_name,
      namespace + [ "tasks" ],
      "tasks",
      implementationDictionary,
      True)

    output.add( generated_solver_files )
    output.makefile.add_cpp_file( "tasks/" + task_name + ".cpp" )


  def create_data_structures(self):
    """
    
     The vanilla enclave variant does not store/copy the enclave cells 
     in-between the primary and secondary sweep. We can't do this trick
     here, as the data flow/update pattern is relatively complex.
     
    """
    EnclaveTaskingFV.create_data_structures(self)

    self._patch.generator.store_persistent_condition = self._store_cell_data_default_predicate()
    self._patch.generator.load_persistent_condition  = self._load_cell_data_default_predicate()


