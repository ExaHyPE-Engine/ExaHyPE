# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .SeparateSweeps import SeparateSweeps
from .PDETerms import PDETerms
from exahype2.solvers.rkfd.actionsets.AbstractRKFDActionSet import AbstractRKFDActionSet
 
import peano4
import exahype2
import jinja2


import os

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor

from exahype2.solvers.ButcherTableau                import ButcherTableau


class MergeEnclaveTaskOutcome(AbstractRKFDActionSet):
  Template = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ( 
    not marker.hasBeenRefined()
    and 
    marker.hasBeenEnclaveCell()
    and
    {{PREDICATES[PREDICATE_NO]}}
  ) {
    #if Dimensions==2
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_2D}} * {{NUMBER_OF_UNKNOWNS}};
    #elif Dimensions==3
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_3D}} * {{NUMBER_OF_UNKNOWNS}};
    #endif
    
    const int taskNumber = fineGridCell{{LABEL_NAME}}.getSemaphoreNumber();
    if ( taskNumber>=0 ) {
      double maxEigenvalue; // not used here
      ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndCopyResultOver( taskNumber, QOut, maxEigenvalue );
    }
  }
  {% endfor %}
"""
  def __init__(self,solver):
    super( MergeEnclaveTaskOutcome, self ).__init__(solver)
    self.label_name = exahype2.grid.UpdateCellLabel.get_attribute_name(solver._name)


  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "LABEL_NAME" ] = self.label_name      
      d[ "PREDICATES" ] = self._solver._secondary_sweeps_of_Runge_Kutta_step_on_cell
      result = jinja2.Template(self.Template).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def get_includes(self):
    return super( MergeEnclaveTaskOutcome, self ).get_includes() + """
#include "exahype2/EnclaveBookkeeping.h"
"""    



class SeparateSweepsWithEnclaveTasking( SeparateSweeps ):
  """
    Probably the simplest solver you could think off. 
    
    This particular variant works only for Runge-Kutta order greater or equal
    to two.
    

    :: Write your own specialisation ::
    
    self._preprocess_reconstructed_patch 
      Has to hold any preprocessing, but it also has to set the doubles
      timeStepSize and timeStamp to valid data.
      
    self._postprocess_updated_patch 
      You don't have to redefine this one, but if you want to alter the
      time step size, then this is the right place to do so. If you don't
      alter timeStepSize, the code will automatically continue with
      the current one subject to a preprocessing in the next step.

  """


  def __init__(self, 
               name, 
               patch_size, 
               overlap, 
               rk_order, 
               unknowns, 
               auxiliary_variables, 
               min_meshcell_h, 
               max_meshcell_h, 
               plot_grid_properties, 
               kernel_namespace,
               pde_terms_without_state
               ):
    """

      Instantiate a generic FV scheme with an overlap of 1.

    """
    super(SeparateSweepsWithEnclaveTasking, self).__init__(name, 
                                                           patch_size, 
                                                           overlap, 
                                                           rk_order, 
                                                           unknowns, 
                                                           auxiliary_variables, 
                                                           min_meshcell_h, 
                                                           max_meshcell_h, 
                                                           plot_grid_properties, 
                                                           kernel_namespace
                                                           )
    
    self._solver_template_file_class_name     = "SeparateSweeps"
    self._pde_terms_without_state             = pde_terms_without_state

    self._fused_volumetric_kernel_call_cpu       = "#error Not yet defined. Set self._fused_volumetric_kernel_call_cpu in your Python solver class."
    self._fused_volumetric_kernel_call_gpu       = "#error Not yet defined. Set self._fused_volumetric_kernel_call_gpu in your Python solver class."

    self.create_action_sets()
    self.create_data_structures()



  def create_data_structures(self):
    """
    
     Call the superclass' create_data_structures() to ensure that all the data
     structures are in place, i.e. each cell can host a patch, that each face hosts 
     patch overlaps, and so forth. These quantities are all set to defaults. See
     FV.create_data_structures().
     
     After that, take the patch overlap (that's the data stored within the faces)
     and ensure that these are sent and received via MPI whenever they are also 
     stored persistently. The default in FV is that no domain boundary data exchange
     is active. Finally, ensure that the old data is only exchanged between the 
     initialisation sweep and the first first grid run-through.
    
    """
    super(SeparateSweepsWithEnclaveTasking, self).create_data_structures()

            
  def create_action_sets(self):
    """

      Call superclass routine and then reconfigure the update cell call.
      Only the UpdateCell action set is specific to a single sweep.
      
      This operation is implicity called via the superconstructor.
      
    """
    super(SeparateSweepsWithEnclaveTasking, self).create_action_sets()

    self._action_set_merge_enclave_task_outcome = MergeEnclaveTaskOutcome(self)                                                                                 


  def add_implementation_files_to_project(self,namespace,output):
    """
      Add the enclave task for the GPU
    """
    super(SeparateSweepsWithEnclaveTasking, self).add_implementation_files_to_project(namespace, output)
    templatefile_prefix = os.path.join(os.path.dirname(os.path.realpath(__file__)), "SeparateSweeps.EnclaveTask.template")

    implementationDictionary = {}
    self._init_dictionary_with_default_parameters(implementationDictionary)
    self.add_entries_to_text_replacement_dictionary(implementationDictionary)
    
    # Some includes might logically belong into the action sets, but now they are 
    # 'outsourced' into the enclave task. So we manually add it here.
    implementationDictionary[ "SOLVER_INCLUDES" ] += self.user_solver_includes
    implementationDictionary[ "SOLVER_INCLUDES" ] += self.user_action_set_includes

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


  def add_actions_to_perform_time_step(self, step):
    """
    
      Add enclave aspect to time stepping. If you study the superclass'
      routine add_actions_to_perform_time_step() and consider that this action
      set is invoked in the secondary grid sweep, then it becomes clear that
      this merger has to come first, i.e. we first add the action set and then
      we call the superclass' add_action_set().
      
      We need the result of the volumetric operation before we sum up this
      volumetric solution and the Riemann solution. 
       
    """
    super(SeparateSweepsWithEnclaveTasking, self).add_actions_to_perform_time_step(step)
    step.add_action_set( self._action_set_merge_enclave_task_outcome, False )


  def add_entries_to_text_replacement_dictionary(self,d):
    super(SeparateSweepsWithEnclaveTasking, self).add_entries_to_text_replacement_dictionary(d)
    d[ "FUSED_VOLUMETRIC_KERNEL_CALL_CPU"] = jinja2.Template(self._fused_volumetric_kernel_call_cpu, undefined=jinja2.DebugUndefined).render( **d )
    d[ "FUSED_VOLUMETRIC_KERNEL_CALL_GPU"] = jinja2.Template(self._fused_volumetric_kernel_call_gpu, undefined=jinja2.DebugUndefined).render( **d )

    d[ "SEMAPHORE_LABEL" ]      = exahype2.grid.UpdateCellLabel.get_attribute_name(self._name)
    d[ "STATELESS_PDE_TERMS" ]  = self._pde_terms_without_state
 
 
  @property
  def user_action_set_includes(self):
    return """
#include "exahype2/CellData.h"
""" + super(SeparateSweeps, self).user_action_set_includes


  def add_entries_to_text_replacement_dictionary(self,d):
    """
     d: Dictionary of string to string
        in/out argument
    """
    pass


  def _enclave_task_name(self): 
    return "{}EnclaveTask".format(self._name)
