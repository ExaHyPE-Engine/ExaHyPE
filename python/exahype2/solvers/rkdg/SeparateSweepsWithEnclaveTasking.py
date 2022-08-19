# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .RungeKuttaDG    import RungeKuttaDG
from .SeparateSweeps  import SeparateSweeps
from .PDETerms        import PDETerms
 
import peano4
import exahype2

import jinja2

import os

from exahype2.solvers.fv.actionsets.AbstractFVActionSet             import AbstractFVActionSet

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor


class MergeEnclaveTaskOutcome(AbstractFVActionSet):
  Template = """
  if ( 
    not marker.hasBeenRefined()
    and 
    {{GUARD}}
    and
    repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::Secondary 
  ) {
    const int taskNumber = fineGridCell{{LABEL_NAME}}.getSemaphoreNumber();
    if ( marker.hasBeenEnclaveCell() and taskNumber>=0 ) {
      double maxEigenvalue;
      ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndCopyResultOver( taskNumber, fineGridCell{{UNKNOWN_IDENTIFIER}}.value, maxEigenvalue );

      ::exahype2::fv::validatePatch(
        fineGridCell{{UNKNOWN_IDENTIFIER}}.value,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        0,
        std::string(__FILE__) + ": " + std::to_string(__LINE__) + "; marker=" + marker.toString()
      );
      
      {{COMPUTE_NEW_TIME_STEP_SIZE}}

      fineGridCell{{LABEL_NAME}}.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
      fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
    }
    
    if ( fineGridCell{{SOLVER_NAME}}CellLabel.getHasUpdated() ) {
      double*  targetPatch = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;

// noe, kommt spaeter automatisch      
      {{POSTPROCESS_UPDATED_CELL}}

      repositories::{{SOLVER_INSTANCE}}.update(
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize(),
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
        marker.h()(0)
      );
    }
    else {
      repositories::{{SOLVER_INSTANCE}}.update(
        0.0,
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
        marker.h()(0)
      );
    }
  }
"""
  def __init__(self,solver):
    AbstractFVActionSet.__init__(self,solver)
    self.label_name = exahype2.grid.UpdateCellLabel.get_attribute_name(solver._name)
    self.guard      = "true"


  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "LABEL_NAME" ] = self.label_name      
      d[ "GUARD" ]      = self.guard
      result = jinja2.Template(self.Template).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


class SeparateSweepsWithEnclaveTasking( SeparateSweeps ):
  """
  
    Two separate sweeps per Runge-Kutta sweep where volumetric operations
    are outsourced into dedicated tasks. 

    

  """


  def __init__(self, 
               name, 
               rk_order, 
               polynomial_basis, 
               unknowns, 
               auxiliary_variables, 
               min_cell_h, 
               max_cell_h, 
               plot_grid_properties,
               pde_terms_without_state):
    """

    See superclass constructor for all the interesting info.
    
    It is important to notice that we still use the separate sweep template classes
    here.

    """
    super(SeparateSweepsWithEnclaveTasking, self).__init__(name, 
                                                           rk_order, 
                                                           polynomial_basis, 
                                                           unknowns, 
                                                           auxiliary_variables, 
                                                           min_cell_h, 
                                                           max_cell_h, 
                                                           plot_grid_properties)
    
    self._solver_template_file_class_name     = "SeparateSweeps"
    self._pde_terms_without_state             = pde_terms_without_state

    self._fused_compute_kernel_call_cpu       = "#error Not yet defined. Set self._fused_Riemann_solver_call in your Python solver class."
    self._fused_compute_kernel_call_gpu       = "#error Not yet defined. Set self._fused_Riemann_solver_call in your Python solver class."
   
    self.create_action_sets()
    self.create_data_structures()
    


  def create_data_structures(self):
    """
    
     First, call the superclass' create_data_structures() to ensure that all 
     the data structures are in place.

     The linear combination is to be computed if and only if      
    
    """
    super(SeparateSweepsWithEnclaveTasking, self).create_data_structures()
    
            
  def create_action_sets(self):
    """

      Call superclass routine and then reconfigure the update cell call.
      Only the UpdateCell action set is specific to a single sweep.
      
      This operation is implicity called via the superconstructor.
      
      :: Guards around linear combination of estimates
      
      We rely on the action set LinearCombination as it is initialised in the 
      superclass. So the only thing we have to do is to ensure that the 
      linear combination over the estimates is computed if and only if this 
      is required. For the separate sweeps, we compute the linear combinations
      if the cell holds data, i.e. is on the finest level, and if it is the 
      primary of the two sweeps which realise a RK step, i.e. the one where
      we later on project onto the faces.  
      
    """
    super(SeparateSweepsWithEnclaveTasking, self).create_action_sets()

    
  def add_implementation_files_to_project(self,namespace,output):
    """
      Add the enclave task for the GPU
    """

    super(SeparateSweepsWithEnclaveTasking, self).add_implementation_files_to_project(namespace, output)
    templatefile_prefix = os.path.join(os.path.dirname(os.path.realpath(__file__)), "SolveVolumeIntegral.EnclaveTask.template")

    implementationDictionary = {}
    self._init_dictionary_with_default_parameters(implementationDictionary)
    self.add_entries_to_text_replacement_dictionary(implementationDictionary)
    
    # Some includes might logically belong into the action sets, but now they are 
    # 'outsourced' into the enclave task. So we manually add it here.
    implementationDictionary[ "SOLVER_INCLUDES" ] += self.user_solver_includes

    task_name = self._enclave_task_name()
    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      "{}.h".format(templatefile_prefix),
      "{}.cpp".format(templatefile_prefix),
      task_name,
      namespace + ["tasks"],
      "tasks",
      implementationDictionary,
      True)

    output.add( generated_solver_files )
    output.makefile.add_cpp_file( "tasks/" + task_name + ".cpp" )

    if self._pde_terms_without_state:
        # We need to explicitly link objects in gpu mode, since PEANO_SOURCE_DIR is gone,
        # this hack has to suffice
        peanodir = templatefile_prefix.split("python/exahype2",1)[0]
        output.makefile.d["PEANODIR"] = peanodir
        exahype2.gpu.add_exahype_objects(output.makefile)


  def _enclave_task_name(self): 
    return "{}EnclaveTask".format(self._name)

