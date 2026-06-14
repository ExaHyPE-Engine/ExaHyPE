# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
from abc import abstractmethod

import jinja2

import dastgen2
import peano4.output
import exahype2.grid
import exahype2.solvers.limiting.kernels

from exahype2.AlignmentHelper import AlignmentHelper

from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.aderdg import Polynomials
from exahype2.solvers.aderdg.ADERDG import get_face_overlap_merge_implementation

from .actionsets import (
    SaveNewCellData,
    SpreadLimiterStatus,
    CopyAndConvertPatch,
    VerifyTroubledness,
)


class PosterioriLimiting(object):
    # Initialisation stores the key information about the solver and initialises a number of variables for later usage
    def __init__(
        self,
        name,
        regular_solver,
        limiting_solver,
        number_of_dmp_observables=0,
        dmp_relaxation_parameter=0.0,
        dmp_differences_scaling=0.0,
        physical_admissibility_criterion=PDETerms.None_Implementation,
        compute_precision=None,
    ):
        self._name = name

        # Respectively solver to be used in stable regions and solver to be used in unstable regions
        self._regular_solver = regular_solver
        self._limiter_solver = limiting_solver

        """
        All of the following are mostly used for limiting, they are set of but some classes that inherit from this
        may turn them on.
          Hold previous time step: if this is specified as true, the previous time step will be stored persistently.
            The old value gets overwritten with the new value at the beginning of the predictor step.
            This allows rollback of the solution to a previous state if for some reason the computations
            yield incorrect results.
            Used for limiting, where troubled cells are rolled back and recomputed with a more robust
            scheme and neighbouring cells need their old solution to project values with.
          PAC: if not none, a user-defined physical admissibility criterion should be checked at the end of
            the correction step which verifies whether or not the produced solution is physically sensible.
          number_of_pac_observables: number of variables which the physical admissibility criterion should consider
          number_of_dmp_observables: if greater than zero, a discrete maximum principle should be checked at the
            end of the corrector step. This essentially verifies whether shocks have formed.
        """
        self._number_of_dmp_observables = number_of_dmp_observables
        self._physical_admissibility_criterion = physical_admissibility_criterion

        self._dmp_relaxation_parameter = dmp_relaxation_parameter
        self._dmp_differences_scaling = dmp_differences_scaling

        self._compute_precision = compute_precision

        self._halo_size = int(limiting_solver._patch_overlap_old.dim[0] / 2)

        self._solver_user_declarations = ""
        self._solver_user_definitions = ""

        self.user_action_set_includes = ""
        self.user_solver_includes = ""

        """
        Used by child classes, will contain code to be executed at the start and end respectively of the
        time step. Typically specifies some time step variable and advances the overall timestamp of the
        entire solver.
        """
        self._start_time_step_implementation = ""
        self._finish_time_step_implementation = ""

        self._constructor_implementation = ""

        """
        Used if the user wants to add some function to postprocessing, meaning
        it should be added after every other computation in ADER-DG is done.
        """
        self._action_set_postprocess_solution = None

        self._regular_solver._split_sweeps = True

        self.create_data_structures()
        self.create_action_sets()

    @abstractmethod
    def create_data_structures(self):
        """
        This will serve as a face marker to communicate with neighbouring cells whether
        a given cell is troubled, is neighbouring a troubled cell, or is fine.
        Cells that are troubled or neighbouring a troubled cell must rollback their solution
        to the previous one and perform one time step using a more robust FV scheme.
        Therefore these and their neighbours must convert the DG representation of their
        previous solution to an FV representation and project it to their faces so that these
        can be exchanged with their neighbours.
        """
        Variants = ["REGULAR", "REGULAR_TO_LIMITER", "LIMITER_TO_REGULAR", "TROUBLED"]
        self._cell_label = exahype2.grid.create_cell_label(self._name)
        self._face_label = exahype2.grid.create_face_label(self._name)
        self._cell_label.data.add_attribute(
            dastgen2.attributes.Enumeration(name="Troubled_Marker", variants=Variants)
        )
        self._face_label.data.add_attribute(
            dastgen2.attributes.Enumeration(name="Troubled_Marker", variants=Variants)
        )

        self._cell_label.data.add_attribute(
            dastgen2.attributes.Boolean(
                name="WasTroubledInPreviousStep", initval="false"
            )
        )

        self._face_label.peano4_mpi_and_storage_aspect.merge_implementation += "\n_Troubled_Marker = std::max(_Troubled_Marker, neighbour.getTroubled_Marker());"

        # This will contain the projection of the face-local minima and maxima
        self._min_and_max_projection = peano4.datamodel.Patch(
            (2, 2, 1),
            self._number_of_dmp_observables,
            self._unknown_identifier() + "MinAndMax",
        )

        self._min_and_max_projection.generator.merge_method_definition = (
            get_face_overlap_merge_implementation(self._min_and_max_projection)
        )

        self._min_and_max_projection.generator.can_receive = self._load_face_data_default_guard()
        self._min_and_max_projection.generator.can_send = self._store_face_data_default_guard()

        self._min_and_max_projection.generator.send_condition = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::RegularSolver"
            " and repositories::"
            + self._regular_solver.get_name_of_global_instance()
            + ".isFirstGridSweepOfTimeStep()"
        )
        self._min_and_max_projection.generator.receive_and_merge_condition = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::RegularSolver"
            " and repositories::"
            + self._regular_solver.get_name_of_global_instance()
            + ".isLastGridSweepOfTimeStep()"
        )

        self._min_and_max_projection.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h"
"""

        # We have to assume that the regular solver's time step size computation also considers troubled cells.
        # Therefore, the minimal time step size of the regular solver is unnecessary small.
        # Albeit mathematically correct, we compute the solution with a time step size that is way too small.
        # Use std::max to choose the more optimistic time step size (which is almost always the limiter's time step size).
        # TODO: Find a better way to compute the time step size without choosing a too small one.
        compute_time_step_size = (
            """
    const double timeStepSize = std::max(repositories::"""
            + self._regular_solver.get_name_of_global_instance()
            + """.getAdmissibleTimeStepSize(),
                                         repositories::"""
            + self._limiter_solver.get_name_of_global_instance()
            + """.getAdmissibleTimeStepSize());
"""
        )

        self._limiter_solver._compute_time_step_size = compute_time_step_size
        self._regular_solver._compute_time_step_size = compute_time_step_size

        self._regular_solver.create_data_structures()

    @abstractmethod
    def create_action_sets(self):
        self._action_set_save_new_cell_data = SaveNewCellData(
            self,
            guard=(
                "not marker.willBeRefined() and not marker.hasBeenRefined()"
                " and repositories::"
                + self.get_name_of_global_instance()
                + ".getSolverState()=="
                + self._name
                + "::SolverState::RegularSolver"
                + " and repositories::"
                + self._regular_solver.get_name_of_global_instance()
                + ".isFirstGridSweepOfTimeStep()"
            ),
            use_DMP=self._number_of_dmp_observables > 0,
            reset_troubled_markers=True,
            transmit_cell_timestamps=True,
        )

        self._action_set_check_troubledness = VerifyTroubledness(
            self,
            guard=(
                "not marker.willBeRefined() and not marker.hasBeenRefined()"
                + " and repositories::"
                + self.get_name_of_global_instance()
                + ".getSolverState()=="
                + self._name
                + "::SolverState::RegularSolver"
                + " and repositories::"
                + self._regular_solver.get_name_of_global_instance()
                + ".isLastGridSweepOfTimeStep()"
            ),
            use_PAC=self._physical_admissibility_criterion
            != PDETerms.None_Implementation,
            use_DMP=self._number_of_dmp_observables > 0,
        )

        self._action_set_spread_limiter_status = SpreadLimiterStatus(
            self,
            guard=(
                "not marker.willBeRefined() and not marker.hasBeenRefined()"
                + " and repositories::"
                + self.get_name_of_global_instance()
                + ".getSolverState()=="
                + self._name
                + "::SolverState::LimiterStatusSpreadingToNeighbours"
            ),
        )

        self._action_set_copy_convert_and_project_data = CopyAndConvertPatch(
            self,
            regularToLimiterGuard=(
                "not marker.willBeRefined() and not marker.hasBeenRefined()"
                + " and repositories::"
                + self.get_name_of_global_instance()
                + ".getSolverState()=="
                + self._name
                + "::SolverState::LimiterStatusSpreadingToSecondNeighbours"
                " and not fineGridCell"
                + self._name
                + "CellLabel.getWasTroubledInPreviousStep()"
            ),
            limiterToRegularGuard=(
                "not marker.willBeRefined() and not marker.hasBeenRefined()"
                + " and repositories::"
                + self.get_name_of_global_instance()
                + ".getSolverState()=="
                + self._name
                + "::SolverState::LimiterSolver"
                + " and repositories::"
                + self._limiter_solver.get_name_of_global_instance()
                + ".isLastGridSweepOfTimeStep()"
                + " and fineGridCell"
                + self._name
                + "CellLabel.getTroubled_Marker()>=celldata::"
                + self._name
                + "CellLabel::Troubled_Marker::LIMITER_TO_REGULAR"
            ),
        )

        """
        Saving cell needs to happen before the cell is used for any computations.
        Checking troubledness needs to happen after any updates have been finalised.
        Spreading limiter status is inconsequential, it can happen at any time since it has its own dedicated time step.
        Copying and converting the data from regular to limiter can happen whenever as it
        happens in a dedicated time step, it needs to be followed up by a projection of
        said data, which in the case of an FV limiter is done automatically.
        Copying from limiter to regular needs to happen after the limiter is finished
        computing the new solution within a given cell.
        """
        self._action_set_save_new_cell_data.descend_invocation_order = 0
        self._action_set_check_troubledness.descend_invocation_order = 1000
        self._action_set_spread_limiter_status.descend_invocation_order = 1
        self._action_set_copy_convert_and_project_data.descend_invocation_order = 1000

        # The limiter solver should be called to recompute TROUBLED cells and their neighbors LIMITER_TO_REGULAR based on the projections
        # that are sent from CopyAndConvertPatch action.
        update_patch_guard = (
            " and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::LimiterSolver"
            + " and fineGridCell"
            + self._name
            + "CellLabel.getTroubled_Marker()>=celldata::"
            + self._name
            + "CellLabel::Troubled_Marker::LIMITER_TO_REGULAR"
        )
        self._limiter_solver._action_set_update_patch.guard += update_patch_guard
        self._limiter_solver._action_set_reconstruct_patch.guard += update_patch_guard

    def create_readme_descriptor(self, domain_offset, domain_size):
        return """ ExaHyPE 2 Limiting solver"""

    def get_user_action_set_includes(self):
        return self.user_action_set_includes

    def get_user_solver_includes(self):
        return self.user_solver_includes

    def add_user_action_set_includes(self, value):
        self.user_action_set_includes += value

    def _store_cell_data_default_guard(self):
        return (
            "not marker.willBeRefined() "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction"
        )

    def _provide_cell_data_to_compute_kernels_default_guard(self):
        return (
            "not marker.willBeRefined() "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction"
        )

    def _provide_face_data_to_compute_kernels_default_guard(self):
        return (
            "not marker.willBeRefined() "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction"
        )

    def _load_cell_data_default_guard(self):
        return (
            "not marker.hasBeenRefined() "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridInitialisation"
        )

    def _store_face_data_default_guard(self):
        return (
            "not marker.willBeRefined() "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction"
        )

    def _load_face_data_default_guard(self):
        return (
            "not marker.hasBeenRefined() "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridInitialisation"
        )

    def _store_boundary_data_default_guard(self):
        return (
            self._store_face_data_default_guard()
            + " and not repositories::"
            + self.get_name_of_global_instance()
            + ".PeriodicBC[marker.getSelectedFaceNumber()%DIMENSIONS]"
            + " and not marker.hasBeenRefined() and fineGridFace"
            + self._name
            + "FaceLabel.getBoundary()"
        )

    def _clear_face_data_default_guard(self):
        return (
            "not marker.willBeRefined()"
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction "
        )

    def _interpolate_face_data_default_guard(self):
        return (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridInitialisation"
        )

    def _restrict_face_data_default_guard(self):
        return "true"

    def _unknown_identifier(self):
        return self._name + "Q"

    def get_name_of_global_instance(self):
        return "instanceOf" + self._name

    def _get_default_includes(self):
        return """
#include "tarch/la/Vector.h"

#include "peano4/Globals.h"
#include "peano4/utils/Loop.h"

#include "repositories/SolverRepository.h"
"""

    def add_to_Peano4_datamodel(self, datamodel):
        datamodel.add_cell(self._cell_label)
        datamodel.add_face(self._face_label)
        if self._number_of_dmp_observables > 0:
            datamodel.add_face(self._min_and_max_projection)

    def add_use_data_statements_to_Peano4_solver_step(self, step):
        step.use_cell(self._cell_label)
        step.use_face(self._face_label)
        if self._number_of_dmp_observables > 0:
            step.use_face(self._min_and_max_projection)

    def add_actions_to_init_grid(self, step):
        pass

    def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
        pass

    def add_actions_to_plot_solution(self, step, output_path):
        pass

    def add_actions_to_perform_time_step(self, step):
        step.add_action_set(self._action_set_save_new_cell_data)
        step.add_action_set(self._action_set_check_troubledness)
        step.add_action_set(self._action_set_spread_limiter_status)
        step.add_action_set(self._action_set_copy_convert_and_project_data)

    def _generate_kernels(self, namespace, d, output, subdirectory, dimensions):
        self.add_entries_to_text_replacement_dictionary(d)
        d["DIMENSIONS"] = dimensions
        d["NUMBER_OF_DOF_3D"] = 1 if dimensions == 2 else d["NUMBER_OF_DOF"]
        d["NUMBER_OF_DOF_LIMITER_3D"] = (
            1 if dimensions == 2 else d["NUMBER_OF_DOF_LIMITER"]
        )
        d["GHOST_LAYER_WIDTH_3D"] = 0 if dimensions == 2 else d["GHOST_LAYER_WIDTH"]
        d["FULL_QUALIFIED_SOLVER_NAME"] = "::".join(namespace) + "::" + self._name

        kernels_namespace = namespace + ["kernels", self._name]
        kernels_output_path = subdirectory + "kernels/" + self._name
        kernels_templates_prefix = (
            os.path.dirname(os.path.realpath(__file__)) + "/kernels/"
        )

        if not os.path.exists(kernels_output_path):
            os.makedirs(kernels_output_path)

        exahype2.solvers.limiting.kernels.Quadrature(d).generate_kernels(
            kernels_namespace,
            output,
            subdirectory,
            kernels_templates_prefix,
            kernels_output_path,
        )

        exahype2.solvers.limiting.kernels.Limiter(d).generate_kernels(
            kernels_namespace,
            output,
            subdirectory,
            kernels_templates_prefix,
            kernels_output_path,
        )

    def add_implementation_files_to_project(
        self, namespace, output, dimensions, subdirectory=""
    ):
        if subdirectory:
            subdirectory += "/"

        d = {}
        self._init_dictionary_with_default_parameters(d)

        generated_abstract_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                os.path.dirname(os.path.realpath(__file__))
                + "/PosterioriLimitingAbstract.template.h",
                os.path.dirname(os.path.realpath(__file__))
                + "/PosterioriLimitingAbstract.template.cpp",
                "Abstract" + self._name,
                namespace,
                subdirectory + ".",
                d,
                True,
                True,
            )
        )

        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                os.path.dirname(os.path.realpath(__file__))
                + "/PosterioriLimiting.template.h",
                os.path.dirname(os.path.realpath(__file__))
                + "/PosterioriLimiting.template.cpp",
                self._name,
                namespace,
                subdirectory + ".",
                d,
                False,
                True,
            )
        )

        output.add(generated_abstract_files)
        output.makefile.add_cpp_file(
            subdirectory + "Abstract" + self._name + ".cpp", generated=True
        )
        output.makefile.add_h_file(
            subdirectory + "Abstract" + self._name + ".h", generated=True
        )

        output.add(generated_solver_files)
        output.makefile.add_cpp_file(subdirectory + self._name + ".cpp", generated=True)
        output.makefile.add_h_file(subdirectory + self._name + ".h", generated=True)

        self._generate_kernels(namespace, d, output, subdirectory, dimensions)

    @abstractmethod
    def add_entries_to_text_replacement_dictionary(self, d):
        pass

    def _init_dictionary_with_default_parameters(self, d):
        d["SOLVER_INSTANCE"] = self.get_name_of_global_instance()
        d[
            "REGULAR_SOLVER_INSTANCE"
        ] = self._regular_solver.get_name_of_global_instance()
        d[
            "LIMITER_SOLVER_INSTANCE"
        ] = self._limiter_solver.get_name_of_global_instance()

        d["SOLVER_NAME"] = self._name
        d["REGULAR_SOLVER_NAME"] = self._regular_solver._name
        d["LIMITER_SOLVER_NAME"] = self._limiter_solver._name

        d["LIMITER_USE_ADJUST_SOLUTION"] = (
            self._limiter_solver._adjust_solution_implementation
            != PDETerms.None_Implementation
        )

        d["UNKNOWN_IDENTIFIER"] = self._unknown_identifier()
        d[
            "REGULAR_SOLVER_UNKNOWN_IDENTIFIER"
        ] = self._regular_solver._unknown_identifier()
        d[
            "LIMITER_SOLVER_UNKNOWN_IDENTIFIER"
        ] = self._limiter_solver._unknown_identifier()

        d[
            "REGULAR_SOLVER_STORAGE_PRECISION"
        ] = self._regular_solver._solution_persistent_storage_precision
        d["LIMITER_SOLVER_STORAGE_PRECISION"] = self._limiter_solver._storage_precision
        d["COMPUTE_PRECISION"] = (
            self._compute_precision
            if self._compute_precision is not None
            else self._regular_solver._solution_persistent_storage_precision
        )

        d["ORDER"] = self._regular_solver._order
        d["NUMBER_OF_DOF"] = self._regular_solver._order + 1
        d["NUMBER_OF_DOF_LIMITER"] = (
            self._limiter_solver.patch_size
            if self._limiter_solver.patch_size >= 0
            else 2 * self._regular_solver._order + 1
        )
        d["NUMBER_OF_UNKNOWNS"] = self._regular_solver.unknowns
        d["NUMBER_OF_AUXILIARY_VARIABLES"] = self._regular_solver.auxiliary_variables
        d["NUMBER_OF_DATA"] = (
            self._regular_solver.unknowns + self._regular_solver.auxiliary_variables
        )
        d["NUMBER_OF_OBSERVABLES"] = self._number_of_dmp_observables
        d["GHOST_LAYER_WIDTH"] = self._halo_size

        d["NUMBER_OF_DOFS_PER_CELL_2D"] = (
            (self._regular_solver.unknowns + self._regular_solver.auxiliary_variables)
            * (self._regular_solver.order + 1)
            * (self._regular_solver.order + 1)
        )
        d["NUMBER_OF_DOFS_PER_CELL_3D"] = (
            (self._regular_solver.unknowns + self._regular_solver.auxiliary_variables)
            * (self._regular_solver.order + 1)
            * (self._regular_solver.order + 1)
            * (self._regular_solver.order + 1)
        )

        alignment_helper = AlignmentHelper(self._regular_solver._simd_width)
        d["NUMBER_OF_DOF_PADDED"] = alignment_helper.get_size_with_padding(
            self._regular_solver._order + 1
        )
        d["NUMBER_OF_DOF_LIMITER_PADDED"] = alignment_helper.get_size_with_padding(
            d["NUMBER_OF_DOF_LIMITER"]
        )
        d["NUMBER_OF_UNKNOWNS_PADDED"] = alignment_helper.get_size_with_padding(
            self._regular_solver.unknowns
        )
        d["NUMBER_OF_AUXILIARY_VARIABLES_PADDED"] = alignment_helper.get_size_with_padding(
            self._regular_solver.auxiliary_variables
        )
        d["NUMBER_OF_DATA_PADDED"] = alignment_helper.get_size_with_padding(
            self._regular_solver.unknowns + self._regular_solver.auxiliary_variables
        )

        d["USE_GAUSS_LOBATTO"] = (
            "true"
            if self._regular_solver._polynomials is Polynomials.Gauss_Lobatto
            else "false"
        )
        d["POLYNOMIAL_TYPE"] = (
            "lobatto"
            if self._regular_solver._polynomials is Polynomials.Gauss_Lobatto
            else "legendre"
        )

        d["SOLVER_INCLUDES"] = self.get_user_solver_includes()

        d["SOLVER_USER_DECLARATIONS"] = jinja2.Template(
            self._solver_user_declarations, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["SOLVER_USER_DEFINITIONS"] = jinja2.Template(
            self._solver_user_definitions, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["START_TIME_STEP_IMPLEMENTATION"] = jinja2.Template(
            self._start_time_step_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["FINISH_TIME_STEP_IMPLEMENTATION"] = jinja2.Template(
            self._finish_time_step_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["CONSTRUCTOR_IMPLEMENTATION"] = jinja2.Template(
            self._constructor_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["DMP_DIFFERENCES_SCALING"] = self._dmp_differences_scaling
        d["DMP_RELAXATION_PARAMETER"] = self._dmp_relaxation_parameter
        d["NUMBER_OF_DMP_OBSERVABLES"] = self._number_of_dmp_observables
        d["ADMISSIBILITY_IMPLEMENTATION"] = self._physical_admissibility_criterion

        d["QUADRATURE_POINTS"] = ", ".join(
            self._regular_solver._basis.quadrature_points()
        )

        d["USE_LIBXSMM"] = self._regular_solver._use_libxsmm
