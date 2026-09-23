# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import math
import os
import re
import textwrap

import exahype2.codegen

from . import error_measurement
from . import time_stepping
from .actionsets.postprocess_solution import PostprocessSolution
from .actionsets.tracing import Tracing
from .codegen.helper import KeepUndefined
from .codegen.helper import render_jinja2_template
from .log import log
from .particles.particle_error_measurement import ParticleErrorMeasurement
from .particles.particle_error_measurement import number_of_particle_unknowns
from .pde_terms import PDETerms
from .precision import Precision
from .state import State


class Solver:
    """One solver of a project.

    Every generated C++ name (``Abstract<Name>``, ``<Name>``,
    ``repositories::instanceOf<Name>``, ``fineGridCell<Name>Q``) follows from ``name``.
    """

    PDE_TERMS = {}
    FAMILY = ""
    FAMILY_SOLVER_INCLUDE = ""
    FAMILY_SOLVER_CONCEPT = ""
    FAMILY_SOLVER_CONCEPT_PRECISIONS = ()

    def __init__(
        self,
        name,
        unknowns,
        auxiliary_variables,
        time_stepping_mode,
        precision="double",
    ):
        self._name = name
        self._solver_number = 0
        self._namespace = []
        self._dimensions = None
        self._project = None
        self.state = State(name, unknowns, auxiliary_variables)
        self.pde_terms = PDETerms(self.PDE_TERMS)
        self.time_stepping = time_stepping.of(time_stepping_mode)
        self.precision = Precision.of(precision)
        self.layout = None
        self.postprocess_solution = PostprocessSolution(self)

        self._includes = []
        self._abstract_solver_user_declarations = ""
        self._abstract_solver_user_definitions = ""
        self._solver_user_declarations = ""
        self._solver_user_definitions = ""
        self._start_grid_initialisation_implementation = ""
        self._start_time_step_implementation = ""
        self._finish_time_step_implementation = ""
        self._start_simulation_implementation = ""
        self._finish_simulation_implementation = ""
        self._error_measurement = None
        self._guard_extensions = []
        self._time_step_size_expression = None
        self._tracings = []
        self._tracing_action_sets = []
        self._particle_error_measurement = None
        self._baseline_action_set_descend_invocation_order = 0
        self._dictionary = None

    def __str__(self):
        return "{}(name={}, pde_terms={}, time_stepping={}, precision={})".format(
            type(self).__name__,
            self._name,
            sorted(
                name for name in self.pde_terms.names() if self.pde_terms.is_set(name)
            ),
            self.time_stepping,
            self.precision,
        )

    @property
    def name(self):
        return self._name

    @property
    def solver_number(self):
        return self._solver_number

    @property
    def unknowns(self):
        return self.state.unknowns

    @property
    def auxiliary_variables(self):
        return self.state.auxiliary_variables

    @property
    def instance_name(self):
        return "instanceOf" + self._name

    def set_pde_terms(self, **pde_terms):
        """Declares PDE terms: ``True`` for a function the project defines, a C++ body for an
        inline function, ``None`` to remove a PDE term."""
        self.pde_terms.set(**pde_terms)
        return self

    def measure_error(self, file="Error", every=0.0, unknowns=None):
        """Records the L1, L2 and Linf errors against the ``analytical_solution`` PDE term.

        ``every`` is the time between two records, zero for every time step; ``unknowns``
        selects the compared unknowns by index or name.
        """
        self._check_error_measurement(every)
        self._error_measurement = {
            "file": file,
            "every": every,
            "unknowns": self.state.indices_of(unknowns),
        }
        return self

    def measure_particle_error(
        self, particle_set, file="Error", every=0.0, descend_invocation_order=12
    ):
        """Records the L1, L2 and Linf position errors of the particles of ``particle_set``
        against the ``particle_analytical_solution`` PDE term."""
        self._check_error_measurement(every)
        self._error_measurement = {
            "file": file,
            "every": every,
            "unknowns": [],
            "particles": particle_set,
            "descend_invocation_order": descend_invocation_order,
        }
        self.add_includes(
            "globaldata/{}.h".format(particle_set.particle_model.name),
            "vertexdata/{}.h".format(particle_set.name),
        )
        return self

    def _check_error_measurement(self, every):
        if self._error_measurement is not None:
            raise ValueError(
                "[{}] records one error measurement only.".format(self._name)
            )
        if (
            isinstance(every, bool)
            or not isinstance(every, (int, float))
            or not math.isfinite(every)
            or every < 0
        ):
            raise ValueError(
                "[{}] every is a finite non-negative time, got {!r}.".format(
                    self._name, every
                )
            )

    def trace(
        self,
        particle_set,
        kernel=None,
        arguments=None,
        forces=None,
        descend_invocation_order=None,
        initialisation=True,
        time_steps=True,
    ):
        """Projects the solution onto the particles of ``particle_set`` after every update of
        the sweeps ``initialisation`` and ``time_steps`` select.

        ``kernel`` names the projection function and ``arguments`` its argument list, a Jinja2
        template over the solver dictionary; ``forces`` moves the particles with
        :class:`exahype2.particles.Forces`.
        """
        self._tracings.append(
            (
                particle_set,
                kernel,
                arguments,
                forces,
                descend_invocation_order,
                initialisation,
                time_steps,
            )
        )
        return self

    def add_guard(self, action_set, condition, guard="guard"):
        """Restricts the named guard of an action set (``"Prediction"``, ``"Correction"``,
        ...) to cells or faces that also satisfy ``condition``; ``guard`` names the attribute,
        ``"guard"``, ``"cell_guard"`` or ``"face_guard"``."""
        self._guard_extensions.append((action_set, guard, condition))
        return self

    def use_time_step_size(self, expression):
        """Replaces the time step size the kernels read by the C++ ``expression``."""
        self._time_step_size_expression = expression
        return self

    def action_set(self, name):
        """Returns the action set ``name`` in CamelCase once ``build`` created it."""
        attribute = (
            "_action_set_"
            + re.sub(
                r"(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])", "_", name
            ).lower()
        )
        if not hasattr(self, attribute):
            raise ValueError("[{}] has no action set {}.".format(self._name, name))
        return getattr(self, attribute)

    def add_declarations(self, code):
        """Adds C++ declarations to the generated abstract solver class."""
        self._abstract_solver_user_declarations += code
        return self

    def add_definitions(self, code):
        """Adds C++ definitions to the generated abstract solver's translation unit."""
        self._abstract_solver_user_definitions += code
        return self

    def add_solver_declarations(self, code):
        """Adds C++ declarations to the generated ``<Name>`` class."""
        self._solver_user_declarations += code
        return self

    def on_start_grid_initialisation(self, code):
        self._start_grid_initialisation_implementation += code
        return self

    def on_start_time_step(self, code):
        self._start_time_step_implementation += code
        return self

    def on_finish_time_step(self, code):
        self._finish_time_step_implementation += code
        return self

    def on_start_simulation(self, code):
        self._start_simulation_implementation += code
        return self

    def on_finish_simulation(self, code):
        self._finish_simulation_implementation += code
        return self

    def add_includes(self, *headers):
        """Adds headers, as paths below ``src`` or below the generated directory, to
        ``Abstract<Name>.h``; the PDE terms and every action set of the solver see them.
        """
        self._includes += headers
        return self

    def action_set_includes(self):
        return list(self.layout.includes) + self._kernel_table_includes()

    def _solver_includes(self, error_measurement, variable_shortcuts):
        headers = [
            "Constants.h",
            "RuntimeParameters.h",
            "exahype2/RefinementControl.h",
            "exahype2/RefinementCriterionHelpers.h",
            "peano4/Constants.h",
            "runner/RuntimeParameters.h",
            "tarch/Assertions.h",
            "tarch/NonCriticalAssertions.h",
            "tarch/la/Vector.h",
            self.FAMILY_SOLVER_INCLUDE,
        ]
        if error_measurement:
            headers.append("exahype2/ErrorMeasurement.h")
        if variable_shortcuts:
            headers.append("VariableShortcuts{}.h".format(self._name))
        library_headers = sorted(set(headers + self._kernel_table_includes()))
        return library_headers + [
            header
            for header in dict.fromkeys(self._includes)
            if header not in library_headers
        ]

    def _unknown_identifier(self):
        return self._name + "Q"

    def statistics_metadata(self):
        return [
            ("Unknowns", self.state.unknowns),
            ("Auxiliary Variables", self.state.auxiliary_variables),
        ]

    def _sweep_is(self, predicate):
        """Returns the call of one sweep predicate of the family's C++ solver class, which
        answers from the sweep state alone."""
        return "repositories::{}.{}()".format(self.instance_name, predicate)

    def _sweep_predicate(self, name):
        """Returns the call of one sweep predicate of the family's C++ solver class, which
        decides from the marker and the sweep state whether the data of a cell or a face is
        there."""
        return "repositories::{}.{}(marker)".format(self.instance_name, name)

    def _provide_cell_data_to_compute_kernels_default_guard(self):
        return self._sweep_predicate("providesCellDataToKernels")

    def _store_cell_data_default_guard(self):
        return self._sweep_predicate("storesCellData")

    def _load_cell_data_default_guard(self):
        return self._sweep_predicate("loadsCellData")

    def _store_face_data_default_guard(self):
        return self._sweep_predicate("storesFaceData")

    def _load_face_data_default_guard(self):
        return self._sweep_predicate("loadsFaceData")

    def _store_boundary_data_default_guard(self):
        return (
            self._store_face_data_default_guard()
            + " and not repositories::"
            + self.instance_name
            + ".PeriodicBoundaries[marker.getSelectedFaceNumber() % DIMENSIONS]"
            + " and not marker.hasBeenRefined() and fineGridFace"
            + self._name
            + "FaceLabel.getBoundary()"
        )

    def _delete_face_data_default_guard(self):
        return "true"

    def _interpolate_face_data_default_guard(self):
        return "not " + self._sweep_is("isGridInitialisationSweep")

    def _restrict_face_data_default_guard(self):
        return "true"

    @property
    def dictionary(self):
        """The template entries of the solver; ``build`` creates them once."""
        if self._dictionary is None:
            raise RuntimeError(
                "[{}] the project has not built the solver.".format(self._name)
            )
        return self._dictionary

    def _add_family_entries(self, d):
        """A family adds its own entries here."""

    def _render_solver_concept_assertion(self, d):
        """Returns the ``static_assert`` of the family concept, which names the PDE terms and the
        constants every kernel of the family reads."""
        arguments = "".join(
            ", " + d[entry] for entry in self.FAMILY_SOLVER_CONCEPT_PRECISIONS
        )
        return 'static_assert(\n  {}<{}{}>,\n  "{} does not satisfy {}; see exahype2/SolverConcepts.h"\n);'.format(
            self.FAMILY_SOLVER_CONCEPT,
            d["FULL_QUALIFIED_SOLVER_NAME"],
            arguments,
            d["SOLVER_NAME"],
            self.FAMILY_SOLVER_CONCEPT,
        )

    def _fill_dictionary(self, d):
        def render_template(template):
            return render_jinja2_template(template, d, KeepUndefined)

        d["DIMENSIONS"] = self._dimensions
        d["NAMESPACE"] = self._namespace
        d["FULL_QUALIFIED_NAMESPACE"] = "::".join(self._namespace)
        d["SOLVER_NAME"] = self._name
        d["FULL_QUALIFIED_SOLVER_NAME"] = "::".join(self._namespace) + "::" + self._name
        d["SOLVER_INSTANCE"] = self.instance_name
        d["UNKNOWN_IDENTIFIER"] = self._unknown_identifier()
        d["NUMBER_OF_UNKNOWNS"] = self.state.unknowns
        d["NUMBER_OF_AUXILIARY_VARIABLES"] = self.state.auxiliary_variables
        d["NUMBER_OF_DATA"] = self.state.number_of_values
        d["MEASURED_UNKNOWNS"] = (
            "{"
            + ", ".join(
                str(index)
                for index in (
                    self._error_measurement["unknowns"]
                    if self._error_measurement is not None
                    else range(self.state.unknowns)
                )
            )
            + "}"
        )

        d["STORAGE_PRECISION"] = self.precision.storage
        d["COMPUTE_PRECISION"] = self.precision.compute
        d["SOLUTION_STORAGE_PRECISION"] = self.precision.storage
        d["COMPUTATION_PRECISIONS"] = self.precision.types

        d["FAMILY"] = self.FAMILY
        d["ABSTRACT_SOLVER_BASE_CLASS"] = self._cpp_base_class()
        d["TIME_STEPPING_FACTOR"] = repr(float(self.time_stepping.factor))
        d["ADAPTIVE_TIME_STEPPING"] = self.time_stepping.adaptive
        d["COMPUTE_MAX_EIGENVALUE"] = self.time_stepping.adaptive
        d["KERNEL_TABLES"] = self._kernel_tables()

        d.update(self.pde_terms.flags())
        d["SOLVER_PDE_TERM_DECLARATIONS"] = textwrap.dedent(
            self.pde_terms.render_declarations(d)
        ).strip()
        d["SOLVER_PDE_TERM_DEFINITIONS"] = self.pde_terms.render_definitions(d)
        d["SOLVER_PDE_TERM_ASSERTIONS"] = self.pde_terms.render_assertions(d)
        d["SOLVER_CONCEPT_ASSERTION"] = self._render_solver_concept_assertion(d)

        d["ERROR_MEASUREMENT"] = self._error_measurement is not None
        if self._error_measurement is not None:
            d["ERROR_MEASUREMENT_FILE"] = self._error_measurement["file"]
            d["ERROR_MEASUREMENT_TIME_DELTA"] = repr(
                float(self._error_measurement["every"])
            )
        if (
            self._error_measurement is not None
            and "particles" in self._error_measurement
        ):
            particle_set = self._error_measurement["particles"]
            d["PARTICLE"] = particle_set.particle_model.name
            d["PARTICLES_CONTAINER"] = particle_set.name
            d["NUMBER_OF_UNKNOWNS_PARTICLE"] = number_of_particle_unknowns(particle_set)

        d["PERIODIC_BOUNDARIES"] = self._project.periodic_boundaries_mask
        d["SOLVER_INCLUDES"] = self._solver_includes(
            d["ERROR_MEASUREMENT"], self.state.has_names
        )

        d["ABSTRACT_SOLVER_USER_DECLARATIONS"] = render_template(
            self._abstract_solver_user_declarations
        ).strip("\n")
        d["ABSTRACT_SOLVER_USER_DEFINITIONS"] = render_template(
            self._abstract_solver_user_definitions
        ).strip("\n")
        d["SOLVER_USER_DECLARATIONS"] = render_template(self._solver_user_declarations)
        d["SOLVER_USER_DEFINITIONS"] = render_template(self._solver_user_definitions)
        d["START_GRID_INITIALISATION_IMPLEMENTATION"] = render_template(
            self._start_grid_initialisation_implementation
        )
        d["START_TIME_STEP_IMPLEMENTATION"] = render_template(
            self._start_time_step_implementation
        )
        d["FINISH_TIME_STEP_IMPLEMENTATION"] = render_template(
            self._finish_time_step_implementation
        )
        d["START_SIMULATION_IMPLEMENTATION"] = render_template(
            self._start_simulation_implementation
        )
        d["FINISH_SIMULATION_IMPLEMENTATION"] = render_template(
            self._finish_simulation_implementation
        )

        self._add_family_entries(d)
        d["COMPUTE_TIME_STEP_SIZE"] = self._compute_time_step_size()

    def _cpp_base_class(self):
        """Returns the instantiation of the family's C++ solver class template."""
        raise NotImplementedError("A solver family names its C++ base class.")

    def _abstract_class_name(self):
        return "::" + "::".join(self._namespace) + "::Abstract" + self._name

    def _kernel_table_includes(self):
        return []

    def _kernel_tables(self):
        """Returns the names of the operator tables the solver initialises at start-up."""
        return []

    def _compute_time_step_size(self):
        expression = self._time_step_size_expression
        if expression is None:
            expression = "repositories::" + self.instance_name + ".getTimeStepSize()"
        return "[[maybe_unused]] const auto timeStepSize = " + expression + ";"

    def _error_condition(self):
        """Returns the C++ condition under which a cell measures its error."""
        raise NotImplementedError("A solver family names its error measurement sweep.")

    def _error_solution(self):
        """Returns which solution buffer the error measurement reads."""
        return "current"

    def _error_adds_per_cell(self):
        """Whether a cell reports the sum over its degrees of freedom once."""
        return True

    def _error_loops(self):
        """Returns the C++ that visits the measured degrees of freedom of a cell."""
        accumulate = (
            error_measurement.TEMPLATE_ACCUMULATE_PER_CELL
            if self._error_adds_per_cell()
            else error_measurement.TEMPLATE_ACCUMULATE_PER_DOF
        )
        return self.layout.for_each_dof(accumulate, when=self._error_solution())

    def error_measurement_kernels(self):
        """Returns the cell kernels of the postprocessing that measure the error."""
        if self._error_measurement is None or "particles" in self._error_measurement:
            return []
        return [
            error_measurement.kernel(
                self._error_condition(),
                self._error_loops(),
                self._error_adds_per_cell(),
            )
        ]

    def _tracing_descend_invocation_order(self):
        return 0

    def _create_tracing_action_set(self, particle_set, kernel, arguments, forces):
        if forces is not None:
            raise ValueError(
                "[{}] only a finite-volume solver moves tracers with forces.".format(
                    self._name
                )
            )
        return Tracing(self, particle_set, kernel, arguments)

    def _create_generic_action_sets(self):
        """Creates the tracers and the particle error measurement once the dictionary exists."""
        self._tracing_action_sets = []
        for (
            particle_set,
            kernel,
            arguments,
            forces,
            order,
            initialisation,
            time_steps,
        ) in self._tracings:
            action_set = self._create_tracing_action_set(
                particle_set, kernel, arguments, forces
            )
            action_set.descend_invocation_order = (
                self._tracing_descend_invocation_order() if order is None else order
            )
            self._tracing_action_sets.append((action_set, initialisation, time_steps))
        self._particle_error_measurement = None
        if (
            self._error_measurement is not None
            and "particles" in self._error_measurement
        ):
            self._particle_error_measurement = ParticleErrorMeasurement(
                self,
                self._error_measurement["particles"],
                self._error_measurement["descend_invocation_order"],
            )

    def tracing_action_sets_per_step(self):
        """Returns every tracing per step its trace() call selects."""
        result = {"init_grid": [], "perform_time_step": []}
        for action_set, initialisation, time_steps in self._tracing_action_sets:
            if initialisation:
                result["init_grid"].append(action_set)
            if time_steps:
                result["perform_time_step"].append(action_set)
        return result

    def _postprocess_action_sets(self):
        """The particle error measurement takes the place of an empty postprocessing."""
        result = []
        if (
            self._particle_error_measurement is None
            or self.postprocess_solution.has_kernels()
        ):
            result.append(self.postprocess_solution)
        if self._particle_error_measurement is not None:
            result.append(self._particle_error_measurement)
        return result

    def _apply_descend_invocation_orders(self, orders, offset=0):
        """Sets the descend invocation order of the action set ``_action_set_<name>`` for every
        entry of ``orders``."""
        for name, order in orders.items():
            getattr(self, "_action_set_" + name).descend_invocation_order = (
                offset + order
            )

    def _apply_guard_extensions(self):
        for name, guard, condition in self._guard_extensions:
            target = self.action_set(name)
            setattr(target, guard, getattr(target, guard) + " and " + condition)

    def _provides_max_eigenvalue(self):
        """Returns whether the kernels obtain the maximum eigenvalue adaptive time stepping reads."""
        return self.pde_terms.is_set("max_eigenvalue")

    def _validate(self):
        """Checks the PDE term configuration once the project generates."""
        if self.time_stepping.adaptive and not self._provides_max_eigenvalue():
            raise ValueError(
                "[{}] adaptive time stepping needs the max_eigenvalue PDE term.".format(
                    self._name
                )
            )
        if self._error_measurement is not None:
            term = (
                "particle_analytical_solution"
                if "particles" in self._error_measurement
                else "analytical_solution"
            )
            if not self.pde_terms.is_set(term):
                raise ValueError(
                    "[{}] the error measurement needs the {} PDE term.".format(
                        self._name, term
                    )
                )
        if self.pde_terms.is_set("particle_analytical_solution") and (
            self._error_measurement is None
            or "particles" not in self._error_measurement
        ):
            raise ValueError(
                "[{}] the particle_analytical_solution PDE term needs measure_particle_error.".format(
                    self._name
                )
            )

    def _create_layout(self):
        """Returns the :class:`exahype2.layout.Layout` of the family."""
        raise NotImplementedError

    def create_data_structures(self):
        raise NotImplementedError

    def create_action_sets(self):
        raise NotImplementedError

    def register_data(self, datamodel):
        raise NotImplementedError

    def use_data_in_step(self, step):
        raise NotImplementedError

    def action_sets_per_step(self):
        """Returns the action sets of the solver in the order each step adds them, keyed by the
        steps ``create_grid``, ``init_grid``, ``plot_solution``, ``perform_time_step`` and
        ``dynamic_amr`` of the project."""
        raise NotImplementedError

    def _generate_kernels(self, output, dictionary):
        """A family writes its operator tables and configures its kernel calls here."""

    def build(self, project):
        """Creates the data, the action sets and the dictionary of the solver, then writes its
        C++ files."""
        self._project = project
        self._dimensions = project.dimensions
        self._namespace = project.namespace
        output = project.output
        self._validate()
        self.layout = self._create_layout()
        self.create_data_structures()
        self.create_action_sets()
        self._apply_guard_extensions()

        dictionary = {}
        self._fill_dictionary(dictionary)
        self._generate_kernels(output, dictionary)
        self._fill_dictionary(dictionary)
        self._dictionary = dictionary
        self._create_generic_action_sets()

        self.state.generate_shortcuts(dictionary, self._namespace, output)

        template_directory = os.path.dirname(os.path.realpath(__file__))

        def template(name):
            return os.path.join(template_directory, name)

        log.debug("Solver configuration: %s.", self)
        output.add(
            exahype2.codegen.TemplatedFile(
                template("AbstractSolver.h.jinja2"),
                template("AbstractSolver.cpp.jinja2"),
                "Abstract" + self._name,
                self._namespace,
                ".",
                dictionary,
                True,
            )
        )
        has_definitions = bool(
            (
                dictionary["SOLVER_PDE_TERM_DEFINITIONS"]
                + dictionary["SOLVER_USER_DEFINITIONS"]
            ).strip()
        )
        output.add(
            exahype2.codegen.TemplatedFile(
                template("Solver.h.jinja2"),
                template("Solver.cpp.jinja2") if has_definitions else None,
                self._name,
                self._namespace,
                ".",
                dictionary,
                True,
            )
        )
        output.add(
            exahype2.codegen.TemplatedFile(
                template("PDETerms.h.jinja2"),
                None,
                "PDETerms" + self._name,
                self._namespace,
                ".",
                dictionary,
                True,
            )
        )
        output.makefile.add_h_file("Abstract" + self._name + ".h", generated=True)
        output.makefile.add_h_file(self._name + ".h", generated=True)
        output.makefile.add_h_file("PDETerms" + self._name + ".h", generated=True)
        output.makefile.add_cpp_file("Abstract" + self._name + ".cpp", generated=True)
        if has_definitions:
            output.makefile.add_cpp_file(self._name + ".cpp", generated=True)

    def _plot_time_stamp(self):
        return (
            "0.5 * (repositories::getMinTimeStamp() + repositories::getMaxTimeStamp())"
        )
