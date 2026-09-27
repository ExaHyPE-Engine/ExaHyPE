# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.dg.operator_tables import flatten
from exahype2.dg.operator_tables import table
from exahype2.fv.scheme import Godunov
from exahype2.fv.scheme import MusclHancock

from .projection_matrices import create_dmp_sampling_matrices
from .projection_matrices import create_subcell_projection_matrices


class SubcellLimiting:
    """The a-posteriori subcell limiter of a discontinuous Galerkin solver.

    The limiter reads the ``physical_admissibility``, ``dmp_observables``,
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

        A discrete maximum principle merges the ranges of the previous step over every cell
        and its neighbours and widens the result by the larger of the absolute relaxation and
        the scaled range (Dumbser et al., arXiv:1406.7416, equation 23).
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

    def _validate_subcell_limiter(self):
        if not self.subcell_limiter_is_enabled:
            return
        if not self._subcell_limiter_uses_dmp and not self.pde_terms.is_set(
            "physical_admissibility"
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
        if getattr(self, "_initialise_patches", False):
            raise ValueError(
                "The subcell limiter samples the initial condition at subcell centres, "
                "which needs the pointwise initial condition; initialise_patches=True "
                "supplies no subcell layout."
            )

    def _subcell_limiter_includes(self):
        """Returns the headers the limiter and its finite-volume scheme read."""
        if not self.subcell_limiter_is_enabled:
            return []
        return [
            self._name + "SubcellProjection.h",
            "exahype2/dg/limiting/SubcellLimiter.h",
        ] + list(self._subcell_limiter_scheme.subcell_scheme().includes)

    def _subcell_limiter_cpp_type(self):
        """Returns the C++ limiter type of this solver, or ``void`` without a limiter."""
        if not self.subcell_limiter_is_enabled:
            return "void"
        return "::exahype2::dg::limiting::SubcellLimiter<{}, {}, {}>".format(
            self._solver_type(), self.precision.storage, self.precision.compute
        )

    def _subcell_limiter_constants(self):
        """Returns the C++ lines of the subcell layout and scheme the limiter and the finite-volume
        kernels read from the generated class, empty without a limiter."""
        if not self.subcell_limiter_is_enabled:
            return []
        scheme = self._subcell_limiter_scheme.subcell_scheme()
        return [
            "static constexpr int  PatchSize                  = {};".format(
                2 * self._order + 1
            ),
            "static constexpr int  NumberOfObservables        = {};".format(
                self._subcell_limiter_number_of_dmp_observables
            ),
            "using Reconstruction         = {};".format(scheme.reconstruction),
            "using FallbackReconstruction = {};".format(
                scheme.fallback_reconstruction or "void"
            ),
            "using TimeIntegration        = {};".format(scheme.time_stepping),
        ]

    def _subcell_limiter_arguments(self):
        """Returns the constructor arguments of the limiter as C++ text, empty without one."""
        if not self.subcell_limiter_is_enabled:
            return ""
        return "{}, {}".format(
            repr(float(self._subcell_limiter_dmp_relaxation_parameter)),
            repr(float(self._subcell_limiter_dmp_differences_scaling)),
        )

    def _subcell_limiter_table_class(self):
        """Returns the projection tables of the limiter as an entry of the operator tables."""
        nodes_to_subcells, subcells_to_nodes = create_subcell_projection_matrices(
            self._basis
        )
        nodes_to_lobatto, _ = create_dmp_sampling_matrices(self._basis)
        return {
            "name": "SubcellProjection",
            "documentation": (
                "1D projections between the nodes and the subcells of the limiter, flattened\n"
                "with the input node as the leading index."
            ),
            "tables": [
                table(
                    "QuadraturePoints1D",
                    flatten(self._basis.quadrature_points),
                    self._basis.QuadraturePoints1DDocumentation,
                ),
                table(
                    "NodesToSubcells1D",
                    flatten(nodes_to_subcells),
                    """
  Averages the polynomial over the nodes onto the subcells.
""",
                ),
                table(
                    "SubcellsToNodes1D",
                    flatten(subcells_to_nodes),
                    """
  Projects the subcell averages back onto the nodes and keeps the cell mean.
""",
                ),
                table(
                    "NodesToLobatto1D",
                    flatten(nodes_to_lobatto),
                    """
  Evaluates the polynomial over the nodes at the Gauss-Lobatto nodes of the same count.
""",
                ),
            ],
        }
