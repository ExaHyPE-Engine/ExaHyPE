# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import jinja2

from abc import abstractmethod
from enum import Enum

import peano4.datamodel
import peano4.output.TemplatedHeaderFile
import peano4.output.TemplatedHeaderImplementationFilePair
import peano4.output.Jinja2TemplatedHeaderFile
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import exahype2.solvers.fv.actionsets

from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.Storage import Storage

class FV(object):
    """!
    Abstract finite volume solver step sizes that works on patch-based AMR with a halo layer of one.

    FV is the base class of all FV solvers. It defines what kind of action sets do
    exist, i.e., what in principle can be done while we run through the grid. It also
    provides some kind of very basic infrastructure, i.e., what the name of a solver
    is, what data is to be held per face or cell, or where in the multiscale mesh
    we actually have to hold data.

    The FV class cannot/should not be instantiated. There are two direct children:
    SingleSweep and EnclaveTasking. The FV base class defines what data are available.
    The two subclasses define how we run through the mesh: once per time step or
    twice per time step with some tasking. So FV defines what can be done, the
    subclasses define how the steps are orchestrated. Again, they do not (yet) say
    what is computed. That's then the responsibility of classes that inherit from
    SingleSweep or EnclaveTasking, respectively.

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
    of the SingleSweep or EnclaveTasking, respectively. This will ensure that the
    baseline configuration of all data is in place. After that, you can modify
    the properties.

    create_action_sets() establishes the action sets, i.e. activities that are to
    be triggered whenever you run a time step, you plot, you initialise the grid.

    Both create_data_structures() and create_action_sets() add attributes to the
    FV class. See self._patch for example within create_action_sets(). These
    attributes have guards such as self._action_set_initial_conditions.guard.
    These guards are set to defaults in FV. It is basically the job of SingleSweep
    or EnclaveTasking - they determine when which data are used - to reset these
    guards from a default to something tailored to the particular data flow.

    If you want to redefine when data is stored or operations are invoked, overwrite
    create_data_structures(), and call the superclass, i.e. either SingleSweep or
    EnclaveTasking. This is what happens:

    - SingleSweep or EnclaveTasking pass on the call to FV.create_data_structures().
      This call ensures that all the data structures are in place. Then it returns.
    - SingleSweep's or EnclaveTasking's create_data_structures() then sets the
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
        min_volume_h,
        max_volume_h,
        plot_grid_properties,
        pde_terms_without_state: bool,
        kernel_namespace,
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

        min_volume_h: double
           This size refers to the individual Finite Volume.

        max_volume_h: double
           This size refers to the individual Finite Volume.

        plot_grid_properties: Boolean
           Clarifies whether a dump of the data should be enriched with grid info
           (such as enclave status flags), too.
        """
        self._name = name

        self._min_volume_h = min_volume_h
        self._max_volume_h = max_volume_h
        self._plot_grid_properties = plot_grid_properties

        self._patch_size = patch_size
        self._overlap = overlap

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
                "Not a valid type for parameter unknowns, needs to be int or dictionary."
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
                "Not a valid type for parameter auxiliary_variables, needs to be int or dictionary."
            )

        self._solver_constants = ""
        self._user_action_set_includes = ""
        self._user_solver_includes = ""

        if min_volume_h > max_volume_h:
            raise Exception(
                "min_volume_h ("
                + str(min_volume_h)
                + ") is bigger than max_volume_h ("
                + str(max_volume_h)
                + ")"
            )

        self._reconstructed_array_memory_location = (
            peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack
        )

        self._solver_template_file_class_name = None

        self._plot_description = ""
        self.select_dofs_to_print = None

        self._action_set_initial_conditions = None
        self._action_set_initial_conditions_for_grid_construction = None
        self._action_set_AMR = None
        self._action_set_AMR_throughout_grid_construction = None
        self._action_set_AMR_commit_without_further_analysis = None
        self._action_set_handle_boundary = None
        self._action_set_project_patch_onto_faces = None
        self._action_set_roll_over_update_of_faces = None
        self._action_set_copy_new_faces_onto_old_faces = None
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = (
            None
        )
        self._action_set_postprocess_solution = None
        self._action_set_preprocess_solution = None

        self._compute_time_step_size = "#error Not yet defined"
        self._compute_new_time_step_size = "#error Not yet defined"
        self._preprocess_reconstructed_patch = ""
        self._postprocess_updated_patch = ""

        self._compute_kernel_call = "#error Not yet defined"
        self._compute_kernel_call_stateless = "#error Not yet defined"
        self._pde_terms_without_state = pde_terms_without_state

        self._compute_eigenvalue = False
        self._abstract_solver_user_declarations = ""
        self._abstract_solver_user_definitions = ""
        self._solver_user_declarations = ""
        self._solver_user_definitions = ""

        self._start_time_step_implementation = ""
        self._finish_time_step_implementation = ""
        self._constructor_implementation = ""

        self._boundary_conditions_implementation = None
        self._refinement_criterion_implementation = None
        self._initial_conditions_implementation = None

        self._interpolation = "piecewise_constant"
        self._restriction = "averaging"

        self._kernel_namespace = kernel_namespace

        self._baseline_action_set_descend_invocation_order = baseline_action_set_descend_invocation_order

        self.switch_storage_scheme(Storage.SmartPointers, Storage.SmartPointers)


    def __str__(self):
        result = (
            """
Name:                   """
            + self._name
            + """
Type:                   """
            + self.__class__.__module__
            + """
Stateless PDE terms:    """
            + str(self._pde_terms_without_state)
            + """
Patch size:             """
            + str(self._patch_size)
            + """
Overlap:                """
            + str(self._overlap)
            + """
Unknowns:               """
            + str(self._unknowns)
            + """
Auxiliary variables:    """
            + str(self._auxiliary_variables)
            + """
h_volume_min:           """
            + str(self._min_volume_h)
            + """
h_volume_max:           """
            + str(self._max_volume_h)
            + """
Initial conditions:     """
            + str(self._initial_conditions_implementation)
            + """
Boundary conditions:    """
            + str(self._boundary_conditions_implementation)
            + """
Refinement criterion:   """
            + str(self._refinement_criterion_implementation)
            + """
Eigenvalues:            """
            + str(self._eigenvalues_implementation)
            + """
Flux:                   """
            + str(self._flux_implementation)
            + """
NCP:                    """
            + str(self._ncp_implementation)
            + """
Source term:            """
            + str(self._source_term_implementation)
            + """
"""
        )
        return result

    __repr__ = __str__


    def get_min_number_of_spacetree_levels(self, domain_size):
        coarsest_tree_level = 0
        while (
            domain_size * 3 ** (-coarsest_tree_level) / self._patch_size
            > self._max_volume_h
        ):
            coarsest_tree_level += 1
        return coarsest_tree_level


    def get_max_number_of_spacetree_levels(self, domain_size):
        finest_tree_level = 0
        while (
            domain_size * 3 ** (-finest_tree_level) / self._patch_size
            > self._min_volume_h
        ):
            finest_tree_level += 1
        return finest_tree_level

    #    self._overlap              = overlap
    #    self._unknowns             = unknowns
    #    self._auxiliary_variables  = auxiliary_variables

    def get_coarsest_number_of_patches(self, domain_size):
        return 3 ** self.get_min_number_of_spacetree_levels(domain_size)


    def get_finest_number_of_patches(self, domain_size):
        return 3 ** self.get_max_number_of_spacetree_levels(domain_size)


    def get_coarsest_number_of_finite_volumes(self, domain_size):
        return self.get_coarsest_number_of_patches(domain_size) * self._patch_size


    def get_finest_number_of_finite_volumes(self, domain_size):
        return self.get_finest_number_of_patches(domain_size) * self._patch_size


    def get_coarsest_volume_size(self, domain_size):
        return domain_size / self.get_coarsest_number_of_finite_volumes(domain_size)


    def get_finest_volume_size(self, domain_size):
        return domain_size / self.get_finest_number_of_finite_volumes(domain_size)


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

The spacetree will at least have """
            + str(self.get_min_number_of_spacetree_levels(domain_size))
            + """ levels.
The spacetree will at most have """
            + str(self.get_max_number_of_spacetree_levels(domain_size))
            + """ levels.

The spacetree will thus span at least """
            + str(self.get_coarsest_number_of_patches(domain_size))
            + """ cells (or patches) per coordinate axis. This means a """
            + str(self.get_coarsest_number_of_patches(domain_size))
            + """^d grid of patches.
The spacetree will thus span at most """
            + str(self.get_finest_number_of_patches(domain_size))
            + """ cells (or patches) per coordinate axis. This means a """
            + str(self.get_finest_number_of_patches(domain_size))
            + """^d grid of patches.

ExaHyPE 2 embeds """
            + str(self._patch_size)
            + """^d patches of Finite Volumes into the finest tree level.

The coarsest possible mesh will consist of """
            + str(self.get_coarsest_number_of_finite_volumes(domain_size))
            + """ Finite Volumes per coordinate axis.
The finest possible mesh will consist of """
            + str(self.get_finest_number_of_finite_volumes(domain_size))
            + """ Finite Volumes per coordinate axis.

The coarsest mesh width of """
            + str(self.get_coarsest_volume_size(domain_size))
            + """ is thus just smaller than the maximum volume size """
            + str(self._max_volume_h)
            + """.
The finest mesh width of """
            + str(self.get_finest_volume_size(domain_size))
            + """ is thus just smaller than the minimum volume size """
            + str(self._min_volume_h)
            + """.
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

        If you have a hierarchy of solvers, every create_data_structure()
        should first(!) call its parent version. This way, you always ensure
        that all data are in place before you continue to alter the more
        specialised versions. So it is (logically) a top-down (general to
        specialised) run through all create_data_structure() variants
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

        _patch_overlap_update: Patch (2xNxN)
          This is the new update. After the time step, I roll this
          information over into _patch_overlap_new, while I backup the
          previous _patch_overlap_new into _patch_overlap_old. If I
          worked with regular meshes only, I would not need this update
          field and could work directly with _patch_overlap_new. However,
          AMR requires me to accumulate data within new while I need
          the new and old data temporarily. Therefore, I employ this
          accumulation/roll-over data which usually is not stored
          persistently.
        """
        self._patch = peano4.datamodel.Patch(
            (self._patch_size, self._patch_size, self._patch_size),
            self._unknowns + self._auxiliary_variables,
            self._unknown_identifier(),
        )
        self._patch_overlap_old = peano4.datamodel.Patch(
            (2 * self._overlap, self._patch_size, self._patch_size),
            self._unknowns + self._auxiliary_variables,
            self._unknown_identifier() + "Old",
        )
        self._patch_overlap_new = peano4.datamodel.Patch(
            (2 * self._overlap, self._patch_size, self._patch_size),
            self._unknowns + self._auxiliary_variables,
            self._unknown_identifier() + "New",
        )
        self._patch_overlap_update = peano4.datamodel.Patch(
            (2 * self._overlap, self._patch_size, self._patch_size),
            self._unknowns + self._auxiliary_variables,
            self._unknown_identifier() + "Update",
        )

        if self._cell_data_storage == Storage.CallStack:
            self._patch.generator = peano4.datamodel.PatchToDoubleArray(
                self._patch, "double"
            )
        elif self._cell_data_storage == Storage.Heap:
            self._patch.generator = peano4.datamodel.PatchToDoubleArrayOnHeap(
                self._patch, "double"
            )
        elif self._cell_data_storage == Storage.SmartPointers:
            self._patch.generator = peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
                self._patch, "double"
            )
        else:
            assert False, "Storage variant {} not supported".format(
                self._cell_data_storage
            )

        if self._face_data_storage == Storage.CallStack:
            self._patch_overlap_old.generator = peano4.datamodel.PatchToDoubleArray(
                self._patch_overlap_old, "double"
            )
            self._patch_overlap_new.generator = peano4.datamodel.PatchToDoubleArray(
                self._patch_overlap_new, "double"
            )
            self._patch_overlap_update.generator = peano4.datamodel.PatchToDoubleArray(
                self._patch_overlap_update, "double"
            )
        elif self._face_data_storage == Storage.Heap:
            self._patch_overlap_old.generator = (
                peano4.datamodel.PatchToDoubleArrayOnHeap(
                    self._patch_overlap_old, "double"
                )
            )
            self._patch_overlap_new.generator = (
                peano4.datamodel.PatchToDoubleArrayOnHeap(
                    self._patch_overlap_new, "double"
                )
            )
            self._patch_overlap_update.generator = (
                peano4.datamodel.PatchToDoubleArrayOnHeap(
                    self._patch_overlap_update, "double"
                )
            )
        elif self._face_data_storage == Storage.SmartPointers:
            self._patch_overlap_old.generator = (
                peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
                    self._patch_overlap_old, "double"
                )
            )
            self._patch_overlap_new.generator = (
                peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
                    self._patch_overlap_new, "double"
                )
            )
            self._patch_overlap_update.generator = (
                peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
                    self._patch_overlap_update, "double"
                )
            )
        else:
            assert False, "Storage variant {} not supported".format(
                self._face_data_storage
            )

        self._patch_overlap_old.generator.merge_method_definition = (
            peano4.toolbox.blockstructured.get_face_merge_implementation(
                self._patch_overlap_old
            )
        )
        self._patch_overlap_new.generator.merge_method_definition = (
            peano4.toolbox.blockstructured.get_face_merge_implementation(
                self._patch_overlap_new
            )
        )
        self._patch.generator.merge_method_definition = (
            peano4.toolbox.blockstructured.get_cell_merge_implementation(self._patch)
        )

        self._patch_overlap_old.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h"
"""
        self._patch_overlap_new.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h"
"""

        self._patch.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
            self._provide_cell_data_to_compute_kernels_default_guard(),
            self._load_cell_data_default_guard(),
            self._store_cell_data_default_guard(),
        )

        self._patch_overlap_old.generator.send_condition = "false"
        self._patch_overlap_old.generator.receive_and_merge_condition = "false"

        self._patch_overlap_new.generator.send_condition = "false"
        self._patch_overlap_new.generator.receive_and_merge_condition = "false"

        self._patch_overlap_update.generator.send_condition = "false"
        self._patch_overlap_update.generator.receive_and_merge_condition = "false"

        self._patch_overlap_old.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
          self._provide_face_data_to_compute_kernels_default_guard(),
          self._load_face_data_default_guard(),
          self._store_face_data_default_guard(),
        )

        self._patch_overlap_new.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
          self._provide_face_data_to_compute_kernels_default_guard(),
          self._load_face_data_default_guard(),
          self._store_face_data_default_guard(),
        )

        self._patch_overlap_update.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
          self._provide_face_data_to_compute_kernels_default_guard(),
          "false",
          "false"
        )

        self._patch.generator.includes += """
#include "../repositories/SolverRepository.h"
"""
        self._patch_overlap_old.generator.includes += """
#include "../repositories/SolverRepository.h"
"""
        self._patch_overlap_new.generator.includes += """
#include "../repositories/SolverRepository.h"
"""
        self._patch_overlap_update.generator.includes += """
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

        If you have a hierarchy of solvers, every create_data_structure()
        should first(!) call its parent version. This way, you always ensure
        that all data are in place before you continue to alter the more
        specialised versions. So it is (logically) a top-down (general to
        specialised) run through all create_data_structure() variants
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
                self, self._store_cell_data_default_guard(), "true"
            )
        )
        self._action_set_initial_conditions_for_grid_construction = (
            exahype2.solvers.fv.actionsets.InitialCondition(
                self, self._store_cell_data_default_guard(), "false"
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
                self, self._store_face_data_default_guard()
            )
        )
        self._action_set_project_patch_onto_faces = (
            exahype2.solvers.fv.actionsets.ProjectPatchOntoFaces(
                self, self._store_cell_data_default_guard()
            )
        )
        self._action_set_roll_over_update_of_faces = (
            exahype2.solvers.fv.actionsets.RollOverUpdatedFace(
                self, self._store_face_data_default_guard()
            )
        )
        self._action_set_copy_new_faces_onto_old_faces = (
            peano4.toolbox.blockstructured.BackupPatchOverlap(
                self._patch_overlap_new,
                self._patch_overlap_old,
                False,
                self._store_face_data_default_guard(),
                self._get_default_includes(),
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

        self._action_set_update_cell = None

        self._action_set_initial_conditions.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_initial_conditions_for_grid_construction.descend_invocation_order = (
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
        self._action_set_roll_over_update_of_faces.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_copy_new_faces_onto_old_faces.descend_invocation_order = (
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

        ExaHyPE allows you to define when you want to load or store data, and
        you can also define if you need some grid data for your computations.
        You might have some temporary data per cell which is neither stored
        nor loaded, but that has to be there while you compute. Or there might
        be cells which don't need to store a particular piece of data. See
        ::peano4::grid::LoadStoreComputeFlag for some details. This predicate
        here is a default. Particular solvers will take it as a starting point
        to make more detailed rules: an enclave solver for example might
        decide to take the information where to store data, but to toggle the
        storage on and off depending on the solver phase.
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


    def get_name_of_global_instance(self):
        return "instanceOf" + self._name


    def add_to_Peano4_datamodel(self, datamodel, verbose):
        """
        Add all required data to the Peano project's datamodel
        so it is properly built up.
        """
        if verbose:
            print("Patch data")
            print("----------")
            print(str(self._patch))
            print("Patch overlap data")
            print("----------")
            print(str(self._patch_overlap_old))
            print("Patch overlap data")
            print("----------")
            print(str(self._patch_overlap_new))
        datamodel.add_cell(self._cell_label)
        datamodel.add_cell(self._patch)
        datamodel.add_face(self._patch_overlap_old)
        datamodel.add_face(self._patch_overlap_new)
        datamodel.add_face(self._patch_overlap_update)
        datamodel.add_face(self._face_label)


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
        step.use_cell(self._cell_label)
        step.use_face(self._patch_overlap_old)
        step.use_face(self._patch_overlap_new)
        step.use_face(self._patch_overlap_update)
        step.use_face(self._face_label)


    def _get_default_includes(self):
        return """
#include "tarch/la/Vector.h"

#include "peano4/utils/Globals.h"
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

        - Project solution onto faces. This happens in touchCellLastTime(). See
          exahype2.solvers.fv.actionsets.ProjectPatchOntoFaces for comments.
          The project will end up in QUpdate.
        - Roll updates over on the faces from QUpdate into Q_new. This is done
          by RollOverUpdateFace, which requires in turn that the getUpdated()
          flag is set. As the roll-over plugs into touchFaceLastTime(), it will
          always be called after the projection, since the latter is a cell
          operation.
        - Copy new face data Q_new into old face data Q_old, as this is the
          initial sweep, i.e. the old face data otherwise might hold rubbish.
          This is a backup operation realised through the action set
          BackupPatchOverlap. This one plugs into touchFaceLastTime() too.
          Therefore, it is important that its priority is smaller than the one
          of the roll-over, since we invert the call order for the touch-last
          events.
        - Restrict the data to the coarser level if we are on a hanging face.
        - The postprocessing is the very last thing. It does not matter if it
          plugs into touchCellFirstTime() or touchCellLastTime(). As we invert the
          order when we backtrace, the postprocessing will have completed before
          we invoke _action_set_project_patch_onto_faces, i.e. any update will
          be projected onto the faces.
        """
        assert (
            self._action_set_copy_new_faces_onto_old_faces.descend_invocation_order
            < self._action_set_roll_over_update_of_faces.descend_invocation_order
        )

        step.add_action_set(self._action_set_preprocess_solution)
        step.add_action_set(
            self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement
        )
        step.add_action_set(self._action_set_copy_new_faces_onto_old_faces)
        step.add_action_set(self._action_set_roll_over_update_of_faces)
        step.add_action_set(self._action_set_initial_conditions)
        step.add_action_set(self._action_set_project_patch_onto_faces)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)


    def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
        """
        The boundary information is set only once. It is therefore important that
        we ues the face label and initialise it properly.
        """
        step.add_action_set(self._action_set_initial_conditions_for_grid_construction)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        if evaluate_refinement_criterion:
            step.add_action_set(self._action_set_AMR_throughout_grid_construction)
        else:
            step.add_action_set(self._action_set_AMR_commit_without_further_analysis)


    @property
    def plot_description(
        self,
    ):
        return self._plot_description


    @plot_description.setter
    def plot_description(self, description):
        """!
        Add a proper description to the plots

        Use this one to set a description within the output patch file that tells
        the solver what the semantics of the entries are. Typically, I use
        a comma-separated list here.
        """
        self._plot_description = description


    def add_actions_to_plot_solution(self, step, output_path):
        """!
        Add action sets to plot solution step

        Consult the discussion in add_actions_to_init_grid() around the order
        of the individual action sets.

        It is important that we have the coupling/dynamic AMR part in here, as
        there might be pending AMR refinement requests that now are realised.
        For the same reason, we need the update of the face label and the update
        of the cell label in here: The AMR might just propagate over into the
        plotting, i.e. we might create new grid entities throughout the plot.
        These entities (faces and cells) have to be initialised properly.
        Otherwise, their un-initialised data will propagate through to the next
        time step.

        To make the restriction work, we have to project the solutions onto the
        faces.
        """
        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        step.add_action_set(
            self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement
        )
        step.add_action_set(self._action_set_roll_over_update_of_faces)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_project_patch_onto_faces)
        # step.add_action_set( self._action_set_AMR_commit_without_further_analysis )

        plot_patches_action_set = peano4.toolbox.blockstructured.PlotPatchesInPeanoBlockFormat(
            filename=output_path + "solution-" + self._name,
            patch=self._patch,
            dataset_name=self._unknown_identifier(),
            description=self._plot_description,
            guard="repositories::plotFilter.plotPatch(marker) and "
            + self._load_cell_data_default_guard(),
            additional_includes="""
#include "exahype2/PlotFilter.h"
#include "../repositories/SolverRepository.h"
""",
            precision="PlotterPrecision",
            time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
            select_dofs=self.select_dofs_to_print,
        )
        plot_patches_action_set.descend_invocation_order = self._baseline_action_set_descend_invocation_order
        step.add_action_set(plot_patches_action_set)

        if self._plot_grid_properties:
            plot_grid_properties_action_set = peano4.toolbox.PlotGridInPeanoBlockFormat(
                filename=output_path + "grid-" + self._name,
                cell_unknown=None,
                guard="repositories::plotFilter.plotPatch(marker) and "
                + self._load_cell_data_default_guard(),
                additional_includes="""
#include "exahype2/PlotFilter.h"
#include "../repositories/SolverRepository.h"
""",
            )
            plot_grid_properties_action_set.descend_invocation_order = (
                self._baseline_action_set_descend_invocation_order
            )
            step.add_action_set(plot_grid_properties_action_set)
        pass


    def add_actions_to_perform_time_step(self, step):
        """!
        Add action sets to time step

        See exahype2.solvers.fv.FV.add_actions_to_init_grid() for details on the
        ordering of the action sets.

        ## AMR

        It is important that we do the inter-grid transfer operators before we
        apply the boundary conditions.
        """
        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        step.add_action_set(self._action_set_preprocess_solution)
        step.add_action_set(
            self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement
        )
        step.add_action_set(self._action_set_roll_over_update_of_faces)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_handle_boundary)
        step.add_action_set(self._action_set_update_cell)
        step.add_action_set(self._action_set_project_patch_onto_faces)
        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)


    @abstractmethod
    def add_entries_to_text_replacement_dictionary(self, d):
        pass

    def add_implementation_files_to_project(self, namespace, output, dimensions, subdirectory=""):
        """
        The ExaHyPE project will call this operation when it sets
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

        header_file_abstr_template = templatefile_prefix + "Abstract.template.h"
        cpp_file_abstr_template = templatefile_prefix + "Abstract.template.cpp"
        abstractHeaderDictionary[ "HEADER_FILE_ABSTR_TEMPLATE" ] = os.path.basename(header_file_abstr_template)
        abstractHeaderDictionary[ "CPP_FILE_ABSTR_TEMPLATE" ] = os.path.basename(cpp_file_abstr_template)
        self.add_entries_to_text_replacement_dictionary(abstractHeaderDictionary)

        header_file_template = templatefile_prefix + ".template.h"
        cpp_file_template = templatefile_prefix + ".template.cpp"
        implementationDictionary[ "HEADER_FILE_TEMPLATE" ] = os.path.basename(header_file_template)
        implementationDictionary[ "CPP_FILE_TEMPLATE" ] = os.path.basename(cpp_file_template)
        self.add_entries_to_text_replacement_dictionary(implementationDictionary)

        generated_abstract_header_file = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                header_file_abstr_template,
                cpp_file_abstr_template,
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
                header_file_template,
                cpp_file_template,
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


    def set_solver_constants(self, datastring):
        self._solver_constants = datastring


    def add_solver_constants(self, datastring):
        self._solver_constants += datastring + "\n"


    def _init_dictionary_with_default_parameters(self, d):
        """
        This one is called by all algorithmic steps before I invoke
        add_entries_to_text_replacement_dictionary().

        See the remarks on set_postprocess_updated_patch_kernel to understand why
        we have to apply the (partially befilled) dictionary to create a new entry
        for this very dictionary.
        """
        d["NUMBER_OF_VOLUMES_PER_AXIS"] = self._patch.dim[0]
        d["OVERLAP"] = self._overlap
        d["HALO_SIZE"] = int(self._patch_overlap_old.dim[0] / 2)
        d["SOLVER_INSTANCE"] = self.get_name_of_global_instance()
        d["SOLVER_NAME"] = self._name
        d["UNKNOWN_IDENTIFIER"] = self._unknown_identifier()
        d["NUMBER_OF_UNKNOWNS"] = self._unknowns
        d["NUMBER_OF_AUXILIARY_VARIABLES"] = self._auxiliary_variables
        d["SOLVER_NUMBER"] = 22

        d["ASSERTION_WITH_1_ARGUMENTS"] = "nonCriticalAssertion1"
        d["ASSERTION_WITH_2_ARGUMENTS"] = "nonCriticalAssertion2"
        d["ASSERTION_WITH_3_ARGUMENTS"] = "nonCriticalAssertion3"
        d["ASSERTION_WITH_4_ARGUMENTS"] = "nonCriticalAssertion4"
        d["ASSERTION_WITH_5_ARGUMENTS"] = "nonCriticalAssertion5"
        d["ASSERTION_WITH_6_ARGUMENTS"] = "nonCriticalAssertion6"

        if self._min_volume_h > self._max_volume_h:
            raise Exception("min/max h are inconsistent")

        d["MAX_VOLUME_H"] = self._max_volume_h
        d["MIN_VOLUME_H"] = self._min_volume_h
        d["SOLVER_CONSTANTS"] = self._solver_constants
        d["SOLVER_INCLUDES"] = self.user_solver_includes
        d["BOUNDARY_CONDITIONS_IMPLEMENTATION"] = self._boundary_conditions_implementation
        d["REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
        d["INITIAL_CONDITIONS_IMPLEMENTATION"] = self._initial_conditions_implementation

        d["COMPUTE_KERNEL_CALL"] = jinja2.Template(
            self._compute_kernel_call, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["COMPUTE_KERNEL_CALL_STATELESS"] = jinja2.Template(
            self._compute_kernel_call_stateless, undefined=jinja2.DebugUndefined
        ).render(**d)

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

        d["PREPROCESS_RECONSTRUCTED_PATCH"] = jinja2.Template(
            self._preprocess_reconstructed_patch, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["POSTPROCESS_UPDATED_PATCH"] = jinja2.Template(
            self._postprocess_updated_patch, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["COMPUTE_TIME_STEP_SIZE"] = jinja2.Template(
            self._compute_time_step_size, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["COMPUTE_NEW_TIME_STEP_SIZE"] = jinja2.Template(
            self._compute_new_time_step_size, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["COMPUTE_MAX_EIGENVALUE"] = self._compute_eigenvalue

        d["STATELESS_PDE_TERMS"] = self._pde_terms_without_state
        d["KERNEL_NAMESPACE"] = self._kernel_namespace

        d["USE_VARIABLE_SHORTCUT"] = self._use_var_shortcut
        d["VARIABLE_NAMES"] = self._variable_names
        d["VARIABLE_POSITIONS"] = self._variable_pos


    @property
    def unknowns(self):
        return self._unknowns


    @property
    def patch_size(self):
        return self._patch_size


    @property
    def auxiliary_variables(self):
        return self._auxiliary_variables


    @patch_size.setter
    def patch_size(self, value):
        self._patch_size = value
        self.create_data_structures()
        self.create_action_sets()


    @unknowns.setter
    def unknowns(self, value):
        self._unknowns = value
        self.create_data_structures()
        self.create_action_sets()


    @auxiliary_variables.setter
    def auxiliary_variables(self, value):
        self._auxiliary_variables = value
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
        halo. One the solution is copied over, the kernel writes in the halo
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

        ## Access

        We recommend that you use the AoS enumerator exahype2::enumerator::AoSLexicographicEnumerator.
        This one has to be given 1 as number of cells/patches, it requires the size
        of the patch, its halo size, you have to pass it the unknowns and the correct
        number of auxiliary parameters. If you want the preprocessing also to alter
        the actual input, you need a second enumerator. In this second one, all
        parameters are the same besides the halo size, which you have to set to 0:
        The input patch has no halo. This halo is only added temporarily in the
        reconstruction step.
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


    def switch_storage_scheme(
        self,
        cell_data_storage: Storage,
        face_data_storage: Storage,
    ):
        """
        By default, we hold all data on the call stacks. You can explicitly switch
        to storage on the heap via smart pointers.

        @see create_data_structures()
        """
        assert isinstance(cell_data_storage, Storage)
        assert isinstance(face_data_storage, Storage)

        self._cell_data_storage = cell_data_storage
        self._face_data_storage = face_data_storage

        self.create_data_structures()
        self.create_action_sets()
