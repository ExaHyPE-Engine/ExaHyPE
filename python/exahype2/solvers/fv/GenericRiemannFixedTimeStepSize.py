# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV                       import FV
 
from .PDETerms import PDETerms

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructedArrayMemoryLocation

import peano4
import exahype2

import jinja2


class GenericRiemannFixedTimeStepSize( FV ):
  """
  
    Probably the simplest solver you could think off. There's a few
    interesting things to try out with this one nevertheless: You 
    can inject symbolic flux/pde term implementations, or you can 
    alter the implementation variant of the cell update kernel. The
    latter can be combined with different dynamic memory allocation 
    schemes and thus offers quite some opportunities to tweak and 
    tune things. 
  
  """

  TemplateUpdateCell = """
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
        {{SOLVER_INSTANCE}}.solveRiemannProblem(
          QL, QR,
          x, dx, t, dt, 
          normal,
          FL, FR
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
  """      


  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size, plot_grid_properties=False, kernel_implementation = FV.CellUpdateImplementation_NestedLoop):
    """

      Instantiate a generic FV scheme with an overlap of 1.

    """
    FV.__init__(self, name, patch_size, 1, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties)

    self._time_step_size = time_step_size

    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation
    self._kernel_implementation               = kernel_implementation

    self._rusanov_call = ""
    self._reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack

    self.set_implementation()
    self.set_update_cell_implementation(kernel_implementation=kernel_implementation)

    self._patch_overlap.generator.send_condition               = self._predicate_face_carrying_data() + " and observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction"
    self._patch_overlap.generator.receive_and_merge_condition  = self._predicate_face_carrying_data() + " and " \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction and " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridInitialisation"

    pass


  def set_implementation(self,boundary_conditions=None,refinement_criterion=None,initial_conditions=None):
    """
      If you pass in User_Defined, then the generator will create C++ stubs 
      that you have to befill manually. If you pass in None_Implementation, it 
      will create nop, i.e. no implementation or defaults. Any other string
      is copied 1:1 into the implementation. If you pass in None, then the 
      set value so far won't be overwritten.
      
      Please note that not all options are supported by all solvers. You 
      cannot set ncp and fluxes for the ClawPack Riemann solvers, e.g.
    """
    if boundary_conditions!=None:
      self._boundary_conditions_implementation        = boundary_conditions
    if refinement_criterion!=None:
      self._refinement_criterion_implementation       = refinement_criterion
    if initial_conditions!=None: 
      self._initial_conditions_implementation         = initial_conditions
    
    self._construct_template_update_cell()
    
  
  def set_update_cell_implementation(self,
    kernel_implementation   = FV.CellUpdateImplementation_NestedLoop,
    memory_location         = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack
  ):
    if memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete or \
       memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapWithoutDelete or \
       memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
      raise Exception( "memory mode without appropriate delete chosen, i.e. this will lead to a memory leak" )

    self._reconstructed_array_memory_location = memory_location
    self._kernel_implementation               = kernel_implementation
    self._construct_template_update_cell()
  
  
  def get_user_includes(self):
    return """
#include "exahype2/fv/Generic.h"
"""    
    
  
  def add_entries_to_text_replacement_dictionary(self,d):
    """
     
     d: Dictionary of string to string
        in/out argument
    
    """
    d[ "TIME_STEP_SIZE" ]               = self._time_step_size

    d[ "BOUNDARY_CONDITIONS_IMPLEMENTATION"]  = self._boundary_conditions_implementation
    d[ "REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
    d[ "INITIAL_CONDITIONS_IMPLEMENTATION"]   = self._initial_conditions_implementation

    pass


  def _construct_template_update_cell(self):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    d[ "LOOP_OVER_PATCH_FUNCTION_CALL" ] = self._kernel_implementation
    d[ "TIME_STAMP" ]                   = "{{SOLVER_INSTANCE}}.getMinTimeStamp()"
    d[ "RUSANOV_ON_FACE"]               = self._rusanov_call
      
    temp = jinja2.Template( self.TemplateUpdateCell ).render(d);
    self._template_update_cell      = jinja2.Template( temp ); 
  
  
  
