# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.pde_terms

from exahype2.fv.scheme import Weno
from exahype2.fv.solver import Solver as FiniteVolumeSolver
from exahype2.fv.solver import pde_terms


def _pde_terms():
    terms = pde_terms()
    terms.update(
        {
            "physical_admissibility": exahype2.pde_terms.physical_admissibility(),
            "dmp_observables": exahype2.pde_terms.dmp_observables(),
            "dmp_relaxation_parameter": exahype2.pde_terms.dmp_relaxation_parameter(),
        }
    )
    return terms


class Solver(FiniteVolumeSolver):
    """An ADER-WENO solver after Dumbser et al.: The polynomial WENO reconstruction of ``scheme``
    from the volume averages, evolved by the space-time predictor of ADER-DG. Every patch predicts
    its volumes, the patches exchange the predicted states and fluxes on their faces, and every
    volume then corrects with the Riemann fluxes on its faces.

    ``scheme`` is an :class:`exahype2.fv.Weno`, which also fixes the reconstruction limiter and the
    fallback. A patch holds at least ``2 * order`` volumes per axis. A patch with the fallback
    repeats its step with the fallback scheme when one of its volumes leaves the finite states,
    fails the ``physical_admissibility`` PDE term, or leaves the extrema of the
    ``number_of_dmp_observables`` observables of the ``dmp_observables`` PDE term over the volumes
    around it, widened by the larger of ``dmp_relaxation_parameter`` and
    ``dmp_differences_scaling`` times their range.
    """

    SOLVER_FAMILY_INCLUDE = "exahype2/fv/ader/Solver.h"
    SOLVER_FAMILY_CLASS = "::exahype2::fv::ader::Solver"
    PDE_TERMS = _pde_terms()

    def __init__(
        self,
        name,
        patch_size,
        unknowns,
        auxiliary_variables,
        time_stepping,
        scheme=None,
        number_of_dmp_observables=0,
        dmp_relaxation_parameter=0.0,
        dmp_differences_scaling=0.0,
        precision="double",
    ):
        super().__init__(
            name, patch_size, unknowns, auxiliary_variables, time_stepping, precision
        )
        self.scheme = Weno() if scheme is None else scheme
        if not isinstance(self.scheme, Weno):
            raise TypeError("[{}] The scheme is exahype2.fv.Weno(...).".format(name))
        if patch_size < self.scheme.minimum_volumes_per_axis:
            raise ValueError(
                "[{}] ADER-WENO of order {} needs a patch size of at least {}.".format(
                    name, self.scheme.order, self.scheme.minimum_volumes_per_axis
                )
            )
        if number_of_dmp_observables < 0:
            raise ValueError(
                "[{}] The number of DMP observables is not negative.".format(name)
            )
        if dmp_relaxation_parameter < 0.0 or dmp_differences_scaling < 0.0:
            raise ValueError(
                "[{}] The relaxation parameters of the DMP are not negative.".format(name)
            )
        self._number_of_dmp_observables = number_of_dmp_observables
        self._dmp_relaxation_parameter = dmp_relaxation_parameter
        self._dmp_differences_scaling = dmp_differences_scaling

    @property
    def order(self):
        return self.scheme.order

    def statistics_metadata(self):
        return super().statistics_metadata() + [
            ("WENO Order", self.scheme.order),
            ("Reconstruction Limiter", self.scheme.reconstruction_limiter),
            ("Fallback", "none" if self.scheme.fallback is None else repr(self.scheme.fallback)),
        ]

    def _validate(self):
        super()._validate()
        if self.scheme.scales_polynomials and not self.pde_terms.is_set(
            "physical_admissibility"
        ):
            raise ValueError(
                "[{}] The admissibility scaling needs the physical_admissibility PDE term.".format(
                    self._name
                )
            )
        if self._number_of_dmp_observables > 0 and not self.pde_terms.is_set(
            "dmp_observables"
        ):
            raise ValueError(
                "[{}] A discrete maximum principle needs the dmp_observables PDE term.".format(
                    self._name
                )
            )
        if self._number_of_dmp_observables > 0 and self.scheme.fallback is None:
            raise ValueError(
                "[{}] The discrete maximum principle selects the patches of the fallback, so it "
                "needs a scheme with a fallback.".format(self._name)
            )

    def _scheme_includes(self):
        return self.scheme.subcell_scheme().includes

    def _scheme_constants(self):
        scheme = self.scheme.subcell_scheme()
        return [
            "using Reconstruction         = {};".format(scheme.reconstruction),
            "using FallbackReconstruction = {};".format(
                scheme.fallback_reconstruction or "void"
            ),
            "static constexpr int    NumberOfObservables    = {};".format(
                self._number_of_dmp_observables
            ),
            "static constexpr double DmpRelaxationParameter = {};".format(
                repr(float(self._dmp_relaxation_parameter))
            ),
            "static constexpr double DmpDifferencesScaling  = {};".format(
                repr(float(self._dmp_differences_scaling))
            ),
        ]
