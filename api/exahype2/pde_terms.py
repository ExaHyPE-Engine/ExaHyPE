# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap

from dataclasses import dataclass

import jinja2

from .codegen.helper import render_jinja2_template

COORDINATES = "const tarch::la::Vector<DIMENSIONS, double>&"


def _concept_header(concept):
    """Returns the header of a concept: ``HasFlux`` lives in ``exahype2/terms/Flux.h``."""
    return "exahype2/terms/" + concept[len("Has") :] + ".h"


def _substitute(text, precision, dictionary):
    substituted = (
        text.replace("{P}", precision)
        .replace("{S}", dictionary["FULL_QUALIFIED_SOLVER_NAME"])
        .replace("{N}", str(dictionary["NUMBER_OF_UNKNOWNS"]))
        .replace("{A}", str(dictionary["NUMBER_OF_AUXILIARY_VARIABLES"]))
    )
    if "{{" not in substituted:
        return substituted
    return render_jinja2_template(substituted, dictionary, jinja2.DebugUndefined)


def _declared_parameter(declarator):
    """Returns the declarator without the ``const`` of a by-value parameter, which a declaration
    omits."""
    if "&" in declarator or "[" in declarator:
        return declarator
    if "*" in declarator:
        pointee, star, name = declarator.rpartition("*")
        return pointee + star + name.replace(" const ", " ", 1)
    return declarator.removeprefix("const ")


def _parameter_lines(term, precision, dictionary, declaration):
    lines = []
    for position, parameter in enumerate(term.parameters):
        declarator, separator, comment = _substitute(
            parameter, precision, dictionary
        ).partition(" //")
        if declaration:
            declarator = _declared_parameter(declarator)
        else:
            declarator = "[[maybe_unused]] " + declarator
        comma = "," if position + 1 < len(term.parameters) else ""
        lines.append("  " + declarator + comma + separator + comment)
    return "\n".join(lines)


def get_inline_attributes(gpu):
    """Returns the attributes of a term that ``<Name>.cpph`` defines."""
    return "INLINE_METHOD GPU_CALLABLE_METHOD" if gpu else "INLINE_METHOD"


def render_declaration(term, precision, dictionary, attributes):
    """Returns the declaration of a term in the solver class; ``attributes`` precede the return
    type of a term that ``<Name>.cpph`` defines."""
    return "static {}{} {}(\n{}\n);".format(
        attributes + " " if attributes else "",
        _substitute(term.returns, precision, dictionary),
        term.cpp,
        _parameter_lines(term, precision, dictionary, True),
    )


def render_definition(term, precision, dictionary, attributes, body):
    """Returns the definition of a term with ``body``; ``attributes`` precede the return type of
    a term that ``<Name>.cpph`` defines."""
    return "{}{} {}::{}(\n{}\n) {{\n{}\n}}".format(
        attributes + " " if attributes else "",
        _substitute(term.returns, precision, dictionary),
        dictionary["FULL_QUALIFIED_SOLVER_NAME"],
        term.cpp,
        _parameter_lines(term, precision, dictionary, False),
        textwrap.indent(textwrap.dedent(body.strip("\n")), "  "),
    )


def render_stub_definition(term, precision, dictionary):
    """Returns the definition of a user-defined term for ``<Name>.cpp`` that terminates the
    application."""
    return render_definition(
        term,
        precision,
        dictionary,
        "",
        'tarch::terminateWithError("{}::{} is not implemented.");'.format(
            dictionary["SOLVER_NAME"], term.cpp
        ),
    )


@dataclass(frozen=True)
class PDETerm:
    """One row of a solver family's PDE term table.

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


def _riemann_solver_parameters(points):
    states = "[{N} + {A}]" if not points else "[" + points + " * ({N} + {A})]"
    fluxes = "[{N}]" if not points else "[" + points + " * {N}]"
    return (
        "const {P}* const NOALIAS QL // " + states,
        "const {P}* const NOALIAS QR // " + states,
        COORDINATES + " x",
        COORDINATES + " h",
        "const double t",
        "const double dt",
        "const int normal",
        "const bool isBoundaryFace",
        "const int faceIndex",
        "{P}* const NOALIAS FL // " + fluxes,
        "{P}* const NOALIAS FR // " + fluxes,
    )


def riemann_solver(points=""):
    """The Riemann solver of every solver family; ``points`` names the number of face nodes a DG
    solver passes at once."""
    return PDETerm(
        cpp="solveRiemannProblem",
        concept="HasRiemannSolver",
        parameters=_riemann_solver_parameters(points),
        returns="double",
        flag="USE_RIEMANN_SOLVER",
    )


def subcell_riemann_solver():
    return PDETerm(
        cpp="solveSubcellRiemannProblem",
        concept="HasSubcellRiemannSolver",
        parameters=_riemann_solver_parameters(""),
        returns="double",
        flag="USE_SUBCELL_RIEMANN_SOLVER",
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
        flag="USE_ANALYTICAL_SOLUTION",
        concept_arguments=(),
    )


def particle_analytical_solution():
    """The solution buffer holds the exact position first, then the data entries of the tracer
    the solver measures."""
    return PDETerm(
        cpp="analyticalSolution",
        concept="HasParticleAnalyticalSolution",
        parameters=(
            "const ::exahype2::particles::Particle* const particle",
            "const double t",
            "const double dt",
            "double* const solution // [DIMENSIONS + {{NUMBER_OF_UNKNOWNS_PARTICLE}}]",
        ),
        precision="storage",
        gpu=False,
        flag="USE_PARTICLE_ANALYTICAL_SOLUTION",
        concept_arguments=("::exahype2::particles::Particle",),
    )


def cell_predicate():
    return PDETerm(
        cpp="ownsCell",
        concept="HasCellPredicate",
        parameters=(COORDINATES + " x", COORDINATES + " h"),
        returns="bool",
        precision="storage",
        gpu=False,
    )


def face_predicate():
    return PDETerm(
        cpp="plotsFace",
        concept="HasFacePredicate",
        parameters=(COORDINATES + " x", COORDINATES + " h", "const int normal"),
        returns="bool",
        precision="storage",
        gpu=False,
    )


def postprocess_updated_patch():
    return PDETerm(
        cpp="postprocessUpdatedPatch",
        concept="HasPostprocessUpdatedPatch",
        parameters=(
            "{P}* const NOALIAS Q // [PointsPerCell * ({N} + {A})]",
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
        ),
        precision="storage",
    )


class PDETerms:
    """The PDE terms a script set on one solver, checked against the solver family's table."""

    def __init__(self, table):
        self._table = dict(table)
        self._values = {}
        self._host_only = set()

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

    def run_on_host(self, name):
        """Renders the named PDE term without the device attribute, so it may read host objects."""
        if name not in self._table:
            raise ValueError("Unknown PDE term '{}'.".format(name))
        self._host_only.add(name)

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
        """Returns whether the generated solver declares the term: Set by the script or defaulted."""
        return name in self._values or self._table[name].default is not None

    def get_host_defined_device_terms(self, user_terms_inline):
        """Returns the C++ names of the user-defined terms that the kernels call on the device and
        that ``<Name>.cpp`` defines on the host, because ``user_terms_inline`` is false.
        """
        return [
            term.cpp
            for name, term in self._table.items()
            if term.gpu
            and name not in self._host_only
            and self.is_user_defined(name)
            and not user_terms_inline
        ]

    def flags(self):
        """Returns the kernel flags of the table, true for every set term."""
        result = {}
        for name, term in self._table.items():
            if term.flag is None:
                continue
            for flag in term.flag if isinstance(term.flag, tuple) else (term.flag,):
                result[flag] = result.get(flag, False) or self.is_set(name)
        return result

    def _rendered_body(self, name, precision, dictionary):
        """Renders the inline body with the dictionary, so a body may name the precision it is
        instantiated for through ``COMPUTE_PRECISION``."""
        context = dict(dictionary)
        context["COMPUTE_PRECISION"] = precision
        return render_jinja2_template(self.body(name), context, jinja2.DebugUndefined)

    def _precisions_of(self, term, dictionary):
        if term.precision == "storage":
            return [dictionary["STORAGE_PRECISION"]]
        return list(dictionary["COMPUTATION_PRECISIONS"])

    def _attributes(self, name, term):
        return get_inline_attributes(term.gpu and name not in self._host_only)

    def names_with_inline_bodies(self):
        """Returns the declared terms whose body comes from the script or the table's default."""
        return [
            name
            for name in self._table
            if self.is_declared(name) and not self.is_user_defined(name)
        ]

    def render_declarations(self, dictionary, user_terms_inline):
        """Returns the declarations of every declared term in ``<Name>.h``; ``user_terms_inline``
        declares the user-defined terms for the application's ``<Name>.cpph``."""
        blocks = []
        for name, term in self._table.items():
            if not self.is_declared(name):
                continue
            attributes = (
                ""
                if self.is_user_defined(name) and not user_terms_inline
                else self._attributes(name, term)
            )
            for precision in self._precisions_of(term, dictionary):
                blocks.append(
                    render_declaration(term, precision, dictionary, attributes)
                )
        return "\n\n".join(blocks)

    def render_definitions(self, dictionary):
        """Returns the definitions of the terms with an inline body in ``<Name>.cpph``."""
        blocks = []
        for name, term in self._table.items():
            if not self.is_declared(name) or self.is_user_defined(name):
                continue
            for precision in self._precisions_of(term, dictionary):
                blocks.append(
                    render_definition(
                        term,
                        precision,
                        dictionary,
                        self._attributes(name, term),
                        self._rendered_body(name, precision, dictionary),
                    )
                )
        return "\n\n".join(blocks)

    def render_stub_definitions(self, dictionary):
        """Returns the terminating definitions of the user-defined terms, one per precision."""
        return [
            render_stub_definition(term, precision, dictionary)
            for name, term in self._table.items()
            if self.is_user_defined(name)
            for precision in self._precisions_of(term, dictionary)
        ]

    def concept_headers(self):
        """Returns the headers of the concepts of the declared terms, one per term."""
        return sorted(
            {
                _concept_header(term.concept)
                for name, term in self._table.items()
                if self.is_declared(name)
            }
        )

    def render_assertions(self, dictionary):
        """Returns one ``static_assert`` per declared term and precision."""
        solver = dictionary["FULL_QUALIFIED_SOLVER_NAME"]
        blocks = []
        for name, term in self._table.items():
            if not self.is_declared(name):
                continue
            for precision in self._precisions_of(term, dictionary):
                arguments = "".join(
                    ", " + _substitute(argument, precision, dictionary)
                    for argument in term.concept_arguments
                )
                blocks.append(
                    'static_assert(\n  ::exahype2::terms::{}<{}{}>,\n  "{}::{} is missing, is not static or has a wrong signature; see {}"\n);'.format(
                        term.concept,
                        solver,
                        arguments,
                        dictionary["SOLVER_NAME"],
                        term.cpp,
                        _concept_header(term.concept),
                    )
                )
        return "\n".join(blocks)
