# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.pde_terms
import exahype2.solver

from .scheme import Godunov
from .scheme import MusclHancock


def _pde_terms():
    return {
        "initial_condition": exahype2.pde_terms.initial_condition(),
        "boundary_conditions": exahype2.pde_terms.boundary_conditions("storage"),
        "refinement_criterion": exahype2.pde_terms.refinement_criterion(
            "[PatchSize^DIMENSIONS * ({N} + {A})]"
        ),
        "max_eigenvalue": exahype2.pde_terms.max_eigenvalue(),
        "flux": exahype2.pde_terms.flux(),
        "ncp": exahype2.pde_terms.ncp(),
        "source": exahype2.pde_terms.source(),
        "diffusive_source": exahype2.pde_terms.diffusive_source(),
        "riemann_solver": exahype2.pde_terms.riemann_solver(),
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


class Solver(exahype2.solver.Solver):
    """A finite-volume solver on patches of ``patch_size``^d volumes per cell.

    ``scheme`` is :class:`exahype2.fv.Godunov` or :class:`exahype2.fv.MusclHancock`.
    """

    SOLVER_FAMILY = "fv"
    SOLVER_FAMILY_INCLUDE = "exahype2/fv/Solver.h"
    SOLVER_FAMILY_CONCEPT = "::exahype2::fv::SolverConcept"
    SOLVER_FAMILY_CONCEPT_PRECISIONS = ("STORAGE_PRECISION",)
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
        super().__init__(name, unknowns, auxiliary_variables, time_stepping, precision)
        self.scheme = Godunov() if scheme is None else scheme
        if not isinstance(self.scheme, (Godunov, MusclHancock)):
            raise TypeError(
                "scheme is exahype2.fv.Godunov() or exahype2.fv.MusclHancock(...)."
            )
        if (
            isinstance(patch_size, bool)
            or not isinstance(patch_size, int)
            or patch_size < 1
        ):
            raise ValueError("patch_size is a positive integer.")
        self._patch_size = patch_size

    def statistics_metadata(self):
        return super().statistics_metadata() + [("Patch Size", self._patch_size)]

    def _provides_max_eigenvalue(self):
        """Returns true for the max_eigenvalue PDE term and for a Riemann solver, which returns
        the maximum wave speed of every face."""
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

    def _solver_family_includes(self):
        includes = list(self.scheme.subcell_scheme().includes)
        if self._tracers:
            includes.append("exahype2/fv/Tracing.h")
        return includes

    def _render_tracer_update(self, tracer, d):
        """Returns the C++ call that moves one tracer set. A time step reads the previous
        solution, so the tracers and the fluid advance from the same state."""
        fields = "".join(", {}".format(field) for field in tracer["FIELDS"])
        return (
            "::exahype2::fv::traceParticles<PatchSize, NumberOfData, {number_of_data}{fields}>(\n"
            "  partition,\n"
            "  initialisation ? getSolution() : getPreviousSolution(),\n"
            "  timeStepSize,\n"
            "  _{name},\n"
            "  _{name}Forces,\n"
            "  DomainOffset,\n"
            "  DomainSize,\n"
            "  static_cast<int>(PeriodicBoundaries.to_ulong())\n"
            ");"
        ).format(
            number_of_data=tracer["NUMBER_OF_DATA"], fields=fields, name=tracer["NAME"]
        )

    def _solver_family_class(self):
        return "::exahype2::fv::Solver<{}, {}, {}>".format(
            self._solver_type(), self.precision.storage, self.precision.compute
        )

    def _solver_family_constants(self):
        scheme = self.scheme.subcell_scheme()
        return [
            "static constexpr int PatchSize                  = {};".format(
                self._patch_size
            ),
            "static constexpr int NumberOfUnknowns           = {};".format(
                self.state.unknowns
            ),
            "static constexpr int NumberOfAuxiliaryVariables = {};".format(
                self.state.auxiliary_variables
            ),
            "static constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;",
            "using Reconstruction  = {};".format(scheme.reconstruction),
            "using TimeIntegration = {};".format(scheme.time_stepping),
            "static constexpr ::exahype2::TimeStepping::Rule TimeSteppingRule = "
            "::exahype2::TimeStepping::Rule::{};".format(self.time_stepping.rule),
        ]

    def _add_solver_family_entries(self, d):
        d["MAX_ADMISSIBLE_MESH_WIDTH"] = (
            "parameters.getMaxAdmissibleCellSize(DomainSize, PatchSize)"
        )
