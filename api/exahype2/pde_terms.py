# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap

from dataclasses import dataclass

import jinja2

from .codegen.helper import render_jinja2_template

COORDINATES = "const tarch::la::Vector<DIMENSIONS, double>&"


@dataclass(frozen=True)
class PDETerm:
    """One row of a family's PDE term table.

    ``parameters`` lists the C++ parameters with ``{P}`` for the term's precision and ``{N}``
    and ``{A}`` for the unknown and auxiliary counts in the comments.
    """

    cpp: str
    concept: str
    parameters: tuple
    returns: str = "void"
    precision: str = "compute"
    gpu: bool = True
    default: str = None
    flag: str = None
    concept_arguments: tuple = ("{P}",)


def initial_condition():
    return PDETerm(
        cpp="initialCondition",
        concept="HasInitialCondition",
        parameters=(
            "{P}* const NOALIAS Q // [{N} + {A}]",
            COORDINATES + " x",
            COORDINATES + " h",
        ),
        precision="storage",
        gpu=False,
    )


def boundary_conditions(precision="storage"):
    return PDETerm(
        cpp="boundaryConditions",
        concept="HasBoundaryConditions",
        parameters=(
            "const {P}* const NOALIAS Qinside // [{N} + {A}]",
            "{P}* const NOALIAS Qoutside // [{N} + {A}]",
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const int normal",
        ),
        precision=precision,
    )


def refinement_criterion(comment):
    return PDETerm(
        cpp="refinementCriterion",
        concept="HasRefinementCriterion",
        parameters=(
            "const {P}* const NOALIAS Q // " + comment,
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
        ),
        returns="::exahype2::RefinementCommand",
        precision="storage",
        default="return ::exahype2::RefinementCommand::Keep;",
        flag="USE_REFINEMENT_CRITERION",
    )


def max_eigenvalue():
    return PDETerm(
        cpp="maxEigenvalue",
        concept="HasMaxEigenvalue",
        parameters=(
            "const {P}* const NOALIAS Q // [{N} + {A}]",
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
            "const int normal",
        ),
        returns="double",
        flag="USE_MAX_EIGENVALUE",
    )


def flux():
    return PDETerm(
        cpp="flux",
        concept="HasFlux",
        parameters=(
            "const {P}* const NOALIAS Q // [{N} + {A}]",
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
            "const int normal",
            "{P}* const NOALIAS F // [{N}]",
        ),
        flag="USE_FLUX",
    )


def diffusive_flux():
    return PDETerm(
        cpp="flux",
        concept="HasDiffusiveFlux",
        parameters=(
            "const {P}* const NOALIAS Q // [{N} + {A}]",
            "const {P}* const NOALIAS deltaQ[DIMENSIONS] // [DIMENSIONS][{N}]",
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
            "const int normal",
            "{P}* const NOALIAS F // [{N}]",
        ),
        flag=("USE_FLUX", "USE_DIFFUSIVE_FLUX"),
    )


def ncp():
    return PDETerm(
        cpp="nonconservativeProduct",
        concept="HasNonconservativeProduct",
        parameters=(
            "const {P}* const NOALIAS Q // [{N} + {A}]",
            "const {P}* const NOALIAS deltaQ // [{N} + {A}]",
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
            "const int normal",
            "{P}* const NOALIAS BTimesDeltaQ // [{N}]",
        ),
        flag="USE_NCP",
    )


def source():
    return PDETerm(
        cpp="sourceTerm",
        concept="HasSourceTerm",
        parameters=(
            "const {P}* const NOALIAS Q // [{N} + {A}]",
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
            "{P}* const NOALIAS S // [{N}]",
        ),
        flag="USE_SOURCE",
    )


def diffusive_source():
    return PDETerm(
        cpp="diffusiveSourceTerm",
        concept="HasDiffusiveSourceTerm",
        parameters=(
            "const {P}* const NOALIAS Q // [{N} + {A}]",
            "const {P}* const NOALIAS deltaQ // [DIMENSIONS * {N}]",
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
            "{P}* const NOALIAS S // [{N}]",
        ),
        flag="USE_DIFFUSIVE_SOURCE",
    )


def finite_volume_riemann_solver(flag="USE_RIEMANN_SOLVER"):
    return PDETerm(
        cpp="solveRiemannProblem",
        concept="HasFiniteVolumeRiemannSolver",
        parameters=(
            "const {P}* const NOALIAS QL // [{N} + {A}]",
            "const {P}* const NOALIAS QR // [{N} + {A}]",
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
            "const int normal",
            "{P}* const NOALIAS FL // [{N}]",
            "{P}* const NOALIAS FR // [{N}]",
        ),
        returns="double",
        flag=flag,
    )


def galerkin_riemann_solver():
    return PDETerm(
        cpp="riemannSolver",
        concept="HasGalerkinRiemannSolver",
        parameters=(
            "{P}* const NOALIAS FL // [{N}]",
            "{P}* const NOALIAS FR // [{N}]",
            "const {P}* const NOALIAS QL // [{N} + {A}]",
            "const {P}* const NOALIAS QR // [{N} + {A}]",
            "const double t",
            "const double dt",
            COORDINATES + " x",
            COORDINATES + " h",
            "const int direction",
            "const bool isBoundaryFace",
            "const int faceIndex",
        ),
        flag="USE_RIEMANN_SOLVER",
    )


def material_parameters():
    return PDETerm(
        cpp="multiplyMaterialParameterMatrix",
        concept="HasMaterialParameters",
        parameters=(
            "const {P}* const NOALIAS Q // [{N} + {A}]",
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
            "const int normal",
            "{P}* const NOALIAS rhs // [{N}]",
        ),
        flag="USE_MATERIAL_PARAMETERS",
    )


def point_source():
    return PDETerm(
        cpp="pointSource",
        concept="HasPointSource",
        parameters=(
            "const {P}* const NOALIAS Q // [{N} + {A}]",
            "const double* const x",
            "const double t",
            "const double dt",
            "{P}* const NOALIAS forceVector // [{N}]",
            "const int n",
        ),
        flag="USE_POINT_SOURCE",
    )


def point_source_locations():
    return PDETerm(
        cpp="initPointSourceLocations",
        concept="HasPointSourceLocations",
        parameters=("double location[NumberOfPointSources][DIMENSIONS]",),
        precision="storage",
        gpu=False,
        concept_arguments=("{S}::NumberOfPointSources",),
    )


def limited_difference():
    return PDETerm(
        cpp="limitedDifference",
        concept="HasLimitedDifference",
        parameters=(
            "const {P}* const NOALIAS QL // [{N} + {A}]",
            "const {P}* const NOALIAS Q // [{N} + {A}]",
            "const {P}* const NOALIAS QR // [{N} + {A}]",
            COORDINATES + " x",
            COORDINATES + " h",
            "const int normal",
            "const int unknown",
        ),
        returns="{P}",
        flag="USE_LIMITED_DIFFERENCE",
    )


def physical_admissibility():
    return PDETerm(
        cpp="isPhysicallyAdmissible",
        concept="HasPhysicalAdmissibility",
        parameters=(
            "const {P}* const NOALIAS Q // [numberOfStates * ({N} + {A})]",
            "const int numberOfStates",
            COORDINATES + " x",
            COORDINATES + " h",
        ),
        returns="bool",
        precision="storage",
        gpu=False,
        flag="USE_PHYSICAL_ADMISSIBILITY",
    )


def dmp_observables():
    return PDETerm(
        cpp="mapDmpObservables",
        concept="HasDmpObservables",
        parameters=(
            "{P}* const NOALIAS observables // [NumberOfDMPObservables]",
            "const {P}* const NOALIAS Q // [{N} + {A}]",
        ),
        precision="storage",
        gpu=False,
        flag="USE_DMP_OBSERVABLES",
    )


def dmp_relaxation_parameter():
    return PDETerm(
        cpp="dmpRelaxationParameter",
        concept="HasDmpRelaxationParameter",
        parameters=(
            "const int observable",
            "const {P} specifiedRelaxationParameter",
        ),
        returns="{P}",
        precision="storage",
        gpu=False,
        flag="USE_DMP_RELAXATION_PARAMETER",
    )


def analytical_solution():
    return PDETerm(
        cpp="analyticalSolution",
        concept="HasAnalyticalSolution",
        parameters=(
            COORDINATES + " x",
            "const double t",
            "const double dt",
            "const double* const Q // [{N} + {A}]",
            "double* const solution // [{N}]",
        ),
        precision="storage",
        gpu=False,
        flag="USE_ANALYTICAL_SOLUTION",
        concept_arguments=(),
    )


def particle_analytical_solution():
    """The particle type is the tracer the solver measures; the dictionary names it."""
    return PDETerm(
        cpp="analyticalSolution",
        concept="HasParticleAnalyticalSolution",
        parameters=(
            "globaldata::{{PARTICLE}}* const particle",
            "const double t",
            "const double dt",
            "double* const solution // [{{NUMBER_OF_UNKNOWNS_PARTICLE}} + DIMENSIONS]",
        ),
        precision="storage",
        gpu=False,
        flag="USE_PARTICLE_ANALYTICAL_SOLUTION",
        concept_arguments=("{{FULL_QUALIFIED_NAMESPACE}}::globaldata::{{PARTICLE}}",),
    )


class PDETerms:
    """The PDE terms a script set on one solver, checked against the family's table."""

    def __init__(self, table):
        self._table = dict(table)
        self._values = {}

    def set(self, **pde_terms):
        """Stores every named PDE term; ``None`` and ``False`` remove a PDE term."""
        for name, value in pde_terms.items():
            if name not in self._table:
                raise ValueError(
                    "Unknown PDE term '{}'. This solver family supports {}.".format(
                        name, ", ".join(sorted(self._table))
                    )
                )
            if value is None or value is False:
                self._values.pop(name, None)
            elif value is True or isinstance(value, str):
                self._values[name] = value
            else:
                raise TypeError(
                    "A PDE term is True, a C++ body or absent, got {!r} for '{}'.".format(
                        value, name
                    )
                )

    def names(self):
        return list(self._table)

    def is_set(self, name):
        return name in self._values

    def is_user_defined(self, name):
        return self._values.get(name) is True

    def body(self, name):
        """Returns the inline body of a term, or the table's default when nothing is set."""
        value = self._values.get(name)
        if isinstance(value, str):
            return value
        return self._table[name].default

    def is_declared(self, name):
        """Whether the generated solver declares the term: Set by the script or defaulted."""
        return name in self._values or self._table[name].default is not None

    def flags(self):
        """Returns the kernel flags of the table, true for every set term."""
        result = {}
        for name, term in self._table.items():
            if term.flag is None:
                continue
            for flag in term.flag if isinstance(term.flag, tuple) else (term.flag,):
                result[flag] = result.get(flag, False) or self.is_set(name)
        return result

    def _substitute(self, text, precision, dictionary):
        substituted = (
            text.replace("{P}", precision)
            .replace("{S}", dictionary["FULL_QUALIFIED_SOLVER_NAME"])
            .replace("{N}", str(dictionary["NUMBER_OF_UNKNOWNS"]))
            .replace("{A}", str(dictionary["NUMBER_OF_AUXILIARY_VARIABLES"]))
        )
        if "{{" not in substituted:
            return substituted
        return render_jinja2_template(substituted, dictionary, jinja2.DebugUndefined)

    def _rendered_body(self, name, precision, dictionary):
        """Renders the inline body with the dictionary, so a body may name the precision it is
        instantiated for through ``COMPUTE_PRECISION``."""
        context = dict(dictionary)
        context["COMPUTE_PRECISION"] = precision
        body = render_jinja2_template(self.body(name), context, jinja2.DebugUndefined)
        return textwrap.indent(textwrap.dedent(body.strip("\n")), "    ")

    def _precisions_of(self, term, dictionary):
        if term.precision == "storage":
            return [dictionary["STORAGE_PRECISION"]]
        return list(dictionary["COMPUTATION_PRECISIONS"])

    def _parameter_lines(self, term, precision, dictionary, prefix, indent):
        lines = []
        for position, parameter in enumerate(term.parameters):
            declarator, separator, comment = self._substitute(
                parameter, precision, dictionary
            ).partition(" //")
            comma = "," if position + 1 < len(term.parameters) else ""
            lines.append(indent + prefix + declarator + comma + separator + comment)
        return "\n".join(lines)

    def render_declarations(self, dictionary):
        """Returns the inline definitions and the user-defined declarations of ``<Name>.h``."""
        blocks = []
        for name, term in self._table.items():
            if not self.is_declared(name):
                continue
            for precision in self._precisions_of(term, dictionary):
                returns = self._substitute(term.returns, precision, dictionary)
                if self.is_user_defined(name):
                    blocks.append(
                        "  static {} {}(\n{}\n  );".format(
                            returns,
                            term.cpp,
                            self._parameter_lines(
                                term, precision, dictionary, "", "    "
                            ),
                        )
                    )
                else:
                    blocks.append(
                        "  static INLINE_METHOD{} {} {}(\n{}\n  ) {{\n{}\n  }}".format(
                            " GPU_CALLABLE_METHOD" if term.gpu else "",
                            returns,
                            term.cpp,
                            self._parameter_lines(
                                term,
                                precision,
                                dictionary,
                                "[[maybe_unused]] ",
                                "    ",
                            ),
                            self._rendered_body(name, precision, dictionary),
                        )
                    )
        return "\n\n".join(blocks)

    def render_definitions(self, dictionary):
        """Returns the stubs of ``<Name>.cpp`` for every user-defined term."""
        blocks = []
        for name, term in self._table.items():
            if not self.is_user_defined(name):
                continue
            for precision in self._precisions_of(term, dictionary):
                blocks.append(
                    '{} {}::{}::{}(\n{}\n) {{\n  tarch::terminateWithError("{}::{} is not implemented.");\n}}'.format(
                        self._substitute(term.returns, precision, dictionary),
                        dictionary["FULL_QUALIFIED_NAMESPACE"],
                        dictionary["SOLVER_NAME"],
                        term.cpp,
                        self._parameter_lines(
                            term, precision, dictionary, "[[maybe_unused]] ", "  "
                        ),
                        dictionary["SOLVER_NAME"],
                        term.cpp,
                    )
                )
        return "\n\n".join(blocks)

    def render_assertions(self, dictionary):
        """Returns one ``static_assert`` per declared term and precision."""
        solver = dictionary["FULL_QUALIFIED_SOLVER_NAME"]
        blocks = []
        for name, term in self._table.items():
            if not self.is_declared(name):
                continue
            for precision in self._precisions_of(term, dictionary):
                arguments = "".join(
                    ", " + self._substitute(argument, precision, dictionary)
                    for argument in term.concept_arguments
                )
                blocks.append(
                    'static_assert(\n  ::exahype2::{}<{}{}>,\n  "{}::{} is missing, is not static or has a wrong signature; see exahype2/SolverConcepts.h"\n);'.format(
                        term.concept,
                        solver,
                        arguments,
                        dictionary["SOLVER_NAME"],
                        term.cpp,
                    )
                )
        return "\n".join(blocks)
