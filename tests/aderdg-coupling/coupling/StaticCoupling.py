# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

import jinja2
import os

import kernels

from abc import abstractmethod

from exahype2.solvers.PDETerms import PDETerms

from .actionsets import (SolveRiemannProblem, VerifyTroubledness)

class StaticCoupling(object):
    # Stores the solver information and initialises the variables the later steps read.
    def __init__(
        self,
        name,
        regularSolver,
        limitingSolver,
        limiting_criterion_implementation  = PDETerms.User_Defined_Implementation
    ):
        # Name of the limiting solver.
        self._name = name

        # The solver of the stable regions and the solver of the unstable regions.
        self._regular_solver = regularSolver
        self._limiter_solver = limitingSolver

        """
        A child class fills these with the declaration and the definition of the mandatory
        and the optional functions.
        """
        self._solver_user_declarations  = ""
        self._solver_user_definitions   = ""

        self.user_action_set_includes   = ""
        self.user_solver_includes       = ""

        """
        A child class fills these with the code the solver runs at the start and at the end of a
        time step. The code sets a time-step variable and advances the time stamp of the solver.
        """
        self._start_time_step_implementation = ""
        self._finish_time_step_implementation = ""
        self._print_time_step_implementation = ""

        self._constructor_implementation = ""

        self._physical_admissibility_criterion = limiting_criterion_implementation
        if (   self._physical_admissibility_criterion==PDETerms.None_Implementation
            or self._physical_admissibility_criterion==PDETerms.Empty_Implementation):
            raise ValueError("Limiting criterion cannot be none")

        # Creates the data structures and the action sets. A change to the solver regenerates them.
        self.create_data_structures()
        self.create_action_sets()

    """
    Creates the structures that the Peano grid holds in a cell or on a face.
    A structure is a Peano patch of a given size that carries persistent data and the
    conditions under which the grid merges, sends, receives, stores and loads it.
    """

    @abstractmethod
    def create_data_structures(self):
        """
        The face marker tells a neighbouring cell whether a cell is troubled, neighbours a
        troubled cell, or is fine.
        A troubled cell and a cell next to a troubled cell roll their solution back to the
        previous one and take one time step with an FV scheme that stays stable at a
        discontinuity.
        Both convert the DG representation of their previous solution to an FV representation
        and project it to their faces, so that the ranks exchange it.
        """
        variants = ["REGULAR","REGULAR_TO_LIMITER","LIMITER_TO_REGULAR","TROUBLED"]
        self._cell_label = exahype2.grid.create_cell_label(self._name)
        self._face_label = exahype2.grid.create_face_label(self._name)
        self._cell_label.add_attribute(peano4.datamodel.Enumeration( name="troubledMarker",      variants=variants, initval="TroubledMarker::REGULAR" ) )
        self._face_label.add_attribute(peano4.datamodel.Enumeration( name="troubledMarker",      variants=variants, initval="TroubledMarker::REGULAR" ) )

        self._face_label.merge_method_definition += "\n  setTroubledMarker( std::max( getTroubledMarker(), neighbour.getTroubledMarker() ) );"

        compute_time_step_size = """
    double timeStepSize = std::min( repositories::""" + self._regular_solver.get_name_of_global_instance() + """.getAdmissibleTimeStepSize(),
                                    repositories::""" + self._limiter_solver.get_name_of_global_instance() + """.getAdmissibleTimeStepSize());
"""

        self._limiter_solver._compute_time_step_size = compute_time_step_size
        self._regular_solver._compute_time_step_size = compute_time_step_size

    """
    Creates the action sets. An action set is an operation tied to the grid that runs at a given
    point of grid generation, of grid construction or of a step of the ADER-DG solver.
    Peano owns the grid structures that carry the data and the operations that run on that data;
    an action set is one of the latter.
    A guard is the condition of an operation: The operation runs where the guard evaluates to true.
    """

    @abstractmethod
    def create_action_sets(self):

        self._action_set_check_troubledness = VerifyTroubledness(self, use_PAC=True,
            guard = ("not marker.willBeRefined() and not marker.hasBeenRefined() and" +
                " repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation"
        ))

        self._action_set_copy_face_data = SolveRiemannProblem(self,
          guard = ("not marker.willBeRefined() and not marker.hasBeenRefined()"
              + " and repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::TimeStepping"
              + " and " + "repositories::" + self.get_name_of_global_instance() + ".isFirstGridSweepOfTimeStep()"),
        )

        """
        The troubledness check runs in the initialisation after the regular solver has
          computed its initial condition.
        The copy and the conversion between the regular and the limiter representation run
          before the first grid traversal reads them, so either at the beginning of the first
          grid traversal or at the end of the second one. Both a regular-to-limiter cell and
          a limiter-to-regular cell then hold both versions and project both to their faces.
        """
        self._action_set_check_troubledness.descend_invocation_order = 1000
        self._action_set_copy_face_data.descend_invocation_order = 10

        self._regular_solver._action_set_prediction.guard += (
            " and fineGridCell" + self._name + "CellLabel.getTroubledMarker()==celldata::" + self._name + "CellLabel::TroubledMarker::REGULAR"
        )

        self._regular_solver._action_set_correction.cell_guard += (
            " and fineGridCell" + self._name + "CellLabel.getTroubledMarker()==celldata::" + self._name + "CellLabel::TroubledMarker::REGULAR"
        )

        self._limiter_solver._action_set_prediction.guard += (
            " and fineGridCell" + self._name + "CellLabel.getTroubledMarker()==celldata::" + self._name + "CellLabel::TroubledMarker::TROUBLED"
        )

        self._limiter_solver._action_set_correction.cell_guard += (
            " and fineGridCell" + self._name + "CellLabel.getTroubledMarker()==celldata::" + self._name + "CellLabel::TroubledMarker::TROUBLED"
        )

        self._limiter_solver._action_set_correction.face_guard += (
            " and fineGridFace" + self._name + "FaceLabel.getTroubledMarker()>=facedata::" + self._name + "FaceLabel::TroubledMarker::LIMITER_TO_REGULAR"
        )

    def create_readme_descriptor(self, domain_offset, domain_size):
        return """ ExaHyPE 2 Limiting solver"""

    def get_user_action_set_includes(self):
        return self.user_action_set_includes

    def get_user_solver_includes(self):
        return self.user_solver_includes

    def add_user_action_set_includes(self, value):
        """
        Appends includes to this property. An action set that calls a routine of another
        header needs the include of that header here.
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
        return "InstanceOf" + self._name

    def _get_default_includes(self):
        return """
#include "tarch/la/Vector.h"

#include "peano4/Loop.h"

#include "repositories/SolverRepository.h"
"""

    """
  The peano4 project calls the following functions. They name the generated data that
  Peano attaches to a grid element and reads.
  """

    def __str__(self):
        result = (
            """
Name:                   """ + self._name + """
Type:                   """ + self.__class__.__name__
        )
        return result

    def add_to_Peano4_datamodel(self, datamodel):
        datamodel.add_cell(self._cell_label)
        datamodel.add_face(self._face_label)

    def add_use_data_statements_to_Peano4_solver_step(self, step):
        step.use_cell(self._cell_label)
        step.use_face(self._face_label)

    def add_actions_to_init_grid(self, step):
        step.add_action_set(self._action_set_check_troubledness)

    def add_actions_to_create_grid(self, step):
        pass

    def add_actions_to_plot_solution(self, step):
        pass

    # these actions are executed during each time step
    def add_actions_to_perform_time_step(self, step):
        step.add_action_set(self._action_set_copy_face_data)

    def _generate_kernels(self, namespace, output):
        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        kernels_namespace = namespace + ["kernels", self._name]
        kernels_output_path = "kernels/" + self._name
        kernels_templates_prefix = os.path.dirname(os.path.realpath(__file__)) + "/kernels/"

        if not os.path.exists(kernels_output_path):
            os.makedirs(kernels_output_path)

        kernels.Limiter(d).generate_kernels(
            kernels_namespace, output,
            kernels_templates_prefix, kernels_output_path,
            regular_solver_quadpoints = self._regular_solver._basis.quadrature_points(render=False),
            limiter_solver_quadpoints = self._limiter_solver._basis.quadrature_points(render=False)
        )

    """
    Adds the solver files to the Peano project. It generates the solver implementation, the
    solver header and the compute kernels.
    """
    def add_implementation_files_to_project(self, namespace, output, dimensions):
        """
        The ExaHyPE 2 project calls this operation when it sets up the environment.

        output: peano4.output.Output
        """

        header_dictionary = {}
        self._init_dictionary_with_default_parameters(header_dictionary)

        generated_abstract_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                os.path.dirname(os.path.realpath(__file__)) + "/StaticCouplingAbstract.template.h",
                os.path.dirname(os.path.realpath(__file__)) + "/StaticCouplingAbstract.template.cpp",
                "Abstract" + self._name,
                namespace,
                ".",
                header_dictionary,
                True
            )
        )

        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                os.path.dirname(os.path.realpath(__file__)) + "/StaticCoupling.template.h",
                os.path.dirname(os.path.realpath(__file__)) + "/StaticCoupling.template.cpp",
                self._name,
                namespace,
                ".",
                header_dictionary,
                True
            )
        )

        output.add(generated_abstract_files)
        output.makefile.add_cpp_file("Abstract" + self._name + ".cpp", generated=True)
        output.makefile.add_h_file("Abstract" + self._name + ".h", generated=True)

        output.add(generated_solver_files)
        output.makefile.add_cpp_file(self._name + ".cpp", generated=True)
        output.makefile.add_h_file(self._name + ".h", generated=True)

        self._generate_kernels(namespace, output)

    @abstractmethod
    def add_entries_to_text_replacement_dictionary(self, d):
        pass

    """
    Fills a dictionary of the keys that the templates read. A value comes either from the
    solver or from the user.
    """
    def _init_dictionary_with_default_parameters(self, d):

        d["SOLVER_INSTANCE"]         = self.get_name_of_global_instance()
        d["REGULAR_SOLVER_INSTANCE"] = self._regular_solver.get_name_of_global_instance()
        d["LIMITER_SOLVER_INSTANCE"] = self._limiter_solver.get_name_of_global_instance()

        d["SOLVER_NAME"]         = self._name
        d["REGULAR_SOLVER_NAME"] = self._regular_solver._name
        d["LIMITER_SOLVER_NAME"] = self._limiter_solver._name

        d["UNKNOWN_IDENTIFIER"] = self._unknown_identifier()
        d["REGULAR_SOLVER_UNKNOWN_IDENTIFIER"] = self._regular_solver._unknown_identifier()
        d["LIMITER_SOLVER_UNKNOWN_IDENTIFIER"] = self._limiter_solver._unknown_identifier()

        d["SOLVER_INCLUDES"] = self.get_user_solver_includes()

        d["REGULAR_SOLVER_ORDER"] = self._regular_solver._order
        d["LIMITER_SOLVER_ORDER"] = self._limiter_solver._order

        d["REGULAR_SOLVER_STORAGE_PRECISION"] = self._regular_solver._solution_persistent_storage_precision

        d["REGULAR_SOLVER_CORRECTION_PRECISION"] = self._regular_solver._corrector_computation_precision
        d["LIMITER_SOLVER_CORRECTION_PRECISION"] = self._limiter_solver._corrector_computation_precision

        d["NUMBER_OF_DOFS_PER_CELL_2D"] = (
            (self._regular_solver.unknowns + self._regular_solver.auxiliary_variables)
            * (self._regular_solver.order+1) * (self._regular_solver.order+1) )
        d["NUMBER_OF_DOFS_PER_CELL_3D"] = (
            (self._regular_solver.unknowns + self._regular_solver.auxiliary_variables)
            * (self._regular_solver.order+1) * (self._regular_solver.order+1) * (self._regular_solver.order+1) )

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
        d["PRINT_TIME_STEP_IMPLEMENTATION"] = jinja2.Template(
            self._print_time_step_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["CONSTRUCTOR_IMPLEMENTATION"] = jinja2.Template(
            self._constructor_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["ADMISSIBILITY_IMPLEMENTATION"]  = self._physical_admissibility_criterion
