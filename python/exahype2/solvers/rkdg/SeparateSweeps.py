# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .RungeKuttaDG  import RungeKuttaDG
from .PDETerms      import PDETerms
 
import peano4
import exahype2

import jinja2

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor


class SeparateSweeps( RungeKuttaDG ):
  """
    Probably the simplest solver you could think off. 

    We run over the grid twice per Runge Kutta step. In the first step,
    we compute the linear reconstruction, project this one onto the faces
    and also compute the volumetric prediction. In the second step, we 
    solve the Riemann problem and add it to the prediction.

  """


  def __init__(self, name, dg_order, rk_order, polynomials, unknowns, auxiliary_variables, min_cell_h, max_cell_h, plot_grid_properties):
    """


    """
    super(SeparateSweeps, self).__init__(name, 
                                         dg_order, 
                                         rk_order, 
                                         polynomials, 
                                         1,     # number of face projections
                                         unknowns, auxiliary_variables, min_cell_h, max_cell_h, plot_grid_properties)
    
    self._solver_template_file_class_name     = "SeparateSweeps"
    
    self.create_action_sets()
    self.create_data_structures()
    


  def create_data_structures(self):
    """
    
     First, call the superclass' create_data_structures() to ensure that all 
     the data structures are in place.

     The linear combination is to be computed if and only if      
    
    """
    super(SeparateSweeps, self).create_data_structures()

    self._initialisation_sweep_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" + \
      ")"
    self._first_iteration_after_initialisation_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::TimeStepAfterGridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingAfterGridInitialisation" + \
    ")"
    self._first_grid_sweep_within_RK_step = "( false "
    for step in range(0,self.number_of_Runge_Kutta_steps()):
      self._first_grid_sweep_within_RK_step += "or ("
      self._first_grid_sweep_within_RK_step += "repositories::{}.getSolverState()=={}::SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep{}".format(self.get_name_of_global_instance(),self._name,step)
      self._first_grid_sweep_within_RK_step += ")"
    self._first_grid_sweep_within_RK_step += ")"
    self._second_grid_sweep_within_RK_step = "( false "
    for step in range(0,self.number_of_Runge_Kutta_steps()):
      self._second_grid_sweep_within_RK_step += "or ("
      self._second_grid_sweep_within_RK_step += "repositories::{}.getSolverState()=={}::SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep{}".format(self.get_name_of_global_instance(),self._name,step)
      self._second_grid_sweep_within_RK_step += ")"
    self._second_grid_sweep_within_RK_step += ")" 

    self._estimates_projection.generator.send_condition               = "true"
    self._estimates_projection.generator.receive_and_merge_condition  = "true"


    #self._patch_overlap_new.generator.send_condition               = "true"
    #self._patch_overlap_new.generator.receive_and_merge_condition  = "true"

    #self._patch_overlap_old.generator.send_condition               = initialisation_sweep_guard
    #self._patch_overlap_old.generator.receive_and_merge_condition  = first_iteration_after_initialisation_guard

            
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
    super(SeparateSweeps, self).create_action_sets()
    self._action_set_linear_combination_of_estimates.guards = [
      "{} and repositories::{}.getSolverState()=={}::SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep{}".format(self._store_cell_data_default_guard(),self.get_name_of_global_instance(),self._name,step) for step in range(0,self.number_of_Runge_Kutta_steps())
    ]
#    self._action_set_project_linear_combination_onto_faces.guard = self._store_cell_data_default_guard() + " and " + self._first_grid_sweep_within_RK_step

    self._action_set_project_linear_combination_onto_faces.guards = [
      "{} and repositories::{}.getSolverState()=={}::SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep{}".format(self._store_cell_data_default_guard(),self.get_name_of_global_instance(),self._name,step) for step in range(0,self.number_of_Runge_Kutta_steps())
    ]

    self._action_set_solve_volume_integral.guards = [
      "{} and repositories::{}.getSolverState()=={}::SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep{}".format(self._store_cell_data_default_guard(),self.get_name_of_global_instance(),self._name,step) for step in range(0,self.number_of_Runge_Kutta_steps())
    ]
    
    self._action_set_add_volume_and_face_solution.guards = [
      "{} and repositories::{}.getSolverState()=={}::SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep{}".format(self._store_cell_data_default_guard(),self.get_name_of_global_instance(),self._name,step) for step in range(0,self.number_of_Runge_Kutta_steps())
    ]
    
#    self._action_set_compute_final_linear_combination_and_project_solution_onto_faces.guard = self._store_cell_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep{}".format(self.get_name_of_global_instance(),self._name,self.number_of_Runge_Kutta_steps()-1)
    self._action_set_compute_final_linear_combination_and_project_solution_onto_faces.guard = self._store_cell_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep{}".format(self.get_name_of_global_instance(),self._name,self.number_of_Runge_Kutta_steps()-1)

    self._action_set_solve_Riemann_problem.guards = [
      "{} and repositories::{}.getSolverState()=={}::SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep{}".format(self._store_cell_data_default_guard(),self.get_name_of_global_instance(),self._name,step) for step in range(0,self.number_of_Runge_Kutta_steps())
    ]


  def get_user_action_set_includes(self):
    return super(SeparateSweeps, self).get_user_action_set_includes() + """
"""


  ## @todo raus
  def set_preprocess_reconstructed_patch_kernel(self,kernel):
    """
  
    Most subclasses will redefine/overwrite this operation as they have
    to incorporate the kernel into their generated stuff
  
    """
    self._preprocess_reconstructed_patch += kernel
    self.create_data_structures()
    self.create_action_sets()


  ## @todo raus
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
