# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import dastgen2
import peano4.toolbox.blockstructured
import peano4.datamodel
import peano4.output
import exahype2.grid
import exahype2.solvers.aderdg.actionsets
from exahype2.solvers.aderdg.LagrangeBasis import GaussLegendreBasis, GaussLobattoBasis
from exahype2.solvers.PDETerms import PDETerms
from .kernels import get_face_overlap_merge_implementation
from .kernelgenerator import generate_aderdg_kernels, Configuration

import os
from abc import abstractmethod

from enum import IntEnum
import jinja2

"""
used to have a central way for all of this class' descendants to differentiate between gauss-legendre
and gauss-lobatto nodes, helps avoid having to constantly pass a string and ensure that this string is
the same everywhere.
"""


class Polynomials(IntEnum):
    Gauss_Legendre = (0,)
    Gauss_Lobatto = 1


"""
Denotes valid precisions for kernels and storage but also is supposed to allow a little more flexibility
in the naming. If a user wants to name it float because they're more used to C-based languages that's
fine but if they'd rather use single we allow it as well.
"""
PrecisionType = {
    "SP":     "float",
    "float":  "float",
    "single": "float",
    "fp32":   "float",
    "DP":     "double",
    "double": "double",
    "fp64":   "double",
    "quadruple":   "long double",
    "long double": "long double",
    "fp128":       "long double",
    "half_c++23":       "std::float16_t",
    "float16_t":        "std::float16_t",
    "std::float16_t":   "std::float16_t",
    "fp16":             "std::float16_t",
    "bfloat":           "std::bfloat16_t",
    "bfloat16_t":       "std::bfloat16_t",
    "std::bfloat16_t":  "std::bfloat16_t",
    "bf16":             "std::bfloat16_t"
}


class ADERDG(object):
    # initialization stores the key information about the solver and initializes a number of variables for later usage
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
        # name of the solver
        self._name = name

        """
        Polynomial order of the representation of the underlying function in each spatial dimension.
        The number of support points used to represent this polynomial is always the order + 1
        """
        self._order = order

        """
        self._unknowns and self._auxiliary_variables respectively hold the number of unknowns and 
        auxiliary variables in the equation to be computed. Unknowns are variables that change over
        time whereas auxiliary variables can be space-dependent but don't vary over time.
        These can be specified either as simple ints or by a dictionary
        (e.g.) unknowns = {'a': 1, 'b': 1, 'c': 3}
        in which the user specifies the multiplicity of the variable (the velocity has one component
        per dimension for example.)
        If they are specified by a dictionary then the code generates a "VariableShortcuts" file which
        allows the user to specify a variable by name and automatically maps this to the right position
        in an array for better legibility. Otherwise they must manually remember the position of each
        variable manually.

        use_var_shortcut is used to know whether or not the user passed their variables via a dict
        variable_names and variable_pos are used internally to remember the names and respective
        positions of variables if set by a dictionary.
        """
        self._use_var_shortcut = False
        self._variable_names = []
        self._variable_pos = [0]

        if type(unknowns) is dict:
            self._unknowns = sum(unknowns.values())
            self._variable_names += list(unknowns.keys())
            for var in list(unknowns.values()):
                self._variable_pos.append(var + self._variable_pos[-1])
            self._use_var_shortcut = True
        elif type(unknowns) is int:
            self._unknowns = unknowns
        else:
            raise Exception(
                "not a valid type for parameter unknowns, needs to be int or dictionary"
            )

        if type(auxiliary_variables) is dict:
            self._auxiliary_variables = sum(auxiliary_variables.values())
            self._variable_names += list(auxiliary_variables.keys())
            for var in list(auxiliary_variables.values()):
                self._variable_pos.append(var + self._variable_pos[-1])
            self._use_var_shortcut = True
        elif type(auxiliary_variables) is int:
            self._auxiliary_variables = auxiliary_variables
        else:
            raise Exception(
                "not a valid type for parameter auxiliary_variables, needs to be int or dictionary"
            )

        """
        Minimal and maximal accepted sizes for the cell, domain will be subdivided until the cell size is smaller
        than the minimal size but will never be subdivided to be smaller than the minimal size.
        """
        self._min_cell_h = min_cell_h
        self._max_cell_h = max_cell_h
        if min_cell_h > max_cell_h:
            raise Exception(
                "Error: min_cell_h ("
                + str(min_cell_h)
                + ") is bigger than max_cell_h ("
                + str(max_cell_h)
                + ")"
            )

        """
        for kernel optimizations, these all default to the "unoptimized" version but can be set using the function
        add_kernel_optimizations(). Some of these are currently unused but are left as placeholders because they
        exist in the kernelgenerator even though they haven't been connected.
        """
        self._is_linear   = False
        self._polynomials = Polynomials.Gauss_Legendre
        self._basis       = GaussLegendreBasis(order + 1)
        self._use_kernel_generator = True
        self._architecture  = "noarch"
        self._use_libxsmm     = False
        self._use_BLIS        = False
        self._use_Eigen       = False
        self._use_libxsmm_JIT = False
        self._use_CERK_Guess      = False
        self._use_vectorized_PDE  = False
        self._use_split_CK        = False
        self._predictor_recompute = False

        self._initialize_patches = False

        self._predictor_computation_precisions      = [PrecisionType["double"]]
        self._corrector_computation_precision       = PrecisionType["double"]
        self._solution_persistent_storage_precision = PrecisionType["double"]
        self._precompute_picard_precision = False
        self._use_half_precision          = False

        #determines whether to hold respectively the cells and faces on the stack or heap (with smart pointers)
        self._hold_cell_data_on_heap_with_smart_pointer = False
        self._hold_face_data_on_heap_with_smart_pointer = False

        """
        Hold previous time step: if this is specified as true, the previous time step will be stored persistently.
          The old value gets overwritten with the new value at the beginning of the predictor step. 
          This allows rollback of the solution to a previous state if for some reason the computations
          yield incorrect results.
          Used for limiting, where troubled cells are rolled back and recomputed with a more robust
          scheme and neighbouring cells need their old solution to project values with.
        """
        self._hold_previous_time_step           = False

        # whether or not to plot information about the grid as well as information about the content of the patches
        self._plot_grid_properties = plot_grid_properties
        self.plot_description = ""

        self.plot_metadata = ""
        self.select_dofs_to_print = None

        # used by children classes or setters in order to add constants or includes to the corresponding part
        self._solver_constants = ""
        self.user_action_set_includes = ""
        self.user_solver_includes = ""

        """
        these specify the time step, they must be set by child classes in order to advance the timestepping
        these are the main difference between child classes as they all use the same underlying routines
        """
        self._compute_time_step_size = "#error Not yet defined"
        self._compute_new_time_step_size = None

        """
        These are all the optional and mandatory plugins that can be specified by the user.
        eigenvalues, boundary conditions, refinement criteria and initial conditions must
        all be specified, all others are optional but can be added via the set_implementation() function.
        There are four choices for these, three of which are specified in PDETerms:
        User_Defined_Implementation creates empty stubs that the user must then manually fill.
        None_implementation leaves out this optional stub and ensures the kernels don't use this function.
        Empty_implementation does... the same thing as user_defined I think?
        The final option is to fill these with a string, in which case this string is directly taken as 
        c++ code and used as the implementation of this function. The user is trusted to provide code that
        compiles and runs properly, it is never checked.
        """
        self._flux_implementation           = PDETerms.None_Implementation
        self._ncp_implementation            = PDETerms.None_Implementation
        self._source_term_implementation    = PDETerms.None_Implementation
        self._point_sources_implementation  = PDETerms.None_Implementation
        self._eigenvalues_implementation    = PDETerms.None_Implementation
        self._material_param_implementation = PDETerms.None_Implementation

        self._refinement_criterion_implementation = PDETerms.None_Implementation
        self._initial_conditions_implementation   = PDETerms.None_Implementation
        self._boundary_conditions_implementation  = PDETerms.None_Implementation

        self._precision_criterion_implementation  = PDETerms.None_Implementation
        self._riemann_solver_implementation       = PDETerms.None_Implementation
        self._number_of_point_sources = 0

        """
        These will be used by children of this class to fill them with the declaration and definition of the
        above mandatory and optional functions.
        """
        self._abstract_solver_user_declarations = ""
        self._abstract_solver_user_definitions = ""
        self._solver_user_declarations = ""
        self._solver_user_definitions = ""

        """
        Used by child classes, will contain code to be executed at the start and end respectively of the
        time step. Typically specifies some timestep variable and advances the overall timestamp of the
        entire solver. 
        """
        self._start_time_step_implementation  = ""
        self._finish_time_step_implementation = ""

        self._constructor_implementation = ""
        self._destructor_implementation = ""

        """
        These are ADER-DG CFL factors, they help determine the maximal stable timestep size
        Up to order 4, computed via von Neumann analysis [1]; above, determined empirically by M. Dumbser)
        [1] M. Dumbser, D. S. Balsara, E. F. Toro, and C.-D. Munz, 
        ‘A unified framework for the construction of one-step finite volume and discontinuous Galerkin schemes on unstructured meshes’, Journal of Computational Physics, vol. 227, no. 18, pp. 8209–8253, Sep. 2008.
        they are necessary to compute stable timestep sizes as dt <= c*dx / (lambda_max*(2N+1)
        with c the CFL for given order, lambda_max the maximal eigenvalue, N the order and dx the minimal mesh distance.
        """
        self._cflAder = [
            1.0,
            0.33,
            0.17,
            0.1,
            0.069,
            0.045,
            0.038,
            0.03,
            0.02,
            0.015,
            0.008821428571428572,
            0.005235326086956522,
            0.0031307249999999996,
            0.0018842326388888888,
            0.001140285614224138,
            0.0006933672202620968,
        ]

        # safety factor for CFL (Courant-Friedrichs-Lewy) condition
        self._cflSafetyFactor = 0.0

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
        self._number_of_face_projections = 1

        #current time step contains the solution, projections are respectively the projection of the solution and of the fluxes on the faces as predicted by the space-time predictor
        self._current_time_step = peano4.datamodel.Patch(
            (self._order + 1, self._order + 1, self._order + 1),
            self._unknowns + self._auxiliary_variables,
            self._unknown_identifier(),
            self._solution_persistent_storage_precision,
        )
        self._previous_time_step = peano4.datamodel.Patch(
            (self._order + 1, self._order + 1, self._order + 1),
            self._unknowns + self._auxiliary_variables,
            self._unknown_identifier() + "_old",
            self._solution_persistent_storage_precision,
        )
        self._rhs_estimates_projection = peano4.datamodel.Patch(
            (2, self._order+1, self._order+1),
            self._unknowns + self._auxiliary_variables,
            self._unknown_identifier() + "Estimates",
            self._corrector_computation_precision,
        )
        self._flux_estimates_projection = peano4.datamodel.Patch(
            (2, self._order+1, self._order+1),
            self._unknowns,
            self._unknown_identifier() + "FluxEstimates",
            self._corrector_computation_precision,
        )

        #if user has specified that they want the data on the heap
        if self._hold_cell_data_on_heap_with_smart_pointer:
          self._current_time_step.generator         = peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
              self._current_time_step,
              self._solution_persistent_storage_precision
          )
          self._previous_time_step.generator            = peano4.datamodel.PatchToDoubleArrayOnHeap(
              self._previous_time_step,
              self._solution_persistent_storage_precision
          )
        if self._hold_face_data_on_heap_with_smart_pointer:
          self._rhs_estimates_projection.generator  = peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
              self._rhs_estimates_projection,
              self._corrector_computation_precision
          )
          self._flux_estimates_projection.generator = peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
              self._flux_estimates_projection,
              self._corrector_computation_precision
          )


        # custom merge implementation takes into account that the faces project such that the left cell projects to the left half of the face and the right cell projects to the right half
        self._rhs_estimates_projection.generator.merge_method_definition = (
            get_face_overlap_merge_implementation(self._rhs_estimates_projection)
        )
        self._rhs_estimates_projection.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h" 
"""

        self._flux_estimates_projection.generator.merge_method_definition = (
            get_face_overlap_merge_implementation(self._flux_estimates_projection)
        )
        self._flux_estimates_projection.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h" 
"""

        self._current_time_step.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
            self._provide_cell_data_to_compute_kernels_default_guard(),
            self._load_cell_data_default_guard(),
            self._store_cell_data_default_guard(),
        )

        self._previous_time_step.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
            self._provide_cell_data_to_compute_kernels_default_guard(),
            self._load_cell_data_default_guard(),
            self._store_cell_data_default_guard(),
        )

        self._rhs_estimates_projection.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
            self._provide_cell_data_to_compute_kernels_default_guard(),
            self._load_face_data_default_guard(),
            self._store_face_data_default_guard()
        )

        # get overwritten in subclasses to take into account when to send or receive faces
        self._rhs_estimates_projection.generator.send_condition = "false"
        self._rhs_estimates_projection.generator.receive_and_merge_condition = "false"

        self._flux_estimates_projection.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
            self._provide_face_data_to_compute_kernels_default_guard(),
            self._load_face_data_default_guard(),
            self._store_face_data_default_guard()
        )

        self._flux_estimates_projection.generator.send_condition = "false"
        self._flux_estimates_projection.generator.receive_and_merge_condition = "false"

        self._current_time_step.generator.includes += """
#include "../repositories/SolverRepository.h"
"""

        self._previous_time_step.generator.includes += """
#include "../repositories/SolverRepository.h"
"""

        self._rhs_estimates_projection.generator.includes += """
#include "../repositories/SolverRepository.h"
"""

        self._flux_estimates_projection.generator.includes += """
#include "../repositories/SolverRepository.h"
"""

        self._cell_label = exahype2.grid.create_cell_label(self._name)
        self._face_label = exahype2.grid.create_face_label(self._name)

    """
    Creates the action sets, these are actions tied to the grid that are executed at given points of 
    grid generation, construction or steps of the ader-dg solver. Peano handles grid structures that
    contain data as well as operations that are executed on that data. These are the latter.
    The guard here are the conditions for the execution of the operation, as in they are only executed
    if the guard evaluates to true.
    """

    @abstractmethod
    def create_action_sets(self):
        # sets initial conditions for the solution
        self._action_set_initial_conditions = (
            exahype2.solvers.aderdg.actionsets.InitialCondition(
                self, self._store_cell_data_default_guard(), "true"
            )
        )
        self._action_set_initial_conditions_for_grid_construction = (
            exahype2.solvers.aderdg.actionsets.InitialCondition(
                self, self._store_cell_data_default_guard(), "false"
            )
        )

        self._action_set_handle_boundary = (
            exahype2.solvers.aderdg.actionsets.HandleBoundary(
                self, self._store_boundary_data_default_guard()
            )
        )

        # handles adaptive mesh refinement
        self._action_set_AMR = exahype2.solvers.aderdg.actionsets.AdaptivityCriterion(
            solver=self,
            guard=self._store_cell_data_default_guard()
            + " and (repositories::"
            + self.get_name_of_global_instance()
            + ".isLastGridSweepOfTimeStep() or repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::GridInitialisation)",
            build_up_new_refinement_instructions=True,
            called_by_grid_construction=False,
            implement_previous_refinement_instructions=True
        )
        self._action_set_AMR_commit_without_further_analysis = (
            exahype2.solvers.aderdg.actionsets.AdaptivityCriterion(
                solver=self,
                guard=self._store_cell_data_default_guard(),
                build_up_new_refinement_instructions=False,
                implement_previous_refinement_instructions=True,
                called_by_grid_construction=False,
            )
        )
        self._action_set_AMR_throughout_grid_construction = (
            exahype2.solvers.aderdg.actionsets.AdaptivityCriterion(
                solver=self,
                guard=self._store_cell_data_default_guard(),
                build_up_new_refinement_instructions=True,
                implement_previous_refinement_instructions=True,
                called_by_grid_construction=True,
            )
        )

        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = exahype2.solvers.aderdg.actionsets.DynamicAMR(
            solver=self,
            clear_guard=self._clear_face_data_default_guard(),
            interpolate_guard=self._interpolate_face_data_default_guard(),
            restrict_guard=self._restrict_face_data_default_guard(),
        )
        if self._action_set_postprocess_solution == None:
            self._action_set_postprocess_solution = (
                exahype2.solvers.aderdg.actionsets.EmptyPostprocessSolution(solver=self)
            )

        # updates faces and cells (whether at boundary, refined etc.)
        self._action_set_update_face_label = exahype2.grid.UpdateFaceLabel(self._name)
        self._action_set_update_cell_label = exahype2.grid.UpdateCellLabel(self._name)

        # all the actual computation of the solution happens in these
        # predictor extrapolates the solution from time t onto time-nodes, then projects this
        #   extrapolated solution to the faces.
        # correction uses the extrapolated face solutions from the predictor and the neighboring
        #   cells to compute a solution at the next timestep
        self._action_set_prediction                   = exahype2.solvers.aderdg.actionsets.Prediction(
            self, self._store_cell_data_default_guard(), on_hanging_cells=False)
        self._action_set_prediction_on_hanging_cells  = exahype2.solvers.aderdg.actionsets.Prediction(
            self, self._store_cell_data_default_guard(), on_hanging_cells=True)
        self._action_set_correction                   = exahype2.solvers.aderdg.actionsets.Correction(
            self, self._store_cell_data_default_guard())

        self._action_set_update_face_label.descend_invocation_order = 0
        self._action_set_update_cell_label.descend_invocation_order = 0

        self._action_set_initial_conditions.descend_invocation_order = 1
        self._action_set_initial_conditions_for_grid_construction.descend_invocation_order = 1
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.descend_invocation_order = 1

        self._action_set_handle_boundary.descend_invocation_order = 1        
        self._action_set_prediction.descend_invocation_order = 1
        self._action_set_prediction_on_hanging_cells.descend_invocation_order = 1

        self._action_set_correction.descend_invocation_order = 2  # needs to happen after handle_boundary

        self._action_set_AMR.descend_invocation_order = 3
        self._action_set_AMR_commit_without_further_analysis.descend_invocation_order = 3
        self._action_set_AMR_commit_throughout_grid_construction.descend_invocation_order = 3
        self._action_set_postprocess_solution.descend_invocation_order = 3

    def create_readme_descriptor(self, domain_offset, domain_size):
        return (
            """ ExaHyPE 2 Ader-DG solver of order: """
            + str(self._order)
            + """
    with a domain size of: """
            + str(domain_size)
            + """ and a domain offset of: """
            + str(domain_offset)
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

    def add_user_solver_includes(self, value):
        """
        Add further includes to this property, if your solver requires some additional
        routines from other header files.
        """
        self.user_solver_includes += value

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
        return "instanceOf" + self._name

    def _get_default_includes(self):
        return """
#include "tarch/la/Vector.h"

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "repositories/SolverRepository.h"
""" + "#include <stdfloat>" if self._use_half_precision else ""


    """
    The following functions will be called by the peano4 project,
    they tell peano which of our generated data it should attach
    to each grid element and use.
    """

    def add_to_Peano4_datamodel(self, datamodel, verbose):
        """
        if verbose:
          print( "Patch data" )
          print( "----------" )
          print( str(self._current_time_step) )
          print( "Patch overlap data" )
          print( "----------" )
          print( str(self._current_time_step_projection) )
        """
        datamodel.add_cell(self._cell_label)
        datamodel.add_cell(self._current_time_step)
        if(self._hold_previous_time_step):
          datamodel.add_cell(self._previous_time_step)
        datamodel.add_face(self._face_label)
        datamodel.add_face(self._rhs_estimates_projection)
        datamodel.add_face(self._flux_estimates_projection)

    def add_use_data_statements_to_Peano4_solver_step(self, step):
        step.use_cell(self._cell_label)
        step.use_cell(self._current_time_step)
        if(self._hold_previous_time_step):
          step.use_cell(self._previous_time_step)
        step.use_face(self._face_label)
        step.use_face(self._rhs_estimates_projection)
        step.use_face(self._flux_estimates_projection)

    def add_actions_to_init_grid(self, step):
        step.add_action_set(self._action_set_initial_conditions)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)

    def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
        step.add_action_set(self._action_set_initial_conditions_for_grid_construction)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        if evaluate_refinement_criterion:
            step.add_action_set(self._action_set_AMR_throughout_grid_construction)
        else:
            step.add_action_set(self._action_set_AMR_commit_without_further_analysis)

    def add_actions_to_plot_solution(self, step, output_path):
        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)

        mapping = []
        for z in self._basis.quadrature_points():
            for y in self._basis.quadrature_points():
                for x in self._basis.quadrature_points():
                    mapping.append((x, y, z))

        # calls a generator which generates a standard plotter that uses the tarch library
        step.add_action_set(
            peano4.toolbox.blockstructured.PlotPatchesInPeanoBlockFormat(
                filename=output_path + "solution-" + self._name,
                patch=self._current_time_step,
                dataset_name=self._unknown_identifier(),
                description=self.plot_description,
                mapping=mapping,
                select_dofs=self.select_dofs_to_print,
                guard="repositories::plotFilter.plotPatch(marker) and "
                + self._load_cell_data_default_guard(),
                additional_includes="""
#include "exahype2/PlotFilter.h"
#include "../repositories/SolverRepository.h"
""",
                precision="PlotterPrecision",
                time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
                plot_cell_data=False,
                dataType=self._solution_persistent_storage_precision,
            )
        )

        if self._plot_grid_properties:
            step.add_action_set(
                peano4.toolbox.PlotGridInPeanoBlockFormat(
                    filename=output_path + "grid-" + self._name,
                    cell_unknown=None,
                    guard_guard="repositories::plotFilter.plotPatch(marker) and "
                    + self._load_cell_data_default_guard(),
                    additional_includes="""
#include "exahype2/PlotFilter.h"
#include "../repositories/SolverRepository.h"
""",
                    plot_cell_data=False,
                )
            )
        pass

    # these actions are executed during each time step
    def add_actions_to_perform_time_step(self, step):
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(
            self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement
        )

        step.add_action_set(self._action_set_prediction)
        step.add_action_set(self._action_set_prediction_on_hanging_cells)
        step.add_action_set(self._action_set_handle_boundary)
        step.add_action_set(self._action_set_correction)

        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)

    def set_plot_description(self, description):
        self.plot_description = description

    """
  This is only called if the generated kernels are used, it calls the kernelgenerator to produce all of the files that will be
  required for the requested aderdg implementation and then adds them to the makefile.
  """

    def generate_kernels(self, namespace, output, dimensions):
        if (not self._is_linear) and (
            self._material_param_implementation != PDETerms.None_Implementation
            or self._point_sources_implementation != PDETerms.None_Implementation
        ):
            raise Exception(
                "Material parameters and point sources are currently only available for linear generated kernels."
            )

        full_qualified_namespace = ""
        for i in namespace:
            full_qualified_namespace += i + "::"
        full_qualified_namespace += self._name

        is_linear = "linear" if self._is_linear else "nonlinear"

        generate_aderdg_kernels(
            full_qualified_namespace,
            is_linear,
            self._unknowns,
            self._auxiliary_variables,
            self._order,
            dimensions,
            useFlux=(
                True
                if self._flux_implementation != PDETerms.None_Implementation
                else False
            ),
            useFluxVect=False,
            useNCP=(
                True
                if self._ncp_implementation != PDETerms.None_Implementation
                else False
            ),
            useNCPVect=False,
            useSource=(
                True
                if self._source_term_implementation != PDETerms.None_Implementation
                else False
            ),
            useSourceVect=False,
            usePointSources=(
                self._number_of_point_sources
                if self._point_sources_implementation != PDETerms.None_Implementation
                else False
            ),
            useMaterialParam=(
                True
                if self._material_param_implementation != PDETerms.None_Implementation
                else False
            ),
            useMaterialParamVect=False,
            useGaussLobatto=(
                True if self._polynomials is Polynomials.Gauss_Lobatto else False
            ),
            useLibxsmm=self._use_libxsmm,
            useBLIS=self._use_BLIS,
            useEigen=self._use_Eigen,
            useLibxsmmJIT=self._use_libxsmm_JIT,
            useVectPDE=self._use_vectorized_PDE,
            architecture=self._architecture,
            useCERKGuess=self._use_CERK_Guess,
            useSplitCK=self._use_split_CK,
            predictorRecompute=self._predictor_recompute,
            predictorComputePrecisions=self._predictor_computation_precisions,
            predictorStoragePrecision=self._corrector_computation_precision,
            correctorComputePrecision=self._corrector_computation_precision,
            correctorStoragePrecision=self._solution_persistent_storage_precision,
            precomputePicardPrecision=self._precompute_picard_precision
        )

        """" 
        this is a very ugly fix that should be replaced asap ;)
        essentially, the libxsmm kernel generator generates faulty code if
        'noarch' is specified. This code works fine except that it contains a
        #ifdef which is missing its corresponding #endif.
        This code just inserts that missing endif
        """
        if self._use_libxsmm and self._architecture == "noarch":
            file = open("generated/kernels/aderdg/" + is_linear + "/asm_fstpvi.c", "r")
            content = file.read()
            file.close()
            file = open("generated/kernels/aderdg/" + is_linear + "/asm_fstpvi.c", "w")
            content = content.replace("__FILE__)\n", "__FILE__)\n#endif\n")
            file.write(content)
            file.close()

            file = open(
                "generated/kernels/aderdg/" + is_linear + "/asm_amrRoutines.c", "r"
            )
            content = file.read()
            file.close()
            file = open(
                "generated/kernels/aderdg/" + is_linear + "/asm_amrRoutines.c", "w"
            )
            content = content.replace("__FILE__)\n", "__FILE__)\n#endif\n")
            file.write(content)
            file.close()

        output.makefile.add_cpp_file(
            "generated/kernels/aderdg/" + is_linear + "/Quadrature.cpp", generated=True
        )
        output.makefile.add_cpp_file(
            "generated/kernels/aderdg/" + is_linear + "/DGMatrices.cpp", generated=True
        )
        output.makefile.add_cpp_file(
            "generated/kernels/aderdg/"
            + is_linear
            + "/fusedSpaceTimePredictorVolumeIntegral.cpp",
            generated=True,
        )
        if self._point_sources_implementation != PDETerms.None_Implementation:
            output.makefile.add_cpp_file(
                "generated/kernels/aderdg/"
                + is_linear
                + "/fusedSpaceTimePredictorVolumeIntegral_WithoutPS.cpp",
                generated=True,
            )
            output.makefile.add_cpp_file(
                "generated/kernels/aderdg/" + is_linear + "/deltaDistribution.cpp",
                generated=True,
            )
        output.makefile.add_cpp_file(
            "generated/kernels/aderdg/" + is_linear + "/riemannSolver.cpp",
            generated=True,
        )
        output.makefile.add_cpp_file(
            "generated/kernels/aderdg/" + is_linear + "/faceIntegral.cpp",
            generated=True,
        )
        output.makefile.add_cpp_file(
            "generated/kernels/aderdg/" + is_linear + "/solutionUpdate.cpp",
            generated=True,
        )
        output.makefile.add_cpp_file(
            "generated/kernels/aderdg/" + is_linear + "/boundaryConditions.cpp",
            generated=True,
        )

        if self._use_libxsmm:
            output.makefile.add_cpp_file(
                "generated/kernels/aderdg/" + is_linear + "/gemmsCPP.cpp",
                generated=True,
            )

    """
    This tells peano to add the solver files to the project. It generates any files that are not
    specifically cell or face data or grid actions such as the actionsets.
    Currently it generates solver implementation and header files as well as abstract solver
    implementation and header files.
    In addition to this if the user has specified that they would like to use shortcuts to refer
    to their variable (e.g., specified their unknowns or aux. variables through a dict) it generates
    a VariableShortcuts file which is then connected to the solver so that users can specify their
    variables through name.
    Finally, if the kernelgenerator option is chosen this calls generate_kernels() which then generates
    optimized aderdg kernels and adds these to the makefile.
    """
    def add_implementation_files_to_project(self, namespace, output, dimensions, subdirectory=""):
        """
        The ExaHyPE2 project will call this operation when it sets
        up the overall environment.

        This routine is typically not invoked by a user.

        output: peano4.output.Output
        """
        templatefile_prefix = os.path.dirname(os.path.realpath(__file__)) + "/"

        if self._solver_template_file_class_name is None:
            templatefile_prefix += self.__class__.__name__
        else:
            templatefile_prefix += self._solver_template_file_class_name

        if(subdirectory):
            subdirectory += "/"

        abstractHeaderDictionary = {}
        implementationDictionary = {}
        self._init_dictionary_with_default_parameters(abstractHeaderDictionary)
        self._init_dictionary_with_default_parameters(implementationDictionary)
        self.add_entries_to_text_replacement_dictionary(abstractHeaderDictionary)
        self.add_entries_to_text_replacement_dictionary(implementationDictionary)

        generated_abstract_header_file = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                templatefile_prefix + "Abstract.template.h",
                templatefile_prefix + "Abstract.template.cpp",
                "Abstract" + self._name,
                namespace,
                subdirectory + ".",
                abstractHeaderDictionary,
                True,
                True,
            )
        )
        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                templatefile_prefix + ".template.h",
                templatefile_prefix + ".template.cpp",
                self._name,
                namespace,
                subdirectory + ".",
                implementationDictionary,
                False,
                True,
            )
        )

        output.add(generated_abstract_header_file)
        output.add(generated_solver_files)
        output.makefile.add_h_file(subdirectory + "Abstract" + self._name + ".h", generated=True)
        output.makefile.add_h_file(subdirectory + self._name + ".h", generated=True)
        output.makefile.add_cpp_file(subdirectory + "Abstract" + self._name + ".cpp", generated=True)
        output.makefile.add_cpp_file(subdirectory + self._name + ".cpp", generated=True)

        if self._use_var_shortcut:
            generated_shortcut_file = peano4.output.Jinja2TemplatedHeaderFile(
                os.path.dirname(os.path.realpath(__file__))
                + "/"
                + "../VariableShortcuts.template.h",
                "VariableShortcuts",
                namespace,
                subdirectory + ".",
                implementationDictionary,
                True,
                True,
            )
            output.add(generated_shortcut_file)
            output.makefile.add_h_file(subdirectory + "VariableShortcuts.h", generated=True)

        if self._use_kernel_generator:
            self.generate_kernels(namespace, output, dimensions)
        elif (
            self._material_param_implementation != PDETerms.None_Implementation
            or self._point_sources_implementation != PDETerms.None_Implementation
        ):
            Exception(
                "Material parameters and point sources are only available with linear generated kernels."
            )

    def set_solver_constants(self, datastring):
        self._solver_constants = datastring

    def add_solver_constants(self, datastring):
        self._solver_constants += datastring

    @property
    def order(self):
        return (self._order)

    @property
    def unknowns(self):
        return self._unknowns

    @property
    def auxiliary_variables(self):
        return self._auxiliary_variables

    @abstractmethod
    def add_entries_to_text_replacement_dictionary(self, d):
        pass

    """
    Generates a dictionary of "words" that will later be used in various templates to fill these out by adding
    information from the solver or which was specified by the user.
    """
    def _init_dictionary_with_default_parameters(self, d):
        d["ORDER"]              = self._order
        d["SOLVER_INSTANCE"]    = self.get_name_of_global_instance()
        d["SOLVER_NAME"]        = self._name
        d["UNKNOWN_IDENTIFIER"] = self._unknown_identifier()
        d["NUMBER_OF_UNKNOWNS"]             = self._unknowns
        d["NUMBER_OF_AUXILIARY_VARIABLES"]  = self._auxiliary_variables

        d["CFL_SAFETY_FACTOR"]  = self._cflSafetyFactor
        d["CFL_ADER"]           = self._cflAder[self._order]  # /(2*(self._order+1)+1)

        d["USE_KERNEL_GENERATOR"] = self._use_kernel_generator
        d["USE_HALF_PRECISION"]   = self._use_half_precision
        d["IS_LINEAR"] = self._is_linear
        d["LINEARITY"] = "linear" if self._is_linear else "nonlinear"

        d["USE_GAUSS_LOBATTO"] = ( "true" if self._polynomials is Polynomials.Gauss_Lobatto else "false" )
        d["POLYNOMIAL_TYPE"]   = ( "lobatto" if self._polynomials is Polynomials.Gauss_Lobatto else "legendre" )

        d["SOLUTION_STORAGE_PRECISION"] = self._solution_persistent_storage_precision

        d["PREDICTOR_COMPUTATION_PRECISIONS"] = self._predictor_computation_precisions
        d["CORRECTOR_COMPUTATION_PRECISION"]  = self._corrector_computation_precision

        d["COMPUTATION_PRECISIONS"] = self._predictor_computation_precisions[:]
        if self._corrector_computation_precision not in d["COMPUTATION_PRECISIONS"]:
            d["COMPUTATION_PRECISIONS"].append(self._corrector_computation_precision)
        if self._precompute_picard_precision!=False and self._precompute_picard_precision not in d["COMPUTATION_PRECISIONS"]:
            d["COMPUTATION_PRECISIONS"].append(self._precompute_picard_precision)
        d["NUMBER_OF_PREDICTOR_PRECISIONS"] = len(d["PREDICTOR_COMPUTATION_PRECISIONS"])
        d["NUMBER_OF_PRECISIONS"] = len(d["COMPUTATION_PRECISIONS"])

        if self._min_cell_h > self._max_cell_h:
            raise Exception("min/max h are inconsistent")
        d["MAX_CELL_H"] = self._max_cell_h
        d["MIN_CELL_H"] = self._min_cell_h

        d["SOLVER_CONSTANTS"] = self._solver_constants
        d["SOLVER_INCLUDES"]  = self.get_user_solver_includes()

        d["BOUNDARY_CONDITIONS_IMPLEMENTATION"] = self._boundary_conditions_implementation
        d["REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
        d["INITIAL_CONDITIONS_IMPLEMENTATION"] = self._initial_conditions_implementation
        d["ADAPTIVE_PRECISION_IMPLEMENTATION"] = self._precision_criterion_implementation
        d["RIEMANN_SOLVER_IMPLEMENTATION"] = self._riemann_solver_implementation

        d["ABSTRACT_SOLVER_USER_DECLARATIONS"] = jinja2.Template(
            self._abstract_solver_user_declarations, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["ABSTRACT_SOLVER_USER_DEFINITIONS"] = jinja2.Template(
            self._abstract_solver_user_definitions, undefined=jinja2.DebugUndefined
        ).render(**d)
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
        d["DESTRUCTOR_IMPLEMENTATION"] = jinja2.Template(
            self._destructor_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["COMPUTE_TIME_STEP_SIZE"] = jinja2.Template(
            self._compute_time_step_size, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["COMPUTE_NEW_TIME_STEP_SIZE"] = jinja2.Template(
            self._compute_new_time_step_size, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["ARCHITECTURE_ALIGNMENT"] = Configuration.alignmentPerArchitectures[self._architecture]

        d["USE_FLUX"]                 = self._flux_implementation
        d["USE_NCP"]                  = self._ncp_implementation
        d["USE_SOURCE"]               = self._source_term_implementation
        d["USE_MATERIAL_PARAMETERS"]  = self._material_param_implementation
        d["USE_POINT_SOURCE"]         = self._point_sources_implementation
        d["NUMBER_OF_POINT_SOURCES"]  = self._number_of_point_sources

        d["USE_VARIABLE_SHORTCUT"]  = self._use_var_shortcut
        d["VARIABLE_NAMES"]         = self._variable_names
        d["VARIABLE_POSITIONS"]     = self._variable_pos

        self._basis.init_dictionary_with_default_parameters(d)

    def set_implementation(
        self,
        flux=None,
        ncp=None,
        eigenvalues=None,
        boundary_conditions=None,
        refinement_criterion=None,
        initial_conditions=None,
        source_term=None,
        material_parameters=None,
        point_source=0,
        additional_action_set_includes="",
        additional_user_includes="",
    ):
        """
        If you pass in User_Defined, then the generator will create C++ stubs
        that you have to befill manually. If you pass in None_Implementation, it
        will create nop, i.e. no implementation or defaults. Any other string
        is copied 1:1 into the implementation. If you pass in None, then the
        set value so far won't be overwritten.
        """
        if boundary_conditions is not None:
            self._boundary_conditions_implementation = boundary_conditions
        if refinement_criterion is not None:
            self._refinement_criterion_implementation = refinement_criterion
        if initial_conditions is not None:
            self._initial_conditions_implementation = initial_conditions

        if flux is not None:
            self._flux_implementation = flux
        if ncp is not None:
            self._ncp_implementation = ncp
        if eigenvalues is not None:
            self._eigenvalues_implementation = eigenvalues
        if source_term is not None:
            self._source_term_implementation = source_term
        if material_parameters is not None:
            self._material_param_implementation = material_parameters

        if type(point_source) != int:
            raise Exception(
                "point_source needs to be an integer, this determines the number of point sources that will be used."
            )

        if point_source > 0:
            self._point_sources_implementation = PDETerms.User_Defined_Implementation
            self._number_of_point_sources = point_source

        self.user_action_set_includes += additional_action_set_includes
        self.user_solver_includes += additional_user_includes

        self.create_data_structures()
        self.create_action_sets()

    """
    Various options that can be specified in order to optimize the generated code for different options.
    """

    def add_kernel_optimizations(
        self,
        is_linear=False,
        polynomials=Polynomials.Gauss_Legendre,
        use_kernel_generator=True,
        use_libxsmm=False,
        use_BLIS=False,
        use_Eigen=False,
        use_libxsmm_JIT=False,
        predictor_computation_precisions=["double"],
        corrector_computation_precision="double",
        solution_persistent_storage_precision="double",
        precompute_picard_precision=False,
        precision_criterion_implementation=PDETerms.None_Implementation,
        riemann_solver_implementation=PDETerms.None_Implementation,
        hold_cell_data_on_heap_with_smart_pointer = False,
        hold_face_data_on_heap_with_smart_pointer = False,
        use_vectorized_PDE=False,
        architecture="noarch",
        use_CERK_guess=False,
        use_split_CK=False,
        predictor_recompute=False,
        initialize_patches=False
    ):
        if polynomials is Polynomials.Gauss_Legendre:
            self._basis = GaussLegendreBasis(self._order + 1)
        elif polynomials is Polynomials.Gauss_Lobatto:
            self._basis = GaussLobattoBasis(self._order + 1)
        else:
            raise Exception(
                "No proper basis chosen: {}, valid options are Gauss_Legendre and Gauss_Lobatto nodes".format(
                    polynomials
                )
            )

        self._is_linear = is_linear
        self._polynomials = polynomials

        self._use_kernel_generator = use_kernel_generator
        self._use_libxsmm = use_libxsmm
        self._use_BLIS = use_BLIS
        self._use_Eigen = use_Eigen
        self._use_libxsmm_JIT = use_libxsmm_JIT

        precisions = [
            solution_persistent_storage_precision,
            corrector_computation_precision,
        ]
        precisions.extend(predictor_computation_precisions)
        if precompute_picard_precision!=False:
            precisions.append(precompute_picard_precision)
        for value in precisions:
            if value not in PrecisionType.keys():
                raise AssertionError(
                    "one of the chosen precisions was not a valid choice. valid choices are: "
                    + ", ".join(PrecisionType.keys())
                )
            if PrecisionType[value]=="std::float16_t" or PrecisionType[value]=="std::bfloat16_t":
              self._use_half_precision = True

        self._predictor_computation_precisions = []
        for precision in predictor_computation_precisions:
            if PrecisionType[precision] not in self._predictor_computation_precisions:
                self._predictor_computation_precisions.append(PrecisionType[precision])
        self._corrector_computation_precision = PrecisionType[
            corrector_computation_precision
        ]
        self._solution_persistent_storage_precision = PrecisionType[
            solution_persistent_storage_precision
        ]

        self._precompute_picard_precision = PrecisionType[
            precompute_picard_precision
        ] if precompute_picard_precision != False else False

        if precision_criterion_implementation is not PDETerms.None_Implementation:
            self._precision_criterion_implementation = (
                precision_criterion_implementation
            )
        if (
            len(self._predictor_computation_precisions) > 1
            and self._precision_criterion_implementation is PDETerms.None_Implementation
        ):
            self._precision_criterion_implementation = (
                PDETerms.User_Defined_Implementation
            )

        if riemann_solver_implementation is not PDETerms.None_Implementation:
            self._riemann_solver_implementation = riemann_solver_implementation

        #move data to heap if requested
        self._hold_cell_data_on_heap_with_smart_pointer = hold_cell_data_on_heap_with_smart_pointer
        self._hold_face_data_on_heap_with_smart_pointer = hold_face_data_on_heap_with_smart_pointer

        if use_CERK_guess or use_split_CK or use_vectorized_PDE or predictor_recompute:
            raise Exception(
                "The option you have chosen has not yet been implemented, it currently only exists in the code as a placeholder for future expansion"
            )

        self._use_vectorized_PDE = use_vectorized_PDE
        self._architecture = architecture
        self._use_CERK_Guess = use_CERK_guess
        self._use_split_CK = use_split_CK
        self._predictor_recompute = predictor_recompute

        self._initialize_patches = initialize_patches

        self.create_data_structures()
        self.create_action_sets()
