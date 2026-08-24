# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
from abc import abstractmethod

import jinja2

import peano4.output
import peano4.datamodel
import peano4.plotter
import peano4.blockstructured

import exahype2.solvers.fv.actionsets

from exahype2.solvers.fv.PatchData import create_patch_and_halo
from exahype2.solvers import PDETerms
from exahype2.VariableShortcuts import VariableShortcuts
from peano4.output.Helper import render_jinja2_template

from peano4.Logger import log


class FV(object):
    """!
    Abstract finite volume solver for patch-based AMR with a configurable halo.

    FV is the base class of all FV solvers. It defines what kind of action sets do
    exist, i.e., what in principle can be done while we run through the grid. It also
    provides the very basic infrastructure, i.e., what the name of a solver
    is, what data is to be held per face or cell, or where in the multiscale mesh
    we have to hold data. The FV class cannot/should not be instantiated.

    A finite volume solver in ExaHyPE 2 (as any solver) is first of all a
    static collection of data logically tied to grid entities plus some operations
    over the mesh. All data and action sets however have guards, i.e., boolean
    expressions that define

    - whether data is stored in-between traversals,
    - whether an action set is invoked in a particular traversal.

    In the baseline FV class, these guards are usually set to default values.
    That is, all action sets per time step are always invoked (the guard is
    true) and all data are always stored on the finest mesh. If you want to alter
    this behaviour, i.e., store data only sometimes or skip steps, you can overwrite
    the corresponding attributes, i.e., the guards of the associated data.

    See the discussion on "Control flow between this class and subclasses" below.

    ## Parallelisation

    Cell patches remain local. The old halo is reconstructed from local data, while
    the new halo is exchanged after a sweep has stored face data.

    ## Control flow between this class and subclasses

    There are three key routines: the constructor, create_data_structures() and
    create_action_sets(). The constructor sets some global variables (such as the
    name) and then invokes the other two routines.

    create_data_structures() establishes all the data structures tied to the
    grid entities. It also sets some properties of these data, such as the patch
    size. If you want to add additional data (such as additional quantities
    per cell) or if you want to alter the configuration of data tied to grid
    entities, you should redefine create_data_structures(). However, any
    subclass still should call FV's create_data_structures() - or the create_data_structures()
    of the SingleSweep. This will ensure that the
    baseline configuration of all data is in place. After that, you can modify
    the properties.

    create_action_sets() establishes the action sets, i.e. activities that are to
    be triggered whenever you run a time step, you plot, you initialise the grid.

    Both create_data_structures() and create_action_sets() add attributes to the
    FV class. See self._patch for example within create_data_structures(). These
    attributes have guards such as self._action_set_initial_conditions.guard.
    These guards are set to defaults in FV. It is the job of SingleSweep
    or any other subclass - they determine when which data are used - to reset these
    guards from a default to something tailored to the particular data flow.

    If you want to redefine when data is stored or operations are invoked, overwrite
    create_data_structures(), and call the superclass. This is what happens:

    - SingleSweep passes on the call to FV.create_data_structures().
      This call ensures that all the data structures are in place. Then it returns.
    - SingleSweep's create_data_structures() then sets the
      guards, i.e. they configure when data is stored or used.
    - Finally, your own overwrite of create_data_structures() can augment the
      data structures (which are now in place and called at the right time) with
      information about what it shall do.

    ## Adaptive mesh refinement (AMR)

    Finite Volume solvers project a coarse cell onto its children piecewise
    constantly, and average the children back onto it. The halo across a
    coarse-fine face instead carries a tangential slope: a piecewise-constant
    halo gives the fine sub-volumes of one coarse volume the same value, and the
    Riemann solver turns that tangential staircase into a spurious O(h)
    tangential velocity.

    ## Data postprocessing or coupling to other codes

    For data postprocessing, the introduction of custom interior conditions or the coupling to
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
        baseline_action_set_descend_invocation_order=0,
    ):
        """!
        Create a solver.

        name: string
           A unique name for the solver. This one will be used for all generated
           classes. Also the C++ object instance later on will incorporate this
           name.

        patch_size: int
           Size of the patch in one dimension. Everything here is dimension-generic.

        overlap: int
           This is the size of the halo layer which is half of the overlap with a
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
        """
        self._name = name
        self._solver_number = 0

        self._patch_size = patch_size
        self._overlap = overlap

        self._dimensions = 2
        self._namespace = ""

        self._plotter = peano4.plotter.PatchPlotter
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
        self._action_set_handle_boundary = None
        self._action_set_project_patch_onto_faces = None
        self._action_set_roll_over_updates = None
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = None
        self._action_set_dynamic_amr_transfer = None
        self._action_set_postprocess_solution = None
        self._action_set_preprocess_solution = None

        self._compute_time_step_size = "#error Not yet defined"
        self._compute_new_time_step_size = "#error Not yet defined"
        self._preprocess_reconstructed_patch = ""
        self._postprocess_updated_patch = ""

        self._compute_kernel_call = "#error Not yet defined"

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
        self._limiter_implementation = PDETerms.None_Implementation

        self._use_stateless_initial_conditions = False
        self._use_stateless_boundary_conditions = False
        # Refinement criteria may query mesh-size information from the solver.
        self._use_stateless_refinement_criterion = False
        self._use_stateless_flux = False
        self._use_stateless_ncp = False
        self._use_stateless_max_eigenvalue = False
        self._use_stateless_riemann_solver = False
        self._use_stateless_source_term = False
        self._use_stateless_diffusive_source_term = False
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

We use a domain with offset ("""
            + str(domain_offset)
            + ") and size ("
            + str(domain_size)
            + """). Peano 4 cuts this domain equidistantly
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
        Additional includes that the generated action sets require.
        """
        return self._user_action_set_includes

    @property
    def user_solver_includes(self):
        """
        Additional includes that the generated solver requires.
        """
        return self._user_solver_includes

    def add_user_action_set_includes(self, value):
        """
        Add further includes, if your action sets require some additional
        routines from other header files.
        """
        self._user_action_set_includes += value

    def add_user_solver_includes(self, value):
        """
        Add further includes, if your solver requires some additional
        routines from other header files.
        """
        self._user_solver_includes += value

    @abstractmethod
    def create_data_structures(self):
        """!

        Create data structures required by all Finite Volume solvers.

        Call again in subclasses if you want to change the number of unknowns
        or auxiliary variables. See the class description's subsection on the
        control flow between this class and its subclasses.

        :: Call order and ownership

        This operation can be called multiple times. However, only the very
        last call matters. All previous calls are wiped out.

        If you have a hierarchy of solvers, every create_data_structures()
        should first(!) call its parent version. This way, you always ensure
        that all data are in place before you continue to alter the more
        specialised versions. So it is (logically) a top-down (general to
        specialised) run through all create_data_structures() variants
        within the inheritance tree.

        :: Attributes

        _patch: Patch (N^d)
          Actual patch data. We use Finite Volumes, so this is
          always the current snapshot, i.e. the valid data at one point.

        _patch_old: Patch (N^d)
          The snapshot of _patch that the preceding time step produced.

        _patch_overlap_old, _patch_overlap_new: Patch (2M x N^(d-1))
          Each face stores M halo layers from both adjacent patches. The old
          and new snapshots support interpolation in time.
        """

        (
            self._patch,
            self._patch_old,
            self._patch_overlap_old,
            self._patch_overlap_new,
        ) = create_patch_and_halo(
            self.current_unknown_identifier(),
            self.previous_unknown_identifier(),
            self._patch_size,
            self._overlap,
            self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables,
            self._storage_precision,
        )

        self._patch_overlap_old.generator.includes += """
#include "peano4/Loop.h"
#include "repositories/SolverRepository.h"
"""
        self._patch_overlap_new.generator.includes += """
#include "peano4/Loop.h"
#include "repositories/SolverRepository.h"
"""

        # The old halo is filled locally, so only the new halo is exchanged. The
        # exchange view copies the sender's persistent overlap: a move-steal merge
        # consumes the source buffer, which is order-dependent and breaks mirror
        # symmetry under dynamic AMR. The toolbox element-wise merge writes only the
        # halo layers the sender owns.
        self._patch_overlap_old.generator.send_condition = "false"
        self._patch_overlap_old.generator.receive_and_merge_condition = "false"
        self._patch_overlap_old.generator.can_receive = "false"
        self._patch_overlap_old.generator.can_send = "false"
        self._patch_overlap_old.generator.use_copy_for_exchange = "true"
        self._patch_overlap_old.generator.merge_method_definition = (
            peano4.blockstructured.get_face_merge_implementation(
                self._patch_overlap_old
            )
        )

        self._patch_overlap_new.generator.send_condition = "true"
        self._patch_overlap_new.generator.receive_and_merge_condition = "true"
        self._patch_overlap_new.generator.can_receive = self._load_face_data_default_guard()
        self._patch_overlap_new.generator.can_send = self._store_face_data_default_guard()
        self._patch_overlap_new.generator.use_copy_for_exchange = "true"
        self._patch_overlap_new.generator.merge_method_definition = (
            peano4.blockstructured.get_face_merge_implementation(
                self._patch_overlap_new
            )
        )

        self._patch.generator.includes += """
#include "repositories/SolverRepository.h"
"""
        self._patch_old.generator.includes += """
#include "repositories/SolverRepository.h"
"""

        self._cell_label = exahype2.grid.create_cell_label(self._name)
        self._face_label = exahype2.grid.create_face_label(self._name)

    @abstractmethod
    def create_action_sets(self):
        """!
        Create all the action sets.

        Overwrite in subclasses if you want to create different
        action sets. Subclasses also might redefine the invocation order.

        ## Call order and ownership

        This operation can be called multiple times. However, only the very
        last call matters. All previous calls are wiped out.

        If you have a hierarchy of solvers, every create_action_sets()
        should first(!) call its parent version. This way, you always ensure
        that all action sets are in place before you continue to alter the more
        specialised versions. So it is (logically) a top-down (general to
        specialised) run through all create_action_sets() variants
        within the inheritance tree.

        ## Recreation vs backup (state discussion)

        Some action sets have to survive a recreation. The postprocessing for
        example is set by the user, who may later on reset the number of
        unknowns; recreating the postprocessing would lose that setting. The
        postprocessing step is therefore never overwritten by the standard
        classes.

        There are further action sets which have a state that users might want
        to alter, such as the number of unknowns. Recreating the action set
        picks up such changes; keeping a default one would not.
        """
        self._action_set_initial_conditions = (
            exahype2.solvers.fv.actionsets.InitialCondition(
                self, self._store_cell_data_default_guard()
            )
        )
        self._action_set_AMR_throughout_grid_construction = exahype2.solvers.fv.actionsets.RefinementCriterion(
            solver=self,
            guard=self._store_cell_data_default_guard(),
            build_up_new_refinement_instructions=True,
            implement_previous_refinement_instructions=True,
            called_by_grid_construction=True,
            skip_after_topology_change=False,
        )
        self._action_set_AMR = exahype2.solvers.fv.actionsets.RefinementCriterion(
            solver=self,
            guard=self._store_cell_data_default_guard(),
            build_up_new_refinement_instructions=True,
            # A time step never realises topology changes; it only commits the
            # refine/coarsen requests. Realisation happens in the DynamicAMR
            # observer, whose DynamicAMRTransfer delivers the committed events.
            implement_previous_refinement_instructions=False,
            called_by_grid_construction=False,
            skip_after_topology_change=True,
        )
        # Re-evaluating the criterion within the realise sweeps lets one round of
        # dedicated mesh-refinement sweeps climb every level the data justifies.
        # Only cells that these sweeps have just created evaluate the criterion;
        # every other cell committed its request in the preceding time step, and
        # re-issuing it each sweep would keep the sweeps running forever.
        self._action_set_AMR_throughout_dynamic_amr = exahype2.solvers.fv.actionsets.RefinementCriterion(
            solver=self,
            guard=self._store_cell_data_default_guard()
            + " and fineGridCell"
            + self._cell_label.name
            + ".getRequiresCriterionEvaluation()",
            build_up_new_refinement_instructions=True,
            implement_previous_refinement_instructions=False,
            called_by_grid_construction=False,
            skip_after_topology_change=False,
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
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = exahype2.solvers.fv.actionsets.AdaptiveMeshFaceCoupling(
            solver=self,
        )
        self._action_set_dynamic_amr_transfer = exahype2.solvers.fv.actionsets.DynamicAMRTransfer(
            solver=self,
        )

        # Preserve user-configured hooks when action sets are recreated.
        if self._action_set_postprocess_solution is None:
            self._action_set_postprocess_solution = (
                exahype2.solvers.fv.actionsets.EmptyPostprocessSolution(self)
            )
        if self._action_set_preprocess_solution is None:
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
        self._action_set_AMR_throughout_dynamic_amr.descend_invocation_order = self._baseline_action_set_descend_invocation_order + 2
        self._action_set_handle_boundary.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_project_patch_onto_faces.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_roll_over_updates.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_dynamic_amr_transfer.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_postprocess_solution.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )
        self._action_set_preprocess_solution.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )

    def _provide_cell_data_to_compute_kernels_default_guard(self):
        """! Default logic for when cell data is created.

        ExaHyPE keeps patch load/store topology-driven. This predicate decides
        when cell data is provided to compute and exchange paths. Particular
        solvers use it as a starting point for more detailed rules, such as
        keeping the topology criterion but toggling data exchange depending on
        the solver phase.
        """
        return (
            "not marker.willBeRefined() and repositories::{}.getSolverState()"
            "!={}::SolverState::GridConstruction"
        ).format(self.get_name_of_global_instance(), self._name)

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
        extend it as well, and they will append further ands.
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
        extend it as well, and they will append further ands.
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
        extend it as well, and they will append further ands.
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
        extend it as well, and they will append further ands.

        Receiving consumes the batch the previous sweep sent, so it is enabled
        from the first sweep that follows a face-data-storing sweep on. A guard
        keyed on the solver state alone cannot express that: the state stays at
        GridInitialisation across the initialise-drain loop, whose every sweep
        sends.
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
            + ".hasStoredFaceData()"
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
        Add all required data to the Peano project's datamodel.
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
        Tell Peano what data to move around.

        Inform the Peano 4 step which data are to be moved around via the
        use_cell and use_face commands. This operation is generic from
        ExaHyPE's point of view, i.e. it serves all grid sweep types.

        Cell and face labels come before the data whose load/store decisions
        depend on them.
        """
        step.use_cell(self._cell_label)
        step.use_cell(self._patch)
        step.use_cell(self._patch_old)
        step.use_face(self._face_label)
        step.use_face(self._patch_overlap_old)
        step.use_face(self._patch_overlap_new)
        for tracer_set in self._tracer_particle_sets:
            step.use_vertex(tracer_set)

    def _get_default_includes(self):
        return """
#include "peano4/Loop.h"

#include "repositories/SolverRepository.h"
"""

    def add_actions_to_init_grid(self, step):
        """!
        Add all the action sets to init grid.

        The AMR action set has to come first. The AMR routines' interpolation
        plays no role here, but the restriction does: we have to obtain the face
        data for the boundary conditions. The action set order is inverted while
        we ascend within the tree again. Therefore, we add the AMR action set
        first which means it will be called last when we go from fine to coarse
        levels within the tree.

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
            for action_set in self._update_particle_association_action_sets:
                step.add_action_set(action_set)
            for action_set in self._update_particle_parallel_state_action_sets:
                step.add_action_set(action_set)

    def add_actions_to_create_grid(self, step):
        """
        The boundary information is set only once. It is therefore important that
        we use the face label and initialise it properly.
        """
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_AMR_throughout_grid_construction)

    def add_actions_to_plot_solution(self, step):
        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        plotter = self._plotter(
            filename=self._name,
            patch=self._patch,
            dataset_name=self.current_unknown_identifier(),
            description=self._variable_shortcuts.plot_description,
            representation="cell",
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
            assoc_idx = self._tracer_particle_sets.index(tracer_set)
            # Particles are a point cloud. NetCDF (UGRID) has no point-cloud
            # representation, so fall back to HDF5 there; otherwise follow the
            # solver's chosen output format.
            particle_file_format = peano4.plotter.get_default_format()
            if particle_file_format == "netcdf":
                particle_file_format = "hdf5"
            plotter = peano4.plotter.ParticlePlotter(
                 filename=particle.name,
                 particle_set=tracer_set,
                 description="description",
                 time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
                 file_format=particle_file_format,
             )
            plotter.descend_invocation_order = self._plot_descend_invocation_order_within_action_sets
            step.add_action_set(plotter)

            # These correspond one-to-one with the tracer sets.
            step.add_action_set(self._update_particle_association_action_sets[assoc_idx])
            step.add_action_set(self._update_particle_parallel_state_action_sets[assoc_idx])

    def add_actions_to_dynamic_mesh_refinement(self, step):
        """!
        Add action sets to the dedicated DynamicAMR observer.

        This is the only observer that changes the topology. It triggers and
        realises the committed refine/coarsen requests, transfers the solution
        across every resolution transition Peano creates or destroys, and
        re-evaluates the refinement criterion on cells it created, so one round
        of these sweeps realises the full depth the data justifies. Each sweep
        ends by projecting every cell a topology change touched onto its faces,
        so the affected face halves hold their owners' boundary volumes again
        and the next time step solves the same Riemann problem on both sides of
        every face.
        """
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_dynamic_amr_transfer)
        step.add_action_set(self._action_set_AMR_throughout_dynamic_amr)

    def add_actions_to_perform_time_step(self, step):
        """!
        Add action sets to the time step.

        See exahype2.solvers.fv.FV.add_actions_to_init_grid() for details on the
        ordering of the action sets.

        ## AMR

        The inter-grid transfer operators have to run before we apply the
        boundary conditions.
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
            for action_set in self._update_particle_association_action_sets:
                step.add_action_set(action_set)
            for action_set in self._update_particle_parallel_state_action_sets:
                step.add_action_set(action_set)

    @abstractmethod
    def add_entries_to_text_replacement_dictionary(self, d):
        pass

    def add_implementation_files_to_project(self, namespace, output, dimensions):
        self._dimensions = dimensions
        self._namespace = namespace

        if (
            self._riemann_solver_implementation == PDETerms.None_Implementation
            and self._max_eigenvalue_implementation == PDETerms.None_Implementation
        ):
            raise AssertionError(
                f"[{self._name}] max_eigenvalue implementation is required when no Riemann solver is used."
            )

        self._generate_kernels(output)
        self._action_set_project_patch_onto_faces.generate_kernels(namespace, output, dimensions)

        # Assemble the template prefix in a local: mutating the attribute would
        # double the class name when the generator runs a second time on the
        # same solver object.
        if self._solver_template_file_class_name is None:
            templatefile_prefix = self._templatefile_prefix + self.__class__.__name__
        else:
            templatefile_prefix = (
                self._templatefile_prefix + self._solver_template_file_class_name
            )

        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        self._variable_shortcuts.generate_shortcut_file(d, namespace, output)

        header_file_abstr_template = templatefile_prefix + "Abstract.template.h"
        cpp_file_abstr_template = templatefile_prefix + "Abstract.template.cpp"
        header_file_template = templatefile_prefix + ".template.h"
        cpp_file_template = templatefile_prefix + ".template.cpp"

        generated_abstract_header_file = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                header_file_abstr_template,
                cpp_file_abstr_template,
                "Abstract" + self._name,
                namespace,
                ".",
                d,
                True,
            )
        )
        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                header_file_template,
                cpp_file_template,
                self._name,
                namespace,
                ".",
                d,
                True,
            )
        )

        output.add(generated_abstract_header_file)
        output.add(generated_solver_files)
        output.makefile.add_h_file("Abstract" + self._name + ".h", generated=True)
        output.makefile.add_h_file(self._name + ".h", generated=True)
        output.makefile.add_cpp_file("Abstract" + self._name + ".cpp", generated=True)
        output.makefile.add_cpp_file(self._name + ".cpp", generated=True)

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
        d["SOLVER_NUMBER"] = self._solver_number
        d["FULL_QUALIFIED_SOLVER_NAME"] = "::".join(self._namespace) + "::" + self._name
        d["SOLVER_INSTANCE"] = self.get_name_of_global_instance()
        d["UNKNOWN_IDENTIFIER"] = self._unknown_identifier()
        d["NUMBER_OF_UNKNOWNS"] = self._variable_shortcuts.unknowns
        d["NUMBER_OF_AUXILIARY_VARIABLES"] = self._variable_shortcuts.auxiliary_variables
        d["MEASURED_UNKNOWNS"] = getattr(self, "_error_norm_measured_unknowns", None) or (
            "{" + ", ".join(str(i) for i in range(self._variable_shortcuts.unknowns)) + "}"
        )
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

        d["INITIAL_CONDITIONS_IMPLEMENTATION"] = self._initial_conditions_implementation
        d["BOUNDARY_CONDITIONS_IMPLEMENTATION"] = self._boundary_conditions_implementation
        d["REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
        d["FLUX_IMPLEMENTATION"] = self._flux_implementation
        d["NCP_IMPLEMENTATION"] = self._ncp_implementation
        d["MAX_EIGENVALUE_IMPLEMENTATION"] = self._max_eigenvalue_implementation
        d["RIEMANN_SOLVER_IMPLEMENTATION"] = self._riemann_solver_implementation
        d["SOURCE_TERM_IMPLEMENTATION"] = self._source_term_implementation
        d["DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION"] = self._diffusive_source_term_implementation

        d["USE_FLUX"]                 = self._flux_implementation != PDETerms.None_Implementation
        d["USE_NCP"]                  = self._ncp_implementation != PDETerms.None_Implementation
        d["USE_MAX_EIGENVALUE"]       = self._max_eigenvalue_implementation != PDETerms.None_Implementation
        d["USE_RIEMANN_SOLVER"]       = self._riemann_solver_implementation != PDETerms.None_Implementation
        d["USE_SOURCE"]               = self._source_term_implementation != PDETerms.None_Implementation
        d["USE_DIFFUSIVE_SOURCE"]     = self._diffusive_source_term_implementation != PDETerms.None_Implementation

        d["STATELESS_INITIAL_CONDITIONS"] = self._use_stateless_initial_conditions
        d["STATELESS_BOUNDARY_CONDITIONS"] = self._use_stateless_boundary_conditions
        d["STATELESS_REFINEMENT_CRITERION"] = self._use_stateless_refinement_criterion
        d["STATELESS_FLUX"] = self._use_stateless_flux
        d["STATELESS_NCP"] = self._use_stateless_ncp
        d["STATELESS_MAX_EIGENVALUE"] = self._use_stateless_max_eigenvalue
        d["STATELESS_RIEMANN_SOLVER"] = self._use_stateless_riemann_solver
        d["STATELESS_SOURCE_TERM"] = self._use_stateless_source_term
        d["STATELESS_DIFFUSIVE_SOURCE_TERM"] = self._use_stateless_diffusive_source_term

        d["COMPUTE_KERNEL_CALL"] = render_template(self._compute_kernel_call)

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
        particle = peano4.particles.Particle(name)
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

        particle_attr = peano4.datamodel.DoubleArray("data", str(unknown_count))
        particle.add_attribute(particle_attr)
        self._particle_attributes.append(particle_attr)

        particle.add_attribute(
            peano4.datamodel.Enumeration("moveState", ["NotMovedYet", "Moved"])
        )

        particle_number = peano4.datamodel.IntegerArray("number", "2")
        particle.add_attribute(particle_number)
        self._particle_number.append(particle_number)

        particles = peano4.particles.ParticleSet(particle)
        particles.association = peano4.datamodel.DoFAssociation.Vertex

        updateParticleGridAssociation_LiftDrop = peano4.particles.UpdateParticleGridAssociation_LiftDrop(particles)
        updateParticleGridAssociation_LiftDrop.descend_invocation_order = self._sort_descend_invocation_order_within_action_sets
        self._update_particle_association_action_sets.append(updateParticleGridAssociation_LiftDrop)
        parallel_state_action_set = peano4.particles.UpdateParallelState(particles)
        parallel_state_action_set.descend_invocation_order = self._sort_descend_invocation_order_within_action_sets
        self._update_particle_parallel_state_action_sets.append(parallel_state_action_set)

        self._tracer_particles.append(particle)
        self._tracer_particle_sets.append(particles)

        return particles

    def get_solver_statistics_metadata(self):
        return [
            ("Unknowns", self._variable_shortcuts.unknowns),
            ("Auxiliary Variables", self._variable_shortcuts.auxiliary_variables),
            ("Volumes/axis", self._patch_size),
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
        Set C++ code that runs immediately before the finite-volume update.

        The snippet can access QIn, the previous cell snapshot, and QOut, the
        current output snapshot. Both patches exclude the halo. It can also use
        marker, timeStamp, and timeStepSize. The update kernel may overwrite
        changes to QOut. Face data are assembled after this hook and are not
        available here.
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
        Set C++ code that runs immediately after the finite-volume update.

        The snippet can access QIn, QOut, faceQIn, faceQOut, marker, timeStamp,
        and timeStepSize. QIn and QOut exclude the halo; the face arrays contain
        the old and new halo snapshots. This hook is part of the cell update,
        while _action_set_postprocess_solution runs as a separate action set.
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

    @abstractmethod
    def _generate_kernels(self, output):
        pass
