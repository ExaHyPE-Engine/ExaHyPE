# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .SeparateSweeps import SeparateSweeps
from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.rkfd.actionsets.AbstractRKFDActionSet import AbstractRKFDActionSet

import peano4
import exahype2
import jinja2


import os

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import (
    ReconstructPatchAndApplyFunctor,
)

from exahype2.solvers.ButcherTableau import ButcherTableau
from exahype2.solvers.Storage import Storage


class UpdateCell(ReconstructPatchAndApplyFunctor):
    """!

    Update one cell that is compute Runge-Kutta step on it

    This routine is significantly simpler than its counterpart in SeparateSweeps,
    as we basically check if a cell is an enclave cell or not. If it is one, we
    spawn a task. If not, we call the static routine from the task class which
    updates the patch. All the computations thus are removed.

    """

    SolveRiemannProblemsOverPatch = jinja2.Template(
        """
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
            oldQWithHalo[ enumeratorWithAuxiliaryVariablesOnReconstructedPatch(0,dof,unknown) ] += 
              timeStepSize * {{BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO][WEIGHT_NO]}} * 
              fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value[ enumeratorWithoutAuxiliaryVariables({{WEIGHT_NO}},dof,unknown) ];
            {% endif %}
            {% endfor %}
          }      
        }
        
        newQ = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value + enumeratorWithoutAuxiliaryVariables({{PREDICATE_NO}},0,0);
      }
      {% endfor %} 
   
      {{PREPROCESS_RECONSTRUCTED_PATCH}}

      assertion2( tarch::la::greaterEquals( timeStamp, 0.0 ),    timeStamp, timeStepSize );
      assertion2( tarch::la::greaterEquals( timeStepSize, 0.0 ), timeStamp, timeStepSize );

      ::exahype2::fd::validatePatch(
        oldQWithHalo,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
        {{HALO_SIZE}}, // halo
        std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
      ); // previous time step has to be valid
  
      double subTimeStamp=timeStamp;
      {% for PREDICATE_NO in range(0,PREDICATES|length) %}
      if ({{PREDICATES[PREDICATE_NO]}}) {
        subTimeStamp += {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}}*timeStepSize;
      }
      {% endfor %} 
  
      if ( marker.willBeSkeletonCell() ) {
        tasks::{{SOLVER_NAME}}EnclaveTask::applyKernelToCell(
          marker,
          subTimeStamp,
          timeStepSize,
          oldQWithHalo,
          newQ
        );
        
        fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::SkeletonTask );
      }
      else {
        assertion( marker.willBeEnclaveCell() );
        assertion( not marker.willBeRefined() );
        auto newEnclaveTask = new tasks::{{SOLVER_NAME}}EnclaveTask(
          marker,
          subTimeStamp,
          timeStepSize,
          oldQWithHalo,
          {% if MAKE_COPY_OF_ENCLAVE_TASK_DATA %}
          nullptr
          {% else %}
          newQ
          {% endif %}
        );
        
        int predecessorEnclaveTaskNumber = fineGridCell{{SEMAPHORE_LABEL}}.getSemaphoreNumber();
        
        tarch::multicore::spawnTask( 
          newEnclaveTask, 
          predecessorEnclaveTaskNumber>=0 ? std::set<int>{predecessorEnclaveTaskNumber} : tarch::multicore::NoInDependencies, 
          newEnclaveTask->getTaskId() 
        );
        
        ::exahype2::EnclaveTask::releaseTaskNumber(predecessorEnclaveTaskNumber);
  
        fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( newEnclaveTask->getTaskId() );

        // Time stamp is not updated, as this will be done by final linear combination
        // fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp + timeStepSize);

      }
  """
    )

    def __init__(self, solver):
        """ """
        one_huge_boolean_guard_expression = "false"
        for expr in solver._primary_sweeps_of_Runge_Kutta_step_on_cell:
            one_huge_boolean_guard_expression += " or (" + expr + ")"

        super(UpdateCell, self).__init__(
            patch=solver._patch,
            patch_overlap=solver._patch_overlap_new,
            functor_implementation="""
#error please switch to your Riemann solver of choice
""",
            reconstructed_array_memory_location=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.ManagedSharedAcceleratorDeviceMemoryThroughTarchWithoutDelete,
            guard=one_huge_boolean_guard_expression,
            add_assertions_to_halo_exchange=False,
        )
        self._solver = solver

        self._Template_TouchCellFirstTime_Preamble = (
            """
  fineGridCell"""
            + solver._name
            + """CellLabel.setHasUpdated(false);
"""
            + self._Template_TouchCellFirstTime_Preamble
        )

        self._butcher_tableau = ButcherTableau(self._solver._rk_order)

    def _add_action_set_entries_to_dictionary(self, d):
        """!

        This is our plug-in point to alter the underlying dictionary

        """
        super(UpdateCell, self)._add_action_set_entries_to_dictionary(d)

        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)

        d["PREDICATES"] = self._solver._primary_sweeps_of_Runge_Kutta_step_on_cell
        d["BUTCHER_TABLEAU_WEIGHTS"] = self._butcher_tableau.weight_matrix()
        d[
            "BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES"
        ] = self._butcher_tableau.time_step_sizes()

        # Has to come after we've set the predicates, as we use these
        # fields in here already
        d["CELL_FUNCTOR_IMPLEMENTATION"] = self.SolveRiemannProblemsOverPatch.render(
            **d
        )

    def get_includes(self):
        return (
            """
#include "tarch/NonCriticalAssertions.h"
#include "exahype2/enumerator/enumerator.h"
#include "exahype2/fd/PatchUtils.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "tarch/multicore/Tasks.h"
"""
            + self._solver._get_default_includes()
            + self._solver.user_action_set_includes
            + """
#include "tasks/{}.h"
""".format(
                self._solver._enclave_task_name()
            )
        )

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_") + "_UpdateCell"


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
    constexpr int NumberOfDoFsPerCell = {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}};
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value + {{PREDICATE_NO}} * NumberOfDoFsPerCell * {{NUMBER_OF_UNKNOWNS}};
    #elif Dimensions==3
    constexpr int NumberOfDoFsPerCell = {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} * {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}};
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value + {{PREDICATE_NO}} * NumberOfDoFsPerCell * {{NUMBER_OF_UNKNOWNS}};
    #endif
    
    const int taskNumber = fineGridCell{{LABEL_NAME}}.getSemaphoreNumber();
    if ( taskNumber>=0 ) {
      double maxEigenvalue; // not used here
      ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndCopyResultOver( taskNumber, QOut, maxEigenvalue );
      fineGridCell{{LABEL_NAME}}.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
    }
  }
  {% endfor %}
"""

    def __init__(self, solver):
        super(MergeEnclaveTaskOutcome, self).__init__(solver)
        self.descend_invocation_order = -1
        self.label_name = exahype2.grid.UpdateCellLabel.get_attribute_name(solver._name)

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d["LABEL_NAME"] = self.label_name
            d["PREDICATES"] = self._solver._secondary_sweeps_of_Runge_Kutta_step_on_cell
            result = jinja2.Template(self.Template).render(**d)
            pass
        return result

    def get_action_set_name(self):
        return (
            __name__.replace(".py", "").replace(".", "_") + "_MergeEnclaveTaskOutcome"
        )

    def get_includes(self):
        return (
            super(MergeEnclaveTaskOutcome, self).get_includes()
            + """
#include "exahype2/EnclaveBookkeeping.h"
"""
        )


class SeparateSweepsWithEnclaveTasking(SeparateSweeps):
    """!

    Enclave variant of the solver where we still run through mesh once per Runge-Kutta sweep

    The concept of (enclave) tasking within ExaHyPE solvers is described in
    detail in the @ref page_exahype_solvers_enclave_solvers "generic enclave discussion of ExaHyPE".
    This class is a prototype realisation of this concept which other solvers
    then specialise for particular numerical schemes.

    The class basically replaces the standard "update a cell" action set with an
    action set that might or might not spawn a task. In return, it adds a further
    action set which merges the arising task outcomes into the actual mesh
    structure. By default, we use peano4::datamanagement::CellMarker::willBeEnclaveCell()
    and peano4::datamanagement::CellMarker::hasBeenEnclaveCell() to guide the
    decision whether to spawn a task or not. You can overwrite this decision
    by redefining the corresponding entry in the dictionary befilled by
    add_entries_to_text_replacement_dictionary().

    ## Task priorities
    
    Use the attributes self.enclave_task_priority to change the priority of the 
    task. This value can either be a string that C++ can evaluate into a 
    priority or a plain numerical value. I set it to 
    
           self.enclave_task_priority  = "tarch::multicore::Task::DefaultPriority-1"
               
    by default.
    
    
    """

    def __init__(
        self,
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
        pde_terms_without_state,
    ):
        """ """
        self._name = name
        self._rk_order = rk_order

        self._primary_sweeps_of_Runge_Kutta_step_on_cell = [
            "{} and repositories::{}.getSolverState()=={}::SolverState::RungeKuttaPrimarySubStep{}".format(
                self._store_cell_data_default_guard(),
                self.get_name_of_global_instance(),
                self._name,
                step,
            )
            for step in range(0, self.number_of_Runge_Kutta_steps())
        ] + [
            "{} and repositories::{}.getSolverState()=={}::SolverState::RungeKuttaPrimarySubStep0AfterGridInitialisation".format(
                self._store_cell_data_default_guard(),
                self.get_name_of_global_instance(),
                self._name,
            )
        ]
        self._secondary_sweeps_of_Runge_Kutta_step_on_cell = [
            "{} and repositories::{}.getSolverState()=={}::SolverState::RungeKuttaSecondarySubStep{}".format(
                self._store_cell_data_default_guard(),
                self.get_name_of_global_instance(),
                self._name,
                step,
            )
            for step in range(0, self.number_of_Runge_Kutta_steps())
        ]
        self._last_secondary_sweep_of_Runge_Kutta_step_on_cell = "repositories::{}.getSolverState()=={}::SolverState::RungeKuttaSecondarySubStep{}".format(
            self.get_name_of_global_instance(),
            self._name,
            self.number_of_Runge_Kutta_steps() - 1,
        )

        self._primary_sweeps_of_Runge_Kutta_step_on_face = [
            "{} and repositories::{}.getSolverState()=={}::SolverState::RungeKuttaPrimarySubStep{}".format(
                self._store_face_data_default_guard(),
                self.get_name_of_global_instance(),
                self._name,
                step,
            )
            for step in range(0, self.number_of_Runge_Kutta_steps())
        ] + [
            "{} and repositories::{}.getSolverState()=={}::SolverState::RungeKuttaPrimarySubStep0AfterGridInitialisation".format(
                self._store_face_data_default_guard(),
                self.get_name_of_global_instance(),
                self._name,
            )
        ]
        self._secondary_sweeps_of_Runge_Kutta_step_on_face = [
            "{} and repositories::{}.getSolverState()=={}::SolverState::RungeKuttaSecondarySubStep{}".format(
                self._store_face_data_default_guard(),
                self.get_name_of_global_instance(),
                self._name,
                step,
            )
            for step in range(0, self.number_of_Runge_Kutta_steps())
        ]

        self._primary_sweep_guard = (
            "("
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::RungeKuttaPrimarySubStep0AfterGridInitialisation "
        )
        for step in range(0, self.number_of_Runge_Kutta_steps()):
            self._primary_sweep_guard += " or repositories::{}.getSolverState()=={}::SolverState::RungeKuttaPrimarySubStep{}".format(
                self.get_name_of_global_instance(), self._name, step
            )
        self._primary_sweep_guard += ")"

        self._primary_sweep_or_plot_guard = """(
       repositories::{}.getSolverState()=={}::SolverState::RungeKuttaPrimarySubStep0AfterGridInitialisation 
    or repositories::{}.getSolverState()=={}::SolverState::PlottingAfterGridInitialisation 
    or repositories::{}.getSolverState()=={}::SolverState::Plotting
    or repositories::{}.getSolverState()=={}::SolverState::Suspended
  )""".format(
          self.get_name_of_global_instance(), self._name,
          self.get_name_of_global_instance(), self._name,
          self.get_name_of_global_instance(), self._name,
          self.get_name_of_global_instance(), self._name,
          )

        
        for step in range(0, self.number_of_Runge_Kutta_steps()):
            self._primary_sweep_or_plot_guard += " or repositories::{}.getSolverState()=={}::SolverState::RungeKuttaPrimarySubStep{}".format(
                self.get_name_of_global_instance(), self._name, step
            )
        self._primary_sweep_or_plot_guard += ")"

        self._secondary_sweep_guard = "( false"
        for step in range(0, self.number_of_Runge_Kutta_steps()):
            self._secondary_sweep_guard += " or repositories::{}.getSolverState()=={}::SolverState::RungeKuttaSecondarySubStep{}".format(
                self.get_name_of_global_instance(), self._name, step
            )
        self._secondary_sweep_guard += ")"

        self._secondary_sweep_or_initialisation_guard = """( 
      repositories::{}.getSolverState()=={}::SolverState::GridInitialisation""".format(
            self.get_name_of_global_instance(), self._name
        )
        for step in range(0, self.number_of_Runge_Kutta_steps()):
            self._secondary_sweep_or_initialisation_guard += " or repositories::{}.getSolverState()=={}::SolverState::RungeKuttaSecondarySubStep{}".format(
                self.get_name_of_global_instance(), self._name, step
            )
        self._secondary_sweep_or_initialisation_guard += ")"

        super(SeparateSweepsWithEnclaveTasking, self).__init__(
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
        )

        self._solver_template_file_class_name = "SeparateSweepsWithEnclaveTasking"

        self._fused_compute_kernel_call_cpu = (
            "#error Not yet defined. Set in your Python solver class."
        )
        self._fused_compute_kernel_call_gpu = (
            "#error Not yet defined. Set in your Python solver class."
        )
        self._pde_terms_without_state = pde_terms_without_state

        self._fused_volumetric_kernel_call_cpu = "#error Not yet defined. Set self._fused_volumetric_kernel_call_cpu in your Python solver class."
        self._fused_volumetric_kernel_call_gpu = "#error Not yet defined. Set self._fused_volumetric_kernel_call_gpu in your Python solver class."

        self.enclave_task_priority = "tarch::multicore::Task::DefaultPriority-1"
            
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

        initialisation_sweep_guard = (
            "("
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::GridInitialisation"
            + ")"
        )
        first_iteration_after_initialisation_guard = (
            "("
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::RungeKuttaPrimarySubStep0AfterGridInitialisation or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PlottingAfterGridInitialisation"
            + ")"
        )

        self._patch_overlap_old.generator.send_condition = initialisation_sweep_guard
        self._patch_overlap_old.generator.receive_and_merge_condition = (
            first_iteration_after_initialisation_guard
        )

        secondary_sweep_or_initialisation_or_plotting_guard = """(
      repositories::{}.getSolverState()=={}::SolverState::GridInitialisation or
      repositories::{}.getSolverState()=={}::SolverState::PlottingAfterGridInitialisation or
      repositories::{}.getSolverState()=={}::SolverState::Plotting or
      repositories::{}.getSolverState()=={}::SolverState::Suspended or
      repositories::{}.isLastGridSweepOfTimeStep()
    )""".format(
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(),
        )

        primary_sweep_or_plotting = """(
      repositories::{}.getSolverState()=={}::SolverState::PlottingAfterGridInitialisation or
      repositories::{}.getSolverState()=={}::SolverState::Plotting or
      repositories::{}.getSolverState()=={}::SolverState::Suspended or
      repositories::{}.isFirstGridSweepOfTimeStep()
    )""".format(
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), 
        )

        self._patch_overlap_new.generator.send_condition = (
            secondary_sweep_or_initialisation_or_plotting_guard
        )
        self._patch_overlap_new.generator.receive_and_merge_condition = (
            primary_sweep_or_plotting
        )

        first_sweep_of_time_step_or_plotting_guard = """(
      repositories::{}.isFirstGridSweepOfTimeStep() or
      repositories::{}.getSolverState()=={}::SolverState::PlottingAfterGridInitialisation or
      repositories::{}.getSolverState()=={}::SolverState::Plotting or
      repositories::{}.getSolverState()=={}::SolverState::Suspended
    )""".format(
            self.get_name_of_global_instance(), 
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
        )

        last_sweep_of_time_step_or_plotting_or_initialisation = """(
      repositories::{}.getSolverState()=={}::SolverState::GridInitialisation or
      repositories::{}.getSolverState()=={}::SolverState::PlottingAfterGridInitialisation or
      repositories::{}.getSolverState()=={}::SolverState::Plotting or
      repositories::{}.getSolverState()=={}::SolverState::Suspended or
      repositories::{}.isLastGridSweepOfTimeStep()
    )""".format(
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), 
        )

        self._patch_estimates.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
            self._provide_cell_data_to_compute_kernels_default_guard(),
            self._load_cell_data_default_guard()
            + """
      and not ("""
            + first_sweep_of_time_step_or_plotting_guard
            + ")",
            self._store_cell_data_default_guard()
            + """
      and not ("""
            + last_sweep_of_time_step_or_plotting_or_initialisation
            + ")",
        )

    def create_action_sets(self):
        """

        Call superclass routine and then reconfigure the update cell call.
        Only the UpdateCell action set is specific to a single sweep.

        This operation is implicity called via the superconstructor.

        ## Guard construction

        We note that the guard sets all contain the storage predicate already,
        i.e. they combine the logic state analysis with an evaluation of
        _load_cell_data_default_guard() and _store_cell_data_default_guard().
        The singular strings like _primary_sweep_guard do not have this check
        built in. We have to add it here.

        """
        super(SeparateSweepsWithEnclaveTasking, self).create_action_sets()

        self._action_set_merge_enclave_task_outcome = MergeEnclaveTaskOutcome(self)
        self._action_set_update_cell = UpdateCell(self)

        self._action_set_update_cell.descend_invocation_order = self._baseline_action_set_descend_invocation_order + 4
        self._action_set_merge_enclave_task_outcome.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 4
        )
        #
        # have a single guard (technically)
        #
        self._action_set_compute_final_linear_combination.guard = (
            self._store_cell_data_default_guard()
            + " and ("
            + self._last_secondary_sweep_of_Runge_Kutta_step_on_cell
            + ")"
        )
        self._action_set_handle_boundary.guard = (
            self._load_face_data_default_guard()
            + " and ("
            + self._primary_sweep_guard
            + ")"
        )
        self._action_set_roll_over_update_of_faces.guard = (
            self._store_face_data_default_guard()
            + " and ("
            + self._secondary_sweep_or_initialisation_guard
            + ")"
        )

        #
        # the following mappings have guards, i.e. a whole set of guards
        #
        self._action_set_project_patch_onto_faces.guards = self._secondary_sweeps_of_Runge_Kutta_step_on_cell + [
            "{} and repositories::{}.getSolverState()=={}::SolverState::GridInitialisation".format(
                self._store_cell_data_default_guard(),
                self.get_name_of_global_instance(),
                self._name,
            )
        ]

        #
        # this one is fine, as it only is used in the initialisation
        #
        # self._action_set_copy_new_faces_onto_old_faces.guard          = self._secondary_sweeps_of_Runge_Kutta_step_on_face

        # last_sweep_of_time_step_or_plotting_or_initialisation
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.guards = (
            self._primary_sweeps_of_Runge_Kutta_step_on_face
        )

    def add_implementation_files_to_project(self, namespace, output, dimensions, subdirectory=""):
        """

        Add the enclave task for the GPU

        See superclass for further information.

        """
        super(
            SeparateSweepsWithEnclaveTasking, self
        ).add_implementation_files_to_project(namespace, output, dimensions, subdirectory)
        templatefile_prefix = os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            "SeparateSweeps.EnclaveTask.template",
        )

        if(subdirectory):
            subdirectory += "/"

        implementationDictionary = {}
        self._init_dictionary_with_default_parameters(implementationDictionary)
        self.add_entries_to_text_replacement_dictionary(implementationDictionary)

        # Some includes might logically belong into the action sets, but now they are
        # 'outsourced' into the enclave task. So we manually add it here.
        implementationDictionary["SOLVER_INCLUDES"] += self.user_solver_includes
        implementationDictionary["SOLVER_INCLUDES"] += self.user_action_set_includes

        task_name = self._enclave_task_name()
        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                "{}.h".format(templatefile_prefix),
                "{}.cpp".format(templatefile_prefix),
                task_name,
                namespace + ["tasks"],
                subdirectory + "tasks",
                implementationDictionary,
                True,
            )
        )

        output.add(generated_solver_files)
        output.makefile.add_cpp_file(subdirectory + "tasks/" + task_name + ".cpp", generated=True)


    def add_actions_to_perform_time_step(self, step):
        """!

        Add enclave aspect

        Add enclave aspect to time stepping. If you study the superclass'
        routine add_actions_to_perform_time_step() and consider that this action
        set is invoked in the secondary grid sweep, then it becomes clear that
        this merger has to come first, i.e. we first add the action set and then
        we call the superclass' add_action_set().

        We need the result of the volumetric operation before we sum up this
        volumetric solution and the Riemann solution.

        """
        super(SeparateSweepsWithEnclaveTasking, self).add_actions_to_perform_time_step(
            step
        )
        step.add_action_set(self._action_set_merge_enclave_task_outcome)

    def add_entries_to_text_replacement_dictionary(self, d):
        super(
            SeparateSweepsWithEnclaveTasking, self
        ).add_entries_to_text_replacement_dictionary(d)

        d["FUSED_COMPUTE_KERNEL_CALL_CPU"] = jinja2.Template(
            self._fused_compute_kernel_call_cpu, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["FUSED_COMPUTE_KERNEL_CALL_GPU"] = jinja2.Template(
            self._fused_compute_kernel_call_gpu, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["SEMAPHORE_LABEL"] = exahype2.grid.UpdateCellLabel.get_attribute_name(
            self._name
        )
        d["STATELESS_PDE_TERMS"] = self._pde_terms_without_state
        d["ENCLAVE_TASK_PRIORITY"] = self.enclave_task_priority
        d["MAKE_COPY_OF_ENCLAVE_TASK_DATA"] = self.make_copy_of_enclave_task_data
        
        
    @property
    def user_action_set_includes(self):
        return (
            """
#include "exahype2/CellData.h"
"""
            + super(SeparateSweeps, self).user_action_set_includes
        )

    def _enclave_task_name(self):
        return "{}EnclaveTask".format(self._name)

    def switch_storage_scheme(
        self,
        cell_data_storage: Storage,
        face_data_storage: Storage,
    ):
        if cell_data_storage == Storage.SmartPointers:
            self.make_copy_of_enclave_task_data = False
        else:
            self.make_copy_of_enclave_task_data = True
            
        super(SeparateSweepsWithEnclaveTasking, self).switch_storage_scheme(
            cell_data_storage,
            face_data_storage
        )
