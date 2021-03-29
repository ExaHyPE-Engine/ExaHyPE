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

from .GenericRusanovAdaptiveTimeStepSizeWithEnclaves import GenericRusanovAdaptiveTimeStepSizeWithEnclaves

import peano4.output.Jinja2TemplatedHeaderImplementationFilePair


class UpdateCellWithOptimisticTimeStep(ReconstructPatchAndApplyFunctor):  
  TemplateUpdateCell = """
  ::exahype2::fv::validatePatch(
      reconstructedPatch,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      1, // halo
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  ); // previous time step has to be valid
    
  if ( {{PREDICATE_COMPUTE_SKELETON}} ) {
    // @todo Debug
    logInfo( "touchCellFirstTime()", "compute skeleton task for " << marker.toString() );

    assertion1( {{PREDICATE_COMPUTE_SKELETON}}, marker.toString() );
    assertion1( not ({{PREDICATE_SPAWN_ENCLAVE}}), marker.toString() );
    assertion1( not ({{PREDICATE_SPAWN_ENCLAVE_AND_DROP_OPTIMISTIC_TASK}}), marker.toString() );
    
    tasks::{{SOLVER_NAME}}EnclaveTask::applyKernelToCell(
      marker,
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
      reconstructedPatch,
      originalPatch
    );
  }
  else if ( {{PREDICATE_SPAWN_ENCLAVE}} ) {
    // @todo Debug
    logInfo( "touchCellFirstTime()", "spawn enclave task for " << marker.toString() );

    assertion1( not ({{PREDICATE_COMPUTE_SKELETON}}), marker.toString() );
    assertion1( {{PREDICATE_SPAWN_ENCLAVE}}, marker.toString() );
    assertion1( not ({{PREDICATE_SPAWN_ENCLAVE_AND_DROP_OPTIMISTIC_TASK}}), marker.toString() );
    
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
  else {
    // @todo Debug
    logInfo( "touchCellFirstTime()", "drop optimistic task and spawn enclave task for " << marker.toString() );

    assertion(false);
    
    assertion1( not ({{PREDICATE_COMPUTE_SKELETON}}), marker.toString() );
    assertion1( not ({{PREDICATE_SPAWN_ENCLAVE}}), marker.toString() );
    assertion1( {{PREDICATE_SPAWN_ENCLAVE_AND_DROP_OPTIMISTIC_TASK}}, marker.toString() );
    
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
    
  def __init__(self,solver,use_split_loop=False,spawn_enclave_task_guard="true"):
    predicate_compute_skeleton                       = """not marker.isRefined() and marker.isSkeletonCell() and (repositories::""" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::Primary or repositories::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::PrimaryAfterGridInitialisation)"
    predicate_spawn_enclave_and_drop_optimistic_task = """not marker.isRefined() and marker.isEnclaveCell()  and tarch::la::equals( repositories::""" + solver.get_name_of_global_instance() + """.getPredictedTimeStepSize(),0.0) and not tarch::la::equals( repositories::""" + solver.get_name_of_global_instance() + """.getMinTimeStepSize(),0.0) and (repositories::""" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::Primary or repositories::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::PrimaryAfterGridInitialisation)"
    predicate_spawn_enclave                          = """not marker.isRefined() and marker.isEnclaveCell()  and tarch::la::equals( repositories::""" + solver.get_name_of_global_instance() + """.getMinTimeStepSize(),0.0) and (repositories::""" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::Primary or repositories::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::PrimaryAfterGridInitialisation)"
    
    d = {}
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)
    d["PREDICATE_COMPUTE_SKELETON"]                       = predicate_compute_skeleton
    d["PREDICATE_SPAWN_ENCLAVE_AND_DROP_OPTIMISTIC_TASK"] = predicate_spawn_enclave_and_drop_optimistic_task
    d["PREDICATE_SPAWN_ENCLAVE"]                          = predicate_spawn_enclave
    
    ReconstructPatchAndApplyFunctor.__init__(self,
      solver._patch,
      solver._patch_overlap,
      jinja2.Template( self.TemplateUpdateCell ).render(**d),
      solver._reconstructed_array_memory_location,
      "(" + predicate_compute_skeleton + """) 
or 
(""" + predicate_spawn_enclave_and_drop_optimistic_task + """) 
or 
(""" + predicate_spawn_enclave + """)""",
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


#    d["PREDICATE_SPAWN_OPTIMISTIC_TASK"]                  = predicate_spawn_optimistic_task
# taus hier
#    predicate_spawn_optimistic_task                  = """not marker.isRefined() and marker.isEnclaveCell()  and repositories::""" + solver.get_name_of_global_instance() + """.getPredictedTimeStepSize()>0.0  and repositories::""" + solver.get_name_of_global_instance() + """.getMinTimeStepSize()>0.0  and repositories::""" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::Secondary"


class GenericRusanovOptimisticTimeStepSizeWithEnclaves( GenericRusanovAdaptiveTimeStepSizeWithEnclaves ):
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, flux=PDETerms.User_Defined_Implementation, ncp=None, plot_grid_properties=False, time_step_relaxation=0.1):
    """
    
    
    
    """
    self._time_step_relaxation                = time_step_relaxation

    GenericRusanovAdaptiveTimeStepSizeWithEnclaves.__init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties)

    self.set_implementation(flux=flux,ncp=ncp)


  def create_action_sets(self):
    GenericRusanovAdaptiveTimeStepSizeWithEnclaves.create_action_sets(self)
    self._action_set_update_cell = UpdateCellWithOptimisticTimeStep(self)
    
    #self._action_set_update_cell = MergeEnclaveTaskOutcomeAndTriggerOptimisticTimeStep(self)


  def add_actions_to_perform_time_step(self, step):
    """
    
    We invoke the FV superclass' operation and add the enclave labels, but 
    we exchange the update cell thing.
 
    """
    FV.add_actions_to_perform_time_step(self,step)
    step.add_action_set( exahype2.grid.EnclaveLabels(self._name) ) 
    #step.add_action_set( MergeEnclaveTaskOutcomeAndTriggerOptimisticTimeStep(self) )


  def add_implementation_files_to_project(self,namespace,output):
    """

      Add the enclave task for the GPU

    """
    GenericRusanovAdaptiveTimeStepSizeWithEnclaves.add_implementation_files_to_project(self,namespace,output)

    # TODO this is not working if we have inheritance
    # templatefile_prefix = os.path.dirname( os.path.realpath(__file__) ) + "/" + self.__class__.__name__
    templatefile_prefix = os.path.join(os.path.dirname(os.path.realpath(__file__)), "GenericRusanovOptimisticTimeStepSizeWithEnclaves")


    implementationDictionary = {}
    self._init_dictionary_with_default_parameters(implementationDictionary)
    self.add_entries_to_text_replacement_dictionary(implementationDictionary)

    implementationDictionary[ "NUMBER_OF_INNER_DOUBLE_VALUES_IN_PATCH_2D" ] = (implementationDictionary["NUMBER_OF_VOLUMES_PER_AXIS"]-2) * (implementationDictionary["NUMBER_OF_VOLUMES_PER_AXIS"]-2) * (implementationDictionary["NUMBER_OF_UNKNOWNS"] + implementationDictionary["NUMBER_OF_AUXILIARY_VARIABLES"])
    implementationDictionary[ "NUMBER_OF_INNER_DOUBLE_VALUES_IN_PATCH_3D" ] = (implementationDictionary["NUMBER_OF_VOLUMES_PER_AXIS"]-2) * (implementationDictionary["NUMBER_OF_VOLUMES_PER_AXIS"]-2) * (implementationDictionary["NUMBER_OF_VOLUMES_PER_AXIS"]-2) * (implementationDictionary["NUMBER_OF_UNKNOWNS"] + implementationDictionary["NUMBER_OF_AUXILIARY_VARIABLES"])

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
