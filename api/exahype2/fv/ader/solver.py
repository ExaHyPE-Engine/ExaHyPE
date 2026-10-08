# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.fv.scheme import Weighting
from exahype2.fv.scheme import Weno
from exahype2.fv.solver import Solver as FiniteVolumeSolver
from exahype2.fv.solver import pde_terms


class Solver(FiniteVolumeSolver):
    """An ADER-WENO solver after Dumbser et al.: A polynomial WENO reconstruction of degree
    ``order`` from the volume averages, evolved by the space-time predictor of ADER-DG. Every
    patch predicts its volumes, the patches exchange the predicted states and fluxes on their
    faces, and every volume then corrects with the Riemann fluxes on its faces.

    ``weighting`` is a policy type of :class:`exahype2.fv.Weighting`. A patch holds at least
    ``2 * order`` volumes per axis.
    """

    SOLVER_FAMILY_INCLUDE = "exahype2/fv/ader/Solver.h"
    SOLVER_FAMILY_CLASS = "::exahype2::fv::ader::Solver"
    PDE_TERMS = pde_terms()

    def __init__(
        self,
        name,
        patch_size,
        order,
        unknowns,
        auxiliary_variables,
        time_stepping,
        weighting=Weighting.jiangshu_r8,
        precision="double",
    ):
        super().__init__(
            name, patch_size, unknowns, auxiliary_variables, time_stepping, precision
        )
        self._weno = Weno(order=order, weighting=weighting)
        if patch_size < self._weno.minimum_volumes_per_axis:
            raise ValueError(
                "[{}] ADER-WENO of order {} needs a patch size of at least {}.".format(
                    name, order, self._weno.minimum_volumes_per_axis
                )
            )

    @property
    def order(self):
        return self._weno.order

    def statistics_metadata(self):
        return super().statistics_metadata() + [("WENO Order", self._weno.order)]

    def _scheme_includes(self):
        return [
            "exahype2/fv/reconstruction/WENO.h",
            "exahype2/fv/reconstruction/weno/NonlinearWeights.h",
        ]

    def _scheme_constants(self):
        return [
            "using Reconstruction = {};".format(
                self._weno.subcell_scheme().reconstruction
            ),
        ]
