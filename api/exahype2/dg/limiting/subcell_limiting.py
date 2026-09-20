# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import functools

from exahype2.fv.patch_data import create_patch_and_halo
from exahype2.fv.scheme import Godunov
from exahype2.fv.scheme import MusclHancock

from .actionsets.correct_and_flag import CorrectAndFlag
from .actionsets.dynamic_amr import DynamicAMR
from .actionsets.initialise_subcells import InitialiseSubcells
from .actionsets.predict import Predict
from .actionsets.recompute_subcells import RecomputeSubcells
from .actionsets.spread_domain import SpreadDomain
from .labels import add_limiter_attributes_to_cell_label
from .labels import add_limiter_attributes_to_face_label
from .labels import create_limiter_vertex_label
from .labels import face_status_merge_snippet

_DESCEND_INVOCATION_ORDER = {
    "subcell_initialise": 2,
    "subcell_predict": 4,
    "subcell_correct_and_flag": 4,
    "subcell_spread_domain": 4,
    "subcell_recompute": 4,
    "subcell_dynamic_amr": 2,
}

# The line the base face-label merge (actionsets/face_label.py) ends its owned-side block
# with.
_FACE_MERGE_ANCHOR = (
    "setAboveHanging(neighbourEntry, neighbour.getAboveHanging(neighbourEntry));"
)


class SubcellLimiting:
    """The limiter reads the ``physical_admissibility``, ``dmp_observables``,
    ``dmp_relaxation_parameter`` and ``subcell_riemann_solver`` PDE terms of the solver.
    """

    @property
    def subcell_limiter_is_enabled(self):
        return getattr(self, "_subcell_limiter_scheme", None) is not None

    def add_subcell_limiter(
        self,
        scheme,
        number_of_dmp_observables=0,
        dmp_relaxation_parameter=0.0,
        dmp_differences_scaling=0.0,
    ):
        """Enables a-posteriori limiting on the finite-volume ``scheme``.

        A DMP merges previous-step ranges across the nodal neighbourhood V_i and widens the
        result by its absolute or scaled relaxation (Dumbser et al., arXiv:1406.7416,
        equation 23).
        """
        if self.subcell_limiter_is_enabled:
            raise RuntimeError("The subcell limiter is already configured.")
        if not isinstance(scheme, (Godunov, MusclHancock)):
            raise TypeError(
                "The subcell limiter scheme is exahype2.fv.Godunov() or "
                "exahype2.fv.MusclHancock(...)."
            )
        if self.precision.storage not in ("float", "double"):
            raise ValueError(
                "The subcell limiter supports float and double solution storage."
            )
        subcells_per_axis = 2 * self._order + 1
        if scheme.halo_width > subcells_per_axis:
            raise ValueError(
                "The subcell scheme's halo width ({}) must not exceed the {} "
                "subcells per axis. Increase the order or choose a scheme with a narrower "
                "stencil.".format(scheme.halo_width, subcells_per_axis)
            )
        if number_of_dmp_observables < 0:
            raise ValueError(
                "The number of discrete-maximum-principle observables cannot be negative."
            )
        if dmp_relaxation_parameter < 0.0 or dmp_differences_scaling < 0.0:
            raise ValueError(
                "The discrete-maximum-principle relaxation parameters cannot be negative."
            )

        self._subcell_limiter_scheme = scheme
        self._subcell_limiter_number_of_dmp_observables = number_of_dmp_observables
        self._subcell_limiter_dmp_relaxation_parameter = dmp_relaxation_parameter
        self._subcell_limiter_dmp_differences_scaling = dmp_differences_scaling
        return self

    @property
    def _subcell_limiter_uses_dmp(self):
        return self._subcell_limiter_number_of_dmp_observables > 0

    @property
    def _subcell_limiter_uses_admissibility_criterion(self):
        return self.pde_terms.is_set("physical_admissibility")

    @property
    def subcell_limiter_admissibility_flag(self):
        """Returns the C++ literal that switches the admissibility term on in a limiter template."""
        return "true" if self._subcell_limiter_uses_admissibility_criterion else "false"

    @property
    def subcell_limiter_relaxation_term_flag(self):
        """Returns the C++ literal that switches the relaxation term on in a limiter template."""
        return "true" if self.pde_terms.is_set("dmp_relaxation_parameter") else "false"

    @property
    def _subcell_limiter_solver(self):
        """Returns the C++ description of the subcell scheme."""
        return self._subcell_limiter_scheme.subcell_scheme()

    def _validate_subcell_limiter(self):
        if not self.subcell_limiter_is_enabled:
            return
        if (
            not self._subcell_limiter_uses_dmp
            and not self._subcell_limiter_uses_admissibility_criterion
        ):
            raise ValueError(
                "The subcell limiter needs the physical_admissibility PDE term, a positive "
                "number of DMP observables, or both."
            )
        if self._subcell_limiter_uses_dmp and not self.pde_terms.is_set(
            "dmp_observables"
        ):
            raise ValueError(
                "A discrete maximum principle needs the dmp_observables PDE term."
            )

    def _solution_plotter(self):
        """Returns the solution plotter; with the limiter it also writes the per-cell limiter
        status, and a recomputed cell plots its subcell values."""
        if not self.subcell_limiter_is_enabled:
            return self._plotter
        status = "fineGridCell{}CellLabel.getLimiterStatus()".format(self._name)
        return functools.partial(
            self._plotter,
            alternative_data_guard=(
                status
                + " == ::exahype2::dg::LimiterStatus::Limited or "
                + status
                + " == ::exahype2::dg::LimiterStatus::Troubled"
            ),
            alternative_data_access="fineGridCell{}SubcellOld.data()".format(
                self._name
            ),
            alternative_data_dofs_per_axis=2 * self._order + 1,
            additional_cell_data_declarations=(
                "\n  std::unique_ptr<tarch::plotter::FileWriter::CellDataPlotter> "
                "_limiterStatusPlotter{};\n"
            ),
            additional_cell_data_construction=(
                '_limiterStatusPlotter = _plotter.createCellDataPlotter("LimiterStatus", 1);'
            ),
            additional_cell_data_plots=(
                "_limiterStatusPlotter->plotCellData(cellIndex, "
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

        self._subcell_limiter_vertex_label = (
            create_limiter_vertex_label(
                self._name, self._subcell_limiter_number_of_dmp_observables
            )
            if self._subcell_limiter_uses_dmp
            else None
        )

        self._create_subcell_patches()

    def _create_subcell_patches(self):
        """Creates the finite-volume subgrid a troubled cell recomputes on and the halo it reads;
        the recompute evolves the subgrid in scratch memory, so no second cell patch exists.
        """
        subcells_per_axis = 2 * self._order + 1
        (
            _,
            self._subcell_solution_old,
            self._subcell_halo_old,
            _,
        ) = create_patch_and_halo(
            self._name + "Subcell",
            self._name + "SubcellOld",
            subcells_per_axis,
            self._subcell_limiter_scheme.halo_width,
            self.state.number_of_values,
            self.precision.storage,
        )
        self._subcell_halo_old.generator.includes += [
            "peano4/Loop.h",
            "repositories/SolverRepository.h",
        ]

    def _add_subcell_limiter_to_datamodel(self, datamodel):
        if not self.subcell_limiter_is_enabled:
            return
        datamodel.add_cell(self._subcell_solution_old)
        datamodel.add_face(self._subcell_halo_old)
        if self._subcell_limiter_vertex_label is not None:
            datamodel.add_vertex(self._subcell_limiter_vertex_label)

    def _use_subcell_limiter_data(self, step):
        """Dynamic mesh refinement transfers the persistent cell patches. Its
        repeated topology sweeps have no matching producer for the transient
        face data, which the next predictor rebuilds on the settled mesh.
        """
        if not self.subcell_limiter_is_enabled:
            return
        step.use_cell(self._subcell_solution_old)
        if self._subcell_limiter_vertex_label is not None:
            step.use_vertex(self._subcell_limiter_vertex_label)
        if step.name == "DynamicAMR":
            return
        step.use_face(self._subcell_halo_old)

    def _subcell_limiter_state_guard(self, state):
        """Returns the guard that fires only in the named limiter pipeline sweep."""
        return (
            "repositories::"
            + self.instance_name
            + ".isLimitingSweep(::exahype2::dg::LimitingSolverState::"
            + state
            + ")"
        )

    def _create_subcell_limiter_action_sets(self):
        """Every pipeline action set descends after the Galerkin kernels, so it reads
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
        self._action_set_subcell_predict = Predict(
            self,
            store_cell + " and " + self._subcell_limiter_state_guard("Predict"),
            self._subcell_limiter_state_guard("Predict"),
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
        self._action_set_subcell_dynamic_amr = DynamicAMR(self)
        self._apply_descend_invocation_orders(_DESCEND_INVOCATION_ORDER)

    def _subcell_limiter_action_sets(self, step_name):
        """Returns the limiter's action sets of one step of the project."""
        if not self.subcell_limiter_is_enabled:
            return []
        return {
            "init_grid": [self._action_set_subcell_initialise],
            "perform_time_step": [
                self._action_set_subcell_predict,
                self._action_set_subcell_correct_and_flag,
                self._action_set_subcell_spread_domain,
                self._action_set_subcell_recompute,
            ],
            "dynamic_amr": [self._action_set_subcell_dynamic_amr],
        }[step_name]

    def _configure_subcell_limiter_exchange(self):
        """Sent face data arrives one sweep later, so the estimates are sent in both
        predictor sweeps and merged in the sweep that follows each send.
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
            "repositories::" + self.instance_name + ".isFinalSubcellSpreadSweep()"
        )
        for generator in (
            self._rhs_estimates_projection.generator,
            self._flux_estimates_projection.generator,
        ):
            generator.send_condition = send
            generator.receive_and_merge_condition = merge
        self._subcell_halo_old.generator.send_condition = final_spread
        self._subcell_halo_old.generator.receive_and_merge_condition = recompute
