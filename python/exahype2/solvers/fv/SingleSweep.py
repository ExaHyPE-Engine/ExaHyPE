# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .FV import FV
from exahype2.solvers.PDETerms import PDETerms

import peano4
import exahype2

import jinja2

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor


class UpdateCell(ReconstructPatchAndApplyFunctor):
  SolveRiemannProblemsOverPatch = jinja2.Template("""
      double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

      // Set the variable
      // double timeStepSize
      {{COMPUTE_TIME_STEP_SIZE}}

      {{PREPROCESS_RECONSTRUCTED_PATCH}}

      assertion2(tarch::la::greaterEquals(timeStamp, 0.0),    timeStamp, timeStepSize);
      assertion2(tarch::la::greaterEquals(timeStepSize, 0.0), timeStamp, timeStepSize);

      ::exahype2::fv::validatePatch(
        oldQWithHalo,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        1, // Halo size
        std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
      ); // Previous time step has to be valid

      ::exahype2::CellData patchData(oldQWithHalo, marker.x(), marker.h(), timeStamp, timeStepSize, newQ);

      {% if STATELESS_PDE_TERMS %}
      if (repositories::{{SOLVER_INSTANCE}}.patchCanUseStatelessPDETerms(
        marker.x(),
        marker.h(),
        timeStamp,
        timeStepSize
      )) {
        ::exahype2::fv::{{KERNEL_NAMESPACE}}::{{COMPUTE_KERNEL_CALL_STATELESS}}
      } else
      {% endif %}

      ::exahype2::fv::{{KERNEL_NAMESPACE}}::{{COMPUTE_KERNEL_CALL}}

      {{POSTPROCESS_UPDATED_PATCH}}

      ::exahype2::fv::validatePatch(
        newQ,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        0, // Halo size
        std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
      ); // Outcome has to be valid

      {% if COMPUTE_MAX_EIGENVALUE %}
      const double maxEigenvalue = patchData.maxEigenvalue[0];
      {% endif %}

      // Set the variable
      // double newTimeStepSize
      {{COMPUTE_NEW_TIME_STEP_SIZE}}

      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp+timeStepSize);
      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
      fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);

      repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, timeStamp+timeStepSize, marker.h()(0));
  """)


  def __init__(self, solver):
    """
    """
    ReconstructPatchAndApplyFunctor.__init__(self,
      patch = solver._patch,
      patch_overlap = solver._patch_overlap_new,
      functor_implementation = """
#error Please switch to your Riemann solver of choice
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

    assertion2(tarch::la::greaterEquals( timeStepSize, 0.0 ), timeStepSize, timeStamp);
    assertion2(tarch::la::greaterEquals( timeStamp, 0.0 ),    timeStepSize, timeStamp);

    repositories::{SOLVER_INSTANCE}.update(0.0, timeStamp, marker.h()(0) );
  }}
"""

    self._Template_TouchCellFirstTime_Preamble = """
  fineGridCell""" + solver._name + """CellLabel.setHasUpdated(false);
""" + self._Template_TouchCellFirstTime_Preamble


  def _add_action_set_entries_to_dictionary(self,d):
    """
    This is our plug-in point to alter the underlying dictionary
    """
    super(UpdateCell,self)._add_action_set_entries_to_dictionary(d)

    self._solver._init_dictionary_with_default_parameters(d)
    self._solver.add_entries_to_text_replacement_dictionary(d)

    d["CELL_FUNCTOR_IMPLEMENTATION"] = self.SolveRiemannProblemsOverPatch.render(**d)


  def get_includes(self):
    return """
#include "exahype2/fv/BoundaryConditions.h"
#include "tarch/NonCriticalAssertions.h"
""" + self._solver._get_default_includes() + self._solver.user_action_set_includes


class SingleSweep(FV):
  """
  Probably the simplest solver you could think off.

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
               unknowns,
               auxiliary_variables,
               min_volume_h,
               max_volume_h,
               pde_terms_without_state: bool,
               plot_grid_properties: bool,
               kernel_namespace):
    """
    Instantiate a generic FV scheme with an overlap of 1.
    """
    super(SingleSweep, self).__init__(name,
                                      patch_size,
                                      overlap,
                                      unknowns,
                                      auxiliary_variables,
                                      min_volume_h,
                                      max_volume_h,
                                      pde_terms_without_state,
                                      plot_grid_properties,
                                      kernel_namespace)

    self._solver_template_file_class_name = "SingleSweep"


  def create_data_structures(self):
    """
    Call the superclass' create_data_structures() to ensure that all the data
    structures are in place, i.e., each cell can host a patch, that each face hosts
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

    This operation is implicitly called via the superconstructor.
    """
    super(SingleSweep, self).create_action_sets()
    self._action_set_update_cell = UpdateCell(self)


  @property
  def user_action_set_includes(self):
    return """
#include "exahype2/CellData.h"
""" + super(SingleSweep, self).user_action_set_includes


  def set_implementation(
    self,
    boundary_conditions,
    refinement_criterion,
    initial_conditions,
    memory_location,
    use_split_loop,
    additional_action_set_includes,
    additional_user_includes
  ):
    """
    If you pass in User_Defined, then the generator will create C++ stubs
    that you have to befill manually. If you pass in None_Implementation, it
    will create nop, i.e., no implementation or defaults. Any other string
    is copied 1:1 into the implementation. If you pass in None, then the
    set value so far won't be overwritten.
    """
    if boundary_conditions  is not None:  self._boundary_conditions_implementation        = boundary_conditions
    if refinement_criterion is not None:  self._refinement_criterion_implementation       = refinement_criterion
    if initial_conditions   is not None:  self._initial_conditions_implementation         = initial_conditions
    if memory_location      is not None:  self._reconstructed_array_memory_location       = memory_location
    if use_split_loop                  :  self._use_split_loop                            = use_split_loop

    if refinement_criterion==exahype2.solvers.PDETerms.None_Implementation:
      assert False, "Refinement criterion cannot be none"
    if initial_conditions==exahype2.solvers.PDETerms.None_Implementation:
      assert False, "Initial conditions cannot be none"

    if self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete or \
       self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapWithoutDelete or \
       self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.ManagedSharedAcceleratorDeviceMemoryThroughTarchWithoutDelete:
      raise Exception("Memory mode without appropriate delete chosen, i.e. this will lead to a memory leak" )

    self._user_action_set_includes += additional_action_set_includes
    self._user_solver_includes     += additional_user_includes

    self.create_action_sets()


  def add_entries_to_text_replacement_dictionary(self, d):
    """
     d: Dictionary of string to string
        in/out argument
    """
    pass
