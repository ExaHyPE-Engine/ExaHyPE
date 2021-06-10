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
    tasks::{{GPU_ENCLAVE_TASK_NAME}}::applyKernelToCell( 
      marker, 
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
      reconstructedPatch, 
      fineGridCell{{UNKNOWN_IDENTIFIER}}.value 
    );
  }
  else { // is an enclave cell
    tasks::{{GPU_ENCLAVE_TASK_NAME}}* newEnclaveTask = new tasks::{{GPU_ENCLAVE_TASK_NAME}}(
      marker,
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
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
      "repositories::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::Primary or " + \
      "repositories::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::PrimaryAfterGridInitialisation" + \
      ")"
    )
    self.label_name = exahype2.grid.EnclaveLabels.get_attribute_name(solver._name)


  def get_includes(self):
    return ReconstructPatchAndApplyFunctor.get_includes(self) + """
#include "tasks/""" + self._solver._GPU_enclave_task_name() + """.h"
"""  + self._solver._get_default_includes() + self._solver.get_user_includes()


class GenericRusanovFixedTimeStepSizeWithAccelerator( GenericRusanovFixedTimeStepSizeWithEnclaves ):
  """

   This is a specialisation of the enclave tasking that works with accelerators
   (GPUs).

  """
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size, flux=PDETerms.User_Defined_Implementation, ncp=None, plot_grid_properties=False, ngrabmax=0):
    self.ngrabmax_ = ngrabmax
    GenericRusanovFixedTimeStepSizeWithEnclaves.__init__(self,
      name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size,
      flux, ncp,
      plot_grid_properties
    )


  def set_implementation(self,
    flux=None,ncp=None,eigenvalues=None,boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    memory_location         = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete,
    use_split_loop          = True
  ):
    """
     Call implementation configuration of master class, but exchange the actual compute kernel
    """
    GenericRusanovFixedTimeStepSizeWithEnclaves.set_implementation(self,
      flux, ncp, eigenvalues, boundary_conditions, refinement_criterion, initial_conditions,source_term,
      memory_location, use_split_loop)

    self._action_set_update_cell = UpdateCellWithEnclavesOnAccelerator(self, use_split_loop=True)


  def _GPU_enclave_task_name(self):
    return self._name + "GPUEnclaveTask"


  def add_implementation_files_to_project(self,namespace,output):
    """

      Add the enclave task for the GPU

    """
    GenericRusanovFixedTimeStepSizeWithEnclaves.add_implementation_files_to_project(self,namespace,output)

    # TODO this is not working if we have inheritance
    # templatefile_prefix = os.path.dirname( os.path.realpath(__file__) ) + "/" + self.__class__.__name__
    templatefile_prefix = os.path.join(os.path.dirname(os.path.realpath(__file__)), "GenericRusanovFixedTimeStepSizeWithAccelerator")


    implementationDictionary = {}
    self._init_dictionary_with_default_parameters(implementationDictionary)
    self.add_entries_to_text_replacement_dictionary(implementationDictionary)

    implementationDictionary["NGRABMAX"] = self.ngrabmax_

    task_name = self._GPU_enclave_task_name()

    # Bit of a hack so we can easily instantiate templates
    implementationDictionary["SKIP_NCP"]  = "true" if implementationDictionary["NCP_IMPLEMENTATION"]  == "<none>" else "false"
    implementationDictionary["SKIP_FLUX"] = "true" if implementationDictionary["FLUX_IMPLEMENTATION"] == "<none>" else "false"

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

    # We need to explicitly link objects in gpu mode, since PEANO_SOURCE_DIR is gone,
    # this hack has to suffice
    peanodir = templatefile_prefix.split("python/exahype2",1)[0]
    output.makefile.d["PEANODIR"] = peanodir

    exahype2.gpu.add_exahype_objects(output.makefile)
