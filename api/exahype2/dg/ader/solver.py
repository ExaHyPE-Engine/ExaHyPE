# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.pde_terms
import exahype2.solver

from exahype2.precision import cpp_type

from exahype2.dg.basis import GaussLegendreBasis
from exahype2.dg.basis import GaussLobattoBasis
from exahype2.dg.limiting.subcell_limiting import SubcellLimiting
from exahype2.dg.tracing import render_tracer_update

from .generate_kernels import generate_kernels


def _pde_terms():
    return {
        "initial_condition": exahype2.pde_terms.initial_condition(),
        "boundary_conditions": exahype2.pde_terms.boundary_conditions("compute"),
        "refinement_criterion": exahype2.pde_terms.refinement_criterion(
            "[PointsPerCell * ({N} + {A})]"
        ),
        "max_eigenvalue": exahype2.pde_terms.max_eigenvalue(),
        "flux": exahype2.pde_terms.flux(),
        "diffusive_flux": exahype2.pde_terms.diffusive_flux(),
        "ncp": exahype2.pde_terms.ncp(),
        "source": exahype2.pde_terms.source(),
        "diffusive_source": exahype2.pde_terms.diffusive_source(),
        "material_parameters": exahype2.pde_terms.material_parameters(),
        "point_source": exahype2.pde_terms.point_source(),
        "point_source_locations": exahype2.pde_terms.point_source_locations(),
        "riemann_solver": exahype2.pde_terms.riemann_solver(points="NodesPerFace"),
        "subcell_riemann_solver": exahype2.pde_terms.subcell_riemann_solver(),
        "physical_admissibility": exahype2.pde_terms.physical_admissibility(),
        "dmp_observables": exahype2.pde_terms.dmp_observables(),
        "dmp_relaxation_parameter": exahype2.pde_terms.dmp_relaxation_parameter(),
        "analytical_solution": exahype2.pde_terms.analytical_solution(),
        "particle_analytical_solution": exahype2.pde_terms.particle_analytical_solution(),
    }


class Solver(exahype2.solver.Solver, SubcellLimiting):
    """Generates an ADER-DG solver of polynomial order ``order``. Every time step of the solver
    runs one predictor and one corrector.

    ``basis`` selects :class:`exahype2.dg.GaussLegendre` or :class:`exahype2.dg.GaussLobatto`.
    ``linear`` selects the linear predictor. Only the linear predictor supports material
    parameters and point sources. ``point_sources`` counts the point sources.
    ``initialise_patches`` passes every node of a cell to the initial condition in one call.
    """

    SOLVER_FAMILY = "ader"
    SOLVER_FAMILY_INCLUDE = "exahype2/dg/ader/Solver.h"
    SOLVER_FAMILY_CONCEPT = "::exahype2::dg::ader::SolverConcept"
    SOLVER_FAMILY_CONCEPT_PRECISIONS = ("STORAGE_PRECISION", "CORRECTOR_PRECISION")
    PDE_TERMS = _pde_terms()

    def __init__(
        self,
        name,
        order,
        unknowns,
        auxiliary_variables,
        time_stepping,
        basis=GaussLegendreBasis,
        linear=False,
        precision="double",
        predictor_precision=None,
        time_average_precision=None,
        corrector_precision=None,
        point_sources=0,
        initialise_patches=False,
    ):
        super().__init__(name, unknowns, auxiliary_variables, time_stepping, precision)
        if basis not in (GaussLegendreBasis, GaussLobattoBasis):
            raise ValueError(
                "The ADER-DG solver supports the bases exahype2.dg.GaussLegendre and "
                "exahype2.dg.GaussLobatto only."
            )
        if isinstance(order, bool) or not isinstance(order, int) or order < 0:
            raise ValueError(
                "The ADER-DG solver expects a non-negative integer order (got {!r}).".format(
                    order
                )
            )
        if (
            not isinstance(point_sources, int)
            or isinstance(point_sources, bool)
            or point_sources < 0
        ):
            raise ValueError(
                "The ADER-DG solver expects a non-negative integer number of point sources "
                "(got {!r}).".format(point_sources)
            )
        self._order = order
        self._basis = basis(order + 1)
        self._is_linear = linear
        self._number_of_point_sources = point_sources
        self._initialise_patches = bool(initialise_patches)
        self._owns_cell_predicate = ""
        self._plots_face_predicate = ""

        if predictor_precision is not None:
          self._predictor_precision = cpp_type(predictor_precision)
          self.precision.add_compute_precision(predictor_precision)
        else:
          self._predictor_precision = self.precision.compute

        if time_average_precision is not None:
          self._time_average_precision = cpp_type(time_average_precision)
          self.precision.add_compute_precision(time_average_precision)
        else:
          self._time_average_precision = self._predictor_precision

        if corrector_precision is not None:
          self._corrector_precision = cpp_type(corrector_precision)
          self.precision.add_compute_precision(corrector_precision)
        else:
          self._corrector_precision = self.precision.compute

    @property
    def order(self):
        return self._order

    @property
    def basis(self):
        return self._basis

    def statistics_metadata(self):
        result = super().statistics_metadata() + [("Order", self._order)]
        if self.subcell_limiter_is_enabled:
            result.append(("Limiter", repr(self._subcell_limiter_scheme)))
        return result

    def restrict_to_cells(self, predicate):
        """Advances only the cells for which the C++ expression ``predicate`` in the cell centre
        ``x`` and the cell width ``h`` is true. A face next to a cell of a coupled solver reads
        the trace of that solver."""
        if not isinstance(predicate, str) or not predicate.strip():
            raise ValueError(
                "The solver {} expects a C++ expression on x and h in restrict_to_cells.".format(
                    self._name
                )
            )
        self._owns_cell_predicate = predicate.strip()
        return self

    @property
    def owns_cells(self):
        return bool(self._owns_cell_predicate)

    def plot_faces(self, predicate):
        """Writes the two traces of every face for which the C++ expression ``predicate`` is
        true into the snapshots ``<name>-face``. The expression reads the face centre ``x``, the
        cell width ``h`` and the normal axis ``normal``."""
        if not isinstance(predicate, str) or not predicate.strip():
            raise ValueError(
                "The solver {} expects a C++ expression on x, h and normal in plot_faces.".format(
                    self._name
                )
            )
        self._plots_face_predicate = predicate.strip()
        return self

    def coupling_matrix_from(self, other):
        """Returns the Lagrange interpolation matrix from the face nodes of ``other`` onto the
        face nodes of this solver, row-major with one row per node of ``other``."""
        return [
            repr(float(other.basis.value1D(point, node)))
            for node in range(other.basis.dofs_per_axis)
            for point in self.basis.quadrature_points
        ]

    def _validate(self):
        super()._validate()
        self._validate_subcell_limiter()
        if not self._is_linear and (
            self.pde_terms.is_set("material_parameters")
            or self.pde_terms.is_set("point_source")
        ):
            raise ValueError(
                "The ADER-DG solver supports material parameters and point sources with linear "
                "kernels only."
            )
        if self._is_linear and (
            self.pde_terms.is_set("diffusive_flux")
            or self.pde_terms.is_set("diffusive_source")
        ):
            raise ValueError(
                "The linear ADER-DG solver cannot add diffusive fluxes or diffusive sources."
            )
        if self.pde_terms.is_set("flux") and self.pde_terms.is_set("diffusive_flux"):
            raise ValueError(
                "A solver takes either the flux or the diffusive_flux PDE term."
            )
        point_source_pde_terms = [
            self.pde_terms.is_set("point_source"),
            self.pde_terms.is_set("point_source_locations"),
        ]
        if self._number_of_point_sources > 0 and not all(point_source_pde_terms):
            raise ValueError(
                "The solver {} with point sources needs the point_source and point_source_locations PDE terms.".format(
                    self._name
                )
            )
        if self._number_of_point_sources == 0 and any(point_source_pde_terms):
            raise ValueError(
                "The solver {} needs point_sources > 0 for the point_source and point_source_locations PDE terms.".format(
                    self._name
                )
            )
        if not self.pde_terms.is_set("riemann_solver") and not self.pde_terms.is_set(
            "max_eigenvalue"
        ):
            raise ValueError(
                "The solver {} without a Riemann solver needs the max_eigenvalue PDE term.".format(
                    self._name
                )
            )
        limiter_terms = [
            "subcell_riemann_solver",
            "physical_admissibility",
            "dmp_observables",
            "dmp_relaxation_parameter",
        ]
        if not self.subcell_limiter_is_enabled and any(
            self.pde_terms.is_set(term) for term in limiter_terms
        ):
            raise ValueError(
                "The solver {} needs add_subcell_limiter() for the PDE terms {}.".format(
                    self._name, ", ".join(limiter_terms)
                )
            )

    def _table_class(self, table):
        """Returns the fully qualified name of a generated table class of this solver."""
        return "::" + "::".join(self._namespace + [self._name + table])

    def _solver_family_includes(self):
        includes = [
            self._name + "DGMatrices.h",
            self._name + "Quadrature.h",
        ] + self._subcell_limiter_includes()
        if self._tracers:
            includes.append("exahype2/dg/Tracing.h")
        return includes

    def _render_tracer_update(self, tracer, d):
        return render_tracer_update(tracer, self._table_class("Quadrature"))

    def _solver_family_class(self):
        return "::exahype2::dg::ader::Solver<{}, {}, {}, {}, {}, {}>".format(
            self._solver_type(),
            self.precision.storage,
            self._predictor_precision,
            self._time_average_precision,
            self._corrector_precision,
            self._subcell_limiter_cpp_type(),
        )

    def _solver_family_constants(self):
        return [
            "static constexpr int  DGOrder                    = {};".format(
                self._order
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
            "static constexpr int  NumberOfPointSources       = {};".format(
                self._number_of_point_sources
            ),
            "static constexpr ::exahype2::TimeStepping::Rule TimeSteppingRule = "
            "::exahype2::TimeStepping::Rule::{};".format(self.time_stepping.rule),
        ] + self._subcell_limiter_constants()

    def _table_precisions(self):
        """Returns the solver precisions plus double for the mesh transfer operators."""
        result = list(self.precision.types)
        if "double" not in result:
            result.append("double")
        return result

    def _generate_solver_family_files(self, output, dictionary):
        generate_kernels(solver=self, dictionary=dictionary, output=output)

    def _add_solver_family_entries(self, d):
        d["DG_ORDER"] = self._order
        d["INITIALISE_PATCHES"] = self._initialise_patches
        d["TABLE_PRECISIONS"] = self._table_precisions()
        d["PREDICTOR_PRECISION"] = self._predictor_precision
        d["TIME_AVERAGE_PRECISION"] = self._time_average_precision
        d["CORRECTOR_PRECISION"] = self._corrector_precision
        d["LIMITER_ARGUMENTS"] = self._subcell_limiter_arguments()
        d["MAX_ADMISSIBLE_MESH_WIDTH"] = (
            "parameters.getMaxAdmissiblePatchSize(DomainSize)"
        )

    def _hooks(self):
        def predicate_body(predicate):
            return "return " + predicate + ";" if predicate else ""

        return super()._hooks() + [
            (
                exahype2.pde_terms.cell_predicate(),
                predicate_body(self._owns_cell_predicate),
            ),
            (
                exahype2.pde_terms.face_predicate(),
                predicate_body(self._plots_face_predicate),
            ),
        ]
