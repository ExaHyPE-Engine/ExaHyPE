# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.pde_terms

from exahype2.fv.scheme import Godunov
from exahype2.fv.scheme import MusclHancock
from exahype2.fv.solver import Solver as FiniteVolumeSolver
from exahype2.fv.solver import pde_terms


def _pde_terms():
    terms = pde_terms()
    terms.update(
        {
            "diffusive_source": exahype2.pde_terms.diffusive_source(),
            "limited_difference": exahype2.pde_terms.limited_difference(),
            "reconstruction_support": exahype2.pde_terms.reconstruction_support(),
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
        }
    )
    return terms


class Solver(FiniteVolumeSolver):
    """A finite-volume solver that advances every volume in one step from the volumes around it.

    ``scheme`` is :class:`exahype2.fv.Godunov` or :class:`exahype2.fv.MusclHancock`. MUSCL-Hancock
    uses zero slopes in a volume when the volume or one of its neighbours fails the
    ``reconstruction_support`` PDE term.
    """

    SOLVER_FAMILY_INCLUDE = "exahype2/fv/singlestep/Solver.h"
    SOLVER_FAMILY_CLASS = "::exahype2::fv::singlestep::Solver"
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
    ):
        super().__init__(
            name, patch_size, unknowns, auxiliary_variables, time_stepping, precision
        )
        self.scheme = Godunov() if scheme is None else scheme
        if not isinstance(self.scheme, (Godunov, MusclHancock)):
            raise TypeError(
                "scheme is exahype2.fv.Godunov() or exahype2.fv.MusclHancock(...). ADER-WENO is "
                "exahype2.fv.ader.Solver."
            )

    def _validate(self):
        super()._validate()
        if isinstance(self.scheme, MusclHancock) and self.pde_terms.is_set(
            "diffusive_source"
        ):
            raise ValueError("MUSCL-Hancock does not support diffusive source terms.")

    def _scheme_includes(self):
        return self.scheme.subcell_scheme().includes

    def _scheme_constants(self):
        scheme = self.scheme.subcell_scheme()
        return [
            "using Reconstruction  = {};".format(scheme.reconstruction),
            "using TimeIntegration = {};".format(scheme.time_stepping),
        ]
