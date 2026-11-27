# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap

from dataclasses import dataclass

import jinja2

from .codegen.helper import render_jinja2_template

COORDINATES = "const Coordinates&"
RUNTIME_PARAMETERS = "const RuntimeParameters& parameters"


@dataclass(frozen=True)
class View:
    """Describes a parameter that a term receives as a generated type and the kernels pass as
    ``pointer``. The pointer overload of the term runs ``conversion`` and passes ``argument`` to
    the term."""

    declarator: str
    pointer: str
    argument: str
    conversion: str = ""


def _local(name):
    """Returns the name of the local copy of the parameter ``name``."""
    return "local" + name[0].upper() + name[1:]


def read_only_state_view(name, alias="State"):
    """Returns the view of one state that a term reads. The pointer overload passes the state in
    the array without a copy."""
    variables = "{T}" + alias
    return View(
        "const " + variables + "& " + name,
        "const {P}* const NOALIAS " + name,
        "*reinterpret_cast<const {}*>({})".format(variables, name),
    )


def state_view(name, alias="State"):
    """Returns the view of one state that a term writes. The pointer overload passes the state in
    the array without a copy."""
    variables = "{T}" + alias
    return View(
        variables + "& " + name,
        "{P}* const NOALIAS " + name,
        "*reinterpret_cast<{}*>({})".format(variables, name),
    )


def states_view(name, alias, writable=False):
    """Returns the view of an array of states such as the nodes of a face. The pointer overload casts
    the pointer without a copy."""
    variables = ("" if writable else "const ") + "{T}" + alias
    return View(
        variables + "& " + name,
        ("" if writable else "const ") + "{P}* const NOALIAS " + name,
        "*reinterpret_cast<{}*>({})".format(variables, name),
    )


def gradient_view(name):
    """Returns the view of the gradients of the unknowns along every axis. The kernels pass one
    pointer per axis. The pointer overload copies the gradients into one local array."""
    return View(
        "const {T}Gradient& " + name,
        "const {P}* const NOALIAS " + name + "[DIMENSIONS]",
        _local(name),
        "{T}Gradient "
        + _local(name)
        + ";\nfor (int axis = 0; axis < DIMENSIONS; axis++) {\n  "
        + _local(name)
        + "[axis] = *reinterpret_cast<const {T}Unknowns*>("
        + name
        + "[axis]);\n}\n",
    )


def get_alias_prefix(precision, dictionary):
    """Returns the prefix of the type aliases of ``precision`` in the solver class. The compute
    precision takes the empty prefix. The storage precision takes ``Stored`` and double ``Double``.
    """
    if precision == dictionary["COMPUTE_PRECISION"]:
        return ""
    if precision == dictionary["STORAGE_PRECISION"]:
        return "Stored"
    if precision == "double":
        return "Double"
    raise ValueError(
        "The PDE terms use the compute precision, the storage precision and double only (got '{}').".format(
            precision
        )
    )


def _concept_header(concept):
    """Returns the header of a concept, ``exahype2/terms/Flux.h`` for ``HasFlux``."""
    return "exahype2/terms/" + concept[len("Has") :] + ".h"


def _substitute(text, precision, dictionary):
    substituted = (
        text.replace("{P}", precision)
        .replace("{T}", get_alias_prefix(precision, dictionary))
        .replace("{S}", dictionary["FULL_QUALIFIED_SOLVER_NAME"])
        .replace("{N}", str(dictionary["NUMBER_OF_UNKNOWNS"]))
        .replace("{A}", str(dictionary["NUMBER_OF_AUXILIARY_VARIABLES"]))
    )
    if "{{" not in substituted:
        return substituted
    return render_jinja2_template(substituted, dictionary, jinja2.DebugUndefined)


def _declared_parameter(declarator):
    """Returns the declarator without the ``const`` of a by-value parameter. A declaration omits
    that ``const``."""
    if "&" in declarator or "[" in declarator:
        return declarator
    if "*" in declarator:
        pointee, star, name = declarator.rpartition("*")
        return pointee + star + name.replace(" const ", " ", 1)
    return declarator.removeprefix("const ")


def _parameter_lines(term, precision, dictionary, declaration):
    """Returns one line per parameter of a term. The runtime parameters are the last parameter of
    every term."""
    parameters = term.parameters + (RUNTIME_PARAMETERS,)
    lines = []
    for position, parameter in enumerate(parameters):
        if isinstance(parameter, View):
            parameter = parameter.declarator
        declarator, separator, comment = _substitute(
            parameter, precision, dictionary
        ).partition(" //")
        if declaration:
            declarator = _declared_parameter(declarator)
        else:
            declarator = "[[maybe_unused]] " + declarator
        comma = "," if position + 1 < len(parameters) else ""
        lines.append("  " + declarator + comma + separator + comment)
    return "\n".join(lines)


def _pointer_overload_argument(parameter, precision, dictionary):
    """Returns the argument that the pointer overload of a term passes to the term."""
    if isinstance(parameter, View):
        return _substitute(parameter.argument, precision, dictionary)
    declarator = parameter.partition(" //")[0].partition("[")[0]
    return declarator.split()[-1]


def get_inline_attributes(gpu):
    """Returns the attributes of a term that ``<Name>.cpph`` defines."""
    return "INLINE_METHOD GPU_CALLABLE_METHOD" if gpu else "INLINE_METHOD"


def render_declaration(term, precision, dictionary, attributes):
    """Returns the declaration of a term in the solver class. ``attributes`` precede the return
    type."""
    return "static {}{} {}(\n{}\n);".format(
        attributes + " " if attributes else "",
        _substitute(term.returns, precision, dictionary),
        term.cpp,
        _parameter_lines(term, precision, dictionary, True),
    )


def render_definition(term, precision, dictionary, attributes, body):
    """Returns the definition of a term with ``body`` and with ``attributes`` before the return
    type."""
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
        'tarch::terminateWithError("The solver {} must define the PDE term {}.");'.format(
            dictionary["SOLVER_NAME"], term.cpp
        ),
    )


def _pointer_overload_body(term, returns, precision, dictionary):
    """Returns the conversions of the views and the call of the term. The call passes the runtime
    parameters last."""
    conversions = ""
    for parameter in term.parameters:
        if isinstance(parameter, View):
            conversions += _substitute(parameter.conversion, precision, dictionary)
    call = "{}::{}(\n{}\n)".format(
        dictionary["SOLVER_NAME"],
        term.cpp,
        ",\n".join(
            "  " + _pointer_overload_argument(parameter, precision, dictionary)
            for parameter in term.parameters + (RUNTIME_PARAMETERS,)
        ),
    )
    if returns == "void":
        return conversions + call + ";\n"
    return conversions + "return " + call + ";\n"


def render_pointer_overload(term, precision, dictionary, attributes):
    """Returns the overload of a term that the kernels call with pointers. It converts the pointer
    of every View and calls the term of the solver class. The runtime parameters are its last
    parameter."""
    returns = _substitute(term.returns, precision, dictionary)
    if term.pointer_parameters is None:
        parameters = [
            parameter.pointer if isinstance(parameter, View) else parameter
            for parameter in term.parameters
        ]
        body = _pointer_overload_body(term, returns, precision, dictionary)
    else:
        parameters = list(term.pointer_parameters)
        body = _substitute(
            term.pointer_body.replace(
                "{TERM}", dictionary["SOLVER_NAME"] + "::" + term.cpp
            ),
            precision,
            dictionary,
        )
    parameters.append(RUNTIME_PARAMETERS)
    declarators = "\n".join(
        "  "
        + _substitute(parameter, precision, dictionary).partition(" //")[0]
        + ("," if position + 1 < len(parameters) else "")
        for position, parameter in enumerate(parameters)
    )
    return "static {} {} {}(\n{}\n) {{\n{}}}".format(
        attributes, returns, term.cpp, declarators, textwrap.indent(body, "  ")
    )


@dataclass(frozen=True)
class PDETerm:
    """Stores one row of the PDE term table of a solver family.

    ``parameters`` lists the C++ parameters. ``_substitute()`` replaces ``{P}`` with the
    precision of the term, ``{T}`` with the prefix of its type aliases, ``{S}`` with the solver
    name and ``{N}`` and ``{A}`` with the numbers of unknowns and auxiliary variables. A term
    whose pointer overload takes other parameters than the term lists them in
    ``pointer_parameters`` and its body in ``pointer_body``. ``pointer_body`` calls the term as
    ``{TERM}``.
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
    pointer_parameters: tuple = None
    pointer_body: str = None


def initial_condition(cells=False):
    """Returns the initial condition of one state. ``cells`` makes the term receive every node of a
    cell in one call."""
    return PDETerm(
        cpp="initialCondition",
        concept="HasInitialCondition",
        parameters=(
            states_view("Q", "CellStates", writable=True) if cells else state_view("Q"),
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
            read_only_state_view("Qinside"),
            state_view("Qoutside"),
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const int normal",
        ),
        precision=precision,
    )


def refinement_criterion():
    """Returns the refinement criterion of one cell. The grid construction passes the null pointer
    as ``Q``."""
    return PDETerm(
        cpp="refinementCriterion",
        concept="HasRefinementCriterion",
        parameters=(
            View(
                "const {T}CellStates* Q",
                "const {P}* const NOALIAS Q",
                "reinterpret_cast<const {T}CellStates*>(Q)",
            ),
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
            read_only_state_view("Q"),
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
            read_only_state_view("Q"),
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
            "const int normal",
            state_view("F", "Flux"),
        ),
        flag="USE_FLUX",
    )


def diffusive_flux():
    return PDETerm(
        cpp="flux",
        concept="HasDiffusiveFlux",
        parameters=(
            read_only_state_view("Q"),
            gradient_view("deltaQ"),
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
            "const int normal",
            state_view("F", "Flux"),
        ),
        flag=("USE_FLUX", "USE_DIFFUSIVE_FLUX"),
    )


def ncp():
    return PDETerm(
        cpp="nonconservativeProduct",
        concept="HasNonconservativeProduct",
        parameters=(
            read_only_state_view("Q"),
            read_only_state_view("deltaQ", "Difference"),
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
            "const int normal",
            state_view("BTimesDeltaQ", "NonconservativeProduct"),
        ),
        flag="USE_NCP",
    )


def source():
    return PDETerm(
        cpp="sourceTerm",
        concept="HasSourceTerm",
        parameters=(
            read_only_state_view("Q"),
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
            state_view("S", "Source"),
        ),
        flag="USE_SOURCE",
    )


def diffusive_source():
    return PDETerm(
        cpp="diffusiveSourceTerm",
        concept="HasDiffusiveSourceTerm",
        parameters=(
            read_only_state_view("Q"),
            states_view("deltaQ", "Gradient"),
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
            state_view("S", "Source"),
        ),
        flag="USE_DIFFUSIVE_SOURCE",
    )


def _riemann_solver_parameters(face_nodes):
    if face_nodes:
        states = (states_view("QL", "FaceStates"), states_view("QR", "FaceStates"))
        fluxes = (
            states_view("FL", "FaceFluxes", writable=True),
            states_view("FR", "FaceFluxes", writable=True),
        )
    else:
        states = (read_only_state_view("QL"), read_only_state_view("QR"))
        fluxes = (state_view("FL", "Flux"), state_view("FR", "Flux"))
    return (
        states
        + (
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
            "const int normal",
            "const bool isBoundaryFace",
            "const int faceIndex",
        )
        + fluxes
    )


def riemann_solver(face_nodes=False):
    """Returns the Riemann solver term of every solver family. ``face_nodes`` makes the term
    receive every node of a DG face in one call."""
    return PDETerm(
        cpp="solveRiemannProblem",
        concept="HasRiemannSolver",
        parameters=_riemann_solver_parameters(face_nodes),
        returns="double",
        flag="USE_RIEMANN_SOLVER",
    )


def subcell_riemann_solver():
    return PDETerm(
        cpp="solveSubcellRiemannProblem",
        concept="HasSubcellRiemannSolver",
        parameters=_riemann_solver_parameters(False),
        returns="double",
        flag="USE_SUBCELL_RIEMANN_SOLVER",
    )


def material_parameters():
    return PDETerm(
        cpp="multiplyMaterialParameterMatrix",
        concept="HasMaterialParameters",
        parameters=(
            read_only_state_view("Q"),
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
            "const int normal",
            state_view("rhs", "Unknowns"),
        ),
        flag="USE_MATERIAL_PARAMETERS",
    )


def point_source():
    return PDETerm(
        cpp="pointSource",
        concept="HasPointSource",
        parameters=(
            read_only_state_view("Q"),
            View("const Coordinates& x", "const double* const x", "Coordinates(x)"),
            "const double t",
            "const double dt",
            state_view("forceVector", "Source"),
            "const int n",
        ),
        flag="USE_POINT_SOURCE",
    )


def point_source_locations():
    return PDETerm(
        cpp="initPointSourceLocations",
        concept="HasPointSourceLocations",
        parameters=(
            View(
                "PointSourceLocations& location",
                "double location[NumberOfPointSources][DIMENSIONS]",
                "*reinterpret_cast<PointSourceLocations*>(location)",
            ),
        ),
        precision="storage",
        gpu=False,
        concept_arguments=("{S}::NumberOfPointSources",),
    )


def limited_difference():
    return PDETerm(
        cpp="limitedDifference",
        concept="HasLimitedDifference",
        parameters=(
            read_only_state_view("QL"),
            read_only_state_view("Q"),
            read_only_state_view("QR"),
            COORDINATES + " x",
            COORDINATES + " h",
            "const int normal",
            state_view("limitedDifferences"),
        ),
        flag="USE_LIMITED_DIFFERENCE",
    )


def reconstruction():
    return PDETerm(
        cpp="reconstruct",
        concept="HasReconstruction",
        parameters=(
            read_only_state_view("QL"),
            read_only_state_view("Q"),
            read_only_state_view("QR"),
            read_only_state_view("limitedDifferences"),
            COORDINATES + " x",
            COORDINATES + " h",
            "const int normal",
            state_view("reconstructedMinus"),
            state_view("reconstructedPlus"),
        ),
        flag="USE_RECONSTRUCTION",
    )


def physical_admissibility():
    """Returns the admissibility term of a span of states."""
    return PDETerm(
        cpp="isPhysicallyAdmissible",
        concept="HasPhysicalAdmissibility",
        parameters=(
            "const {T}States Q",
            COORDINATES + " x",
            COORDINATES + " h",
        ),
        returns="bool",
        precision="storage",
        flag="USE_PHYSICAL_ADMISSIBILITY",
        pointer_parameters=(
            "const {P}* const NOALIAS Q",
            "const int numberOfStates",
            COORDINATES + " x",
            COORDINATES + " h",
        ),
        pointer_body=(
            "ASSERTION(numberOfStates >= 0, numberOfStates);\n"
            "return {TERM}(\n"
            "  {T}States(reinterpret_cast<const {T}State*>(Q), static_cast<std::size_t>(numberOfStates)),\n"
            "  x,\n"
            "  h,\n"
            "  parameters\n"
            ");\n"
        ),
    )


def dmp_observables():
    return PDETerm(
        cpp="mapDmpObservables",
        concept="HasDmpObservables",
        parameters=(
            state_view("observables", "Observables"),
            read_only_state_view("Q"),
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
            read_only_state_view("Q"),
            state_view("solution", "Solution"),
        ),
        precision="double",
        flag="USE_ANALYTICAL_SOLUTION",
        concept_arguments=(),
    )


def particle_analytical_solution():
    """Returns the analytical solution term of a tracer set. The solution contains the exact position
    and the measured data of one particle."""
    return PDETerm(
        cpp="analyticalSolution",
        concept="HasParticleAnalyticalSolution",
        parameters=(
            View(
                "const ::exahype2::particles::Particle& particle",
                "const ::exahype2::particles::Particle* const particle",
                "*particle",
            ),
            "const double t",
            "const double dt",
            View(
                "TracerSolution& solution",
                "double* const NOALIAS solution",
                "*reinterpret_cast<TracerSolution*>(solution)",
            ),
        ),
        precision="double",
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
            states_view("Q", "CellStates", writable=True),
            COORDINATES + " x",
            COORDINATES + " h",
            "const double t",
            "const double dt",
        ),
        precision="storage",
    )


class PDETerms:
    """Stores the PDE terms that a script sets on one solver. ``set()`` checks every term
    against the table of the solver family."""

    def __init__(self, table):
        self._table = dict(table)
        self._values = {}

    def set(self, **pde_terms):
        """Stores every named PDE term. ``None`` and ``False`` remove a PDE term."""
        for name, value in pde_terms.items():
            if name not in self._table:
                raise ValueError(
                    "The solver family supports the PDE terms {} only (got '{}').".format(
                        ", ".join(sorted(self._table)), name
                    )
                )
            if value is None or value is False:
                self._values.pop(name, None)
            elif value is True or isinstance(value, str):
                self._values[name] = value
            else:
                raise TypeError(
                    "The PDE term '{}' expects True, False, None or a C++ body (got {!r}).".format(
                        name, value
                    )
                )

    def names(self):
        return list(self._table)

    def is_set(self, name):
        return name in self._values

    def is_user_defined(self, name):
        return self._values.get(name) is True

    def body(self, name):
        """Returns the inline body that the script sets for a term. Returns the default of the
        table for a term without a body."""
        value = self._values.get(name)
        if isinstance(value, str):
            return value
        return self._table[name].default

    def is_declared(self, name):
        """Returns whether the generated solver declares the term. The solver declares a term if
        the script sets it or if the table defines a default for it."""
        return name in self._values or self._table[name].default is not None

    def get_host_defined_device_terms(self, user_terms_inline):
        """Returns the C++ names of the user-defined device terms that ``<Name>.cpp`` defines on
        the host. The list is empty when ``user_terms_inline`` is true."""
        return [
            term.cpp
            for name, term in self._table.items()
            if term.gpu and self.is_user_defined(name) and not user_terms_inline
        ]

    def flags(self):
        """Returns the kernel flags of the table. A flag is true if the script sets one of its
        terms."""
        result = {}
        for name, term in self._table.items():
            if term.flag is None:
                continue
            for flag in term.flag if isinstance(term.flag, tuple) else (term.flag,):
                result[flag] = result.get(flag, False) or self.is_set(name)
        return result

    def _rendered_body(self, name, precision, dictionary):
        """Renders the inline body with the dictionary. A body reads the precision of its
        instantiation as ``COMPUTE_PRECISION``."""
        context = dict(dictionary)
        context["COMPUTE_PRECISION"] = precision
        return render_jinja2_template(self.body(name), context, jinja2.DebugUndefined)

    def _precisions_of(self, term, dictionary):
        if term.precision == "storage":
            return [dictionary["STORAGE_PRECISION"]]
        if term.precision == "double":
            return ["double"]
        return list(dictionary["COMPUTATION_PRECISIONS"])

    def names_with_inline_bodies(self):
        """Returns the declared terms whose body comes from the script or the table's default."""
        return [
            name
            for name in self._table
            if self.is_declared(name) and not self.is_user_defined(name)
        ]

    def uses_double_aliases(self, dictionary):
        """Returns whether a declared term uses the type aliases of double while neither the
        compute nor the storage precision is double."""
        return "double" not in dictionary["COMPUTATION_PRECISIONS"] and any(
            self.is_declared(name) and term.precision == "double"
            for name, term in self._table.items()
        )

    def render_declarations(self, dictionary, user_terms_inline, on_device):
        """Returns the declarations in ``<Name>.h`` of the declared terms that run on the device
        for ``on_device`` and of the host-only terms otherwise. ``user_terms_inline`` gives the
        user-defined terms the inline attributes. The application's ``<Name>.cpph`` defines
        them."""
        blocks = []
        for name, term in self._table.items():
            if not self.is_declared(name) or term.gpu != on_device:
                continue
            attributes = (
                ""
                if self.is_user_defined(name) and not user_terms_inline
                else get_inline_attributes(term.gpu)
            )
            for precision in self._precisions_of(term, dictionary):
                blocks.append(
                    render_declaration(term, precision, dictionary, attributes)
                )
        return "\n\n".join(blocks)

    def render_definitions(self, dictionary, on_device):
        """Returns the definitions in ``<Name>.cpph`` of the terms with an inline body that run on
        the device for ``on_device`` and of the host-only ones otherwise."""
        blocks = []
        for name, term in self._table.items():
            if (
                not self.is_declared(name)
                or self.is_user_defined(name)
                or term.gpu != on_device
            ):
                continue
            for precision in self._precisions_of(term, dictionary):
                blocks.append(
                    render_definition(
                        term,
                        precision,
                        dictionary,
                        get_inline_attributes(term.gpu),
                        self._rendered_body(name, precision, dictionary),
                    )
                )
        return "\n\n".join(blocks)

    def render_pointer_overloads(self, dictionary, on_device):
        """Returns one pointer overload per precision of the declared terms with a View that run
        on the device for ``on_device`` and of the host-only ones otherwise."""
        blocks = []
        for name, term in self._table.items():
            if (
                not self.is_declared(name)
                or term.gpu != on_device
                or (
                    term.pointer_parameters is None
                    and not any(
                        isinstance(parameter, View) for parameter in term.parameters
                    )
                )
            ):
                continue
            for precision in self._precisions_of(term, dictionary):
                blocks.append(
                    render_pointer_overload(
                        term, precision, dictionary, get_inline_attributes(term.gpu)
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
        """Returns the sorted headers of the concepts of the declared terms."""
        return sorted(
            {
                _concept_header(term.concept)
                for name, term in self._table.items()
                if self.is_declared(name)
            }
        )

    def render_assertions(self, dictionary):
        """Returns one ``static_assert`` per declared term and precision. The kernels call the terms
        of ``<Name>PDETerms``."""
        solver = dictionary["FULL_QUALIFIED_PDE_TERMS_NAME"]
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
                    'static_assert(\n  ::exahype2::terms::{}<{}{}>,\n  "The solver {} must define the PDE term {} as a static function with the expected signature."\n);'.format(
                        term.concept,
                        solver,
                        arguments,
                        dictionary["SOLVER_NAME"],
                        term.cpp,
                    )
                )
        return "\n".join(blocks)
