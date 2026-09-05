# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import functools

import jinja2

import peano4.checkpoint

from exahype2.solvers.fv import create_patch_and_halo

from .Labels import (
    add_limiter_attributes_to_cell_label,
    add_limiter_attributes_to_face_label,
    face_status_merge_snippet,
)
from exahype2.solvers.fv.SubcellScheme import SubcellScheme
from .actionsets.CheckpointLoading import CheckpointLoading
from .actionsets.InitialiseSubcells import InitialiseSubcells
from .actionsets.Predict import Predict
from .actionsets.CorrectAndFlag import CorrectAndFlag
from .actionsets.SpreadDomain import SpreadDomain
from .actionsets.RecomputeSubcells import RecomputeSubcells
from .actionsets.DynamicAMR import DynamicAMR


# The pipeline action sets overlay the discontinuous Galerkin predictor and
# corrector, so they descend after those kernels (correction is 2, prediction 3)
# to read the candidate the corrector wrote.
_SUBCELL_LIMITER_DESCEND_ORDER = 4


# The DG solution transfer descends first. The limiter transfer then uses the
# prolonged DG state to initialise a newly created regular cell's subcell data.
_SUBCELL_DYNAMIC_AMR_DESCEND_ORDER = 2


# The subcell sampling follows the nodal initial conditions, which descend at 1.
_SUBCELL_INITIALISE_DESCEND_ORDER = 2


# The checkpoint loader overwrites the sampled subcells, so it descends after
# the initialisation.
_SUBCELL_CHECKPOINT_LOADING_DESCEND_ORDER = 3


# The line the base face-label merge (grid/FaceLabel.py) ends its owned-side block
# with. The limiter status is spliced right after it, so a change to that merge
# raises an error here if the status would otherwise remain unexchanged.
_FACE_MERGE_ANCHOR = (
    "setAboveHanging( neighbourEntry, neighbour.getAboveHanging( neighbourEntry ) );"
)


_SUBCELL_HALO_INCLUDES = """
#include "peano4/Loop.h"
#include "repositories/SolverRepository.h"
"""


class SubcellLimiting:
    @property
    def subcell_limiter_is_enabled(self):
        return getattr(self, "_subcell_limiter_solver", None) is not None

    def add_subcell_limiter(
        self,
        limiter_solver,
        number_of_dmp_observables=0,
        dmp_relaxation_parameter=0.0,
        dmp_differences_scaling=0.0,
        dmp_observables=None,
        dmp_relaxation_parameter_implementation=None,
        physical_admissibility_criterion=None,
    ):
        """Enables a-posteriori limiting with ``limiter_solver``.

        ``limiter_solver`` is a finite-volume :class:`SubcellScheme`, and the
        solver requires a ``physical_admissibility_criterion``, a positive
        ``number_of_dmp_observables``, or both. ``dmp_observables`` maps one nodal
        ``Q`` state to that many ``observables``, and
        ``dmp_relaxation_parameter_implementation`` returns each relaxation from
        ``observable`` and ``specifiedRelaxationParameter``. The admissible range
        widens by the larger of ``dmp_relaxation_parameter`` and
        ``dmp_differences_scaling`` times the reference range.
        """
        if self.subcell_limiter_is_enabled:
            raise RuntimeError("The subcell limiter is already configured.")
        if limiter_solver is None:
            raise ValueError("The subcell limiter needs a finite-volume solver.")
        if not isinstance(limiter_solver, SubcellScheme):
            raise TypeError(
                "The subcell limiter solver must be a finite-volume SubcellScheme, "
                "such as exahype2.solvers.fv.godunov.subcell_scheme() or "
                "exahype2.solvers.fv.musclhancock.subcell_scheme()."
            )
        # The adaptive transfer sums its contractions in double and rounds once, so a
        # wider storage type would be announced here and then dropped on the first
        # resolution transition.
        if self._solution_persistent_storage_precision not in ("float", "double"):
            raise ValueError(
                "The subcell limiter supports float and double solution storage."
            )
        subcells_per_axis = 2 * self._order + 1
        if limiter_solver.halo_width > subcells_per_axis:
            raise ValueError(
                "The subcell scheme's halo width ({}) must not exceed the {} "
                "subcells per axis, so a recomputed cell's halo reaches only its "
                "immediate neighbour and the finite-volume domain grows a fixed two "
                "rings. Increase the order or choose a scheme with a narrower stencil.".format(
                    limiter_solver.halo_width, subcells_per_axis
                )
            )
        if number_of_dmp_observables < 0:
            raise ValueError(
                "The number of discrete-maximum-principle observables cannot be negative."
            )
        if dmp_relaxation_parameter < 0.0 or dmp_differences_scaling < 0.0:
            raise ValueError(
                "The discrete-maximum-principle relaxation parameters cannot be negative."
            )

        uses_dmp = number_of_dmp_observables > 0
        uses_criterion = physical_admissibility_criterion is not None
        if not uses_dmp and not uses_criterion:
            raise ValueError(
                "The subcell limiter needs a physical-admissibility criterion, "
                "a discrete maximum principle, or both."
            )
        if uses_dmp and dmp_observables is None:
            raise ValueError(
                "A discrete maximum principle needs a dmp_observables mapping."
            )

        self._subcell_limiter_solver = limiter_solver
        self._subcell_limiter_uses_dmp = uses_dmp
        self._subcell_limiter_uses_admissibility_criterion = uses_criterion
        self._subcell_limiter_number_of_dmp_observables = number_of_dmp_observables
        self._subcell_limiter_dmp_relaxation_parameter = dmp_relaxation_parameter
        self._subcell_limiter_dmp_differences_scaling = dmp_differences_scaling
        self._subcell_limiter_dmp_observables = dmp_observables
        self._subcell_limiter_dmp_relaxation_parameter_implementation = (
            dmp_relaxation_parameter_implementation
        )
        self._subcell_limiter_physical_admissibility_criterion = (
            physical_admissibility_criterion
        )

        self._refresh_solver_user_code()
        self.create_data_structures()
        self.create_action_sets()

        if hasattr(self, "_plotter"):
            self._plotter = self._subcell_limiter_status_plotter()

    def _subcell_limiter_status_plotter(self):
        """Wraps the solution plotter so it also writes the per-cell limiter status.

        A recomputed cell's polynomial can leave the admissible range between the
        subcell averages, so such a cell plots its subcell values.
        """
        status = "fineGridCell{}CellLabel.getLimiterStatus()".format(self._name)
        return functools.partial(
            self._plotter,
            alternative_data_guard=(
                status
                + " == ::exahype2::dg::LimiterStatus::Limited or "
                + status
                + " == ::exahype2::dg::LimiterStatus::Troubled"
            ),
            alternative_data_access="fineGridCell{}Subcell.data()".format(self._name),
            alternative_data_dofs_per_axis=2 * self._order + 1,
            additional_cell_data_declarations=(
                "\n  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> "
                "_limiterStatusPlotter;\n"
            ),
            additional_cell_data_construction=(
                "\n  _limiterStatusPlotter.reset();\n"
                "  _limiterStatusPlotter = _fileWriter->createCellDataPlotter("
                '_pieceId, "LimiterStatus", 1);\n'
            ),
            additional_cell_data_plots=(
                "\n            _limiterStatusPlotter->plotCellData(cellIndex, "
                "static_cast<double>(static_cast<int>(" + status + ")));"
            ),
        )

    def _add_subcell_limiter_data_structures(self):
        """Attaches the limiter labels once the base labels exist.

        A limiter-capable solver calls this at the end of ``create_data_structures``.
        """
        if not self.subcell_limiter_is_enabled:
            return
        add_limiter_attributes_to_cell_label(self._cell_label)
        add_limiter_attributes_to_face_label(self._face_label)

        merge = self._face_label.merge_method_definition
        if _FACE_MERGE_ANCHOR not in merge:
            raise RuntimeError(
                "The base face-label merge does not contain the expected anchor, "
                "so the limiter status cannot be spliced into it."
            )
        self._face_label.merge_method_definition = merge.replace(
            _FACE_MERGE_ANCHOR, _FACE_MERGE_ANCHOR + face_status_merge_snippet()
        )

        self._create_subcell_patches()

    def _create_subcell_patches(self):
        """Creates the finite-volume subgrid a troubled cell recomputes on.

        The subgrid holds ``2 * order + 1`` subcells per axis and a halo
        ``limiter_solver.halo_width`` subcells deep.
        """
        subcells_per_axis = 2 * self._order + 1
        (
            self._subcell_solution,
            self._subcell_solution_old,
            self._subcell_halo_old,
            self._subcell_halo_new,
        ) = create_patch_and_halo(
            self._name + "Subcell",
            self._name + "SubcellOld",
            subcells_per_axis,
            self._subcell_limiter_solver.halo_width,
            self._variable_shortcuts.unknowns
            + self._variable_shortcuts.auxiliary_variables,
            self._solution_persistent_storage_precision,
        )

        for halo in (self._subcell_halo_old, self._subcell_halo_new):
            halo.generator.includes += _SUBCELL_HALO_INCLUDES

    def _add_subcell_limiter_to_datamodel(self, datamodel):
        """Registers the limiter's own patches with the Peano data model."""
        if not self.subcell_limiter_is_enabled:
            return
        datamodel.add_cell(self._subcell_solution)
        datamodel.add_cell(self._subcell_solution_old)
        datamodel.add_face(self._subcell_halo_old)
        datamodel.add_face(self._subcell_halo_new)

    def _use_subcell_limiter_data(self, step):
        """Makes the limiter's required patches available to a solver sweep.

        Dynamic mesh refinement transfers the persistent cell patches. Its
        repeated topology sweeps have no matching producer for the transient
        face data, which the next predictor rebuilds on the settled mesh.
        """
        if not self.subcell_limiter_is_enabled:
            return
        step.use_cell(self._subcell_solution)
        step.use_cell(self._subcell_solution_old)
        if step.name == "DynamicAMR":
            return
        step.use_face(self._subcell_halo_old)
        step.use_face(self._subcell_halo_new)

    def _subcell_limiter_state_guard(self, state):
        """Returns the guard that fires only in the named limiter pipeline sweep."""
        return (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getLimitingSolverState() == ::exahype2::dg::LimitingSolverState::"
            + state
        )

    def _create_subcell_limiter_action_sets(self):
        """Creates the limiter's pipeline and mesh-transfer action sets.

        Every pipeline action set descends after the Galerkin kernels, so it reads
        the candidate the corrector wrote.
        """
        if not self.subcell_limiter_is_enabled:
            return
        if self._initialise_patches:
            raise ValueError(
                "The subcell limiter samples the initial condition at subcell "
                "centres, which needs the pointwise initial-condition callback; "
                "initialise_patches=True supplies no subcell layout."
            )
        store_cell = self._store_cell_data_default_guard()
        self._action_set_subcell_initialise = InitialiseSubcells(self, store_cell)
        self._action_set_subcell_initialise.descend_invocation_order = (
            _SUBCELL_INITIALISE_DESCEND_ORDER
        )
        self._action_set_subcell_checkpoint_loading = CheckpointLoading(
            self, store_cell
        )
        self._action_set_subcell_checkpoint_loading.descend_invocation_order = (
            _SUBCELL_CHECKPOINT_LOADING_DESCEND_ORDER
        )
        self._action_set_subcell_predict = Predict(
            self, store_cell + " and " + self._subcell_limiter_state_guard("Predict")
        )
        self._action_set_subcell_correct_and_flag = CorrectAndFlag(
            self,
            store_cell + " and " + self._subcell_limiter_state_guard("CorrectAndFlag"),
            self._subcell_limiter_state_guard("CorrectAndFlag"),
        )
        self._action_set_subcell_spread_domain = SpreadDomain(
            self,
            store_cell + " and " + self._subcell_limiter_state_guard("SpreadDomain"),
            self._subcell_limiter_state_guard("SpreadDomain"),
        )
        self._action_set_subcell_recompute = RecomputeSubcells(
            self,
            store_cell
            + " and "
            + self._subcell_limiter_state_guard("RecomputeSubcells"),
        )
        for action_set in (
            self._action_set_subcell_predict,
            self._action_set_subcell_correct_and_flag,
            self._action_set_subcell_spread_domain,
            self._action_set_subcell_recompute,
        ):
            action_set.descend_invocation_order = _SUBCELL_LIMITER_DESCEND_ORDER
        self._action_set_subcell_dynamic_amr = DynamicAMR(self)
        self._action_set_subcell_dynamic_amr.descend_invocation_order = (
            _SUBCELL_DYNAMIC_AMR_DESCEND_ORDER
        )

    def _add_subcell_limiter_actions_to_dynamic_mesh_refinement(self, step):
        """Adds limiter-state transfer to the dedicated DynamicAMR observer."""
        if not self.subcell_limiter_is_enabled:
            return
        step.add_action_set(self._action_set_subcell_dynamic_amr)

    def _add_subcell_limiter_actions_to_init_grid(self, step):
        """Adds the direct subcell sampling and the checkpoint loader to the grid-initialisation step."""
        if not self.subcell_limiter_is_enabled:
            return
        step.add_action_set(self._action_set_subcell_initialise)
        step.add_action_set(self._action_set_subcell_checkpoint_loading)

    def _add_subcell_limiter_actions_to_checkpoint_solution(self, step):
        """Adds the limiter file's writer: Per cell the status, the troubled bit and the subcell values."""
        if not self.subcell_limiter_is_enabled:
            return
        dictionary = {}
        self._init_dictionary_with_default_parameters(dictionary)
        self.add_entries_to_text_replacement_dictionary(dictionary)
        values_per_cell = jinja2.Template(
            "2 + BasePowerExponent<2 * {{ORDER}} + 1, DIMENSIONS>::value"
            " * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}})"
        ).render(**dictionary)
        record_statements = jinja2.Template(
            """std::array<double, {{VALUES_PER_CELL}}> limiterRecord;
    limiterRecord[0]
      = static_cast<double>(static_cast<int>(fineGridCell{{SOLVER_NAME}}CellLabel.getLimiterStatus()));
    limiterRecord[1] = fineGridCell{{SOLVER_NAME}}CellLabel.getWasTroubledInPreviousStep() ? 1.0 : 0.0;
    std::copy_n(
      fineGridCell{{SOLVER_NAME}}SubcellOld.data(), {{VALUES_PER_CELL}} - 2, limiterRecord.begin() + 2
    );"""
        ).render(VALUES_PER_CELL=values_per_cell, **dictionary)
        step.add_action_set(
            peano4.checkpoint.CellDataWriter(
                group_name="/solvers/" + self._name + "Limiter",
                output_path_expression=(
                    "::runner::RuntimeParameters::getInstance().getCheckpointOutputPath()"
                ),
                cell_data_name="",
                values_per_cell_expression=values_per_cell,
                guard=self._load_cell_data_default_guard(),
                scalars_expression="std::vector<double>{}",
                time_stamp_expression="repositories::getMinTimeStamp()",
                additional_includes="""
#include "repositories/SolverRepository.h"
#include "runner/RuntimeParameters.h"
""",
                record_statements=record_statements,
                record_expression="limiterRecord.data()",
            )
        )

    def _add_subcell_limiter_actions_to_perform_time_step(self, step):
        """Adds the pipeline action sets to the time step while limiting is on."""
        if not self.subcell_limiter_is_enabled:
            return
        step.add_action_set(self._action_set_subcell_predict)
        step.add_action_set(self._action_set_subcell_correct_and_flag)
        step.add_action_set(self._action_set_subcell_spread_domain)
        step.add_action_set(self._action_set_subcell_recompute)

    def _configure_subcell_limiter_exchange(self):
        """Sets when the limiter's face data travels between cells.

        Sent face data arrives one sweep later, so the estimates are sent in both
        predictor sweeps and merged in the sweep that follows each send. The old
        subcell halo travels from the final spreading sweep to the recompute, and
        the new halo never travels.
        """
        if not self.subcell_limiter_is_enabled:
            return
        predict = self._subcell_limiter_state_guard("Predict")
        predict_on_hanging = self._subcell_limiter_state_guard("PredictOnHangingCells")
        correct = self._subcell_limiter_state_guard("CorrectAndFlag")
        recompute = self._subcell_limiter_state_guard("RecomputeSubcells")
        send = "(" + predict + " or " + predict_on_hanging + ")"
        merge = "(" + predict_on_hanging + " or " + correct + ")"
        final_spread = (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".isFinalSubcellSpreadSweep()"
        )
        for generator in (
            self._rhs_estimates_projection.generator,
            self._flux_estimates_projection.generator,
        ):
            generator.send_condition = send
            generator.receive_and_merge_condition = merge
        self._subcell_halo_old.generator.send_condition = final_spread
        self._subcell_halo_old.generator.receive_and_merge_condition = recompute
        self._subcell_halo_new.generator.send_condition = "false"
        self._subcell_halo_new.generator.receive_and_merge_condition = "false"

    def subcell_limiter_candidate_state(self):
        """Returns the solver state whose sweep first holds the DG candidate.

        A limiter-capable solver overrides this.
        """
        raise NotImplementedError(
            "A limiter-capable solver must name the state that holds its candidate."
        )

    def subcell_limiter_basis(self):
        """Returns the nodal basis the DG-to-subgrid projection uses.

        A limiter-capable solver overrides this.
        """
        raise NotImplementedError(
            "A limiter-capable solver must supply its projection basis."
        )
