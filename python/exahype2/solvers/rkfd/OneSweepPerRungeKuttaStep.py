# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .CellCenteredFiniteDifferences       import CellCenteredFiniteDifferences
from .PDETerms import PDETerms
 
import peano4
import exahype2

import jinja2

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor

from exahype2.solvers.ButcherTableau                import ButcherTableau


class UpdateCell(ReconstructPatchAndApplyFunctor):
  """
  
  Update one cell, i.e. compute Runge-Kutta step on it
  
  This routine is very similar to the Finite Volume step. The big difference in 
  the context of Runge-Kutta is that we always have to construct a linear 
  combination of the input data and then write the new estimate for the 
  right-hand side into the large estimate field.

  So we operate in two steps: First, we let the default block-structured code
  reconstruct the old time step data. After that, we add the rhs estimates 
  onto this reconstructed data. The latter can be read as a correction step
  to the reconstructed values. It is correct if and only if the haloes are
  properly initialised with a corrected value, as we cannot correct the halo
  data at this point. 
  
  
  # Correction terms with Runge-Kutta trials

  We current the values with a linear combination of all of the estimates according to the
  Butcher Tableau.
  
  The linear combination is a volumetric representation which includes both
  the degrees of freedom and the auxiliary variables. However, the auxiliary
  variables are not developing over time. In Runge-Kutta, I have estimates 
  for the right-hand side, i.e. for the derivatives
  
  @f$ \partial _t Q = F(Q) @f$
  
  This is the stuff stored in RhsEstimates. It does not comprise any auxiliary
  variables. So I have to copy the auxiliary variables from the last valid
  time step every time I reconstruct a new guess.


  
  """  
    
    
    

  SolveRiemannProblemsOverPatch = jinja2.Template( """
      double timeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

      // Set the variable
      // double timeStepSize
      {{COMPUTE_TIME_STEP_SIZE}}

      {% for PREDICATE_NO in range(0,PREDICATES|length) %}
      if ({{PREDICATES[PREDICATE_NO]}}) {
        ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariablesOnReconstructedPatch( 1, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}, {{HALO_SIZE}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}});
        ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithoutAuxiliaryVariables( {{RK_STEPS}}, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}, 0, {{NUMBER_OF_UNKNOWNS}}, 0 );
    
        dfor( dof, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} ) {
          for (int unknown=0; unknown<{{NUMBER_OF_UNKNOWNS}}; unknown++) {
            {% for WEIGHT_NO in range(0,BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO]|length) %}
            {% if BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO][WEIGHT_NO]!=0 %}
            reconstructedPatch[ enumeratorWithAuxiliaryVariablesOnReconstructedPatch(0,dof,unknown) ] += 
              {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize * {{BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO][WEIGHT_NO]}} * 
              fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value[ enumeratorWithoutAuxiliaryVariables({{PREDICATE_NO-1}},dof,unknown) ];
            {% endif %}
            {% endfor %}
          }      
        }
        
        targetPatch = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value + enumeratorWithoutAuxiliaryVariables({{PREDICATE_NO}},0,0);
      }
      {% endfor %} 
   
      {{PREPROCESS_RECONSTRUCTED_PATCH}}

      assertion2( tarch::la::greaterEquals( timeStamp, 0.0 ),    timeStamp, timeStepSize );
      assertion2( tarch::la::greaterEquals( timeStepSize, 0.0 ), timeStamp, timeStepSize );

      ::exahype2::fv::validatePatch(
        reconstructedPatch,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
        1, // halo
        std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
      ); // previous time step has to be valid
  
      ::exahype2::CellData  patchData( reconstructedPatch, marker.x(), marker.h(), timeStamp, timeStepSize, targetPatch );
           
      ::exahype2::fd::{{KERNEL_NAMESPACE}}::{{COMPUTE_KERNEL_CALL}} 

      {{POSTPROCESS_UPDATED_PATCH}}

      ::exahype2::fv::validatePatch(
        targetPatch,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
        0, // halo
        std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
      ); // outcome has to be valid
      
      {% if COMPUTE_MAX_EIGENVALUE %}
      const double maxEigenvalue = patchData.maxEigenvalue[0];
      {% endif %}
      
      // Set the variable
      // double newTimeStepSize
      {{COMPUTE_NEW_TIME_STEP_SIZE}}
    
      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp+timeStepSize);
      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
      fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
    
      repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, timeStamp+timeStepSize, marker.h()(0) );
  """ )


  def __init__(self, solver):
    """
    
    
    """
    ReconstructPatchAndApplyFunctor.__init__(self,
      patch = solver._patch,
      patch_overlap = solver._patch_overlap_new,
      functor_implementation = """
#error please switch to your Riemann solver of choice
""",
      reconstructed_array_memory_location = solver._reconstructed_array_memory_location,
      guard = "not marker.willBeRefined() and not marker.hasBeenRefined()",
      add_assertions_to_halo_exchange = False
    )
    self._solver = solver
    
    self._Template_TouchCellFirstTime_Epilogue += """
  else if (not marker.willBeRefined() and not marker.hasBeenRefined()) {{
    const double timeStamp    = fineGridCell{SOLVER_NAME}CellLabel.getTimeStamp();
    const double timeStepSize = fineGridCell{SOLVER_NAME}CellLabel.getTimeStepSize();
    
    assertion2( tarch::la::greaterEquals( timeStepSize, 0.0 ), timeStepSize, timeStamp );
    assertion2( tarch::la::greaterEquals( timeStamp, 0.0 ),    timeStepSize, timeStamp );

    repositories::{SOLVER_INSTANCE}.update(0.0, timeStamp, marker.h()(0) );
  }}
"""

    self._Template_TouchCellFirstTime_Preamble = """
  fineGridCell""" + solver._name + """CellLabel.setHasUpdated(false);
""" + self._Template_TouchCellFirstTime_Preamble

    self._butcher_tableau     = ButcherTableau(self._solver._rk_order)


  def _add_action_set_entries_to_dictionary(self,d):
    """
    
    This is our plug-in point to alter the underlying dictionary
    
    """
    super(UpdateCell,self)._add_action_set_entries_to_dictionary(d)
    
    self._solver._init_dictionary_with_default_parameters(d)
    self._solver.add_entries_to_text_replacement_dictionary(d)
    
    d[ "PREDICATES" ]                  = [
      "{} and repositories::{}.getSolverState()=={}::SolverState::RungeKuttaSubStep{}".format(self._solver._store_cell_data_default_guard(),self._solver.get_name_of_global_instance(),self._solver._name,step) for step in range(0,self._solver.number_of_Runge_Kutta_steps())
    ]
    d[ "BUTCHER_TABLEAU_WEIGHTS" ]                  = self._butcher_tableau.weight_matrix()
    d[ "BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES" ] = self._butcher_tableau.time_step_sizes()

    # Has to come after we've set the predicates, as we use these
    # fields in here already
    d[ "CELL_FUNCTOR_IMPLEMENTATION" ] = self.SolveRiemannProblemsOverPatch.render(**d)

  
  def get_includes(self):
    return """
#include "exahype2/fd/BoundaryConditions.h"
#include "exahype2/NonCriticalAssertions.h"
""" + self._solver._get_default_includes() + self._solver.user_action_set_includes





class OneSweepPerRungeKuttaStep( CellCenteredFiniteDifferences ):
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


  def __init__(self, name, patch_size, overlap, rk_order, unknowns, auxiliary_variables, min_volume_h, max_volume_h, plot_grid_properties, kernel_namespace):
    """

      Instantiate a generic FV scheme with an overlap of 1.

    """
    super(OneSweepPerRungeKuttaStep, self).__init__(name, patch_size, overlap, rk_order, unknowns, auxiliary_variables, min_volume_h, max_volume_h, plot_grid_properties, kernel_namespace)
    
    if rk_order<1:
      raise Exception( "Runge-Kutta order has to be at least 1 but was {}".format(rk_order) )

    self._solver_template_file_class_name     = "OneSweepPerRungeKuttaStep"

    self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.None_Implementation
    self._source_term_implementation          = PDETerms.None_Implementation



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
    super(OneSweepPerRungeKuttaStep, self).create_data_structures()

    initialisation_sweep_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" + \
      ")"
    first_iteration_after_initialisation_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::RungeKuttaSubStep0AfterGridInitialisation or " + \
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
    super(OneSweepPerRungeKuttaStep, self).create_action_sets()
    self._action_set_update_cell = UpdateCell(self)
    self._action_set_compute_final_linear_combination.guard = self._store_cell_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::RungeKuttaSubStep{}".format(self.get_name_of_global_instance(),self._name,self.number_of_Runge_Kutta_steps()-1)
    self._action_set_project_patch_onto_faces.guards        = [
      "{} and repositories::{}.getSolverState()=={}::SolverState::RungeKuttaSubStep{}".format(self._store_cell_data_default_guard(),self.get_name_of_global_instance(),self._name,step) for step in range(0,self.number_of_Runge_Kutta_steps())
    ] + [ "{} and repositories::{}.getSolverState()=={}::SolverState::GridInitialisation".format(self._store_cell_data_default_guard(),self.get_name_of_global_instance(),self._name) ]


  @property
  def user_action_set_includes(self):
    return """
#include "exahype2/CellData.h"
""" + super(OneSweepPerRungeKuttaStep, self).user_action_set_includes


  def set_implementation(self,
    flux, ncp, source_term, eigenvalues,
    boundary_conditions, refinement_criterion, initial_conditions,
    memory_location,
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

    if flux                 is not None:  self._flux_implementation                       = flux
    if ncp                  is not None:  self._ncp_implementation                        = ncp
    if eigenvalues          is not None:  self._eigenvalues_implementation                = eigenvalues
    if source_term          is not None:  self._source_term_implementation                = source_term
    
    if self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete or \
       self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapWithoutDelete or \
       self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
      raise Exception( "memory mode without appropriate delete chosen, i.e. this will lead to a memory leak" )

    self._user_action_set_includes += additional_action_set_includes
    self._user_solver_includes     += additional_user_includes

    self.create_action_sets()


  def add_entries_to_text_replacement_dictionary(self,d):
    """
     d: Dictionary of string to string
        in/out argument
    """
    pass




  #def set_preprocess_reconstructed_patch_kernel(self,kernel):
    """
  
    Most subclasses will redefine/overwrite this operation as they have
    to incorporate the kernel into their generated stuff
  
    """
  #  self._preprocess_reconstructed_patch += kernel
  #  self.create_data_structures()
  #  self.create_action_sets()


  #def set_postprocess_updated_patch_kernel(self, kernel):
    """

    Define a postprocessing routine over the data

    The postprocessing kernel often looks similar to the following code:

  {
    int index = 0;
    dfor( volume, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} ) {
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
    #self._postprocess_updated_patch += kernel
    #self.create_data_structures()
    #self.create_action_sets()
