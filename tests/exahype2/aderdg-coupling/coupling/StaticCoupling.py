# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import dastgen2
import peano4
import exahype2

import jinja2
import os

import kernels

from abc import abstractmethod

from exahype2.solvers.PDETerms import PDETerms

from .actionsets import ( SpreadLimiterStatus, SolveRiemannProblem, VerifyTroubledness)

class StaticCoupling(object):
    # initialization stores the key information about the solver and initializes a number of variables for later usage
    def __init__(
        self,
        name,
        regularSolver,
        limitingSolver,
        limiting_criterion_implementation  = PDETerms.User_Defined_Implementation
    ):


        # name of the limiting solver
        self._name = name

        # respectively solver to be used in stable regions and solver to be used in unstable regions
        self._regular_solver = regularSolver
        self._limiter_solver = limitingSolver

        """
        These will be used by children of this class to fill them with the declaration and definition of the
        above mandatory and optional functions.
        """
        self._solver_user_declarations  = ""
        self._solver_user_definitions   = ""

        self.user_action_set_includes   = ""
        self.user_solver_includes       = ""

        """
        Used by child classes, will contain code to be executed at the start and end respectively of the
        time step. Typically specifies some timestep variable and advances the overall timestamp of the
        entire solver. 
        """
        self._start_time_step_implementation = ""
        self._finish_time_step_implementation = ""
        self._print_time_step_implementation = ""

        self._constructor_implementation = ""

        self._physical_admissibility_criterion = limiting_criterion_implementation
        if (   self._physical_admissibility_criterion==PDETerms.None_Implementation
            or self._physical_admissibility_criterion==PDETerms.Empty_Implementation):
            raise Exception("Limiting criterion cannot be none")


        # create initial structures and action_sets, these are re-generated essentially anytime something about the solver is modified
        self.create_data_structures()
        self.create_action_sets()

    """
    Creates all the structures which are attached on the peano grid either in a cell or on the faces.
    These are defined as peano patches which have given sizes to contain persistent data as well as 
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
        Variants = ["REGULAR","REGULAR_TO_LIMITER","LIMITER_TO_REGULAR","TROUBLED"]
        self._cell_label = exahype2.grid.create_cell_label(self._name)
        self._face_label = exahype2.grid.create_face_label(self._name)
        self._cell_label.data.add_attribute(dastgen2.attributes.Enumeration( name="Troubled_Marker",      variants=Variants, initval="Troubled_Marker::REGULAR" ) )
        self._face_label.data.add_attribute(dastgen2.attributes.Enumeration( name="Troubled_Marker",      variants=Variants, initval="Troubled_Marker::REGULAR" ) )

        self._face_label.peano4_mpi_and_storage_aspect.merge_implementation += "\n  _Troubled_Marker = std::max(_Troubled_Marker, neighbour.getTroubled_Marker());"

        compute_time_step_size = """
    double timeStepSize = std::min( repositories::""" + self._regular_solver.get_name_of_global_instance() + """.getAdmissibleTimeStepSize(),
                                    repositories::""" + self._limiter_solver.get_name_of_global_instance() + """.getAdmissibleTimeStepSize());
"""

        self._limiter_solver._compute_time_step_size = compute_time_step_size
        self._regular_solver._compute_time_step_size = compute_time_step_size

    """
    Creates the action sets, these are actions tied to the grid that are executed at given points of 
    grid generation, construction or steps of the ader-dg solver. Peano handles grid structures that
    contain data as well as operations that are executed on that data. These are the latter.
    The guard here are the conditions for the execution of the operation, as in they are only executed
    if the guard evaluates to true.
    """

    @abstractmethod
    def create_action_sets(self):

        self._action_set_check_troubledness = VerifyTroubledness(self, use_PAC=True,
            guard = ("not marker.willBeRefined() and not marker.hasBeenRefined() and" +
                " repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation"
        ))

        # self._action_set_spread_limiter_status = SpreadLimiterStatus(self,
        #     guard = ("not marker.willBeRefined() and not marker.hasBeenRefined() and" +
        #         " repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::LimiterStatusSpreadingToNeighbours"
        # ))

        self._action_set_copy_face_data = SolveRiemannProblem(self,
          guard = ("not marker.willBeRefined() and not marker.hasBeenRefined()"
              + " and repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::TimeStepping"
              # + " and fineGridCell" + self._name + "CellLabel.getTroubled_Marker()==celldata::" + self._name + "CellLabel::Troubled_Marker::REGULAR"
              + " and " + "repositories::" + self.get_name_of_global_instance() + ".isFirstGridSweepOfTimeStep()"),
        )

        """
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
        self._action_set_check_troubledness.descend_invocation_order            = 1000
        # self._action_set_spread_limiter_status.descend_invocation_order         = 0
        self._action_set_copy_face_data.descend_invocation_order = 10

        self._regular_solver._action_set_prediction.guard += (
            " and fineGridCell" + self._name + "CellLabel.getTroubled_Marker()==celldata::" + self._name + "CellLabel::Troubled_Marker::REGULAR"
        )

        self._regular_solver._action_set_correction.guard += (
            " and fineGridCell" + self._name + "CellLabel.getTroubled_Marker()==celldata::" + self._name + "CellLabel::Troubled_Marker::REGULAR"
        )

        # self._regular_solver._action_set_correction.riemann_guard += (
        #     " and fineGridFace" + self._name + "FaceLabel.getTroubled_Marker()<=facedata::" + self._name + "FaceLabel::Troubled_Marker::LIMITER_TO_REGULAR"
        # )

        self._limiter_solver._action_set_prediction.guard += (
            " and fineGridCell" + self._name + "CellLabel.getTroubled_Marker()==celldata::" + self._name + "CellLabel::Troubled_Marker::TROUBLED"
        )

        self._limiter_solver._action_set_correction.guard += (
            " and fineGridCell" + self._name + "CellLabel.getTroubled_Marker()==celldata::" + self._name + "CellLabel::Troubled_Marker::TROUBLED"
        )

        self._limiter_solver._action_set_correction.face_guard += (
            " and fineGridFace" + self._name + "FaceLabel.getTroubled_Marker()>=facedata::" + self._name + "FaceLabel::Troubled_Marker::LIMITER_TO_REGULAR"
        )



    def create_readme_descriptor(self, domain_offset, domain_size):
        return (
            """ ExaHyPE 2 Limiting solver"""
        )

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
            + ".PeriodicBC[marker.getSelectedFaceNumber()%Dimensions]"
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
        return "InstanceOf" + self._name

    def _get_default_includes(self):
        return """
#include "tarch/la/Vector.h" 

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "repositories/SolverRepository.h"
"""


    """
  The following functions will be called by the peano4 project,
  they tell peano which of our generated data it should attach
  to each grid element and use.
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
        pass

    def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
        pass

    def add_actions_to_plot_solution(self, step, output_path):
        pass

    # these actions are executed during each time step
    def add_actions_to_perform_time_step(self, step):
        # step.add_action_set(self._action_set_spread_limiter_status)
        step.add_action_set(self._action_set_copy_face_data)

    def set_plot_description(self, description):
        self.plot_description = description

    def _generate_kernels(self, namespace, output, subdirectory, dimensions):
        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)
        # d["DIMENSIONS"] = dimensions
        # d["NUMBER_OF_DOF_3D"] = 1 if dimensions == 2 else d["NUMBER_OF_DOF"]
        # d["NUMBER_OF_DOF_LIMITER_3D"] = 1 if dimensions == 2 else d["NUMBER_OF_DOF_LIMITER"]
        # d["GHOST_LAYER_WIDTH_3D"] = 0 if dimensions == 2 else d["GHOST_LAYER_WIDTH"]
        # d["FULL_QUALIFIED_SOLVER_NAME"] ="::".join(namespace) + "::" + self._name

        kernels_namespace = namespace + ["kernels", self._name]
        kernels_output_path = subdirectory + "kernels/" + self._name
        kernels_templates_prefix = os.path.dirname(os.path.realpath(__file__)) + "/kernels/"

        if not os.path.exists(kernels_output_path):
           os.makedirs(kernels_output_path)

        kernels.Limiter(d).generate_kernels(
            kernels_namespace, output, subdirectory, 
            kernels_templates_prefix, kernels_output_path,
            regular_solver_quadpoints = self._regular_solver._basis.quadrature_points(render=False),
            limiter_solver_quadpoints = self._limiter_solver._basis.quadrature_points(render=False)
        )

    """
    This tells peano to add the solver files to the project. It generates any files that are not
    specifically cell or face data or grid actions such as the actionsets.
    Currently it generates solver implementation and header files.
    """
    def add_implementation_files_to_project(self, namespace, output, dimensions, subdirectory=""):
        """
        The ExaHyPE2 project will call this operation when it sets
        up the overall environment.

        This routine is typically not invoked by a user.

        output: peano4.output.Output
        """

        HeaderDictionary = {}
        self._init_dictionary_with_default_parameters(HeaderDictionary)

        generated_abstract_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                os.path.dirname(os.path.realpath(__file__)) + "/StaticCouplingAbstract.template.h",
                os.path.dirname(os.path.realpath(__file__)) + "/StaticCouplingAbstract.template.cpp",
                "Abstract" + self._name,
                namespace,
                subdirectory + ".",
                HeaderDictionary,
                True,
                True
            )
        )

        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                os.path.dirname(os.path.realpath(__file__)) + "/StaticCoupling.template.h",
                os.path.dirname(os.path.realpath(__file__)) + "/StaticCoupling.template.cpp",
                self._name,
                namespace,
                subdirectory + ".",
                HeaderDictionary,
                False,
                True
            )
        )

        output.add(generated_abstract_files)
        output.makefile.add_cpp_file(subdirectory + "Abstract" + self._name + ".cpp", generated=True)
        output.makefile.add_h_file(subdirectory + "Abstract" + self._name + ".h", generated=True)

        output.add(generated_solver_files)
        output.makefile.add_cpp_file(subdirectory + self._name + ".cpp", generated=True)
        output.makefile.add_h_file(subdirectory + self._name + ".h", generated=True)

        self._generate_kernels(namespace, output, subdirectory, dimensions)

    @abstractmethod
    def add_entries_to_text_replacement_dictionary(self, d):
        pass

    """
    Generates a dictionary of "words" that will later be used in various templates to fill these out by adding
    information from the solver or which was specified by the user.
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