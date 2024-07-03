# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .ADERDG import ADERDG


class SingleSweep(ADERDG):
    def __init__(
        self,
        name,
        order,
        unknowns,
        auxiliary_variables,
        min_cell_h,
        max_cell_h,
        plot_grid_properties=False,
    ):
        super(SingleSweep, self).__init__(
            name,
            order,
            unknowns,
            auxiliary_variables,
            min_cell_h,
            max_cell_h,
            plot_grid_properties,
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
            + "::SolverState::InitialPrediction or "
            + "(repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Plotting"
            + " and repositories::"
            + self.get_name_of_global_instance()
            + ".getMinTimeStamp()==0.))"
        )

        self._rhs_estimates_projection.generator.send_condition = (
            "true"
            # "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::InitialPrediction"
            # + " or repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PredictionOnHangingCells "
            # + " or repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Timestep"
            # + " or repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Plotting"
        )
        self._rhs_estimates_projection.generator.receive_and_merge_condition = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PredictionOnHangingCells"
            + " or repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Timestep"
            + " or repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Plotting"
        )

        self._flux_estimates_projection.generator.send_condition = (
            "true"
            # "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::InitialPrediction"
            # + " or repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PredictionOnHangingCells"
            # + " or repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Timestep"
            # + " or repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Plotting"
        )
        self._flux_estimates_projection.generator.receive_and_merge_condition = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PredictionOnHangingCells"
            + " or repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Timestep"
            + " or repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Plotting"
        )

        self._preprocess_reconstructed_patch = ""
        self._postprocess_updated_patch = ""

    def create_action_sets(self):
        super(SingleSweep, self).create_action_sets()

        self._action_set_prediction.guard = (
            self._store_cell_data_default_guard()
            + " and not isHangingCell"
            + " and repositories::{}.isFirstGridSweepOfTimeStep()".format(
                self.get_name_of_global_instance()
            )
        )

        self._action_set_prediction_on_hanging_cells.guard = self._store_cell_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::PredictionOnHangingCells and isHangingCell".format(
            self.get_name_of_global_instance(), self._name
        )

        self._action_set_correction.guard = (
            self._store_cell_data_default_guard()
            + " and repositories::{}.isLastGridSweepOfTimeStep()".format(
                self.get_name_of_global_instance()
            )
        )

        self._action_set_correction.riemann_guard = (
            self._store_cell_data_default_guard()
            + " and repositories::{}.isLastGridSweepOfTimeStep()".format(
                self.get_name_of_global_instance()
            )
            + " and not fineGridFace"
            + self._name
            + "FaceLabel.getAboveHanging()"
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
        )._store_boundary_data_default_guard() + " and repositories::{}.isLastGridSweepOfTimeStep()".format(
            self.get_name_of_global_instance()
        )

    def _shift_face_data_default_guard(self):
        return (
            super(SingleSweep, self)._shift_face_data_default_guard()
            + " and repositories::{}.getSolverState()=={}::SolverState::PredictionOnHangingCells".format(
                self.get_name_of_global_instance(), self._name
            )
            + " and fineGridFace"
            + self._name
            + "FaceLabel.getAboveHanging()"
        )

    def _restrict_face_data_default_guard(self):
        return super(
            SingleSweep, self
        )._restrict_face_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::PredictionOnHangingCells".format(
            self.get_name_of_global_instance(), self._name
        )

    def _interpolate_face_data_default_guard(self):
        return super(
            SingleSweep, self
        )._interpolate_face_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::PredictionOnHangingCells".format(
            self.get_name_of_global_instance(), self._name
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
