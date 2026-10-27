# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.pde_terms
import exahype2.solver


def pde_terms():
    """Returns the PDE terms that every finite-volume solver family accepts."""
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
        "riemann_solver": exahype2.pde_terms.riemann_solver(),
        "analytical_solution": exahype2.pde_terms.analytical_solution(),
        "particle_analytical_solution": exahype2.pde_terms.particle_analytical_solution(),
    }


class Solver(exahype2.solver.Solver):
    """The part of a finite-volume solver on patches of ``patch_size``^d volumes per cell that
    every family shares. :class:`exahype2.fv.singlestep.Solver` and
    :class:`exahype2.fv.ader.Solver` are the families an application instantiates.
    """

    SOLVER_FAMILY = "fv"
    SOLVER_FAMILY_CONCEPT = "::exahype2::fv::SolverConcept"
    SOLVER_FAMILY_CONCEPT_PRECISIONS = ("STORAGE_PRECISION",)

    def __init__(
        self,
        name,
        patch_size,
        unknowns,
        auxiliary_variables,
        time_stepping,
        precision="double",
    ):
        super().__init__(name, unknowns, auxiliary_variables, time_stepping, precision)
        if (
            isinstance(patch_size, bool)
            or not isinstance(patch_size, int)
            or patch_size < 1
        ):
            raise ValueError(
                "The finite-volume solver expects a positive integer patch_size "
                "(got {!r}).".format(patch_size)
            )
        self._patch_size = patch_size

    def statistics_metadata(self):
        return super().statistics_metadata() + [("Patch Size", self._patch_size)]

    def _provides_max_eigenvalue(self):
        """Returns true for the max_eigenvalue PDE term and for a Riemann solver. A Riemann solver
        returns the maximum wave speed of every face."""
        return self.pde_terms.is_set("max_eigenvalue") or self.pde_terms.is_set(
            "riemann_solver"
        )

    def _validate(self):
        super()._validate()
        if not self.pde_terms.is_set("riemann_solver") and not self.pde_terms.is_set(
            "max_eigenvalue"
        ):
            raise ValueError(
                "The solver {} without a Riemann solver needs the max_eigenvalue PDE term.".format(
                    self._name
                )
            )

    def _scheme_includes(self):
        """Returns the headers of the reconstruction and the time integration of the family."""
        raise NotImplementedError

    def _scheme_constants(self):
        """Returns the C++ type aliases of the reconstruction and the time integration."""
        raise NotImplementedError

    def _solver_family_includes(self):
        includes = list(self._scheme_includes())
        if self._tracers:
            includes.append("exahype2/fv/Tracing.h")
        return includes

    def _render_tracer_update(self, tracer, d):
        """Returns the C++ call that moves one tracer set through the patches of the solution.
        After a time step, the patches contain the updated solution."""
        fields = "".join(", {}".format(field) for field in tracer["FIELDS"])
        return (
            "::exahype2::fv::traceParticles<PatchSize, NumberOfData, {number_of_data}{fields}>(\n"
            "  cellConnectivity,\n"
            "  getSolution(),\n"
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
        return "{}<{}, {}, {}>".format(
            self.SOLVER_FAMILY_CLASS,
            self._solver_type(),
            self.precision.storage,
            self.precision.compute,
        )

    def _solver_family_constants(self):
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
        ] + list(self._scheme_constants()) + [
            "static constexpr ::exahype2::TimeStepping::Rule TimeSteppingRule = "
            "::exahype2::TimeStepping::Rule::{};".format(self.time_stepping.rule),
        ]

    def _add_solver_family_entries(self, d):
        d["MAX_ADMISSIBLE_MESH_WIDTH"] = (
            "parameters.getMaxAdmissibleCellSize(DomainSize, PatchSize)"
        )
