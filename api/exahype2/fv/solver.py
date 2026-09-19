# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.actionsets
import exahype2.fv.actionsets
import exahype2.particles
import exahype2.pde_terms
import exahype2.plotter
import exahype2.solver

from exahype2.layout import Layout
from exahype2.layout import volume_loop

from .patch_data import create_patch_and_halo
from .scheme import Godunov
from .scheme import MusclHancock


def _pde_terms():
    return {
        "initial_condition": exahype2.pde_terms.initial_condition(),
        "boundary_conditions": exahype2.pde_terms.boundary_conditions("storage"),
        "refinement_criterion": exahype2.pde_terms.refinement_criterion(
            "[NumberOfFiniteVolumesPerAxisPerPatch^DIMENSIONS * ({N} + {A})]"
        ),
        "max_eigenvalue": exahype2.pde_terms.max_eigenvalue(),
        "flux": exahype2.pde_terms.flux(),
        "ncp": exahype2.pde_terms.ncp(),
        "source": exahype2.pde_terms.source(),
        "diffusive_source": exahype2.pde_terms.diffusive_source(),
        "riemann_solver": exahype2.pde_terms.finite_volume_riemann_solver(),
        "limited_difference": exahype2.pde_terms.limited_difference(),
        "reconstruction": exahype2.pde_terms.PDETerm(
            cpp="reconstruct",
            concept="HasReconstruction",
            parameters=(
                "const {P}* const NOALIAS QL // [{N} + {A}]",
                "const {P}* const NOALIAS Q // [{N} + {A}]",
                "const {P}* const NOALIAS QR // [{N} + {A}]",
                "const {P}* const NOALIAS limitedDifferences // [{N}]",
                "const tarch::la::Vector<DIMENSIONS, double>& x",
                "const tarch::la::Vector<DIMENSIONS, double>& h",
                "const int normal",
                "const int unknown",
                "{P}& reconstructedMinus",
                "{P}& reconstructedPlus",
            ),
            flag="USE_RECONSTRUCTION",
        ),
        "analytical_solution": exahype2.pde_terms.analytical_solution(),
        "particle_analytical_solution": exahype2.pde_terms.particle_analytical_solution(),
    }


_DESCEND_INVOCATION_ORDER = {
    "roll_over_updates": 0,
    "update_face_label": 1,
    "update_cell_label": 1,
    "initial_conditions": 1,
    "apply_boundary_conditions": 1,
    "project_patch_onto_faces": 1,
    "adaptive_mesh_face_coupling": 1,
    "dynamic_amr_transfer": 1,
    "amr": 2,
    "amr_throughout_grid_construction": 2,
    "amr_throughout_dynamic_amr": 2,
    "update_patch": 2,
}


class Solver(exahype2.solver.Solver):
    """A finite-volume solver on patches of ``patch_size``^d volumes per cell.

    ``scheme`` is :class:`exahype2.fv.Godunov` or :class:`exahype2.fv.MusclHancock`.
    """

    FAMILY = "fv"
    FAMILY_SOLVER_INCLUDE = "exahype2/fv/Solver.h"
    FAMILY_SOLVER_CONCEPT = "::exahype2::fv::SolverConcept"
    FAMILY_SOLVER_CONCEPT_PRECISIONS = ("STORAGE_PRECISION",)
    PDE_TERMS = _pde_terms()

    def __init__(
        self,
        name,
        patch_size,
        unknowns,
        auxiliary_variables,
        time_stepping,
        scheme=None,
        precision="double",
        baseline_action_set_descend_invocation_order=0,
    ):
        super().__init__(name, unknowns, auxiliary_variables, time_stepping, precision)
        self.scheme = Godunov() if scheme is None else scheme
        if not isinstance(self.scheme, (Godunov, MusclHancock)):
            raise TypeError(
                "scheme is exahype2.fv.Godunov() or exahype2.fv.MusclHancock(...)."
            )
        self._patch_size = patch_size
        self._overlap = self.scheme.halo_width

        self._plotter = exahype2.plotter.PatchPlotter

        self._postprocess_updated_patch = ""
        self._baseline_action_set_descend_invocation_order = (
            baseline_action_set_descend_invocation_order
        )

    @property
    def patch_size(self):
        return self._patch_size

    @property
    def overlap(self):
        return self._overlap

    def statistics_metadata(self):
        return super().statistics_metadata() + [("Patch Size", self._patch_size)]

    def current_unknown_identifier(self):
        return self._name + "QNew"

    def previous_unknown_identifier(self):
        return self._name + "QOld"

    def _create_layout(self):
        return Layout(
            dofs_per_axis=self._patch_size,
            representation="cell",
            loop=volume_loop(),
            solutions={
                "current": self.current_unknown_identifier(),
                "previous": self.previous_unknown_identifier(),
            },
            includes=["exahype2/fv/ForEachVolume.h"],
            tracer_include="exahype2/fv/Tracer.h",
            tracer_arguments=(
                "marker,\n"
                "{{PATCH_SIZE}},\n"
                "{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}},\n"
                "fineGridCell{{CURRENT_UNKNOWN_IDENTIFIER}}.data(),\n"
                "*particle"
            ),
        )

    def create_data_structures(self):
        """The old halo is filled locally, so only the new halo is exchanged."""
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
            self.state.number_of_values,
            self.precision.storage,
        )

        for halo in (self._patch_overlap_old, self._patch_overlap_new):
            halo.generator.includes += [
                "peano4/Loop.h",
                "repositories/SolverRepository.h",
            ]
        self._patch_overlap_old.generator.send_condition = "false"
        self._patch_overlap_old.generator.receive_and_merge_condition = "false"
        self._patch_overlap_old.generator.can_receive = "false"
        self._patch_overlap_old.generator.can_send = "false"

        self._patch_overlap_new.generator.send_condition = "true"
        self._patch_overlap_new.generator.receive_and_merge_condition = "true"
        self._patch_overlap_new.generator.can_receive = (
            self._load_face_data_default_guard()
        )
        self._patch_overlap_new.generator.can_send = (
            self._store_face_data_default_guard()
        )
        self._patch_overlap_new.generator.halo_layout = "overlapping_volumes"

        for patch in (self._patch, self._patch_old):
            patch.generator.includes.append("repositories/SolverRepository.h")

        self._cell_label = exahype2.actionsets.create_cell_label(self._name)
        self._face_label = exahype2.actionsets.create_face_label(self._name)

    def create_action_sets(self):
        order = self._baseline_action_set_descend_invocation_order
        store_cell = self._store_cell_data_default_guard()
        self._action_set_initial_conditions = exahype2.actionsets.InitialCondition(
            self, store_cell
        )
        self._action_set_amr_throughout_grid_construction = (
            exahype2.actionsets.RefinementCriterion(
                solver=self,
                guard=store_cell,
                build_up_new_refinement_instructions=True,
                implement_previous_refinement_instructions=True,
                called_by_grid_construction=True,
                skip_after_topology_change=False,
            )
        )
        self._action_set_amr = exahype2.actionsets.RefinementCriterion(
            solver=self,
            guard=store_cell,
            build_up_new_refinement_instructions=True,
            implement_previous_refinement_instructions=False,
            called_by_grid_construction=False,
            skip_after_topology_change=True,
        )
        self._action_set_amr_throughout_dynamic_amr = (
            exahype2.actionsets.RefinementCriterion(
                solver=self,
                guard=store_cell
                + " and fineGridCell"
                + self._cell_label.name
                + ".getRequiresCriterionEvaluation()",
                build_up_new_refinement_instructions=True,
                implement_previous_refinement_instructions=False,
                called_by_grid_construction=False,
                skip_after_topology_change=False,
            )
        )
        self._action_set_apply_boundary_conditions = (
            exahype2.fv.actionsets.ApplyBoundaryConditions(self, store_cell)
        )
        self._action_set_project_patch_onto_faces = (
            exahype2.fv.actionsets.ProjectPatchOntoFaces(self, store_cell)
        )
        self._action_set_roll_over_updates = exahype2.fv.actionsets.RollOverUpdates(
            self, self._store_face_data_default_guard(), store_cell
        )
        self._action_set_adaptive_mesh_face_coupling = (
            exahype2.fv.actionsets.AdaptiveMeshFaceCoupling(solver=self)
        )
        self._action_set_dynamic_amr_transfer = (
            exahype2.fv.actionsets.DynamicAMRTransfer(solver=self)
        )
        self._action_set_update_patch = exahype2.fv.actionsets.UpdatePatch(self)

        self._action_set_update_face_label = exahype2.actionsets.UpdateFaceLabel(
            self._name
        )
        self._action_set_update_cell_label = exahype2.actionsets.UpdateCellLabel(
            self._name
        )

        self._apply_descend_invocation_orders(_DESCEND_INVOCATION_ORDER, order)
        self.postprocess_solution.descend_invocation_order = (
            1 if self._error_measurement is not None else order
        )

    def register_data(self, datamodel):
        datamodel.add_cell(self._cell_label)
        datamodel.add_cell(self._patch)
        datamodel.add_cell(self._patch_old)
        datamodel.add_face(self._patch_overlap_old)
        datamodel.add_face(self._patch_overlap_new)
        datamodel.add_face(self._face_label)

    def use_data_in_step(self, step):
        """The labels come before the data whose load and store decisions depend on them."""
        step.use_cell(self._cell_label)
        step.use_cell(self._patch)
        step.use_cell(self._patch_old)
        step.use_face(self._face_label)
        step.use_face(self._patch_overlap_old)
        step.use_face(self._patch_overlap_new)

    def action_sets_per_step(self):
        """The dynamic mesh refinement sweeps alone change the topology: They realise the
        committed refine and coarsen requests and move the solution across every resolution
        transition."""
        plotter = self._plotter(
            filename=self._name,
            patch=self._patch,
            dataset_name=self.current_unknown_identifier(),
            description=self.state.plot_description,
            representation="cell",
            time_stamp_evaluation=self._plot_time_stamp(),
            guard=self._load_cell_data_default_guard(),
        )
        plotter.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )
        return {
            "create_grid": [
                self._action_set_update_face_label,
                self._action_set_update_cell_label,
                self._action_set_amr_throughout_grid_construction,
            ],
            "init_grid": [
                self._action_set_adaptive_mesh_face_coupling,
                self._action_set_roll_over_updates,
                self._action_set_initial_conditions,
                self._action_set_project_patch_onto_faces,
                self._action_set_update_face_label,
                self._action_set_update_cell_label,
                self._action_set_amr,
            ]
            + self._postprocess_action_sets(),
            "plot_solution": [plotter],
            "perform_time_step": [
                self._action_set_adaptive_mesh_face_coupling,
                self._action_set_roll_over_updates,
                self._action_set_update_face_label,
                self._action_set_update_cell_label,
                self._action_set_apply_boundary_conditions,
                self._action_set_update_patch,
                self._action_set_project_patch_onto_faces,
                self._action_set_amr,
            ]
            + self._postprocess_action_sets(),
            "dynamic_amr": [
                self._action_set_update_face_label,
                self._action_set_update_cell_label,
                self._action_set_dynamic_amr_transfer,
                self._action_set_amr_throughout_dynamic_amr,
            ],
        }

    @property
    def postprocess_updated_patch(self):
        return self._postprocess_updated_patch

    @postprocess_updated_patch.setter
    def postprocess_updated_patch(self, kernel):
        """Sets the C++ code that runs after the patch update; it reads ``QOut``, ``marker``,
        ``timeStamp`` and ``timeStepSize``."""
        self._postprocess_updated_patch = kernel

    def _provides_max_eigenvalue(self):
        """The Riemann solver returns the maximum wave speed of every face it solves."""
        return self.pde_terms.is_set("max_eigenvalue") or self.pde_terms.is_set(
            "riemann_solver"
        )

    def _validate(self):
        super()._validate()
        if not self.pde_terms.is_set("riemann_solver") and not self.pde_terms.is_set(
            "max_eigenvalue"
        ):
            raise ValueError(
                "[{}] the max_eigenvalue PDE term is required when no Riemann solver is set.".format(
                    self._name
                )
            )
        if isinstance(self.scheme, MusclHancock) and self.pde_terms.is_set(
            "diffusive_source"
        ):
            raise ValueError("MUSCL-Hancock does not support diffusive source terms.")

    def _error_condition(self):
        return (
            "not marker.hasBeenRefined() and "
            "(repositories::{{SOLVER_INSTANCE}}.isGridInitialisationSweep() or "
            "repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep())"
        )

    def _error_adds_per_cell(self):
        return False

    def _tracing_descend_invocation_order(self):
        return self._action_set_update_patch.descend_invocation_order + 1

    def _create_tracing_action_set(self, particle_set, kernel, arguments, forces):
        if forces is None:
            return super()._create_tracing_action_set(
                particle_set, kernel, arguments, forces
            )
        return exahype2.particles.FiniteVolumesTracingWithForce(
            particle_set, self, self._project, **forces.arguments()
        )

    def use_time_step_size(self, expression):
        raise NotImplementedError(
            "[{}] use_time_step_size applies to ADER-DG solvers only.".format(
                self._name
            )
        )

    def _cpp_base_class(self):
        return "::exahype2::fv::Solver<{}, {}, {}, {}, {}>".format(
            self._abstract_class_name(),
            self._patch_size,
            self.state.unknowns,
            self.state.auxiliary_variables,
            self.time_stepping.cpp,
        )

    def _add_family_entries(self, d):
        d["NUMBER_OF_VOLUMES_PER_AXIS"] = self._patch.dim[0]
        d["OVERLAP"] = self._overlap
        d["HALO_SIZE"] = int(self._patch_overlap_old.dim[0] / 2)
        d["PATCH_CARDINALITY_1D"] = (
            d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_DATA"]
        )
        d["PATCH_CARDINALITY_2D"] = (
            d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["PATCH_CARDINALITY_1D"]
        )
        d["PATCH_CARDINALITY_3D"] = (
            d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["PATCH_CARDINALITY_2D"]
        )
        d["FACE_CARDINALITY_2D"] = int(
            self._patch_overlap_new.no_of_unknowns
            * self._patch_overlap_new.dim[0]
            * self._patch_overlap_new.dim[1]
        )
        d["FACE_CARDINALITY_3D"] = int(
            self._patch_overlap_new.no_of_unknowns
            * self._patch_overlap_new.dim[0]
            * self._patch_overlap_new.dim[1]
            * self._patch_overlap_new.dim[2]
        )
        scheme = self.scheme.subcell_scheme()
        d["RECONSTRUCTION"] = scheme.reconstruction
        d["TIME_STEPPING"] = scheme.time_stepping
        d["PATCH_SIZE"] = self._patch_size
        d["CURRENT_UNKNOWN_IDENTIFIER"] = self.current_unknown_identifier()
