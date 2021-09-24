# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV       import FV
from .PDETerms import PDETerms
 
import peano4
import exahype2

import jinja2

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor



class UpdateCell(ReconstructPatchAndApplyFunctor):
  SolveRiemannProblemsOverPatch = jinja2.Template( """
    double cellTimeStepSize = -1.0;
    double cellTimeStamp    = -1.0;
     
    {{PREPROCESS_RECONSTRUCTED_PATCH}}
    
    assertion2( tarch::la::greaterEquals( cellTimeStepSize, 0.0 ), cellTimeStepSize, cellTimeStamp );
    assertion2( tarch::la::greaterEquals( cellTimeStamp, 0.0 ), cellTimeStepSize, cellTimeStamp );

    const double usedTimeStepSize = cellTimeStepSize;

    ::exahype2::fv::validatePatch(
        reconstructedPatch,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        1, // halo
        std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
    ); // previous time step has to be valid
  
    ::exahype2::fv::copyPatch(
        reconstructedPatch,
        targetPatch,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        1 // halo size
    );

    {% if USE_SPLIT_LOOP %}
    #if Dimensions==2
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
    #else
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
    #endif
    {% else %}
    #if Dimensions==2
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(
    #else
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(
    #endif
    {% endif %}
        [&](
          const double * __restrict__                  QL,
          const double * __restrict__                  QR,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       dx,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double                                       FL[],
          double                                       FR[]
        ) -> void {
          {{RIEMANN_SOLVER_CALL}}
        },
        [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       dx,
          double                                       t,
          double                                       dt,
          double * __restrict__                        S
        ) -> void {
          {{SOURCE_TERM_CALL}}
        },
        marker.x(),
        marker.h(),
        cellTimeStamp,
        cellTimeStepSize,
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        reconstructedPatch,
        targetPatch
    );
  
    {{POSTPROCESS_UPDATED_PATCH}}
    
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(cellTimeStamp + usedTimeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(cellTimeStepSize);
    
    repositories::{{SOLVER_INSTANCE}}.update(cellTimeStepSize, cellTimeStamp + usedTimeStepSize, marker.h()(0) );

    ::exahype2::fv::validatePatch(
        targetPatch,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        0, // halo
        std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
    ); // outcome has to be valid
  """ )


  def __init__(self, solver):
    """
    
    
    """
    ReconstructPatchAndApplyFunctor.__init__(self,
      patch = solver._patch,
      # todo hier muessen beide rein, denn ich muss ja interpolieren
      patch_overlap = solver._patch_overlap_new,
      functor_implementation = """
#error please switch to your Riemann solver of choice
""",
      reconstructed_array_memory_location = solver._reconstructed_array_memory_location,
      guard = "not marker.isRefined()",
      add_assertions_to_halo_exchange = False
    )
    self._solver = solver


  def _add_action_set_entries_to_dictionary(self,d):
    """
    
    This is our plug-in point to alter the underlying dictionary
    
    """
    super(UpdateCell,self)._add_action_set_entries_to_dictionary(d)
    
    self._solver._init_dictionary_with_default_parameters(d)
    self._solver.add_entries_to_text_replacement_dictionary(d)
    
    d[ "CELL_FUNCTOR_IMPLEMENTATION" ] = self.SolveRiemannProblemsOverPatch.render(**d)
    d[ "USE_SPLIT_LOOP" ]              = self._solver._use_split_loop

  
  def get_includes(self):
    return """
#include "exahype2/fv/BoundaryConditions.h"
#include "exahype2/NonCriticalAssertions.h"
""" + self._solver._get_default_includes() + self._solver.get_user_action_set_includes()





class SingleSweep( FV ):
  """
    Probably the simplest solver you could think off. 

    :: Write your own specialisation ::
    
    self._preprocess_reconstructed_patch 
      Has to hold any preprocessing, but it also has to set the doubles
      cellTimeStepSize and cellTimeStamp to valid data.
      
    self._postprocess_updated_patch 
      You don't have to redefine this one, but if you want to alter the
      time step size, then this is the right place to do so. If you don't
      alter cellTimeStepSize, the code will automatically continue with
      the current one subject to a preprocessing in the next step.

  """


  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_volume_h, max_volume_h, plot_grid_properties):
    """

      Instantiate a generic FV scheme with an overlap of 1.

    """
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation

    self._reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack
    self._use_split_loop                      = False
    
    self._preprocess_reconstructed_patch      = ""
    self._postprocess_updated_patch           = ""
    
    self._source_term_call          = "#error Not yet defined"
    self._Riemann_solver_call       = "#error Not yet defined"
    self._abstract_solver_user_declarations        = ""
    self._abstract_solver_user_definitions         = ""
    self._solver_user_declarations                 = ""
    self._solver_user_definitions                  = ""

    self._start_time_step_implementation           = ""
    self._finish_time_step_implementation          = ""
    
    self._constructor_implementation = ""
    
    super(SingleSweep, self).__init__(name, patch_size, 1, unknowns, auxiliary_variables, min_volume_h, max_volume_h, plot_grid_properties)

    self._solver_template_file_class_name     = "SingleSweep"


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
    super(SingleSweep, self).create_data_structures()

    initialisation_sweep_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" + \
      ")"
    first_iteration_after_initialisation_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::TimeStepAfterGridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingAfterGridInitialisation" + \
    ")"

    self._patch_overlap_new.generator.send_condition               = "true"
    self._patch_overlap_new.generator.receive_and_merge_condition  = "true"

    self._patch_overlap_old.generator.send_condition               = initialisation_sweep_guard
    self._patch_overlap_old.generator.receive_and_merge_condition  = first_iteration_after_initialisation_guard

            
  def create_action_sets(self):
    """

      Call superclass routine and then reconfigure the update cell call.
      Only the UpdateCell action set is specific to a single sweep.
      
      This operation is implicity called via the superconstructor.
      
    """
    super(SingleSweep, self).create_action_sets()
    self._action_set_update_cell = UpdateCell(self)


  def get_user_action_set_includes(self):
    return super(SingleSweep, self).get_user_action_set_includes() + """
#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
"""


  def set_implementation(self,
    boundary_conditions, refinement_criterion, initial_conditions,
    memory_location,
    use_split_loop,
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
    if memory_location      is not None:  self._reconstructed_array_memory_location       = memory_location
    if use_split_loop                  :  self._use_split_loop                            = use_split_loop

    if self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete or \
       self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapWithoutDelete or \
       self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
      raise Exception( "memory mode without appropriate delete chosen, i.e. this will lead to a memory leak" )

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
    
    d[ "SOURCE_TERM_CALL"]                    = jinja2.Template(self._source_term_call, undefined=jinja2.DebugUndefined).render( **d )
    d[ "RIEMANN_SOLVER_CALL"]                 = jinja2.Template(self._Riemann_solver_call, undefined=jinja2.DebugUndefined).render( **d )
    d[ "PREPROCESS_RECONSTRUCTED_PATCH" ]     = jinja2.Template(self._preprocess_reconstructed_patch, undefined=jinja2.DebugUndefined).render( **d )
    d[ "POSTPROCESS_UPDATED_PATCH" ]          = jinja2.Template(self._postprocess_updated_patch, undefined=jinja2.DebugUndefined).render( **d )
    d[ "ABSTRACT_SOLVER_USER_DECLARATIONS" ]  = jinja2.Template(self._abstract_solver_user_declarations, undefined=jinja2.DebugUndefined).render( **d )
    d[ "ABSTRACT_SOLVER_USER_DEFINITIONS" ]   = jinja2.Template(self._abstract_solver_user_definitions, undefined=jinja2.DebugUndefined).render( **d )
    d[ "SOLVER_USER_DECLARATIONS" ]           = jinja2.Template(self._solver_user_declarations, undefined=jinja2.DebugUndefined).render( **d )
    d[ "SOLVER_USER_DEFINITIONS" ]            = jinja2.Template(self._solver_user_definitions, undefined=jinja2.DebugUndefined).render( **d )
    d[ "START_TIME_STEP_IMPLEMENTATION" ]     = jinja2.Template(self._start_time_step_implementation, undefined=jinja2.DebugUndefined).render( **d )
    d[ "FINISH_TIME_STEP_IMPLEMENTATION" ]    = jinja2.Template(self._finish_time_step_implementation, undefined=jinja2.DebugUndefined).render( **d )
    d[ "CONSTRUCTOR_IMPLEMENTATION"]          = jinja2.Template(self._constructor_implementation, undefined=jinja2.DebugUndefined).render( **d )


  def set_preprocess_reconstructed_patch_kernel(self,kernel):
    """
  
    Most subclasses will redefine/overwrite this operation as they have
    to incorporate the kernel into their generated stuff
  
    """
    self._preprocess_reconstructed_patch += kernel
    self.create_data_structures()
    self.create_action_sets()


  def set_postprocess_updated_patch_kernel(self, kernel):
    """

    Define a postprocessing routine over the data

    The postprocessing kernel often looks similar to the following code:

  {
    int index = 0;
    dfor( volume, {{NUMBER_OF_VOLUMES_PER_AXIS}} ) {
      enforceCCZ4constraints( targetPatch+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }


    Within this kernel, you have at least the following variables available:

    - targetPatch This is a pointer to the whole data structure (one large
        array).
        The patch is not supplemented by a halo layer.
    - reconstructedPatch This is a pointer to the data snapshot before the
        actual update. This data is combined with the halo layer, i.e. if you
        work with 7x7 patches and a halo of 2, the pointer points to a 11x11
        patch.
    - marker

    Furthermore, you can use all the symbols (via Jinja2 syntax) from
    _init_dictionary_with_default_parameters().

    kernel: String
      C++ code that holds the postprocessing kernel

    """
    self._postprocess_updated_patch += kernel
    self.create_data_structures()
    self.create_action_sets()
