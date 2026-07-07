# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .ADERDG import ADERDG

from .AbstractSolverDeclarations import AbstractSolverDeclarations
from .SolverDeclarations import SolverDeclarations
from .SolverDefinitions import SolverDefinitions


class SingleSweep(ADERDG):
    def __init__(
        self,
        name,
        order,
        unknowns,
        auxiliary_variables,
        split_sweeps=False,
    ):
        # If turned on it turns the solver from a single sweep to a separate sweeps solver,
        #   i.e. the predictor and corrector steps occur on separate traversals of the domain.
        #   this can be useful for debugging, or for some additional features such as limiting.
        self._split_sweeps = split_sweeps

        super(SingleSweep, self).__init__(
            name=name,
            order=order,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
        )

        self._solver_template_file_class_name = "SingleSweep"

    def create_data_structures(self):
        super(SingleSweep, self).create_data_structures()

        solver_state = "repositories::" + self.get_name_of_global_instance() + ".getSolverState()"
        is_last_sweep = "repositories::" + self.get_name_of_global_instance() + ".isLastGridSweepOfTimeStep()"
        is_first_sweep = "repositories::" + self.get_name_of_global_instance() + ".isFirstGridSweepOfTimeStep()"
        min_time_stamp = "repositories::" + self.get_name_of_global_instance() + ".getMinTimeStamp()"

        initial_prediction = self._name + "::SolverState::InitialPrediction"
        grid_initialisation = self._name + "::SolverState::GridInitialisation"
        plotting = self._name + "::SolverState::Plotting"
        prediction_on_hanging = self._name + "::SolverState::PredictionOnHangingCells"

        self.first_iteration_after_initialisation_guard = (
            "(" + solver_state + "==" + initial_prediction + " or "
            + "(" + solver_state + "==" + plotting + " and " + min_time_stamp + "==0.))"
        )
        self.initialisation_sweep_guard = (
            "(" + solver_state + "==" + grid_initialisation + ")"
        )

        send_condition = (
            "(" + is_first_sweep + ") or " +
            "(" + solver_state + "==" + prediction_on_hanging + ") or " +
            "(" + solver_state + "==" + plotting + " and " + min_time_stamp + "!=0.0 " +")"
        )

        send_condition_split_sweeps = (
            "(" + is_first_sweep + ") or " +
            "(" + solver_state + "==" + prediction_on_hanging + ")"
        )

        receive_condition = (
            "(" + solver_state + "==" + prediction_on_hanging + ") or" +
            "(" + is_last_sweep + ") or " +
            "(" + solver_state + "==" + plotting + " and " + min_time_stamp + "!=0.0 " +")"
        )

        receive_condition_split_sweeps = (
            "(" + solver_state + "==" + prediction_on_hanging + ") or" +
            "(" + is_last_sweep + ")"
        )

        self._rhs_estimates_projection.generator.send_condition = send_condition if not self._split_sweeps else send_condition_split_sweeps
        self._rhs_estimates_projection.generator.receive_and_merge_condition = receive_condition if not self._split_sweeps else receive_condition_split_sweeps

        self._flux_estimates_projection.generator.send_condition = send_condition if not self._split_sweeps else send_condition_split_sweeps
        self._flux_estimates_projection.generator.receive_and_merge_condition = receive_condition if not self._split_sweeps else receive_condition_split_sweeps

    def create_action_sets(self):
        super(SingleSweep, self).create_action_sets()

        self._action_set_prediction.guard = (
            self._store_cell_data_default_guard()
            + " and repositories::{}.isFirstGridSweepOfTimeStep()".format(
                self.get_name_of_global_instance()
            )
        )

        self._action_set_correction.cell_guard = (
            self._store_cell_data_default_guard()
            + " and repositories::{}.isLastGridSweepOfTimeStep()".format(
                self.get_name_of_global_instance()
            )
        )

        self._action_set_correction.face_guard = (
            self._store_cell_data_default_guard()
            + " and repositories::{}.isLastGridSweepOfTimeStep()".format(
                self.get_name_of_global_instance()
            )
            + " and not (fineGridFace"
            + self._name
            + "FaceLabel.getAboveHanging()[0] or fineGridFace"
            + self._name
            + "FaceLabel.getAboveHanging()[1])"
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

    def _delete_face_data_default_guard(self):
        return super(
            SingleSweep, self
        )._delete_face_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::PredictionOnHangingCells".format(
            self.get_name_of_global_instance(), self._name
        ) + " and (fineGridFace{}FaceLabel.getAboveHanging()[0] or fineGridFace{}FaceLabel.getAboveHanging()[1])".format(
            self._name, self._name
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

    def _init_dictionary_with_default_parameters(self, d):
        super(SingleSweep, self)._init_dictionary_with_default_parameters(d)
        d["USE_SEPARATE_SWEEPS"] = self._split_sweeps

    def set_implementation(
        self,
        initial_conditions=None,
        boundary_conditions=None,
        refinement_criterion=None,
        flux=None,
        ncp=None,
        max_eigenvalue=None,
        riemann_solver=None,
        source_term=None,
        diffusive_source_term=None,
        adjust_solution=None,
        material_parameters=None,
        number_of_point_sources=0,
        point_source=None,
        init_point_source_location=None,
        use_diffusive_flux=None,
    ):
        super(SingleSweep, self).set_implementation(
            initial_conditions=initial_conditions,
            boundary_conditions=boundary_conditions,
            refinement_criterion=refinement_criterion,
            flux=flux,
            ncp=ncp,
            max_eigenvalue=max_eigenvalue,
            riemann_solver=riemann_solver,
            source_term=source_term,
            diffusive_source_term=diffusive_source_term,
            adjust_solution=adjust_solution,
            material_parameters=material_parameters,
            number_of_point_sources=number_of_point_sources,
            point_source=point_source,
            init_point_source_location=init_point_source_location,
            use_diffusive_flux=use_diffusive_flux,
        )

        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        abstract_solver_declarations = AbstractSolverDeclarations(d)
        solver_declarations = SolverDeclarations(d)
        solver_definitions = SolverDefinitions(d)

        self._abstract_solver_user_declarations = (
            abstract_solver_declarations.create_abstract_solver_user_declarations()
        )
        self._abstract_solver_user_declarations += (
            self._time_stepping_code_snippets.create_abstract_solver_user_declarations()
        )
        self._abstract_solver_user_definitions = (
            self._time_stepping_code_snippets.create_abstract_solver_user_definitions()
        )
        self._solver_user_declarations = (
            solver_declarations.create_solver_user_declarations()
        )
        self._solver_user_definitions = (
            solver_definitions.create_solver_user_definitions()
        )

        self._compute_time_step_size = (
            self._time_stepping_code_snippets.create_compute_time_step_size()
        )
        self._compute_new_time_step_size = (
            self._time_stepping_code_snippets.create_compute_new_time_step_size()
        )
        self._start_time_step_implementation = (
            self._time_stepping_code_snippets.create_start_time_step_implementation()
        )
        self._finish_time_step_implementation = (
            self._time_stepping_code_snippets.create_finish_time_step_implementation()
        )
        self._constructor_implementation = (
            self._time_stepping_code_snippets.create_abstract_solver_constructor_statements()
        )

        self.create_action_sets()
