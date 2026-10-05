# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.pde_terms
import exahype2.solver

from exahype2.dg.basis import GaussLegendreBasis
from exahype2.dg.basis import GaussLobattoBasis
from exahype2.dg.tracing import render_tracer_update

from .generate_kernels import generate_kernels

MIN_RUNGE_KUTTA_ORDER = 1
MAX_RUNGE_KUTTA_ORDER = 4


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
        "material_parameters": exahype2.pde_terms.material_parameters(),
        "riemann_solver": exahype2.pde_terms.riemann_solver(points="NodesPerFace"),
        "analytical_solution": exahype2.pde_terms.analytical_solution(),
        "particle_analytical_solution": exahype2.pde_terms.particle_analytical_solution(),
    }


class Solver(exahype2.solver.Solver):
    """A Runge-Kutta DG solver of polynomial order ``dg_order``. A time step runs ``rk_order``
    Runge-Kutta stages.

    ``basis`` is :class:`exahype2.dg.GaussLegendre` or :class:`exahype2.dg.GaussLobatto`.
    ``linear`` selects the linear Riemann solver, which supports material parameters.
    """

    SOLVER_FAMILY = "rk"
    SOLVER_FAMILY_INCLUDE = "exahype2/dg/rk/Solver.h"
    SOLVER_FAMILY_CONCEPT = "::exahype2::dg::rk::SolverConcept"
    SOLVER_FAMILY_CONCEPT_PRECISIONS = ("STORAGE_PRECISION", "COMPUTE_PRECISION")
    PDE_TERMS = _pde_terms()

    def __init__(
        self,
        name,
        rk_order,
        dg_order,
        unknowns,
        auxiliary_variables,
        time_stepping,
        basis=GaussLegendreBasis,
        linear=False,
        precision="double",
    ):
        super().__init__(name, unknowns, auxiliary_variables, time_stepping, precision)
        if basis not in (GaussLegendreBasis, GaussLobattoBasis):
            raise ValueError(
                "The Runge-Kutta DG solver supports the bases exahype2.dg.GaussLegendre and "
                "exahype2.dg.GaussLobatto only."
            )
        if isinstance(dg_order, bool) or not isinstance(dg_order, int) or dg_order < 0:
            raise ValueError(
                "The Runge-Kutta DG solver expects a non-negative integer dg_order "
                "(got {!r}).".format(dg_order)
            )
        if (
            isinstance(rk_order, bool)
            or not isinstance(rk_order, int)
            or rk_order < MIN_RUNGE_KUTTA_ORDER
            or rk_order > MAX_RUNGE_KUTTA_ORDER
        ):
            raise ValueError(
                "The Runge-Kutta DG solver expects an integer rk_order from {} to {} "
                "(got {!r}).".format(
                    MIN_RUNGE_KUTTA_ORDER, MAX_RUNGE_KUTTA_ORDER, rk_order
                )
            )
        self._rk_order = rk_order
        self._dg_order = dg_order
        self._basis = basis(dg_order + 1)
        self._is_linear = linear

    @property
    def basis(self):
        return self._basis

    def statistics_metadata(self):
        return super().statistics_metadata() + [
            ("DG-Order", self._dg_order),
            ("RK-Order", self._rk_order),
        ]

    @property
    def postprocess_updated_patch(self):
        return self._postprocess_updated_patch

    @postprocess_updated_patch.setter
    def postprocess_updated_patch(self, kernel):
        """Sets the C++ code that runs on every cell once the stages of a time step combine
        into the new nodes ``Q``. The code reads and writes ``Q`` and reads ``x``, ``h``, ``t``
        and ``dt``. ``True`` leaves the definition to the application."""
        self._postprocess_updated_patch = kernel

    def _validate(self):
        super()._validate()
        if self._is_linear and self.pde_terms.is_set("diffusive_flux"):
            raise ValueError(
                "The linear Runge-Kutta DG solver cannot add a diffusive flux."
            )
        if not self._is_linear and self.pde_terms.is_set("material_parameters"):
            raise ValueError(
                "The Runge-Kutta DG solver supports material parameters with linear kernels "
                "only."
            )
        if self.pde_terms.is_set("flux") and self.pde_terms.is_set("diffusive_flux"):
            raise ValueError(
                "A solver takes either the flux or the diffusive_flux PDE term."
            )
        if not self.pde_terms.is_set("riemann_solver") and not self.pde_terms.is_set(
            "max_eigenvalue"
        ):
            raise ValueError(
                "The solver {} without a Riemann solver needs the max_eigenvalue PDE term.".format(
                    self._name
                )
            )

    def _table_class(self, table):
        """Returns the fully qualified name of a generated table class of this solver."""
        return "::" + "::".join(self._namespace + [self._name + table])

    def _solver_family_includes(self):
        includes = [
            self._name + "DGMatrices.h",
            self._name + "Quadrature.h",
        ]
        if self._tracers:
            includes.append("exahype2/dg/Tracing.h")
        return includes

    def _render_tracer_update(self, tracer, d):
        return render_tracer_update(tracer, self._table_class("Quadrature"))

    def _solver_family_class(self):
        return "::exahype2::dg::rk::Solver<{}, {}, {}>".format(
            self._solver_type(), self.precision.storage, self.precision.compute
        )

    def _solver_family_constants(self):
        return [
            "static constexpr int  DGOrder                    = {};".format(
                self._dg_order
            ),
            "static constexpr int  RungeKuttaOrder            = {};".format(
                self._rk_order
            ),
            "static constexpr int  NumberOfUnknowns           = {};".format(
                self.state.unknowns
            ),
            "static constexpr int  NumberOfAuxiliaryVariables = {};".format(
                self.state.auxiliary_variables
            ),
            "static constexpr bool IsLinear                   = {};".format(
                "true" if self._is_linear else "false"
            ),
            "static constexpr ::exahype2::TimeStepping::Rule TimeSteppingRule = "
            "::exahype2::TimeStepping::Rule::{};".format(self.time_stepping.rule),
        ]

    def _table_precisions(self):
        """Returns the solver precisions plus double for the mesh transfer operators."""
        result = list(self.precision.types)
        if "double" not in result:
            result.append("double")
        return result

    def _generate_solver_family_files(self, output, dictionary):
        generate_kernels(solver=self, dictionary=dictionary, output=output)

    def _add_solver_family_entries(self, d):
        d["DG_ORDER"] = self._dg_order
        d["TABLE_PRECISIONS"] = self._table_precisions()
        d["MAX_ADMISSIBLE_MESH_WIDTH"] = (
            "parameters.getMaxAdmissiblePatchSize(DomainSize)"
        )
