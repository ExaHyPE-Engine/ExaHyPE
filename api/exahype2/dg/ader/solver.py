# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap

import jinja2

import exahype2.actionsets
import exahype2.datamodel
import exahype2.dg.ader.actionsets
import exahype2.error_measurement
import exahype2.pde_terms
import exahype2.plotter
import exahype2.solver

from exahype2.codegen.helper import KeepUndefined
from exahype2.codegen.helper import render_jinja2_template
from exahype2.dg.basis import GaussLegendreBasis
from exahype2.dg.basis import GaussLobattoBasis
from exahype2.dg.limiting.subcell_limiting import SubcellLimiting
from exahype2.layout import Layout
from exahype2.layout import node_loop
from exahype2.layout import subcell_loop

from .generate_kernels import generate_kernels


def _basis_tables(solver_name, precision):
    """Returns the initialiser of the BasisTables aggregate that points at the solver's operator
    tables of one precision."""
    quadrature = "kernels::{}::Quadrature<{}>".format(solver_name, precision)
    matrices = "kernels::{}::DGMatrices<{}>".format(solver_name, precision)
    return "::exahype2::dg::ader::BasisTables<{}>{{{}}}".format(
        precision,
        ", ".join(
            [
                quadrature + "::QuadraturePoints1D",
                quadrature + "::QuadratureWeights1D",
                quadrature + "::InverseQuadratureWeights1D",
                quadrature + "::QuadratureWeightsInCell",
                quadrature + "::InverseQuadratureWeightsInCell",
                matrices + "::StiffnessOperator1D",
                matrices + "::StiffnessOperatorTransposed1D",
                matrices + "::DerivativeOperator1D",
                matrices + "::InversePicardOperator1D",
                matrices + "::BasisFunctionValuesLeft1D",
                matrices + "::BasisFunctionValuesRight1D",
            ]
        ),
    )


def _pde_terms():
    return {
        "initial_condition": exahype2.pde_terms.initial_condition(),
        "boundary_conditions": exahype2.pde_terms.boundary_conditions("compute"),
        "refinement_criterion": exahype2.pde_terms.refinement_criterion("[{N} + {A}]"),
        "max_eigenvalue": exahype2.pde_terms.max_eigenvalue(),
        "flux": exahype2.pde_terms.flux(),
        "diffusive_flux": exahype2.pde_terms.diffusive_flux(),
        "ncp": exahype2.pde_terms.ncp(),
        "source": exahype2.pde_terms.source(),
        "diffusive_source": exahype2.pde_terms.diffusive_source(),
        "material_parameters": exahype2.pde_terms.material_parameters(),
        "point_source": exahype2.pde_terms.point_source(),
        "point_source_locations": exahype2.pde_terms.point_source_locations(),
        "riemann_solver": exahype2.pde_terms.galerkin_riemann_solver(),
        "subcell_riemann_solver": exahype2.pde_terms.finite_volume_riemann_solver(
            flag="USE_SUBCELL_RIEMANN_SOLVER"
        ),
        "physical_admissibility": exahype2.pde_terms.physical_admissibility(),
        "dmp_observables": exahype2.pde_terms.dmp_observables(),
        "dmp_relaxation_parameter": exahype2.pde_terms.dmp_relaxation_parameter(),
        "analytical_solution": exahype2.pde_terms.analytical_solution(),
    }


_DESCEND_INVOCATION_ORDER = {
    "update_face_label": 0,
    "update_cell_label": 0,
    "initial_conditions": 1,
    "adaptive_mesh_face_coupling": 1,
    "dynamic_amr_transfer": 1,
    "correction": 2,
    "prediction": 3,
    "amr": 4,
    "amr_throughout_grid_construction": 4,
    "amr_throughout_dynamic_amr": 4,
}


class Solver(SubcellLimiting, exahype2.solver.Solver):
    """An ADER-DG solver of polynomial order ``order`` with the predictor-corrector sweeps.

    ``basis`` is :class:`exahype2.dg.GaussLegendre` or :class:`exahype2.dg.GaussLobatto`;
    ``linear`` selects the linear predictor, which supports material parameters and point
    sources; ``point_sources`` counts them.
    """

    FAMILY = "ader"
    FAMILY_SOLVER_INCLUDE = "exahype2/dg/ader/Solver.h"
    FAMILY_SOLVER_CONCEPT = "::exahype2::dg::ader::SolverConcept"
    FAMILY_SOLVER_CONCEPT_PRECISIONS = ("STORAGE_PRECISION", "COMPUTE_PRECISION")
    PDE_TERMS = _pde_terms()

    def __init__(
        self,
        name,
        order,
        unknowns,
        auxiliary_variables,
        time_stepping,
        basis=GaussLegendreBasis,
        linear=False,
        precision="double",
        point_sources=0,
        initialise_patches=False,
    ):
        super().__init__(name, unknowns, auxiliary_variables, time_stepping, precision)
        if basis not in (GaussLegendreBasis, GaussLobattoBasis):
            raise ValueError(
                "basis is exahype2.dg.GaussLegendre or exahype2.dg.GaussLobatto."
            )
        if (
            not isinstance(point_sources, int)
            or isinstance(point_sources, bool)
            or point_sources < 0
        ):
            raise ValueError(
                "point_sources counts the point sources and cannot be negative."
            )
        self._order = order
        self._basis = basis(order + 1)
        self._is_linear = linear
        self._number_of_point_sources = point_sources
        self._initialise_patches = initialise_patches

        self._plotter = exahype2.plotter.PatchPlotter
        self._face_plotter = None
        self._face_plotter_guard = None

        self._postprocess_updated_patch = ""

    @property
    def order(self):
        return self._order

    @property
    def basis(self):
        return self._basis

    def statistics_metadata(self):
        return super().statistics_metadata() + [("Order", self._order)]

    def current_unknown_identifier(self):
        return self._name + "Q"

    def previous_unknown_identifier(self):
        return self._name + "Q_old"

    def _create_layout(self):
        includes = ["exahype2/dg/DGUtils.h", "exahype2/dg/ForEachNode.h"]
        if self.subcell_limiter_is_enabled and self._error_measurement is not None:
            includes += [
                "exahype2/dg/LimiterStatus.h",
                "exahype2/dg/limiting/ForEachSubcell.h",
            ]
        return Layout(
            dofs_per_axis=self._order + 1,
            representation="nodal",
            loop=node_loop(),
            solutions={
                "current": self.current_unknown_identifier(),
                "previous": self.previous_unknown_identifier(),
            },
            includes=includes,
            tracer_include="exahype2/dg/Tracer.h",
            tracer_arguments=(
                "marker,\n"
                "{{DG_ORDER}},\n"
                "kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::QuadraturePoints1D,\n"
                "{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}},\n"
                "fineGridCell{{CURRENT_UNKNOWN_IDENTIFIER}}.data(),\n"
                "*particle"
            ),
            plot_mapping=self._plot_mapping(),
            initialises_patches=self._initialise_patches,
        )

    def create_data_structures(self):
        """The two projections hold the solution and the fluxes the space-time predictor writes
        on the faces; the left cell projects to the left half and the right cell to the right
        half, so the merge copies half a face."""
        nodes = (self._order + 1, self._order + 1, self._order + 1)
        self._current_time_step = exahype2.datamodel.Patch(
            nodes,
            self.state.number_of_values,
            self.current_unknown_identifier(),
            self.precision.storage,
        )
        self._previous_time_step = exahype2.datamodel.Patch(
            nodes,
            self.state.number_of_values,
            self.previous_unknown_identifier(),
            self.precision.storage,
        )
        self._rhs_estimates_projection = exahype2.datamodel.Patch(
            (2, self._order + 1, self._order + 1),
            self.state.number_of_values,
            self._unknown_identifier() + "Estimates",
            self.precision.compute,
        )
        self._flux_estimates_projection = exahype2.datamodel.Patch(
            (2, self._order + 1, self._order + 1),
            self.state.unknowns,
            self._unknown_identifier() + "FluxEstimates",
            self.precision.compute,
        )
        for patch, precision in (
            (self._current_time_step, self.precision.storage),
            (self._previous_time_step, self.precision.storage),
            (self._rhs_estimates_projection, self.precision.compute),
            (self._flux_estimates_projection, self.precision.compute),
        ):
            patch.generator = exahype2.datamodel.PatchGenerator(patch, precision)
            patch.generator.includes.append("repositories/SolverRepository.h")

        for projection in (
            self._rhs_estimates_projection,
            self._flux_estimates_projection,
        ):
            projection.generator.halo_layout = "projection_halves"
            projection.generator.send_condition = self._projection_send_condition()
            projection.generator.receive_and_merge_condition = (
                self._projection_receive_condition()
            )
            projection.generator.can_receive = self._load_face_data_default_guard()
            projection.generator.can_send = self._store_face_data_default_guard()

        for patch in (self._current_time_step, self._previous_time_step):
            patch.generator.can_receive = self._load_cell_data_default_guard()
            patch.generator.can_send = self._store_cell_data_default_guard()

        self._cell_label = exahype2.actionsets.create_cell_label(self._name)
        self._face_label = exahype2.actionsets.create_face_label(self._name)

        self._add_subcell_limiter_data_structures()
        self._configure_subcell_limiter_exchange()

    def _prediction_on_hanging_cells_sweep(self):
        return self._sweep_is("isPredictionOnHangingCellsSweep")

    def _interpolate_face_data_default_guard(self):
        return (
            super()._interpolate_face_data_default_guard()
            + " and "
            + self._prediction_on_hanging_cells_sweep()
        )

    def _restrict_face_data_default_guard(self):
        return (
            super()._restrict_face_data_default_guard()
            + " and "
            + self._prediction_on_hanging_cells_sweep()
        )

    def _projection_send_condition(self):
        instance = "repositories::" + self.instance_name
        return (
            "((" + instance + ".isFirstGridSweepOfTimeStep()) or "
            "(" + self._prediction_on_hanging_cells_sweep() + ") or "
            "("
            + self._sweep_is("isPlottingSweep")
            + " and "
            + instance
            + ".getMinTimeStamp() != 0.0))"
        )

    def _projection_receive_condition(self):
        instance = "repositories::" + self.instance_name
        return (
            "((" + self._prediction_on_hanging_cells_sweep() + ") or "
            "(" + instance + ".isLastGridSweepOfTimeStep()) or "
            "("
            + self._sweep_is("isPlottingSweep")
            + " and "
            + instance
            + ".getMinTimeStamp() != 0.0))"
        )

    def create_action_sets(self):
        instance = "repositories::" + self.instance_name
        store_cell = self._store_cell_data_default_guard()

        self._action_set_update_face_label = exahype2.actionsets.UpdateFaceLabel(
            self._name
        )
        self._action_set_update_cell_label = exahype2.actionsets.UpdateCellLabel(
            self._name
        )

        self._action_set_initial_conditions = exahype2.actionsets.InitialCondition(
            self,
            store_cell,
            epilogue="""
  std::copy_n(
    fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    {{NUMBER_OF_DOFS_PER_CELL}} * {{NUMBER_OF_DATA}},
    fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data()
  );
""",
        )

        self._action_set_adaptive_mesh_face_coupling = (
            exahype2.dg.ader.actionsets.AdaptiveMeshFaceCoupling(
                solver=self,
                interpolate_guard=self._interpolate_face_data_default_guard(),
                restrict_guard=self._restrict_face_data_default_guard(),
                solve_guard=store_cell
                + " and "
                + self._prediction_on_hanging_cells_sweep(),
            )
        )
        self._action_set_dynamic_amr_transfer = (
            exahype2.dg.ader.actionsets.DynamicAMRTransfer(solver=self)
        )

        if self.subcell_limiter_is_enabled:
            corrector_guard = self._subcell_limiter_state_guard("CorrectAndFlag")
        else:
            corrector_guard = instance + ".isLastGridSweepOfTimeStep()"
        self._action_set_correction = exahype2.dg.ader.actionsets.Correction(
            solver=self,
            cell_guard=store_cell + " and " + corrector_guard,
            face_guard=self._store_face_data_default_guard()
            + " and "
            + corrector_guard
            + " and not (fineGridFace"
            + self._name
            + "FaceLabel.getAboveHanging()[0] or fineGridFace"
            + self._name
            + "FaceLabel.getAboveHanging()[1])",
            hanging_face_guard=self._interpolate_face_data_default_guard(),
        )

        self._action_set_prediction = exahype2.dg.ader.actionsets.Prediction(
            self, store_cell + " and " + instance + ".isFirstGridSweepOfTimeStep()"
        )

        self._action_set_amr = exahype2.actionsets.RefinementCriterion(
            solver=self,
            guard=store_cell
            + " and ("
            + instance
            + ".isFirstGridSweepOfTimeStep() or "
            + self._sweep_is("isGridInitialisationSweep")
            + ")",
            build_up_new_refinement_instructions=True,
            called_by_grid_construction=False,
            implement_previous_refinement_instructions=False,
            skip_after_topology_change=True,
            solution="previous",
        )
        self._action_set_amr_throughout_grid_construction = (
            exahype2.actionsets.RefinementCriterion(
                solver=self,
                guard=store_cell,
                build_up_new_refinement_instructions=True,
                implement_previous_refinement_instructions=True,
                called_by_grid_construction=True,
                skip_after_topology_change=False,
                solution="previous",
            )
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
                solution="previous",
            )
        )
        self._apply_descend_invocation_orders(_DESCEND_INVOCATION_ORDER)

        self.postprocess_solution.descend_invocation_order = (
            3 if self._error_measurement is not None else 4
        )

        self._create_subcell_limiter_action_sets()

    def _plotted_solution_patch(self):
        """Fused ADER-DG corrects into ``Q`` and copies ``Q`` into ``Q_old`` in one traversal;
        with subcell limiting ``Q_old`` stays the predictor backup and ``Q`` carries the accepted
        solution."""
        if self.subcell_limiter_is_enabled:
            return self._current_time_step, self.current_unknown_identifier()
        return self._previous_time_step, self.previous_unknown_identifier()

    def register_data(self, datamodel):
        datamodel.add_cell(self._cell_label)
        datamodel.add_cell(self._current_time_step)
        datamodel.add_cell(self._previous_time_step)
        datamodel.add_face(self._face_label)
        datamodel.add_face(self._rhs_estimates_projection)
        datamodel.add_face(self._flux_estimates_projection)
        self._add_subcell_limiter_to_datamodel(datamodel)

    def use_data_in_step(self, step):
        step.use_cell(self._cell_label)
        step.use_cell(self._current_time_step)
        step.use_cell(self._previous_time_step)
        step.use_face(self._face_label)
        step.use_face(self._rhs_estimates_projection)
        step.use_face(self._flux_estimates_projection)
        self._use_subcell_limiter_data(step)

    def _plot_mapping(self):
        quadrature_points = [float(point) for point in self._basis.quadrature_points]
        return [
            (x, y, z)
            for z in quadrature_points
            for y in quadrature_points
            for x in quadrature_points
        ]

    def _plotters(self):
        patch, dataset = self._plotted_solution_patch()
        mapping = self._plot_mapping()
        result = [
            self._solution_plotter()(
                filename=self._name,
                patch=patch,
                dataset_name=dataset,
                description=self.state.plot_description,
                representation="nodal",
                mapping=mapping,
                time_stamp_evaluation=self._plot_time_stamp(),
                guard=self._load_cell_data_default_guard(),
            )
        ]
        if self._face_plotter is not None:
            result.append(
                self._face_plotter(
                    filename=self._name + "-face",
                    patch=self._rhs_estimates_projection,
                    dataset_name=self._unknown_identifier() + "Estimates",
                    description=self.state.plot_description,
                    mapping=mapping,
                    time_stamp_evaluation=self._plot_time_stamp(),
                    guard=self._load_face_data_default_guard()
                    + " and not (marker.hasCellBeenRefined() and fineGridFace"
                    + self._name
                    + "FaceLabel.getBoundary()) and "
                    + self._face_plotter_guard,
                )
            )
        return result

    def action_sets_per_step(self):
        """The dynamic mesh refinement sweep alone changes the topology: It realises the refine
        and erase requests the time step committed."""
        return {
            "create_grid": [
                self._action_set_update_face_label,
                self._action_set_update_cell_label,
                self._action_set_amr_throughout_grid_construction,
            ],
            "init_grid": [self._action_set_initial_conditions]
            + self._subcell_limiter_action_sets("init_grid")
            + [
                self._action_set_update_face_label,
                self._action_set_update_cell_label,
                self._action_set_amr,
            ]
            + self._postprocess_action_sets(),
            "plot_solution": self._plotters(),
            "perform_time_step": [
                self._action_set_update_face_label,
                self._action_set_update_cell_label,
                self._action_set_adaptive_mesh_face_coupling,
                self._action_set_correction,
                self._action_set_prediction,
                self._action_set_amr,
            ]
            + self._postprocess_action_sets()
            + self._subcell_limiter_action_sets("perform_time_step"),
            "dynamic_amr": [
                self._action_set_update_face_label,
                self._action_set_update_cell_label,
                self._action_set_dynamic_amr_transfer,
            ]
            + self._subcell_limiter_action_sets("dynamic_amr")
            + [self._action_set_amr_throughout_dynamic_amr],
        }

    def add_face_plotter(self, guard="true"):
        """Adds a plotter that writes the projected face estimates to ``<name>-face``."""
        self._face_plotter = exahype2.plotter.FacePlotter
        self._face_plotter_guard = guard

    @property
    def postprocess_updated_patch(self):
        return self._postprocess_updated_patch

    @postprocess_updated_patch.setter
    def postprocess_updated_patch(self, kernel):
        """Sets the C++ code that runs after the corrector; it reads ``QNew``, ``marker``,
        ``timeStamp`` and ``timeStepSize``."""
        self._postprocess_updated_patch = kernel

    def _validate(self):
        super()._validate()
        self._validate_subcell_limiter()
        if not self._is_linear and (
            self.pde_terms.is_set("material_parameters")
            or self.pde_terms.is_set("point_source")
        ):
            raise ValueError(
                "Material parameters and point sources are only available for linear kernels."
            )
        if self._is_linear and (
            self.pde_terms.is_set("diffusive_flux")
            or self.pde_terms.is_set("diffusive_source")
        ):
            raise ValueError(
                "Linear ADER-DG does not support diffusive fluxes or source terms."
            )
        if self.pde_terms.is_set("flux") and self.pde_terms.is_set("diffusive_flux"):
            raise ValueError("A solver sets flux or diffusive_flux, not both.")
        point_source_pde_terms = [
            self.pde_terms.is_set("point_source"),
            self.pde_terms.is_set("point_source_locations"),
        ]
        if self._number_of_point_sources > 0 and not all(point_source_pde_terms):
            raise ValueError(
                "[{}] point sources need the point_source and point_source_locations PDE terms.".format(
                    self._name
                )
            )
        if self._number_of_point_sources == 0 and any(point_source_pde_terms):
            raise ValueError(
                "[{}] the point_source and point_source_locations PDE terms need point_sources > 0.".format(
                    self._name
                )
            )
        if not self.pde_terms.is_set("riemann_solver") and not self.pde_terms.is_set(
            "max_eigenvalue"
        ):
            raise ValueError(
                "[{}] the max_eigenvalue PDE term is required when no Riemann solver is set.".format(
                    self._name
                )
            )

    def _error_condition(self):
        return (
            "(repositories::{{SOLVER_INSTANCE}}.isGridInitialisationSweep() or "
            "repositories::{{SOLVER_INSTANCE}}.isFirstGridSweepOfTimeStep()) and "
            + self._provide_cell_data_to_compute_kernels_default_guard()
        )

    def _error_solution(self):
        return "previous"

    _TEMPLATE_ERROR_LOOPS_WITH_SUBCELLS = """
  const bool measureSubcellSolution =
    not repositories::{{SOLVER_INSTANCE}}.isLimitingSweep(::exahype2::dg::LimitingSolverState::GridInitialisation)
    and ::exahype2::dg::runsSubcellScheme(fineGridCell{{SOLVER_NAME}}CellLabel.getLimiterStatus());
  if (measureSubcellSolution) {
    {{SUBCELL_LOOP | indent(4)}}
  } else {
    {{NODE_LOOP | indent(4)}}
  }
"""

    def _error_loops(self):
        """A cell the subcell scheme carries measures its subcell averages, which hold its
        accepted state."""
        nodes = super()._error_loops()
        if not self.subcell_limiter_is_enabled:
            return nodes
        subcells = render_jinja2_template(
            subcell_loop(),
            {
                "ACCESSOR": "fineGridCell{{SOLVER_NAME}}SubcellOld",
                "BODY": textwrap.dedent(
                    exahype2.error_measurement.TEMPLATE_ACCUMULATE_PER_CELL
                ).strip(),
            },
            KeepUndefined,
        )
        return render_jinja2_template(
            self._TEMPLATE_ERROR_LOOPS_WITH_SUBCELLS,
            {
                "SUBCELL_LOOP": textwrap.dedent(subcells).strip(),
                "NODE_LOOP": textwrap.dedent(nodes).strip(),
            },
            KeepUndefined,
        )

    def _cpp_base_class(self):
        return "::exahype2::dg::ader::Solver<{}, {}, {}, {}, {}, {}, {}>".format(
            self._abstract_class_name(),
            self._order,
            self.state.unknowns,
            self.state.auxiliary_variables,
            self.time_stepping.cpp,
            "true" if self.subcell_limiter_is_enabled else "false",
            self._number_of_point_sources,
        )

    def _kernel_table_includes(self):
        return [
            "kernels/{}/Quadrature.h".format(self._name),
            "kernels/{}/DGMatrices.h".format(self._name),
        ]

    def _kernel_tables(self):
        result = []
        for precision in self._table_precisions():
            result.append("kernels::{}::Quadrature<{}>".format(self._name, precision))
            result.append("kernels::{}::DGMatrices<{}>".format(self._name, precision))
        return result

    def _table_precisions(self):
        """The mesh transfer operators read double tables whatever the solver computes in."""
        result = list(self.precision.types)
        if "double" not in result:
            result.append("double")
        return result

    def _generate_kernels(self, output, dictionary):
        generate_kernels(solver=self, dictionary=dictionary, output=output)

    def _add_family_entries(self, d):
        d["ORDER"] = self._order
        d["DG_ORDER"] = self._order
        d["NUMBER_OF_DOF"] = self._order + 1
        d["NUMBER_OF_DOF_3D"] = 1 if self._dimensions == 2 else self._order + 1
        d["NUMBER_OF_DOFS_PER_CELL"] = (self._order + 1) ** self._dimensions
        d["CURRENT_UNKNOWN_IDENTIFIER"] = self.current_unknown_identifier()
        d["QUADRATURE_TABLE"] = "kernels::{}::Quadrature<{}>".format(
            self._name, self.precision.storage
        )
        d["IS_LINEAR"] = self._is_linear
        d["LINEARITY"] = "linear" if self._is_linear else "nonlinear"
        d["INITIALISE_PATCHES"] = self._initialise_patches
        d["NUMBER_OF_POINT_SOURCES"] = self._number_of_point_sources
        d["USE_GAUSS_LOBATTO"] = (
            "true" if isinstance(self._basis, GaussLobattoBasis) else "false"
        )
        d["POLYNOMIAL_TYPE"] = (
            "lobatto" if isinstance(self._basis, GaussLobattoBasis) else "legendre"
        )

        d["PREDICTOR_COMPUTATION_PRECISIONS"] = [self.precision.compute]
        d["CORRECTOR_COMPUTATION_PRECISION"] = self.precision.compute
        d["PICARD_COMPUTATION_PRECISION"] = self.precision.compute
        d["NUMBER_OF_PREDICTOR_PRECISIONS"] = 1
        d["NUMBER_OF_PRECISIONS"] = len(self.precision.types)
        d["TABLE_PRECISIONS"] = self._table_precisions()

        d["SUBCELL_LIMITER"] = self.subcell_limiter_is_enabled
        d["SUBCELL_NUMBER_OF_DMP_OBSERVABLES"] = (
            self._subcell_limiter_number_of_dmp_observables
            if self.subcell_limiter_is_enabled
            else 0
        )
        d["POSTPROCESS_UPDATED_PATCH"] = textwrap.dedent(
            render_jinja2_template(
                self._postprocess_updated_patch, d, jinja2.DebugUndefined
            )
        ).strip()
        d["PREDICTOR_TABLES"] = _basis_tables(self._name, self.precision.compute)
        d["PICARD_TABLES"] = _basis_tables(self._name, self.precision.compute)
