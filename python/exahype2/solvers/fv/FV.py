# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
from abc import abstractmethod

import jinja2
import dastgen2

import peano4.output
import peano4.datamodel
import peano4.plotter
import peano4.toolbox.blockstructured

import exahype2.solvers.fv.actionsets

from exahype2.solvers import PDETerms
from exahype2.VariableShortcuts import VariableShortcuts
from peano4.output.Helper import render_jinja2_template

from peano4.Logger import log

class FV(object):
    """!
    Abstract finite volume solver step sizes that works on patch-based AMR with a halo layer of one.

    FV is the base class of all FV solvers. It defines what kind of action sets do
    exist, i.e., what in principle can be done while we run through the grid. It also
    provides some kind of very basic infrastructure, i.e., what the name of a solver
    is, what data is to be held per face or cell, or where in the multiscale mesh
    we actually have to hold data. The FV class cannot/should not be instantiated.

    A finite volume solver in ExaHyPE 2 (actually any solver) is first of all a
    static collection of data logically tied to grid entities and some operations
    over the mesh that are just there. All data and action sets however have guards,
    i.e., boolean guards that define

    - are data to be stored in-between traversals,
    - are action sets really invoked in a particular traversal or can they be
      skipped.

    In the baseline FV class, these guards are usually set to default values.
    That is, all action sets per time step are invoked always (the guard is
    true) and all data are always stored on the finest mesh. If you want to alter
    this behaviour, i.e., store data not always or skip steps, you can overwrite
    the corresponding attributes of the attributes, i.e., the guards of the
    associated data.

    See the discussion on "Control flow between this class and subclasses" below.

    ## Parallelisation

    I do equip both Q and NewQ with proper merge routines. However, all merge guards
    set to "never" by default. If you need some data exchange, you have to activate
    them manually.

    ## Control flow between this class and subclasses

    There are three key routines: the constructor, create_data_structures() and
    create_action_sets(). The constructor sets some global variables (such as the
    name) and then invokes the other two routines.

    create_data_structures() establishes all the data structures tied to the
    grid entities. It also sets some properties of these data such as the patch
    size, e.g. If you want to add additional data (such as additional quantities
    per cell) or if you want to alter the configuration of data tied to grid
    entities, you should redefine create_data_structures(). However, any
    subclass still should call FV's create_data_structures() - or the create_data_structures()
    of the SingleSweep. This will ensure that the
    baseline configuration of all data is in place. After that, you can modify
    the properties.

    create_action_sets() establishes the action sets, i.e. activities that are to
    be triggered whenever you run a time step, you plot, you initialise the grid.

    Both create_data_structures() and create_action_sets() add attributes to the
    FV class. See self._patch for example within create_action_sets(). These
    attributes have guards such as self._action_set_initial_conditions.guard.
    These guards are set to defaults in FV. It is basically the job of SingleSweep
    or any other subclass - they determine when which data are used - to reset these
    guards from a default to something tailored to the particular data flow.

    If you want to redefine when data is stored or operations are invoked, overwrite
    create_data_structures(), and call the superclass. This is what happens:

    - SingleSweep passes on the call to FV.create_data_structures().
      This call ensures that all the data structures are in place. Then it returns.
    - SingleSweep's create_data_structures() then sets the
      guard, i.e. they configure when data is stored or used.
    - Finally, your own overwrite of create_data_structures() can augment the
      data structures (which are now in place and called at the right time) with
      information what it actually shall do.

    ## Adaptive mesh refinement (AMR)

    We use by default a linear interpolation and averaging. For the linear interpolation,
    I do not compute the operators on demand. Instead, I use the optimised scheme which
    computes the operators once and then reuses them as static operation.

    If you wanna alter the inter-resolution transfer operators, please use

            self._interpolation = "tensor_product< " + self._name + ">"
            self.add_solver_constants( "" "static constexpr double  NormalInterpolationMatrix1d[]     = {
              1.0, 0.0
            };
            "" " )
            self.add_solver_constants( "" "static constexpr double  TangentialInterpolationMatrix1d[] = {
            1.0,0.0,0.0,0.0,
            1.0,0.0,0.0,0.0,
            1.0,0.0,0.0,0.0,
            0.0,1.0,0.0,0.0,

            0.0,1.0,0.0,0.0,
            0.0,1.0,0.0,0.0,
            0.0,0.0,1.0,0.0,
            0.0,0.0,1.0,0.0,

            0.0,0.0,1.0,0.0,
            0.0,0.0,0.0,1.0,
            0.0,0.0,0.0,1.0,
            0.0,0.0,0.0,1.0
            };"" " )

    The above snippet refers to a overlap of one and five unknowns. So the interpolation
    along a 1d tangential direction is three 5x5 matrices. Along the normal, we have to
    project onto one target element, so our projection matrix has one row. We have two
    values (inside and outside) along this normal, so two columns. In the standard
    enumeration scheme (refers to the face with normal 0), we have one real coarse
    grid value in this case. The other one would be the restricted value. We don't
    often use it (and therefore have a 0 here).

    ## Data postprocessing or coupling to other codes

    For data postprocessing, the introduction of fancy interior conditions or the coupling to
    other codes, each Finite Volume solver has an attribute

         _action_set_postprocess_solution

    You can use this attribute to add postprocessing routines to the code.
    """

    def __init__(
        self,
        name,
        patch_size,
        overlap,
        unknowns,
        auxiliary_variables,
        solvers_before_in_coupling: list=[],
        fuse_compute_kernels=False,
        baseline_action_set_descend_invocation_order=0,
    ):
        """!
        Create solver

        name: string
           A unique name for the solver. This one will be used for all generated
           classes. Also the C++ object instance later on will incorporate this
           name.

        patch_size: int
           Size of the patch in one dimension. All stuff here's dimension-generic.

        overlap: int
           That's the size of the halo layer which is half of the overlap with a
           neighbour. A value of 1 means that a patch_size x patch_size patch in
           2d is surrounded by one additional cell layer. The overlap has to be
           bigger or equal to one. It has to be smaller or equal to patch_size.

        unknowns: int
           Number of unknowns per Finite Volume voxel.

        auxiliary_variables: int
           Number of auxiliary variables per Finite Volume voxel. Eventually, both
           unknowns and auxiliary_variables are merged into one big vector if we
           work with AoS. But the solver has to be able to distinguish them, as
           only the unknowns are subject to a hyperbolic formulation.

        solvers_before_in_coupling: list[exahype2.solvers]
            List of solvers that come before this solver in a coupled simulation.
        """
        self._name = name

        self._patch_size = patch_size
        self._overlap = overlap

        self._solvers_before_in_coupling = solvers_before_in_coupling

        """!
        Adapted so that this solver class has an attribute of type exahype2.solver,
        which is supposed to be the last solver that is solving. Therefore, after it, all solvers
        reset themselves. This attribute should thus be the same for all instances of this class.
        """
        self._last_solver = None  # TODO: This can be done better. With Python we can easily index the last solver in the array above

        self._dimensions = 2
        self._namespace = ""

        self._plotter = peano4.plotter.HDF5PatchAMRPlotter
        self._variable_shortcuts = VariableShortcuts(name, unknowns, auxiliary_variables)

        self._particle_unknowns = []
        self._particle_variable_names = []
        self._particle_variable_pos = []
        self._use_var_shortcut_tracer = False
        self._tracer_names = []
        self._tracer_particles = []
        self._tracer_particle_sets = []
        self._sort_descend_invocation_order_within_action_sets = -1
        self._plot_descend_invocation_order_within_action_sets = 65536
        self._update_particle_association_action_sets = []
        self._update_particle_parallel_state_action_sets = []
        self._particle_attributes = []
        self._particle_number = []
        self._particle_plotters = []

        self._solver_constants = ""
        self._user_action_set_includes = ""
        self._user_solver_includes = ""

        self._solver_template_file_class_name = None
        self._templatefile_prefix = os.path.dirname(os.path.realpath(__file__)) + "/"

        self._action_set_initial_conditions = None
        self._action_set_AMR = None
        self._action_set_AMR_throughout_grid_construction = None
        self._action_set_AMR_commit_without_further_analysis = None
        self._action_set_handle_boundary = None
        self._action_set_project_patch_onto_faces = None
        self._action_set_roll_over_updates = None
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = None
        self._action_set_postprocess_solution = None
        self._action_set_preprocess_solution = None

        self._compute_time_step_size = "#error Not yet defined"
        self._compute_new_time_step_size = "#error Not yet defined"
        self._preprocess_reconstructed_patch = ""
        self._postprocess_updated_patch = ""

        self._compute_kernel_call = "#error Not yet defined"
        self._fused_compute_kernel_call = "#error Not yet defined"

        self._fuse_compute_kernels = fuse_compute_kernels

        self._initial_conditions_implementation = PDETerms.User_Defined_Implementation
        self._boundary_conditions_implementation = PDETerms.User_Defined_Implementation
        self._refinement_criterion_implementation = PDETerms.Empty_Implementation
        self._flux_implementation = PDETerms.None_Implementation
        self._ncp_implementation = PDETerms.None_Implementation
         # Can be none with fixed time stepping, but has to be defined for adaptive time stepping.
        self._max_eigenvalue_implementation = PDETerms.None_Implementation
        self._riemann_solver_implementation = PDETerms.None_Implementation
        self._source_term_implementation = PDETerms.None_Implementation
        self._diffusive_source_term_implementation = PDETerms.None_Implementation
        self._adjust_solution_implementation = PDETerms.None_Implementation
        self._limiter_implementation = PDETerms.None_Implementation

        self._use_stateless_initial_conditions = False
        self._use_stateless_boundary_conditions = False
        self._use_stateless_refinement_criterion = False  # TODO: Should not be stateless, as we might want to call helper functions such as getMinCellSize()
        self._use_stateless_flux = False
        self._use_stateless_ncp = False
        self._use_stateless_max_eigenvalue = False
        self._use_stateless_riemann_solver = False
        self._use_stateless_source_term = False
        self._use_stateless_diffusive_source_term = False
        self._use_stateless_adjust_solution = False
        self._use_stateless_limiter = False

        self._compute_eigenvalue = True
        self._time_stepping_code_snippets = None
        self._abstract_solver_user_declarations = ""
        self._abstract_solver_user_definitions = ""
        self._solver_user_declarations = ""
        self._solver_user_definitions = ""

        self._start_time_step_implementation = ""
        self._finish_time_step_implementation = ""
        self._constructor_implementation = ""
        self._start_simulation_implementation = ""
        self._finish_simulation_implementation = ""

        self._interpolation = "piecewise_constant"
        # must use inject, as we currently do not clear the half of the coarse face we restrict into
        self._restriction = "inject" # TODO: address this

        self._compute_precision = "double"
        self._storage_precision = "double"

        self._baseline_action_set_descend_invocation_order = baseline_action_set_descend_invocation_order

        self.create_data_structures()
        self.create_action_sets()

    def create_readme_descriptor(self, domain_offset, domain_size):
        return (
            """
### ExaHyPE 2 solver

"""
            + str(self)
            + """

Real type of this solver: """
            + str(type(self))
            + """

We assume that you use a domain size of (0,"""
            + str(domain_size)
            + """)^d. Peano 4 will cut this domain equidistantly
and recursively into three parts along each coordinate axis. This yields a spacetree.

ExaHyPE 2 embeds """
            + str(self._patch_size)
            + """^d Finite Volumes into each patch. The number of Peano grid levels
and additional AMR levels is configured by the executable at runtime.
"""
        )

    @property
    def user_action_set_includes(self):
        """
        Add further includes to this property, if your action sets require some additional
        routines from other header files.
        """
        return self._user_action_set_includes

    @property
    def user_solver_includes(self):
        """
        Add further includes to this property, if your solver requires some additional
        routines from other header files.
        """
        return self._user_solver_includes

    def add_user_action_set_includes(self, value):
        """
        Add further includes to this property, if your action sets require some additional
        routines from other header files.
        """
        self._user_action_set_includes += value

    def add_user_solver_includes(self, value):
        """
        Add further includes to this property, if your solver requires some additional
        routines from other header files.
        """
        self._user_solver_includes += value

    @abstractmethod
    def create_data_structures(self):
        """!

        Create data structures required by all Finite Volume solvers

        Recall in subclasses if you wanna change the number of unknowns
        or auxiliary variables. See class description's subsection on
        data flow.

        :: Call order and ownership

        This operation can be called multiple times. However, only the very
        last call matters. All previous calls are wiped out.

        If you have a hierarchy of solvers, every create_data_structures()
        should first(!) call its parent version. This way, you always ensure
        that all data are in place before you continue to alter the more
        specialised versions. So it is (logically) a top-down (general to
        specialised) run through all create_data_structures() variants
        within the inheritance tree.

        :: Arguments

        _patch: Patch (NxNxN)
          Actual patch data. We use Finite Volumes, so this is
          always the current snapshot, i.e. the valid data at one point.

        _patch_overlap_old, _patch_overlap_new: Patch (2xNxN)
          This is a copy/excerpt from the two adjacent finite volume
          snapshots plus the old data as backup. If I want to implement
          local timestepping, I don't have to backup the whole patch
          (see _patch), but I need a backup of the face data to be able
          to interpolate in time.
        """

        self._patch = peano4.datamodel.Patch(
            (self._patch_size, self._patch_size, self._patch_size),
            self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables,
            self.current_unknown_identifier(), self._storage_precision
        )
        self._patch_old = peano4.datamodel.Patch(
            (self._patch_size, self._patch_size, self._patch_size),
            self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables,
            self.previous_unknown_identifier(), self._storage_precision
        )
        self._patch_overlap_old = peano4.datamodel.Patch(
            (2 * self._overlap, self._patch_size, self._patch_size),
            self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables,
            self.previous_unknown_identifier(), self._storage_precision
        )
        self._patch_overlap_new = peano4.datamodel.Patch(
            (2 * self._overlap, self._patch_size, self._patch_size),
            self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables,
            self.current_unknown_identifier(), self._storage_precision
        )

        self._patch.generator = peano4.datamodel.PatchToData(
            self._patch, self._storage_precision
        )
        self._patch_old.generator = peano4.datamodel.PatchToData(
            self._patch_old, self._storage_precision
        )
        self._patch_overlap_old.generator = peano4.datamodel.PatchToData(
            self._patch_overlap_old, self._storage_precision
        )
        self._patch_overlap_new.generator = peano4.datamodel.PatchToData(
            self._patch_overlap_new, self._storage_precision
        )

        self._patch_overlap_old.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h"
"""
        self._patch_overlap_new.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h"
"""

        # Cell data is not sent/received
        self._patch.generator.send_condition = "false"
        self._patch.generator.receive_and_merge_condition = "false"
        self._patch.generator.can_receive = "false"
        self._patch.generator.can_send = "false"

        self._patch_old.generator.send_condition = "false"
        self._patch_old.generator.receive_and_merge_condition = "false"
        self._patch_old.generator.can_receive = "false"
        self._patch_old.generator.can_send = "false"

        self._patch_overlap_old.generator.send_condition = "false"
        self._patch_overlap_old.generator.receive_and_merge_condition = "false"
        self._patch_overlap_old.generator.can_receive = "false"
        self._patch_overlap_old.generator.can_send = "false"
        self._patch_overlap_old.generator.use_copy_for_exchange = "false"
        self._patch_overlap_old.generator.merge_method_definition = "*this = std::move(neighbour);"

        self._patch_overlap_new.generator.send_condition = "true"
        self._patch_overlap_new.generator.receive_and_merge_condition = "true"
        self._patch_overlap_new.generator.can_receive = self._load_face_data_default_guard()
        self._patch_overlap_new.generator.can_send = self._store_face_data_default_guard()
        self._patch_overlap_new.generator.use_copy_for_exchange = "false"
        self._patch_overlap_new.generator.merge_method_definition = "*this = std::move(neighbour);"

        self._patch.generator.includes += """
#include "../repositories/SolverRepository.h"
"""
        self._patch_old.generator.includes += """
#include "../repositories/SolverRepository.h"
"""
        self._patch_overlap_old.generator.includes += """
#include "../repositories/SolverRepository.h"
"""
        self._patch_overlap_new.generator.includes += """
#include "../repositories/SolverRepository.h"
"""

        self._cell_label = exahype2.grid.create_cell_label(self._name)
        self._face_label = exahype2.grid.create_face_label(self._name)

    @abstractmethod
    def create_action_sets(self):
        """!
        Create all the action sets

        Overwrite in subclasses if you wanna create different
        action sets. Subclasses also might redefine the invocation order.

        ## Call order and ownership

        This operation can be called multiple times. However, only the very
        last call matters. All previous calls are wiped out.

        If you have a hierarchy of solvers, every create_data_structures()
        should first(!) call its parent version. This way, you always ensure
        that all data are in place before you continue to alter the more
        specialised versions. So it is (logically) a top-down (general to
        specialised) run through all create_data_structures() variants
        within the inheritance tree.

        ## Recreation vs backup (state discussion)

        We faced some issues with action sets that should not be
        overwritten. For example, the postprocessing should not be overwritten
        as users might want to set it and then later on reset the number of
        unknowns, e.g. In this case, you would loose your postprocessing if
        create_action_sets() recreated them. So I decided to make an exception
        here: the postprocessing step is never overwritten by the standard
        classes.

        There are further action sets which have a state, which users might
        want to alter. The most prominent one is the AMR routines, where users
        often alter the interpolation and restriction scheme. Here, things are
        tricky: If we keep the default one, the action set would not pick up
        if you changed the number of unknowns, e.g. However, if we recreated
        the action set, we'd miss out on any changed interpolation/restriction
        scheme. Therefore, I have to hold the interpolation and restriction
        scheme separate.
        """
        self._action_set_initial_conditions = (
            exahype2.solvers.fv.actionsets.InitialCondition(
                self, self._store_cell_data_default_guard()
            )
        )
        self._action_set_AMR_throughout_grid_construction = exahype2.solvers.fv.actionsets.AdaptivityCriterion(
            solver=self,
            guard=self._store_cell_data_default_guard(),
            build_up_new_refinement_instructions=True,
            implement_previous_refinement_instructions=True,
            called_by_grid_construction=True,
        )
        self._action_set_AMR_commit_without_further_analysis = (
            exahype2.solvers.fv.actionsets.AdaptivityCriterion(
                solver=self,
                guard=self._store_cell_data_default_guard(),
                build_up_new_refinement_instructions=False,
                implement_previous_refinement_instructions=True,
                called_by_grid_construction=True,
            )
        )
        self._action_set_AMR = exahype2.solvers.fv.actionsets.AdaptivityCriterion(
            solver=self,
            guard=self._store_cell_data_default_guard(),
            build_up_new_refinement_instructions=True,
            implement_previous_refinement_instructions=True,
            called_by_grid_construction=False,
        )
        self._action_set_handle_boundary = (
            exahype2.solvers.fv.actionsets.HandleBoundary(
                self, self._store_cell_data_default_guard()
            )
        )
        self._action_set_project_patch_onto_faces = (
            exahype2.solvers.fv.actionsets.ProjectPatchOntoFaces(
                self, self._store_cell_data_default_guard()
            )
        )
        self._action_set_roll_over_updates = (
            exahype2.solvers.fv.actionsets.RollOverUpdates(
                self, self._store_face_data_default_guard(), self._store_cell_data_default_guard()
            )
        )
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = exahype2.solvers.fv.actionsets.DynamicAMR(
            solver=self,
            interpolation=self._interpolation,
            restriction=self._restriction,
        )

        # This one is different: it is the hook-in point for other solvers, so we create it
        # only once if it does not exist.
        if self._action_set_postprocess_solution == None:
            self._action_set_postprocess_solution = (
                exahype2.solvers.fv.actionsets.EmptyPostprocessSolution(self)
            )
        if self._action_set_preprocess_solution == None:
            self._action_set_preprocess_solution = (
                exahype2.solvers.fv.actionsets.EmptyPreprocessSolution(self)
            )

        self._action_set_update_face_label = exahype2.grid.UpdateFaceLabel(self._name)
        self._action_set_update_cell_label = exahype2.grid.UpdateCellLabel(self._name)

        self._action_set_update_patch = None

        self._action_set_initial_conditions.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_AMR.descend_invocation_order = self._baseline_action_set_descend_invocation_order + 2
        self._action_set_AMR_throughout_grid_construction.descend_invocation_order = self._baseline_action_set_descend_invocation_order + 2
        self._action_set_AMR_commit_without_further_analysis.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 2
        )
        self._action_set_handle_boundary.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_project_patch_onto_faces.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )  # touch cell last time, i.e. leave cell
        self._action_set_roll_over_updates.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_postprocess_solution.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )  # touch cell last time, i.e. leave cell
        self._action_set_preprocess_solution.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )

    def _provide_cell_data_to_compute_kernels_default_guard(self):
        """! Default logic when to create cell data or not

        ExaHyPE keeps patch load/store topology-driven. This predicate decides
        when cell data is provided to compute and exchange paths. Particular
        solvers use it as a starting point for more detailed rules, such as
        keeping the topology criterion but toggling data exchange depending on
        the solver phase.
        """
        return "not marker.willBeRefined() and repositories::{}.getSolverState()!={}::SolverState::GridConstruction".format(
                   self.get_name_of_global_instance(),
                   self._name
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

    def _store_cell_data_default_guard(self):
        """!
        Extend the guard via ands only. Never use an or, as subclasses might
        extend it as well, and they will append further ends.
        """
        return (
            "not marker.willBeRefined() "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction"
        )

    def _load_cell_data_default_guard(self):
        """!
        Extend the guard via ands only. Never use an or, as subclasses might
        extend it as well, and they will append further ends.
        """
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
        """!
        Extend the guard via ands only. Never use an or, as subclasses might
        extend it as well, and they will append further ends.
        """
        return (
            "not marker.willBeRefined() "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction"
        )

    def _load_face_data_default_guard(self):
        """!
        Extend the guard via ands only. Never use an or, as subclasses might
        extend it as well, and they will append further ends.
        """
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

    def _unknown_identifier(self):
        return self._name + "Q"

    def current_unknown_identifier(self):
        return self._name + "QNew"

    def previous_unknown_identifier(self):
        return self._name + "QOld"

    def get_name_of_global_instance(self):
        return "instanceOf" + self._name

    def add_to_Peano4_datamodel(self, datamodel):
        """
        Add all required data to the Peano project's datamodel
        so it is properly built up.
        """
        log.debug("Patch data")
        log.debug("----------")
        log.debug(str(self._patch))
        log.debug(str(self._patch_old))
        log.debug("----------")
        log.debug("Patch overlap data")
        log.debug(str(self._patch_overlap_old))
        log.debug(str(self._patch_overlap_new))
        log.debug("----------")
        datamodel.add_cell(self._cell_label)
        datamodel.add_cell(self._patch)
        datamodel.add_cell(self._patch_old)
        datamodel.add_face(self._patch_overlap_old)
        datamodel.add_face(self._patch_overlap_new)
        datamodel.add_face(self._face_label)

        if self._tracer_names:
            for particle in self._tracer_particles:
                datamodel.add_global_object(particle)
            for tracer_set in self._tracer_particle_sets:
                datamodel.add_vertex(tracer_set)

    def add_use_data_statements_to_Peano4_solver_step(self, step):
        """
        Tell Peano what data to move around

        Inform Peano4 step which data are to be moved around via the
        use_cell and use_face commands. This operation is generic from
        ExaHyPE's point of view, i.e. I use it for all grid sweep types.

        It is important that the cell label comes first, as some other
        data might make their load/store decisions depend on the label.
        """
        step.use_cell(self._patch)
        step.use_cell(self._patch_old)
        step.use_cell(self._cell_label)
        step.use_face(self._patch_overlap_old)
        step.use_face(self._patch_overlap_new)
        step.use_face(self._face_label)

    def _get_default_includes(self):
        return """
#include "tarch/la/Vector.h"

#include "peano4/Globals.h"
#include "peano4/utils/Loop.h"

#include "repositories/SolverRepository.h"
"""

    def add_actions_to_init_grid(self, step):
        """!
        Add all the action sets to init grid

        The AMR stuff has to be the very first thing. Actually, the AMR routines'
        interpolation doesn't play any role here. But the restriction indeed is
        very important, as we have to get the face data for BCs et al. The action
        set order is inverted while we ascend within the tree again. Therefore, we
        add the AMR action set first which means it will be called last when we go
        from fine to coarse levels within the tree.

        ## Ordering

        The order of the action sets is preserved throughout the steps down within
        the tree hierarchy. It is inverted throughout the backrolling.

        This is what we want to achieve:

        - Roll over QNew into QOld before the next update reads the old state.
          During the first solve sweep, this also backs up data produced by the
          initialisation sweep.
        - Project the initial condition onto faces. This happens in
          touchCellLastTime(). See
          exahype2.solvers.fv.actionsets.ProjectPatchOntoFaces for comments.
          The projection writes into QNew and marks the face for the first
          solve-sweep rollover.
        - Restrict the data to the coarser level if we are on a hanging face.
        - The postprocessing is the very last thing. It does not matter if it
          plugs into touchCellFirstTime() or touchCellLastTime(). As we invert the
          order when we backtrace, the postprocessing will have completed before
          we invoke _action_set_project_patch_onto_faces, i.e. any update will
          be projected onto the faces.
        """

        step.add_action_set(self._action_set_preprocess_solution)
        step.add_action_set(self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement)
        step.add_action_set(self._action_set_roll_over_updates)
        step.add_action_set(self._action_set_initial_conditions)
        step.add_action_set(self._action_set_project_patch_onto_faces)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)

        if self._tracer_names:
            for tracer_set in self._tracer_particle_sets:
                step.use_vertex(tracer_set)
            for action_set in self._update_particle_association_action_sets:
                step.add_action_set(action_set)
            for action_set in self._update_particle_parallel_state_action_sets:
                step.add_action_set(action_set)

    def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
        """
        The boundary information is set only once. It is therefore important that
        we use the face label and initialise it properly.
        """
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        if evaluate_refinement_criterion:
            step.add_action_set(self._action_set_AMR_throughout_grid_construction)
        else:
            step.add_action_set(self._action_set_AMR_commit_without_further_analysis)

        if self._tracer_particle_sets:
            for tracer_set in self._tracer_particle_sets:
                step.use_vertex(tracer_set)

    def add_actions_to_plot_solution(self, step, output_path):
        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        step.add_action_set(
            self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement
        )
        step.add_action_set(self._action_set_roll_over_updates)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_project_patch_onto_faces)

        plotter = self._plotter(
            output_path=output_path,
            filename=self._name,
            patch=self._patch,
            dataset_name=self.current_unknown_identifier(),
            description=self._variable_shortcuts.plot_description,
            time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
            guard=self._load_cell_data_default_guard(),
        )
        plotter.descend_invocation_order = self._baseline_action_set_descend_invocation_order
        step.add_action_set(plotter)

        for tracer_set, particle, particle_attr, particle_number, particle_unknown in zip(
            self._tracer_particle_sets,
            self._tracer_particles,
            self._particle_attributes,
            self._particle_number,
            self._particle_unknowns
        ):
            step.use_vertex(tracer_set)
            assoc_idx = self._tracer_particle_sets.index(tracer_set)
            plotter = peano4.plotter.HDF5ParticlePlotter(
                 output_path=output_path,
                 filename=particle.name,
                 particle_set=tracer_set,
                 description="description",
                 time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
             )
            # if particle_attr:
            #     plotter.add_attribute_to_plot(particle_attr, particle_unknown)
            # plotter.add_attribute_to_plot(
            #     particle_number, 2, "tarch::la::convertScalar<double>(p->getNumber())"
            # )
            plotter.descend_invocation_order = self._plot_descend_invocation_order_within_action_sets
            step.add_action_set(plotter)

            # These should correspond 1-to-1 with tracer sets

            step.add_action_set(self._update_particle_association_action_sets[assoc_idx])
            step.add_action_set(self._update_particle_parallel_state_action_sets[assoc_idx])

    def add_actions_to_perform_time_step(self, step):
        """!
        Add action sets to time step

        See exahype2.solvers.fv.FV.add_actions_to_init_grid() for details on the
        ordering of the action sets.

        ## AMR

        It is important that we do the inter-grid transfer operators before we
        apply the boundary conditions.
        """
        step.add_action_set(self._action_set_preprocess_solution)
        step.add_action_set(self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement)
        step.add_action_set(self._action_set_roll_over_updates)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_handle_boundary)
        step.add_action_set(self._action_set_update_patch)
        step.add_action_set(self._action_set_project_patch_onto_faces)
        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)

        if self._tracer_names:
            for tracer_set in self._tracer_particle_sets:
                step.use_vertex(tracer_set)
            for action_set in self._update_particle_association_action_sets:
                step.add_action_set(action_set)
            for action_set in self._update_particle_parallel_state_action_sets:
                step.add_action_set(action_set)

    @abstractmethod
    def add_entries_to_text_replacement_dictionary(self, d):
        pass

    def add_implementation_files_to_project(self, namespace, output, dimensions, subdirectory="", abstract_overwrite=True):
        self._dimensions = dimensions
        self._namespace = namespace

        if (
                self._riemann_solver_implementation == PDETerms.None_Implementation
            and self._max_eigenvalue_implementation == PDETerms.None_Implementation
        ):
            raise AssertionError(
                f"[{self._name}] max_eigenvalue implementation is required when no Riemann solver is used."
            )

        self._generate_kernels(output, subdirectory)
        self._action_set_handle_boundary.generate_kernels(namespace, output, dimensions, subdirectory)
        self._action_set_project_patch_onto_faces.generate_kernels(namespace, output, dimensions, subdirectory)

        if self._solver_template_file_class_name is None:
            self._templatefile_prefix += self.__class__.__name__
        else:
            self._templatefile_prefix += self._solver_template_file_class_name

        if subdirectory:
            subdirectory += "/"

        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        self._variable_shortcuts.generate_shortcut_file(d, namespace, output, subdirectory)

        header_file_abstr_template = self._templatefile_prefix + "Abstract.template.h"
        cpp_file_abstr_template = self._templatefile_prefix + "Abstract.template.cpp"
        header_file_template = self._templatefile_prefix + ".template.h"
        cpp_file_template = self._templatefile_prefix + ".template.cpp"

        generated_abstract_header_file = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                header_file_abstr_template,
                cpp_file_abstr_template,
                "Abstract" + self._name,
                namespace,
                subdirectory + ".",
                d,
                abstract_overwrite,
                True,
            )
        )
        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                header_file_template,
                cpp_file_template,
                self._name,
                namespace,
                subdirectory + ".",
                d,
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

    def set_solver_constants(self, datastring):
        self._solver_constants = datastring

    def add_solver_constants(self, datastring):
        self._solver_constants += datastring + "\n"

    def set_solver_precisions(self, storage_precision="double", compute_precision="double"):
      self._storage_precision = storage_precision
      self._compute_precision = compute_precision
      self.set_implementation()
      self.create_data_structures()

    def _init_dictionary_with_default_parameters(self, d):
        def render_template(template):
            return render_jinja2_template(template, d, jinja2.DebugUndefined)

        d["DIMENSIONS"] = self._dimensions
        d["NUMBER_OF_VOLUMES_PER_AXIS"] = self._patch.dim[0]
        d["OVERLAP"] = self._overlap
        d["HALO_SIZE"] = int(self._patch_overlap_old.dim[0] / 2)
        d["NAMESPACE"] = self._namespace
        d["SOLVER_NAME"] = self._name
        d["FULL_QUALIFIED_SOLVER_NAME"] = "::".join(self._namespace) + "::" + self._name
        d["SOLVER_INSTANCE"] = self.get_name_of_global_instance()
        d["UNKNOWN_IDENTIFIER"] = self._unknown_identifier()
        d["NUMBER_OF_UNKNOWNS"] = self._variable_shortcuts.unknowns
        d["NUMBER_OF_AUXILIARY_VARIABLES"] = self._variable_shortcuts.auxiliary_variables
        d["NUMBER_OF_DATA"] = self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables

        d["COMPUTE_PRECISION"] = self._compute_precision
        d["STORAGE_PRECISION"] = self._storage_precision

        d["PATCH_CARDINALITY_1D"] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_DATA"]
        d["PATCH_CARDINALITY_2D"] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["PATCH_CARDINALITY_1D"]
        d["PATCH_CARDINALITY_3D"] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["PATCH_CARDINALITY_2D"]

        d["FACE_CARDINALITY_2D"] = (
            int(self._patch_overlap_new.no_of_unknowns * self._patch_overlap_new.dim[0] * self._patch_overlap_new.dim[1])
        )
        d["FACE_CARDINALITY_3D"] = (
            int(self._patch_overlap_new.no_of_unknowns * self._patch_overlap_new.dim[0] * self._patch_overlap_new.dim[1] * self._patch_overlap_new.dim[2])
        )

        d["SOLVER_CONSTANTS"] = self._solver_constants
        d["SOLVER_INCLUDES"] = self.user_solver_includes

        d["SOLVERS_BEFORE_IN_COUPLING"] = [(solver.get_name_of_global_instance(), solver.name) for solver in self._solvers_before_in_coupling]
        if self._last_solver is None:
            self._last_solver = self
        d["LAST_SOLVER"] = (self._last_solver.get_name_of_global_instance(), self._last_solver.name)

        d["INITIAL_CONDITIONS_IMPLEMENTATION"] = self._initial_conditions_implementation
        d["BOUNDARY_CONDITIONS_IMPLEMENTATION"] = self._boundary_conditions_implementation
        d["REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
        d["FLUX_IMPLEMENTATION"] = self._flux_implementation
        d["NCP_IMPLEMENTATION"] = self._ncp_implementation
        d["MAX_EIGENVALUE_IMPLEMENTATION"] = self._max_eigenvalue_implementation
        d["RIEMANN_SOLVER_IMPLEMENTATION"] = self._riemann_solver_implementation
        d["SOURCE_TERM_IMPLEMENTATION"] = self._source_term_implementation
        d["DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION"] = self._diffusive_source_term_implementation
        d["ADJUST_SOLUTION_IMPLEMENTATION"] = self._adjust_solution_implementation
        d["LIMITER_IMPLEMENTATION"] = self._limiter_implementation

        d["USE_FLUX"]                 = self._flux_implementation != PDETerms.None_Implementation
        d["USE_NCP"]                  = self._ncp_implementation != PDETerms.None_Implementation
        d["USE_MAX_EIGENVALUE"]       = self._max_eigenvalue_implementation != PDETerms.None_Implementation
        d["USE_RIEMANN_SOLVER"]       = self._riemann_solver_implementation != PDETerms.None_Implementation
        d["USE_SOURCE"]               = self._source_term_implementation != PDETerms.None_Implementation
        d["USE_DIFFUSIVE_SOURCE"]     = self._diffusive_source_term_implementation != PDETerms.None_Implementation
        d["USE_ADJUST_SOLUTION"]      = self._adjust_solution_implementation != PDETerms.None_Implementation

        d["STATELESS_INITIAL_CONDITIONS"] = self._use_stateless_initial_conditions
        d["STATELESS_BOUNDARY_CONDITIONS"] = self._use_stateless_boundary_conditions
        d["STATELESS_REFINEMENT_CRITERION"] = self._use_stateless_refinement_criterion
        d["STATELESS_FLUX"] = self._use_stateless_flux
        d["STATELESS_NCP"] = self._use_stateless_ncp
        d["STATELESS_MAX_EIGENVALUE"] = self._use_stateless_max_eigenvalue
        d["STATELESS_RIEMANN_SOLVER"] = self._use_stateless_riemann_solver
        d["STATELESS_SOURCE_TERM"] = self._use_stateless_source_term
        d["STATELESS_DIFFUSIVE_SOURCE_TERM"] = self._use_stateless_diffusive_source_term
        d["STATELESS_ADJUST_SOLUTION"] = self._use_stateless_adjust_solution
        d["STATELESS_LIMITER"] = self._use_stateless_limiter

        d["COMPUTE_KERNEL_CALL"] = render_template(self._compute_kernel_call)
        d["FUSED_COMPUTE_KERNEL_CALL"] = render_template(self._fused_compute_kernel_call)

        d["ABSTRACT_SOLVER_USER_DECLARATIONS"] = render_template(
            self._abstract_solver_user_declarations
        )
        d["ABSTRACT_SOLVER_USER_DEFINITIONS"] = render_template(
            self._abstract_solver_user_definitions
        )
        d["SOLVER_USER_DECLARATIONS"] = render_template(
            self._solver_user_declarations
        )
        d["SOLVER_USER_DEFINITIONS"] = render_template(
            self._solver_user_definitions
        )
        d["START_TIME_STEP_IMPLEMENTATION"] = render_template(
            self._start_time_step_implementation
        )
        d["FINISH_TIME_STEP_IMPLEMENTATION"] = render_template(
            self._finish_time_step_implementation
        )
        d["CONSTRUCTOR_IMPLEMENTATION"] = render_template(
            self._constructor_implementation
        )
        d["START_SIMULATION_IMPLEMENTATION"] = render_template(
            self._start_simulation_implementation
        )
        d["FINISH_SIMULATION_IMPLEMENTATION"] = render_template(
            self._finish_simulation_implementation
        )

        d["PREPROCESS_RECONSTRUCTED_PATCH"] = render_template(
            self._preprocess_reconstructed_patch
        )
        d["POSTPROCESS_UPDATED_PATCH"] = render_template(
            self._postprocess_updated_patch
        )

        d["COMPUTE_MAX_EIGENVALUE"] = self._compute_eigenvalue
        d["COMPUTE_TIME_STEP_SIZE"] = render_template(
            self._compute_time_step_size
        )
        d["COMPUTE_NEW_TIME_STEP_SIZE"] = render_template(
            self._compute_new_time_step_size
        )

        d["NUMBER_OF_UNKNOWNS_PARTICLE"] = self._particle_unknowns
        if self._tracer_particle_sets:
            d["PARTICLES_CONTAINER"] = self._tracer_particle_sets[0].name
            d["PARTICLE"] = self._tracer_particles[0].name

    def add_tracer(
        self,
        name,
        particle_attributes,
        sort_descend_invocation_order_within_action_sets=-1,
        plot_descend_invocation_order_within_action_sets=65536,
    ):
        self._tracer_names.append(name)
        particle = peano4.toolbox.particles.Particle(name)
        particle.association = peano4.datamodel.DoFAssociation.Global
        self._plot_descend_invocation_order_within_action_sets = plot_descend_invocation_order_within_action_sets
        self._sort_descend_invocation_order_within_action_sets = sort_descend_invocation_order_within_action_sets

        particle_variable_names = []
        particle_variable_pos = [0]
        if isinstance(particle_attributes, dict):
            unknown_count = sum(particle_attributes["unknowns"].values())
            particle_variable_names += list(particle_attributes["unknowns"].keys())
            for var in particle_attributes["unknowns"].values():
                particle_variable_pos.append(var + particle_variable_pos[-1])
            self._use_var_shortcut_tracer = True
        elif isinstance(particle_attributes, int):
            unknown_count = particle_attributes
        else:
            raise Exception("Invalid format for 'unknowns'. Must be an integer or a dictionary.")

        self._particle_unknowns.append(unknown_count)
        self._particle_variable_names.append(particle_variable_names)
        self._particle_variable_pos.append(particle_variable_pos)

        particle_attr = peano4.dastgen2.Peano4DoubleArray("data", str(unknown_count))
        particle.data.add_attribute(particle_attr)
        self._particle_attributes.append(particle_attr)

        particle.data.add_attribute(
            dastgen2.attributes.Enumeration("MoveState", ["NotMovedYet", "Moved"])
        )

        particle_number = peano4.dastgen2.Peano4IntegerArray("number", "2")
        particle.data.add_attribute(particle_number)
        self._particle_number.append(particle_number)

        particles = peano4.toolbox.particles.ParticleSet(particle)
        particles.subdirectory = ""
        particles.association = peano4.datamodel.DoFAssociation.Vertex

        updateParticleGridAssociation_LiftDrop = peano4.toolbox.particles.api.UpdateParticleGridAssociation_LiftDrop(particles)
        updateParticleGridAssociation_LiftDrop.descend_invocation_order = self._sort_descend_invocation_order_within_action_sets
        updateParticleGridAssociation_LiftDrop.parallel = True
        self._update_particle_association_action_sets.append(updateParticleGridAssociation_LiftDrop)
        parallel_state_action_set = peano4.toolbox.particles.api.UpdateParallelState(particles)
        parallel_state_action_set.descend_invocation_order = self._sort_descend_invocation_order_within_action_sets
        parallel_state_action_set.parallel = True
        self._update_particle_parallel_state_action_sets.append(parallel_state_action_set)

        self._tracer_particles.append(particle)
        self._tracer_particle_sets.append(particles)

        return particles

    def get_solver_statistics_metadata(self):
        return [
            ("Patch", self._patch_size),
        ]

    @property
    def unknowns(self):
        return self._variable_shortcuts.unknowns

    @property
    def variable_names(self):
        return self._variable_shortcuts.names

    @property
    def patch_size(self):
        return self._patch_size

    @property
    def last_solver(self):
        return self._last_solver

    @last_solver.setter
    def last_solver(self, solver):
        self._last_solver = solver

    @property
    def auxiliary_variables(self):
        return self._variable_shortcuts.auxiliary_variables

    @patch_size.setter
    def patch_size(self, value):
        self._patch_size = value
        self.create_data_structures()
        self.create_action_sets()

    @property
    def preprocess_reconstructed_patch(self):
        return self._preprocess_reconstructed_patch

    @preprocess_reconstructed_patch.setter
    def preprocess_reconstructed_patch(self, kernel):
        """!
        Set a new processing kernel

        Before the actual update, the numerical solver takes the actual solution
        including all auxiliary variables and copies it into another array which
        is calls reconstructedPatch (in FD4, this field is called oldQWithHalo).
        This array is bigger than the actual patch data, as it also holds the
        halo. Once the solution is copied over, the kernel writes in the halo
        data using data from the faces. This reconstructed object is now passed
        into the compute kernel, which derives new values for all evolution
        variables. That is, the compute kernels' output does not hold the auxiliary
        variables anymore, as they are not subject to the PDE. The new variables
        are eventually written back into the real data, where they replace the
        old quantities (the auxiliary variables in there remain untouched).
        After the compute kernel has terminated, the reconstructed data including
        the halo is thrown away.

        ## Auxiliary variables

        If you alter the auxiliary variables (material parameters) in the patch
        preparation, this is absolutely fine, but the change will not be committed
        into the real data. It is only there temporarily while the actual patch
        will continue to hold the original material parameters.

        You can alter the actual patch data Q in the preprocessing. If this
        actual data are evolution (PDE) parameters, these changes however will
        be overwritten by the compute kernel. If you alter auxiliary variables
        within the preprocessing within the patch (Q), then this is fine, but the
        changes will not be visible to the reconstructed patch immediately. They
        will be visible there in the next time step. So you might be well-advised
        to alter the auxiliary variables in both the patch data and the
        reconstructed data.
        """
        self._preprocess_reconstructed_patch = kernel
        self.create_data_structures()
        self.create_action_sets()

    @property
    def name(self):
        return self._name

    @property
    def postprocess_updated_patch(self):
        return self._postprocess_updated_patch

    @postprocess_updated_patch.setter
    def postprocess_updated_patch(self, kernel):
        """
        Define a postprocessing routine over the data

        The postprocessing kernel often looks similar to the following code:

             {
               int index = 0;
               dfor( volume, {{NUMBER_OF_VOLUMES_PER_AXIS}} ) {
                 enforceCCZ4constraints( newQ+index );
                 index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
               }
             }

         but often, it is nicer ot user one of the pre-defined kernels in ExaHyPE
         for postprocessing:

             ::exahype2::fv::mapInnerNeighbourVoxelAlongBoundayOntoAuxiliaryVariable(
               newQ,
               {{NUMBER_OF_VOLUMES_PER_AXIS}},
               {{NUMBER_OF_UNKNOWNS}},
               {{NUMBER_OF_AUXILIARY_VARIABLES}}
             );

        As highlighted from the examples above, you can use normal ExaHyPE constant
        within your injected code. Furthermore, you have the following two variables
        available:

        - newQ This is a pointer to the whole data structure (one large
            array). The patch is not supplemented by a halo layer.
        - marker An instance of the CellMarker which identifies the cell.

        In ExaHyPE's finite volume solvers, there are two different ways how to inject
        postprocessing: You can either define your own postprocessing step by setting
        _action_set_postprocess_solution, or you can use this setter to inject some
        source code. The two options are not equivalent: The snippet injected here is
        executed directly after the kernel update. Consequently, it also runs on a GPU
        if GPUs are employed, or it runs directly after a task has been launched to
        update a cell.

        The postprocessing action set is launched when the cell update is committed,
        i.e., after the cell task has terminated or a task result came back from the
        GPU. Consequently, the snippet injected here might not have access to global
        variables (if it executes on a GPU) while the postprocessing action set
        always has access to the global program state.

        ## Attributes

        kernel: String
          C++ code that holds the postprocessing kernel
        """
        self._postprocess_updated_patch = kernel
        self.create_data_structures()
        self.create_action_sets()

    @property
    def overlap(self):
        return self._overlap

    @overlap.setter
    def overlap(self, value):
        if value < 1:
            raise Exception(
                "Halo (overlap) size has to be bigger than zero but was {}".format(
                    value
                )
            )
        self._overlap = value
        self.create_data_structures()
        self.create_action_sets()

    @property
    def interpolation(self):
        return self._interpolation

    @interpolation.setter
    def interpolation(self, value):
        self._interpolation = value
        self.create_data_structures()
        self.create_action_sets()

    @property
    def restriction(self):
        return self._restriction

    @restriction.setter
    def restriction(self, value):
        self._restriction = value
        self.create_data_structures()
        self.create_action_sets()

    def set_plotter(self, plotter):
        self._plotter = plotter

    @abstractmethod
    def _generate_kernels(self, namespace, output, subdirectory, dimensions):
        pass
