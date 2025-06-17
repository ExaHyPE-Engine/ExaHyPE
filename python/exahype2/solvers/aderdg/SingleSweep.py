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
        min_cell_h,
        max_cell_h,
        plot_grid_properties=False,
        split_sweeps=False,
    ):
        super(SingleSweep, self).__init__(
            name=name,
            order=order,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
            min_cell_h=min_cell_h,
            max_cell_h=max_cell_h,
            plot_grid_properties=plot_grid_properties,
        )

        self._solver_template_file_class_name = "SingleSweep"

        # If turned on it turns the solver from a single sweep to a separate sweeps solver,
        #   i.e. the predictor and corrector steps occur on separate traversals of the domain.
        #   this can be useful for debugging, or for some additional features such as limiting.
        self._split_sweeps = split_sweeps

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
            "repositories::"
            + self.get_name_of_global_instance()
            + ".isFirstGridSweepOfTimeStep() or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PredictionOnHangingCells or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Plotting"
        )

        self._rhs_estimates_projection.generator.receive_and_merge_condition = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PredictionOnHangingCells or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".isLastGridSweepOfTimeStep()"
        )

        self._flux_estimates_projection.generator.send_condition = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".isFirstGridSweepOfTimeStep() or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PredictionOnHangingCells or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Plotting"
        )

        self._flux_estimates_projection.generator.receive_and_merge_condition = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PredictionOnHangingCells or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".isLastGridSweepOfTimeStep()"
        )

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

    def _delete_face_data_default_guard(self):
        return super(
            SingleSweep, self
        )._delete_face_data_default_guard() + " and repositories::{}.isFirstGridSweepOfTimeStep()".format(
            self.get_name_of_global_instance()
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
        super(
            SingleSweep, self
        )._init_dictionary_with_default_parameters(d)
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
        init_point_source_location=None
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
            init_point_source_location=init_point_source_location
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
        self._print_time_step_implementation = (
            self._time_stepping_code_snippets.create_print_time_step_implementation()
        )
        self._constructor_implementation = (
            self._time_stepping_code_snippets.create_abstract_solver_constructor_statements()
        )

        self.create_action_sets()
