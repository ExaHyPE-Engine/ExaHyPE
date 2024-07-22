# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .RungeKuttaDG import RungeKuttaDG
from .SeparateSweeps import SeparateSweeps
from exahype2.solvers.PDETerms import PDETerms

import peano4
import exahype2

import jinja2

import os

from exahype2.solvers.fv.actionsets.AbstractFVActionSet import AbstractFVActionSet

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor
from exahype2.solvers.Storage import Storage


class MergeEnclaveTaskOutcome(AbstractFVActionSet):
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
      fineGridCell{{LABEL_NAME}}.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
    }
  }
  {% endfor %}
"""


    def __init__(self, solver):
        super(MergeEnclaveTaskOutcome, self).__init__(solver)
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
    """
    Two separate sweeps per Runge-Kutta sweep where volumetric operations
    are outsourced into dedicated tasks.


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
        rk_order,
        polynomial_basis,
        number_of_face_projections,
        unknowns,
        auxiliary_variables,
        min_cell_h,
        max_cell_h,
        plot_grid_properties,
        pde_terms_without_state,
    ):
        """
        See superclass constructor for all the interesting info.

        It is important to notice that we still use the separate sweep template classes
        here.
        """
        super(SeparateSweepsWithEnclaveTasking, self).__init__(
            name,
            rk_order,
            polynomial_basis,
            number_of_face_projections,
            unknowns,
            auxiliary_variables,
            min_cell_h,
            max_cell_h,
            plot_grid_properties,
            pde_terms_without_state,
        )

        self._solver_template_file_class_name = "SeparateSweeps"
        self._pde_terms_without_state = pde_terms_without_state

        self._fused_volumetric_compute_kernel_call_stateless_cpu = "#error Not yet defined. Set self._fused_volumetric_compute_kernel_call_stateless_cpu in your Python solver class."
        self._fused_volumetric_compute_kernel_call_stateless_gpu = "#error Not yet defined. Set self._fused_volumetric_compute_kernel_call_stateless_gpu in your Python solver class."
        self._fused_Riemann_compute_kernel_call_stateless_cpu = "#error Not yet defined. Set self._fused_Riemann_compute_kernel_call_stateless_cpu in your Python solver class."
        self._fused_Riemann_compute_kernel_call_stateless_gpu = "#error Not yet defined. Set self._fused_Riemann_compute_kernel_call_stateless_gpu in your Python solver class."

        self.enclave_task_priority = "tarch::multicore::Task::DefaultPriority-1"

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
        Only the UpdateCell action set is specific to an enclave solver.

        This operation is implicitly called via the superconstructor.

        It is important that we add the action set at the right point. See
        add_actions_to_perform_time_step() for a discussion.
        """
        super(SeparateSweepsWithEnclaveTasking, self).create_action_sets()

        self._action_set_solve_volume_integral.spawn_volume_kernel_as_task = True

        self._action_set_merge_enclave_task_outcome = MergeEnclaveTaskOutcome(self)
        self._action_set_merge_enclave_task_outcome.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )


    def add_implementation_files_to_project(self, namespace, output, dimensions, subdirectory=""):
        super(
            SeparateSweepsWithEnclaveTasking, self
        ).add_implementation_files_to_project(namespace, output, dimensions)
        templatefile_prefix = os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            "SolveVolumeIntegral.EnclaveTask.template",
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

        task_name = self._volumetric_solver_enclave_task_name()
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


    def _volumetric_solver_enclave_task_name(self):
        return "{}_VolumetricSolverEnclaveTask".format(self._name)


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
        super(SeparateSweepsWithEnclaveTasking, self).add_actions_to_perform_time_step(
            step
        )
        step.add_action_set(self._action_set_merge_enclave_task_outcome)


    def add_entries_to_text_replacement_dictionary(self, d):
        super(SeparateSweepsWithEnclaveTasking, self).add_entries_to_text_replacement_dictionary(d)

        d["FUSED_VOLUMETRIC_COMPUTE_KERNEL_CALL_STATELESS_CPU"] = jinja2.Template(
            self._fused_volumetric_compute_kernel_call_stateless_cpu, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["FUSED_VOLUMETRIC_COMPUTE_KERNEL_CALL_STATELESS_GPU"] = jinja2.Template(
            self._fused_volumetric_compute_kernel_call_stateless_gpu, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["FUSED_RIEMANN_COMPUTE_KERNEL_CALL_STATELESS_CPU"] = jinja2.Template(
            self._fused_Riemann_compute_kernel_call_stateless_cpu, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["FUSED_RIEMANN_COMPUTE_KERNEL_CALL_STATELESS_GPU"] = jinja2.Template(
            self._fused_Riemann_compute_kernel_call_stateless_gpu, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["SEMAPHORE_LABEL"] = exahype2.grid.UpdateCellLabel.get_attribute_name(
            self._name
        )
        d["ENCLAVE_TASK_PRIORITY"] = self.enclave_task_priority
        d["MAKE_COPY_OF_ENCLAVE_TASK_DATA"] = self.make_copy_of_enclave_task_data


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
