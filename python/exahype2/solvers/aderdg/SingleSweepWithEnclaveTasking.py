# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .SingleSweep import SingleSweep
from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.aderdg.actionsets.PredictionWithEnclaveTasking import PredictionWithEnclaveTasking
from exahype2.solvers.aderdg.actionsets.PredictionMergeEnclaveTaskOutcome import PredictionMergeEnclaveTaskOutcome

import os
import jinja2
import peano4
import exahype2


class SingleSweepWithEnclaveTasking(SingleSweep):
    def __init__(
        self,
        name,
        order,
        unknowns,
        auxiliary_variables,
        min_cell_h,
        max_cell_h,
        plot_grid_properties,
        pde_terms_without_state,
        # Kernel optimisations
        is_linear,
        polynomials,
        use_kernel_generator,
        use_libxsmm,
        use_BLIS,
        use_Eigen,
        use_libxsmm_JIT,
        predictor_computation_precisions,
        corrector_computation_precision,
        solution_persistent_storage_precision,
        precision_criterion_implementation,
        hold_cell_data_on_heap_with_smart_pointer,
        hold_face_data_on_heap_with_smart_pointer,
        use_vectorized_PDE,
        architecture,
        use_CERK_guess,
        use_split_CK,
        predictor_recompute,
        initialize_patches,
    ):
        super(SingleSweepWithEnclaveTasking, self).__init__(
            name,
            order,
            unknowns,
            auxiliary_variables,
            min_cell_h,
            max_cell_h,
            plot_grid_properties,
            pde_terms_without_state,
            is_linear,
            polynomials,
            use_kernel_generator,
            use_libxsmm,
            use_BLIS,
            use_Eigen,
            use_libxsmm_JIT,
            predictor_computation_precisions,
            corrector_computation_precision,
            solution_persistent_storage_precision,
            precision_criterion_implementation,
            hold_cell_data_on_heap_with_smart_pointer,
            hold_face_data_on_heap_with_smart_pointer,
            use_vectorized_PDE,
            architecture,
            use_CERK_guess,
            use_split_CK,
            predictor_recompute,
            initialize_patches,
        )

        self._solver_template_file_class_name = "SingleSweepWithEnclaveTasking"

        self.enclave_task_priority = "tarch::multicore::Task::DefaultPriority-1"

        self.create_action_sets()
        self.create_data_structures()


    def create_data_structures(self):
        super(SingleSweepWithEnclaveTasking, self).create_data_structures()


    def create_action_sets(self):
        super(SingleSweepWithEnclaveTasking, self).create_action_sets()

        self._action_set_prediction                   = PredictionWithEnclaveTasking(
            self, self._store_cell_data_default_guard(), on_hanging_cells=False)
        self._action_set_prediction.descend_invocation_order = 1

        #self._action_set_prediction_on_hanging_cells  = PredictionWithEnclaveTasking(
        #    self, self._store_cell_data_default_guard(), on_hanging_cells=True)
        #self._action_set_prediction_on_hanging_cells.descend_invocation_order = 1

        self._action_set_prediction_merge_enclave_task_outcome = PredictionMergeEnclaveTaskOutcome(self)
        self._action_set_prediction_merge_enclave_task_outcome.descend_invocation_order = 1


    def add_implementation_files_to_project(self, namespace, output, dimensions):
        super(SingleSweepWithEnclaveTasking, self
        ).add_implementation_files_to_project(namespace, output, dimensions)

        templatefile_prefix = os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            "Prediction.EnclaveTask.template",
        )

        implementationDictionary = {}
        self._init_dictionary_with_default_parameters(implementationDictionary)
        self.add_entries_to_text_replacement_dictionary(implementationDictionary)

        # Some includes might logically belong into the action sets, but now they are
        # 'outsourced' into the enclave task. So we manually add it here.
        implementationDictionary["SOLVER_INCLUDES"] += self.user_solver_includes
        implementationDictionary["SOLVER_INCLUDES"] += self.user_action_set_includes

        task_name = self._prediction_enclave_task_name()
        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                "{}.h".format(templatefile_prefix),
                "{}.cpp".format(templatefile_prefix),
                task_name,
                namespace + ["tasks"],
                "tasks",
                implementationDictionary,
                True,
            )
        )

        output.add(generated_solver_files)
        output.makefile.add_cpp_file("tasks/" + task_name + ".cpp", generated=True)


    def _prediction_enclave_task_name(self):
        return "{}PredictionEnclaveTask".format(self._name)


    def add_actions_to_perform_time_step(self, step):
        super(SingleSweepWithEnclaveTasking, self).add_actions_to_perform_time_step(
            step
        )
        step.add_action_set(self._action_set_prediction_merge_enclave_task_outcome)


    def add_entries_to_text_replacement_dictionary(self, d):
        super(SingleSweepWithEnclaveTasking, self
        ).add_entries_to_text_replacement_dictionary(d)

        d["SEMAPHORE_LABEL"] = exahype2.grid.UpdateCellLabel.get_attribute_name(
            self._name
        )
        d["STATLESS_PDE_TERMS"] = self._pde_terms_without_state
        d["ENCLAVE_TASK_PRIORITY"] = self.enclave_task_priority
