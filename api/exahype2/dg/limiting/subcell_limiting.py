# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.dg.operator_tables import flatten
from exahype2.dg.operator_tables import table
from exahype2.fv.scheme import Godunov
from exahype2.fv.scheme import MusclHancock
from exahype2.fv.scheme import Weno
from exahype2.fv.scheme import scheme_fallback_constants

from .projection_matrices import create_dmp_sampling_matrices
from .projection_matrices import create_subcell_projection_matrices


class SubcellLimiting:
    """The a-posteriori subcell limiter of a discontinuous Galerkin solver.

    The limiter reads the ``physical_admissibility``, ``dmp_observables``,
    ``dmp_relaxation_parameter``, ``subcell_riemann_solver``, ``reconstruction_support`` and
    ``reconstruction_weights`` PDE terms of the solver. A subcell whose reconstruction stencil
    holds a subcell that fails ``reconstruction_support`` reconstructs to first order, and a WENO
    subcell reconstruction weighs every entry by the entry that ``reconstruction_weights`` names.
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
        galerkin_flux_coupling=True,
    ):
        """Enables a-posteriori limiting with the finite-volume ``scheme``.

        The discrete maximum principle widens the range of the previous step over a cell and
        its neighbours by the larger of ``dmp_relaxation_parameter`` and
        ``dmp_differences_scaling`` times that range (Dumbser et al., arXiv:1406.7416,
        equation 23).

        With ``galerkin_flux_coupling``, a subgrid applies on a face to a Galerkin cell the numerical
        flux of the Galerkin step, so both sides exchange the same flux. Without it, the subgrid
        solves the subcell Riemann problem with the subcells of the Galerkin cell in its halo, which
        keeps the subcell scheme's own stability at the price of conservation on these faces.
        """
        if self.subcell_limiter_is_enabled:
            raise RuntimeError("The subcell limiter is already configured.")
        if not isinstance(scheme, (Godunov, MusclHancock, Weno)):
            raise TypeError(
                "The subcell limiter scheme is exahype2.fv.Godunov(), "
                "exahype2.fv.MusclHancock(...) or exahype2.fv.Weno(...)."
            )
        if self.precision.storage not in ("float", "double"):
            raise ValueError(
                "The subcell limiter supports float and double solution storage only."
            )
        subcells_per_axis = 2 * self._order + 1
        if scheme.halo_width > subcells_per_axis:
            raise ValueError(
                "The subcell scheme's halo width ({}) must not exceed the {} "
                "subcells per axis. Increase the order or choose a scheme with a narrower "
                "stencil.".format(scheme.halo_width, subcells_per_axis)
            )
        if isinstance(scheme, Weno) and scheme.minimum_volumes_per_axis > subcells_per_axis:
            raise ValueError(
                "The WENO subcell limiter of order {} needs {} subcells per axis, but the DG "
                "solver of order {} has {}.".format(
                    scheme.order, scheme.minimum_volumes_per_axis, self._order, subcells_per_axis
                )
            )
        if number_of_dmp_observables < 0:
            raise ValueError(
                "The subcell limiter expects a non-negative number of "
                "discrete-maximum-principle observables."
            )
        if dmp_relaxation_parameter < 0.0 or dmp_differences_scaling < 0.0:
            raise ValueError(
                "The subcell limiter expects non-negative relaxation parameters of the "
                "discrete maximum principle."
            )

        self._subcell_limiter_scheme = scheme
        self._subcell_limiter_number_of_dmp_observables = number_of_dmp_observables
        self._subcell_limiter_dmp_relaxation_parameter = dmp_relaxation_parameter
        self._subcell_limiter_dmp_differences_scaling = dmp_differences_scaling
        self._subcell_limiter_galerkin_flux_coupling = bool(galerkin_flux_coupling)
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
                "The subcell limiter needs the physical_admissibility PDE term or a positive "
                "number of DMP observables."
            )
        if self._subcell_limiter_uses_dmp and not self.pde_terms.is_set(
            "dmp_observables"
        ):
            raise ValueError(
                "A discrete maximum principle needs the dmp_observables PDE term."
            )
        if isinstance(self._subcell_limiter_scheme, Weno) and self.pde_terms.is_set(
            "diffusive_source"
        ):
            raise ValueError("The WENO subcell limiter does not support diffusive source terms.")
        if (
            isinstance(self._subcell_limiter_scheme, Weno)
            and self._subcell_limiter_scheme.scales_polynomials
            and not self.pde_terms.is_set("physical_admissibility")
        ):
            raise ValueError(
                "The admissibility scaling of the WENO subcell limiter needs the "
                "physical_admissibility PDE term."
            )

        if getattr(self, "_initialise_patches", False):
            raise ValueError(
                "The subcell limiter needs the pointwise initial condition and cannot work "
                "with initialise_patches=True."
            )

    def _subcell_limiter_includes(self):
        """Returns the headers of the limiter and its finite-volume scheme."""
        if not self.subcell_limiter_is_enabled:
            return []
        scheme = self._subcell_limiter_scheme.subcell_scheme()
        return [
            self._name + "SubcellProjection.h",
            "exahype2/dg/limiting/SubcellLimiter.h",
            scheme.subcell_family_include,
        ] + list(scheme.includes)

    def _subcell_limiter_cpp_type(self):
        """Returns the C++ limiter type of this solver, or ``void`` without a limiter."""
        if not self.subcell_limiter_is_enabled:
            return "void"
        return "::exahype2::dg::limiting::SubcellLimiter<{}, {}, {}, {}>".format(
            self._solver_type(),
            self.precision.storage,
            self.precision.compute,
            self._subcell_limiter_scheme.subcell_scheme().subcell_family,
        )

    def _subcell_limiter_constants(self):
        """Returns the C++ constants and type aliases of the subcell layout and the scheme, or
        an empty list without a limiter."""
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
            "static constexpr double DmpRelaxationParameter = {};".format(
                repr(float(self._subcell_limiter_dmp_relaxation_parameter))
            ),
            "static constexpr double DmpDifferencesScaling  = {};".format(
                repr(float(self._subcell_limiter_dmp_differences_scaling))
            ),
            "using Reconstruction         = {};".format(scheme.reconstruction),
            "using FallbackReconstruction = {};".format(
                scheme.fallback_reconstruction or "void"
            ),
            "static constexpr bool GalerkinFluxCoupling = {};".format(
                "true" if self._subcell_limiter_galerkin_flux_coupling else "false"
            ),
        ] + scheme_fallback_constants(scheme)

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
                    self._basis.rounded_quadrature_points,
                    self._basis.QuadraturePoints1DDocumentation,
                ),
                table(
                    "NodesToSubcells1D",
                    flatten(nodes_to_subcells),
                    """
  Averaging operator from the nodal polynomial onto the subcells.
""",
                ),
                table(
                    "SubcellsToNodes1D",
                    flatten(subcells_to_nodes),
                    """
  Projection operator from the subcell averages onto the nodes. It keeps the cell mean.
""",
                ),
                table(
                    "NodesToLobatto1D",
                    flatten(nodes_to_lobatto),
                    """
  Evaluation operator of the nodal polynomial at the same number of Gauss-Lobatto nodes.
""",
                ),
            ],
        }
