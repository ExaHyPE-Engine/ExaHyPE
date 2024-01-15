# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .ADERDG import ADERDG, PrecisionType

from exahype2.solvers.PDETerms import PDETerms

import jinja2


class SingleSweep(ADERDG):
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
        super(SingleSweep, self).__init__(
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

        self._solver_template_file_class_name = "SingleSweep"

        self.create_action_sets()
        self.create_data_structures()


    def create_data_structures(self):
        super(SingleSweep, self).create_data_structures()

        self.initialisation_sweep_guard = (
            "("
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::GridInitialisation"
            + ")"
        )
        self.first_iteration_after_initialisation_guard = (
            "("
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::TimeStepAfterGridInitialisation or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PlottingAfterGridInitialisation"
            + ")"
        )

        # Face estimates from the space-time predictor should be sent in predictor step and received and used in the correction step.
        self._rhs_estimates_projection.generator.send_condition = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Prediction or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PredictionOnHangingCells"
        )
        self._rhs_estimates_projection.generator.receive_and_merge_condition = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PredictionOnHangingCells or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name + "::SolverState::Correction"
        )

        self._flux_estimates_projection.generator.send_condition = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Prediction or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PredictionOnHangingCells"
        )
        self._flux_estimates_projection.generator.receive_and_merge_condition = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PredictionOnHangingCells or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Correction"
        )

        self._preprocess_reconstructed_patch = ""
        self._postprocess_updated_patch = ""
       #self._riemann_estimates.generator.send_condition                        = "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Correction"
       #self._riemann_estimates.generator.receive_and_merge_condition           = "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Prediction"


    def create_action_sets(self):
        super(SingleSweep, self).create_action_sets()

        self._action_set_prediction.guard = (
            self._store_cell_data_default_guard()
            + " and repositories::{}.getSolverState()=={}::SolverState::Prediction and not isHangingCell".format(
                self.get_name_of_global_instance(),
                self._name
            )
        )

        self._action_set_correction.guard = self._store_cell_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::Correction".format(
            self.get_name_of_global_instance(), self._name
        )

        if self._use_kernel_generator:
            self._action_set_correction.riemann_guard = (
                self._store_cell_data_default_guard()
                + " and repositories::{}.getSolverState()=={}::SolverState::Correction".format(
                    self.get_name_of_global_instance(),self._name
                    )
                + " and not fineGridFace"
                + self._name
                + "FaceLabel.getAboveHanging()"
            )
            self._action_set_prediction_on_hanging_cells.guard = (
              self._store_cell_data_default_guard() +
              " and repositories::{}.getSolverState()=={}::SolverState::PredictionOnHangingCells and isHangingCell".format(
                  self.get_name_of_global_instance(),
                  self._name
            )
        )
        else:
            self._action_set_correction.riemann_guard = (
                self._store_cell_data_default_guard()
                + " and repositories::{}.getSolverState()=={}::SolverState::Correction".format(
                    self.get_name_of_global_instance(),
                    self._name
                )
            )


    def get_user_action_set_includes(self):
        return (
            super(SingleSweep, self).get_user_action_set_includes()
            + """
"""
        )


    def _store_boundary_data_default_guard(self):
        return super(
            SingleSweep, self
        )._store_boundary_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::Correction".format(
            self.get_name_of_global_instance(), self._name
        )


    def _clear_face_data_default_guard(self):
        return super(
            SingleSweep, self
        )._clear_face_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::PredictionOnHangingCells".format(
            self.get_name_of_global_instance(), self._name
        ) + " and fineGridFace" + self._name + "FaceLabel.getAboveHanging()"


    def _restrict_face_data_default_guard(self):
        return super(
            SingleSweep, self
        )._restrict_face_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::PredictionOnHangingCells".format(
            self.get_name_of_global_instance(), self._name
        )


    def _interpolate_face_data_default_guard(self):
        return super(
            SingleSweep,
            self
        )._interpolate_face_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::PredictionOnHangingCells".format(
            self.get_name_of_global_instance(),
            self._name
        )


    def set_preprocess_reconstructed_patch_kernel(self, kernel):
        """
        Most subclasses will redefine/overwrite this operation as they have
        to incorporate the kernel into their generated stuff.
        """
        self._preprocess_reconstructed_patch += kernel
        self.create_data_structures()
        self.create_action_sets()


    def set_postprocess_updated_patch_kernel(self, kernel):
        self._postprocess_updated_patch += kernel
        self.create_data_structures()
        self.create_action_sets()
