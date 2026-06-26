# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
from abc import abstractmethod

import jinja2

import peano4.output
import peano4.datamodel
import exahype2.grid
import exahype2.solvers.limiting.kernels

from exahype2.AlignmentHelper import AlignmentHelper

from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.aderdg import Polynomials

from .actionsets import (
    SaveNewCellData,
    SpreadLimiterStatus,
    CopyAndConvertPatch,
    VerifyTroubledness,
)


class StaticLimiting(object):
    # Initialisation stores the key information about the solver and initialises a number of variables for later usage
    def __init__(
        self,
        name,
        regular_solver,
        limiting_solver,
        physical_admissibility_criterion=PDETerms.User_Defined_Implementation,
        compute_precision=None,
    ):
        self._name = name

        # Respectively solver to be used in stable regions and solver to be used in unstable regions
        self._regular_solver = regular_solver
        self._limiter_solver = limiting_solver

        self._compute_precision = compute_precision

        self._halo_size = int(limiting_solver._patch_overlap_old.dim[0] / 2)

        """
        These will be used by children of this class to fill them with the declaration and definition of the
        above mandatory and optional functions.
        """
        self._solver_user_declarations = ""
        self._solver_user_definitions = ""

        self.user_action_set_includes = ""
        self.user_solver_includes = ""

        """
        Used by child classes, will contain code to be executed at the start and end respectively of the
        time step. Typically specifies some timestep variable and advances the overall timestamp of the
        entire solver.
        """
        self._start_time_step_implementation = ""
        self._finish_time_step_implementation = ""

        self._constructor_implementation = ""

        self._physical_admissibility_criterion = physical_admissibility_criterion
        if (
            self._physical_admissibility_criterion == PDETerms.None_Implementation
            or self._physical_admissibility_criterion == PDETerms.Empty_Implementation
        ):
            raise Exception("Limiting criterion cannot be none")

        # create initial structures and action_sets, these are re-generated essentially anytime something about the solver is modified
        self.create_data_structures()
        self.create_action_sets()

    """
    Creates all the structures which are attached on the Peano grid either in a cell or on the faces.
    These are defined as Peano patches which have given sizes to contain persistent data as well as
    conditions for when they should be merged, sent, received, stored, loaded, etc.
    """

    @abstractmethod
    def create_data_structures(self):
        """
        This will serve as a face marker to communicate with neighbouring cells whether
        a given cell is troubled, is neighbouring a troubled cell, or is fine.
        Cells that are troubled or neighbouring a troubled cell must rollback their solution
        to the previous one and perform one timestep using a more robust FV scheme.
        Therefore these and their neighbours must convert the DG representation of their
        previous solution to an FV representation and project it to their faces so that these
        can be exchanged with their neighbours.
        """
        Variants = ["REGULAR", "REGULAR_TO_LIMITER", "LIMITER_TO_REGULAR", "TROUBLED"]
        self._cell_label = exahype2.grid.create_cell_label(self._name)
        self._face_label = exahype2.grid.create_face_label(self._name)
        self._cell_label.add_attribute(
            peano4.datamodel.Enumeration(
                name="troubledMarker",
                variants=Variants,
                initval="TroubledMarker::REGULAR",
            )
        )
        self._face_label.add_attribute(
            peano4.datamodel.Enumeration(
                name="troubledMarker",
                variants=Variants,
                initval="TroubledMarker::REGULAR",
            )
        )

        self._face_label.merge_method_definition = (
            self._face_label.merge_method_definition
            + "\n  setTroubledMarker( std::max( getTroubledMarker(), neighbour.getTroubledMarker() ) );"
        )

        compute_time_step_size = (
            """
    double timeStepSize = std::min(repositories::"""
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

    """
    Creates the action sets, these are actions tied to the grid that are executed at given points of
    grid generation, construction or steps of the ADER-DG solver. Peano handles grid structures that
    contain data as well as operations that are executed on that data. These are the latter.
    The guard here are the conditions for the execution of the operation, as in they are only executed
    if the guard evaluates to true.
    """

    @abstractmethod
    def create_action_sets(self):
        self._action_set_save_new_cell_data = SaveNewCellData(
            self,
            guard=(
                "not marker.willBeRefined() and not marker.hasBeenRefined()"
                + " and repositories::"
                + self.get_name_of_global_instance()
                + ".getSolverState()=="
                + self._name
                + "::SolverState::TimeStep"
                + " and "
                + "repositories::"
                + self.get_name_of_global_instance()
                + ".isFirstGridSweepOfTimeStep()"
            ),
            transmit_cell_timestamps=True,
        )

        self._action_set_check_troubledness = VerifyTroubledness(
            self,
            use_PAC=True,
            guard=(
                "not marker.willBeRefined() and not marker.hasBeenRefined() and"
                + " repositories::"
                + self.get_name_of_global_instance()
                + ".getSolverState()=="
                + self._name
                + "::SolverState::GridInitialisation"
            ),
        )

        self._action_set_spread_limiter_status = SpreadLimiterStatus(
            self,
            guard=(
                "not marker.willBeRefined() and not marker.hasBeenRefined() and"
                + " repositories::"
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
                + "::SolverState::TimeStep"
                + " and fineGridCell"
                + self._name
                + "CellLabel.getTroubledMarker()<=celldata::"
                + self._name
                + "CellLabel::TroubledMarker::REGULAR_TO_LIMITER"
                + " and "
                + "repositories::"
                + self.get_name_of_global_instance()
                + ".isFirstGridSweepOfTimeStep()"
            ),
            limiterToRegularGuard=(
                "not marker.willBeRefined() and not marker.hasBeenRefined()"
                + " and "
                + "repositories::"
                + self.get_name_of_global_instance()
                + ".isLastGridSweepOfTimeStep()"
                + " and fineGridCell"
                + self._name
                + "CellLabel.getTroubledMarker()==celldata::"
                + self._name
                + "CellLabel::TroubledMarker::LIMITER_TO_REGULAR"
            ),
        )

        """
        Saving cell needs to happen before the cell is used for any computations
        checking troubledness needs to happen in the initialization after the regular solver
        has computed its initial condition.
        spreading limiter status happens once in its own dedicated step after initialization
          but before either solvers are allowed to start computing, and once in the first
          timestep before any other operations are performed
        copying and converting the data from regular to limiter and from limiter to regular
          happens before they are used in the first grid traversal, so either at the very
          beginning of the first grid traversal or at the end of the second, so that
          regular-to-limiter and limiter-to-regular cells both have access to either version
          and can therefore project both variants of their faces
        """
        self._action_set_save_new_cell_data.descend_invocation_order = 0
        self._action_set_check_troubledness.descend_invocation_order = 1000
        self._action_set_spread_limiter_status.descend_invocation_order = 0
        self._action_set_copy_convert_and_project_data.descend_invocation_order = 0

        self._regular_solver._action_set_prediction.guard += (
            " and fineGridCell"
            + self._name
            + "CellLabel.getTroubledMarker()<=celldata::"
            + self._name
            + "CellLabel::TroubledMarker::LIMITER_TO_REGULAR"
        )

        self._regular_solver._action_set_correction.cell_guard += (
            " and fineGridCell"
            + self._name
            + "CellLabel.getTroubledMarker()<=celldata::"
            + self._name
            + "CellLabel::TroubledMarker::REGULAR_TO_LIMITER"
        )

        self._regular_solver._action_set_correction.face_guard += (
            " and fineGridFace"
            + self._name
            + "FaceLabel.getTroubledMarker()<=facedata::"
            + self._name
            + "FaceLabel::TroubledMarker::LIMITER_TO_REGULAR"
        )

        update_patch_guard = (
            " and "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".isLastGridSweepOfTimeStep()"
            + " and fineGridCell"
            + self._name
            + "CellLabel.getTroubledMarker()>=celldata::"
            + self._name
            + "CellLabel::TroubledMarker::LIMITER_TO_REGULAR"
        )
        self._limiter_solver._action_set_update_patch.guard += update_patch_guard
        if hasattr(self._limiter_solver, "_action_set_reconstruct_patch"):
            self._limiter_solver._action_set_reconstruct_patch.guard += update_patch_guard

    def create_readme_descriptor(self, domain_offset, domain_size):
        return """ ExaHyPE 2 Limiting solver"""

    def get_user_action_set_includes(self):
        return self.user_action_set_includes

    def get_user_solver_includes(self):
        return self.user_solver_includes

    def add_user_action_set_includes(self, value):
        """
        Add further includes to this property, if your action sets require some additional
        routines from other header files.
        """
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
#include "peano4/Loop.h"

#include "repositories/SolverRepository.h"
"""

    """
    The following functions will be called by the Peano4 project,
    they tell Peano which of our generated data it should attach
    to each grid element and use.
    """

    def add_to_Peano4_datamodel(self, datamodel):
        datamodel.add_cell(self._cell_label)
        datamodel.add_face(self._face_label)

    def add_use_data_statements_to_Peano4_solver_step(self, step):
        step.use_cell(self._cell_label)
        step.use_face(self._face_label)

    def add_actions_to_init_grid(self, step):
        step.add_action_set(self._action_set_check_troubledness)
        pass

    def add_actions_to_create_grid(self, step):
        pass

    def add_actions_to_plot_solution(self, step, output_path):
        pass

    # These actions are executed during each time step
    def add_actions_to_perform_time_step(self, step):
        step.add_action_set(self._action_set_save_new_cell_data)
        step.add_action_set(self._action_set_spread_limiter_status)
        step.add_action_set(self._action_set_copy_convert_and_project_data)

    def _generate_kernels(self, namespace, d, output, dimensions):
        self.add_entries_to_text_replacement_dictionary(d)
        d["DIMENSIONS"] = dimensions
        d["NUMBER_OF_DOF_3D"] = 1 if dimensions == 2 else d["NUMBER_OF_DOF"]
        d["NUMBER_OF_DOF_LIMITER_3D"] = (
            1 if dimensions == 2 else d["NUMBER_OF_DOF_LIMITER"]
        )
        d["GHOST_LAYER_WIDTH_3D"] = 0 if dimensions == 2 else d["GHOST_LAYER_WIDTH"]
        d["FULL_QUALIFIED_SOLVER_NAME"] = "::".join(namespace) + "::" + self._name

        kernels_namespace = namespace + ["kernels", self._name]
        kernels_output_path = "kernels/" + self._name
        kernels_templates_prefix = (
            os.path.dirname(os.path.realpath(__file__)) + "/kernels/"
        )

        exahype2.solvers.limiting.kernels.Quadrature(d).generate_kernels(
            kernels_namespace,
            output,
            kernels_templates_prefix,
            kernels_output_path,
        )

        exahype2.solvers.limiting.kernels.Limiter(d).generate_kernels(
            kernels_namespace,
            output,
            kernels_templates_prefix,
            kernels_output_path,
        )

    def add_implementation_files_to_project(
        self, namespace, output, dimensions
    ):

        d = {}
        self._init_dictionary_with_default_parameters(d)

        generated_abstract_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                os.path.dirname(os.path.realpath(__file__))
                + "/StaticLimitingAbstract.template.h",
                os.path.dirname(os.path.realpath(__file__))
                + "/StaticLimitingAbstract.template.cpp",
                "Abstract" + self._name,
                namespace,
                ".",
                d,
                True,
            )
        )

        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                os.path.dirname(os.path.realpath(__file__))
                + "/StaticLimiting.template.h",
                os.path.dirname(os.path.realpath(__file__))
                + "/StaticLimiting.template.cpp",
                self._name,
                namespace,
                ".",
                d,
                True,
            )
        )

        output.add(generated_abstract_files)
        output.makefile.add_cpp_file(
            "Abstract" + self._name + ".cpp", generated=True
        )
        output.makefile.add_h_file(
            "Abstract" + self._name + ".h", generated=True
        )

        output.add(generated_solver_files)
        output.makefile.add_cpp_file(self._name + ".cpp", generated=True)
        output.makefile.add_h_file(self._name + ".h", generated=True)

        self._generate_kernels(namespace, d, output, dimensions)

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
            "REGULAR_SOLVER_CURRENT_UNKNOWN_IDENTIFIER"
        ] = self._regular_solver.current_unknown_identifier()
        d[
            "REGULAR_SOLVER_PREVIOUS_UNKNOWN_IDENTIFIER"
        ] = self._regular_solver.previous_unknown_identifier()
        d[
            "LIMITER_SOLVER_CURRENT_UNKNOWN_IDENTIFIER"
        ] = self._limiter_solver.current_unknown_identifier()
        d[
            "LIMITER_SOLVER_PREVIOUS_UNKNOWN_IDENTIFIER"
        ] = self._limiter_solver.previous_unknown_identifier()

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
        d["NUMBER_OF_OBSERVABLES"] = 0
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

        d["ADMISSIBILITY_IMPLEMENTATION"] = self._physical_admissibility_criterion

        d["QUADRATURE_POINTS"] = ", ".join(
            self._regular_solver._basis.quadrature_points()
        )

        d["USE_LIBXSMM"] = self._regular_solver._use_libxsmm
