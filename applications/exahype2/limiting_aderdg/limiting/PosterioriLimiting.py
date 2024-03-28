# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import dastgen2
import peano4
import exahype2

import jinja2
import os

from abc import abstractmethod

from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.aderdg.kernels  import get_face_overlap_merge_implementation

from .actionsets import ( SaveNewCellData, SpreadLimiterStatus,
    CopyAndConvertPatch, VerifyTroubledness)
from exahype2.solvers.aderdg import Polynomials
from exahype2.solvers.aderdg.kernelgenerator import generate_limiter_kernels

class PosterioriLimiting(object):
    # initialization stores the key information about the solver and initializes a number of variables for later usage
    def __init__(
        self,
        name,
        regularSolver,
        limitingSolver,
        number_of_dmp_observables = 0,
        dmp_relaxation_parameter  = 0.0,
        dmp_differences_scaling   = 0.0,
        physical_admissibility_criterion  = PDETerms.None_Implementation
    ):


        # name of the limiting solver
        self._name = name

        # respectively solver to be used in stable regions and solver to be used in unstable regions
        self._regular_solver  = regularSolver
        self._limiter_solver = limitingSolver

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
          number_of_pac_observables: number of variables which the physical admisibility criterion should consider
          number_of_dmp_observables: if greater than zero, a discrete maximum principle should be checked at the
            end of the corrector step. This essentially verifies whether shocks have formed.
        """
        self._number_of_dmp_observables         = number_of_dmp_observables
        self._physical_admissibility_criterion  = physical_admissibility_criterion

        self._dmp_relaxation_parameter = dmp_relaxation_parameter
        self._dmp_differences_scaling  = dmp_differences_scaling

        self._halo_size = int(limitingSolver._patch_overlap_old.dim[0]/2)

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

        self._constructor_implementation = ""

        """
    used if the user wants to add some function to postprocessing, meaning
    it should be added after every other computation in ADER-DG is done.
    """
        self._action_set_postprocess_solution = None

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
        
        regular_patch_size = self._regular_solver.order+1  

        self._previous_time_step = peano4.datamodel.Patch(
            (regular_patch_size, regular_patch_size, regular_patch_size),
            self._regular_solver.unknowns + self._regular_solver.auxiliary_variables,
            self._unknown_identifier() + "_regularOld"
        )

        # if user has specified that they want the data on the heap
        # if self._hold_cell_data_on_heap_with_smart_pointer:
        #   self._previous_time_step.generator            = peano4.datamodel.PatchToDoubleArrayOnHeap(
        #       self._previous_time_step,
        #       self._solution_persistent_storage_precision
        #   )

        self._previous_time_step.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
            self._provide_cell_data_to_compute_kernels_default_guard(),
            self._load_cell_data_default_guard(),
            self._store_cell_data_default_guard(),
        )

        self._previous_time_step.generator.includes += """
#include "../repositories/SolverRepository.h"
"""

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
        self._cell_label.data.add_attribute(dastgen2.attributes.Enumeration( name="Troubled_Marker",      variants=Variants ) )
        self._face_label.data.add_attribute(dastgen2.attributes.Enumeration( name="Troubled_Marker",      variants=Variants ) )

        self._face_label.peano4_mpi_and_storage_aspect.merge_implementation += "\n  _Troubled_Marker = std::max(_Troubled_Marker, neighbour.getTroubled_Marker());"


        # This will contain the projection of the face-local minima and maxima
        self._min_and_max_projection = peano4.datamodel.Patch(
            (2, 2, 1),
            self._number_of_dmp_observables,
            self._unknown_identifier() + "_min_and_max"
        )
        # if self._hold_face_data_on_heap_with_smart_pointer:
        #   self._min_and_max_projection.generator  = peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
        #       self._min_and_max_projection,
        #       self._corrector_computation_precision
        # )
          
        self._min_and_max_projection.generator.merge_method_definition = get_face_overlap_merge_implementation(self._min_and_max_projection)

        self._min_and_max_projection.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
            self._provide_cell_data_to_compute_kernels_default_guard(),
            self._load_face_data_default_guard(),
            self._store_face_data_default_guard()
        )

        self._min_and_max_projection.generator.send_condition = (
            "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::RegularSolver"
            " and " + "repositories::" + self._regular_solver.get_name_of_global_instance() + ".isFirstGridSweepOfTimeStep()"
        )
        self._min_and_max_projection.generator.receive_and_merge_condition = (
            "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::RegularSolver"
            " and " + "repositories::" + self._regular_solver.get_name_of_global_instance() + ".isLastGridSweepOfTimeStep()"
        )

        self._min_and_max_projection.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h" 
"""

        #TODO: is this the best way to do this?
        self._limiter_solver._compute_time_step_size = """
    double timeStepSize = repositories::""" + self._regular_solver.get_name_of_global_instance() + """.getAdmissibleTimeStepSize();
"""


    """
    Creates the action sets, these are actions tied to the grid that are executed at given points of 
    grid generation, construction or steps of the ader-dg solver. Peano handles grid structures that
    contain data as well as operations that are executed on that data. These are the latter.
    The guard here are the conditions for the execution of the operation, as in they are only executed
    if the guard evaluates to true.
    """

    @abstractmethod
    def create_action_sets(self):

        self._action_set_save_new_cell_data = SaveNewCellData(self,
            guard = ("not marker.willBeRefined() and not marker.hasBeenRefined() and" +
              " repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::RegularSolver" +
              " and " + "repositories::" + self._regular_solver.get_name_of_global_instance() + ".isFirstGridSweepOfTimeStep()"),
            copy_old_values=True,
            use_dmp=True,
            reset_troubled_markers=True
        )

        self._action_set_check_troubledness = VerifyTroubledness(self,
            guard = ("not marker.willBeRefined() and not marker.hasBeenRefined() and" +
                " repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::RegularSolver" +
                " and " + "repositories::" + self._regular_solver.get_name_of_global_instance() + ".isLastGridSweepOfTimeStep()"),
            use_PAC=self._physical_admissibility_criterion!=PDETerms.None_Implementation,
            use_DMP=self._number_of_dmp_observables>0
        )

        self._action_set_spread_limiter_status = SpreadLimiterStatus(self,
            guard = ("not marker.willBeRefined() and not marker.hasBeenRefined() and" +
                " repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::LimiterStatusSpreadingToNeighbours"
        ))

        self._action_set_copy_convert_and_project_data = CopyAndConvertPatch(self,
            regularToLimiterGuard = ("not marker.willBeRefined() and not marker.hasBeenRefined() and" +
                " repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::LimiterStatusSpreadingToSecondNeighbours"),
            limiterToRegularGuard = ( "not marker.willBeRefined() and not marker.hasBeenRefined() and"
              + " repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::LimiterSolver"
              + " and " + "repositories::" + self._limiter_solver.get_name_of_global_instance() + ".isLastGridSweepOfTimeStep()"
              + " and fineGridCell" + self._name + "CellLabel.getTroubled_Marker()>=celldata::" + self._name + "CellLabel::Troubled_Marker::LIMITER_TO_REGULAR")
        )

        """
        saving cell needs to happen before the cell is used for any computations
        checking troubledness needs to happen after any updates have been finalized
        spreading limiter status is inconsequential, it can happen at any time since it has
          its own dedicated timestep
        copying and converting the data from regular to limiter can happen whenever as it
          happens in a dedicated timestep, it needs to be followed up by a projection of
          said data, which in the case of an fv limiter is done automatically
          copying from limiter to regular needs to happen after the limiter is finished
          computing the new solution within a given cell
        """
        self._action_set_save_new_cell_data.descend_invocation_order            = 0
        self._action_set_check_troubledness.descend_invocation_order            = 1000
        self._action_set_spread_limiter_status.descend_invocation_order         = 1
        self._action_set_copy_convert_and_project_data.descend_invocation_order = 1000

        self._limiter_solver._action_set_update_cell.guard += ( 
           " and repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::LimiterSolver"
         + " and fineGridCell" + self._name + "CellLabel.getTroubled_Marker()>=celldata::" + self._name + "CellLabel::Troubled_Marker::LIMITER_TO_REGULAR")


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
Name:                   """
            + self._name
            + """
Type:                   """
            + self.__class__.__name__
        )
        return result

    def add_to_Peano4_datamodel(self, datamodel, verbose):
      datamodel.add_cell(self._previous_time_step)
      datamodel.add_cell(self._cell_label)
      datamodel.add_face(self._face_label)
      datamodel.add_face(self._min_and_max_projection)

    def add_use_data_statements_to_Peano4_solver_step(self, step):
      step.use_cell(self._previous_time_step)
      step.use_cell(self._cell_label)
      step.use_face(self._face_label)
      step.use_face(self._min_and_max_projection)

    def add_actions_to_init_grid(self, step):
        pass

    def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
        pass

    def add_actions_to_plot_solution(self, step, output_path):
        pass

    # these actions are executed during each time step
    def add_actions_to_perform_time_step(self, step):
        step.add_action_set(self._action_set_save_new_cell_data)
        step.add_action_set(self._action_set_check_troubledness)
        step.add_action_set(self._action_set_spread_limiter_status)
        step.add_action_set(self._action_set_copy_convert_and_project_data)

    def set_plot_description(self, description):
        self.plot_description = description


    def generate_kernels(self, namespace, output, dimensions):
        full_qualified_namespace = ""
        for i in namespace:
            full_qualified_namespace += i + "::"
        full_qualified_namespace += self._name

        generate_limiter_kernels(
            full_qualified_namespace,
            self._regular_solver._unknowns,
            self._regular_solver._auxiliary_variables,
            self._regular_solver._order,
            dimensions,
            limPatchSize=self._limiter_solver.patch_size,
            numberOfObservable=self._number_of_dmp_observables,
            ghostLayerWidth=self._halo_size,
            useGaussLobatto=(
                True if self._regular_solver._polynomials is Polynomials.Gauss_Lobatto else False
            ),
            # (
            #   True if self._polynomials is Polynomials.Gauss_Lobatto else False
            # ),
            # useLibxsmm=self._use_libxsmm,
            # useBLIS=self._use_BLIS,
            # useEigen=self._use_Eigen,
            # useLibxsmmJIT=self._use_libxsmm_JIT,
            # architecture=self._architecture
        )

        output.makefile.add_cpp_file(
          "generated/kernels/limiter/fv/Quadrature.cpp",
          generated=True,
        )

        output.makefile.add_cpp_file(
          "generated/kernels/limiter/fv/limiter.cpp",
          generated=True,
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

        if(subdirectory):
            subdirectory += "/"

        HeaderDictionary = {}
        self._init_dictionary_with_default_parameters(HeaderDictionary)

        generated_abstract_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                os.path.dirname(os.path.realpath(__file__)) + "/PosterioriLimitingAbstract.template.h",
                os.path.dirname(os.path.realpath(__file__)) + "/PosterioriLimitingAbstract.template.cpp",
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
                os.path.dirname(os.path.realpath(__file__)) + "/PosterioriLimiting.template.h",
                os.path.dirname(os.path.realpath(__file__)) + "/PosterioriLimiting.template.cpp",
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

        self.generate_kernels(namespace, output, dimensions)

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

        d["LIMITER_PATCH_SIZE"] = self._limiter_solver.patch_size
        d["LIMITER_VARIABLES"]  = self._limiter_solver.unknowns + self._limiter_solver.auxiliary_variables

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

        d["CONSTRUCTOR_IMPLEMENTATION"] = jinja2.Template(
            self._constructor_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["DMP_DIFFERENCES_SCALING"]       = self._dmp_differences_scaling
        d["DMP_RELAXATION_PARAMETER"]      = self._dmp_relaxation_parameter
        d["NUMBER_OF_DMP_OBSERVABLES"]     = self._number_of_dmp_observables
        d["ADMISSIBILITY_IMPLEMENTATION"]  = self._physical_admissibility_criterion
