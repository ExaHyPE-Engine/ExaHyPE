# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .ADERDG  import ADERDG

from .PDETerms import PDETerms
 
import jinja2
from .actionsets  import UpdateCell

class SingleSweep( ADERDG ):
  def __init__(self, name, order, polynomials, unknowns, auxiliary_variables, min_cell_h, max_cell_h, plot_grid_properties=False):

    super(SingleSweep, self).__init__(name, order, polynomials, unknowns, auxiliary_variables, min_cell_h, max_cell_h, plot_grid_properties)
        
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation

    self._compute_time_step_size              = "#error Not yet defined"
    
    self._preprocess_reconstructed_patch      = ""
    self._postprocess_updated_patch           = ""
    
    #self._source_term_call          = "#error Not yet defined"
    #self._Riemann_solver_call       = "#error Not yet defined"
    self._abstract_solver_user_declarations        = ""
    self._abstract_solver_user_definitions         = ""
    self._solver_user_declarations                 = ""
    self._solver_user_definitions                  = ""

    self._start_time_step_implementation           = ""
    self._finish_time_step_implementation          = ""    
    self._constructor_implementation = ""

        
    self._solver_template_file_class_name     = "SingleSweep"
    
    self.create_action_sets()
    self.create_data_structures()
  
  
  
  def create_data_structures(self):
    #TODO: erweitern
    super(SingleSweep, self).create_data_structures()
    
    self.initialisation_sweep_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" + \
      ")"
    self.first_iteration_after_initialisation_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::TimeStepAfterGridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingAfterGridInitialisation" + \
    ")"
    

    
              
  def create_action_sets(self):
    #TODO: ask Tobias about the project_linear_combination, which I think is not needed in ader-dg
    super(SingleSweep, self).create_action_sets()
    #self._action_set_project_linear_combination_onto_faces.guard = self._store_cell_data_default_guard()
    self._action_set_update_cell = UpdateCell(self, self._store_cell_data_default_guard() )

  def get_user_action_set_includes(self):
    return super(SingleSweep, self).get_user_action_set_includes() + """
#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
"""



  def set_implementation(self,
    boundary_conditions, refinement_criterion, initial_conditions,
    additional_action_set_includes,
    additional_user_includes
  ):
    """
      If you pass in User_Defined, then the generator will create C++ stubs
      that you have to befill manually. If you pass in None_Implementation, it
      will create nop, i.e. no implementation or defaults. Any other string
      is copied 1:1 into the implementation. If you pass in None, then the
      set value so far won't be overwritten.
    """
    if boundary_conditions  is not None:  self._boundary_conditions_implementation        = boundary_conditions
    if refinement_criterion is not None:  self._refinement_criterion_implementation       = refinement_criterion
    if initial_conditions   is not None:  self._initial_conditions_implementation         = initial_conditions

    self.user_action_set_includes += additional_action_set_includes
    self.user_solver_includes     += additional_user_includes

    self.create_action_sets()


  def add_entries_to_text_replacement_dictionary(self,d):
    """
     d: Dictionary of string to string
        in/out argument
    """
    d[ "BOUNDARY_CONDITIONS_IMPLEMENTATION"]  = self._boundary_conditions_implementation
    d[ "REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
    d[ "INITIAL_CONDITIONS_IMPLEMENTATION"]   = self._initial_conditions_implementation
    
    d[ "COMPUTE_TIME_STEPSIZE" ]              = jinja2.Template(self._compute_time_step_size, undefined=jinja2.DebugUndefined).render( **d )

    #d[ "SOURCE_TERM_CALL"]                    = jinja2.Template(self._source_term_call, undefined=jinja2.DebugUndefined).render( **d )
    #d[ "RIEMANN_SOLVER_CALL"]                 = jinja2.Template(self._Riemann_solver_call, undefined=jinja2.DebugUndefined).render( **d )
    d[ "PREPROCESS_RECONSTRUCTED_PATCH" ]     = jinja2.Template(self._preprocess_reconstructed_patch, undefined=jinja2.DebugUndefined).render( **d )
    #d[ "POSTPROCESS_UPDATED_PATCH" ]          = jinja2.Template(self._postprocess_updated_patch, undefined=jinja2.DebugUndefined).render( **d )
    d[ "ABSTRACT_SOLVER_USER_DECLARATIONS" ]  = jinja2.Template(self._abstract_solver_user_declarations, undefined=jinja2.DebugUndefined).render( **d )
    d[ "ABSTRACT_SOLVER_USER_DEFINITIONS" ]   = jinja2.Template(self._abstract_solver_user_definitions, undefined=jinja2.DebugUndefined).render( **d )
    d[ "SOLVER_USER_DECLARATIONS" ]           = jinja2.Template(self._solver_user_declarations, undefined=jinja2.DebugUndefined).render( **d )
    d[ "SOLVER_USER_DEFINITIONS" ]            = jinja2.Template(self._solver_user_definitions, undefined=jinja2.DebugUndefined).render( **d )
    d[ "START_TIME_STEP_IMPLEMENTATION" ]     = jinja2.Template(self._start_time_step_implementation, undefined=jinja2.DebugUndefined).render( **d )
    d[ "FINISH_TIME_STEP_IMPLEMENTATION" ]    = jinja2.Template(self._finish_time_step_implementation, undefined=jinja2.DebugUndefined).render( **d )
    d[ "CONSTRUCTOR_IMPLEMENTATION"]          = jinja2.Template(self._constructor_implementation, undefined=jinja2.DebugUndefined).render( **d )
    
    d[ "COMPUTE_NEW_TIME_STEP_SIZE" ]         = jinja2.Template(self._compute_new_time_step_size, undefined=jinja2.DebugUndefined).render( **d )


  def set_preprocess_reconstructed_patch_kernel(self,kernel):
    """
  
    Most subclasses will redefine/overwrite this operation as they have
    to incorporate the kernel into their generated stuff
  
    """
    self._preprocess_reconstructed_patch += kernel
    self.create_data_structures()
    self.create_action_sets()


  def set_postprocess_updated_patch_kernel(self, kernel):
    self._postprocess_updated_patch += kernel
    self.create_data_structures()
    self.create_action_sets()




      