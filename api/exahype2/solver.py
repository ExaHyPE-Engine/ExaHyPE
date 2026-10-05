# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import math
import os

import exahype2.codegen

from . import time_stepping
from .codegen.helper import KeepUndefined
from .codegen.helper import render_jinja2_template
from .log import log
from .pde_terms import PDETerms
from .pde_terms import get_inline_attributes
from .pde_terms import postprocess_updated_patch
from .pde_terms import render_declaration
from .pde_terms import render_definition
from .pde_terms import render_stub_definition
from .precision import Precision
from .state import State


class Solver:
    """One solver of a project.

    ``name`` sets the generated C++ names ``<Name>``, ``<Name>Base``, ``<Name>PDETerms`` and
    ``instanceOf<Name>``.
    """

    PDE_TERMS = {}
    SOLVER_FAMILY = ""
    SOLVER_FAMILY_INCLUDE = ""
    SOLVER_FAMILY_CONCEPT = ""
    SOLVER_FAMILY_CONCEPT_PRECISIONS = ()

    def __init__(
        self,
        name,
        unknowns,
        auxiliary_variables,
        time_stepping_mode,
        precision="double",
    ):
        self._name = name
        self._namespace = []
        self._dimensions = None
        self._project = None
        self.state = State(name, unknowns, auxiliary_variables)
        self.pde_terms = PDETerms(self.PDE_TERMS)
        self.time_stepping = time_stepping.of(time_stepping_mode)
        self.precision = Precision.of(precision)

        self._includes = []
        self._solver_base_user_declarations = ""
        self._solver_base_user_definitions = ""
        self._solver_user_declarations = ""
        self._start_grid_initialisation_implementation = ""
        self._start_time_step_implementation = ""
        self._finish_time_step_implementation = ""
        self._start_simulation_implementation = ""
        self._finish_simulation_implementation = ""
        self._error_measurement = None
        self._tracers = []
        self._dictionary = None
        self._postprocess_updated_patch = ""

    @property
    def postprocess_updated_patch(self):
        return self._postprocess_updated_patch

    @postprocess_updated_patch.setter
    def postprocess_updated_patch(self, kernel):
        """Sets the C++ code that runs on every cell after its update. The code reads and writes
        the ``PointsPerCell`` updated points ``Q`` of the cell and reads ``x``, ``h``, ``t`` and
        ``dt``. ``True`` leaves the definition to the application."""
        self._postprocess_updated_patch = kernel

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

        ``every`` is the time between two records, zero for every time step. ``unknowns``
        selects the compared unknowns by index or name.
        """
        if self._error_measurement is not None:
            raise ValueError(
                "The solver {} records one error measurement only.".format(self._name)
            )
        if (
            isinstance(every, bool)
            or not isinstance(every, (int, float))
            or not math.isfinite(every)
            or every < 0
        ):
            raise ValueError(
                "The solver {} expects a finite non-negative time for every (got {!r}).".format(
                    self._name, every
                )
            )
        self._error_measurement = {
            "file": file,
            "every": every,
            "unknowns": self.state.indices_of(unknowns),
        }
        return self

    def trace(self, tracers, forces=None, initialisation=True, time_steps=True):
        """Moves the tracers through the solution with ``forces``, a
        :class:`exahype2.particles.Forces`. ``initialisation`` and ``time_steps`` select whether
        the tracers move after the grid initialisation and after every time step."""
        tracers.attach(self, forces, initialisation, time_steps)
        if len(tracers.forces.interpolated_fields) > self.state.number_of_values:
            raise ValueError(
                "The tracers of the solver {} interpolate at most {} fields.".format(
                    self._name, self.state.number_of_values
                )
            )
        if tracers not in self._tracers:
            self._tracers.append(tracers)
        return self

    def measure_particle_error(self, tracers, file="Error", every=0.0):
        """Records the L1, L2 and Linf position errors of the tracers against the
        ``particle_analytical_solution`` PDE term."""
        if tracers.solver is not self:
            raise ValueError(
                "The solver {} measures the error of its own tracers only.".format(
                    self._name
                )
            )
        tracers.measure_error(file, every)
        return self

    def add_declarations(self, code):
        """Adds C++ declarations to the generated ``<Name>Base`` class."""
        self._solver_base_user_declarations += code
        return self

    def add_definitions(self, code):
        """Adds C++ definitions to the translation unit of the generated ``<Name>Base``."""
        self._solver_base_user_definitions += code
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
        """Adds headers to ``<Name>Base.h``, so the PDE terms see them. A header path is
        relative to ``src`` or to the generated directory."""
        self._includes += headers
        return self

    def _solver_includes(self, variable_shortcuts):
        headers = [
            "Constants.h",
            "RuntimeParameters.h",
            "exahype2/MultiresolutionDetail.h",
            "exahype2/RefinementCommand.h",
            "exahype2/runner/RuntimeParameters.h",
            "tarch/Assertions.h",
            "tarch/la/Vector.h",
            self.SOLVER_FAMILY_INCLUDE,
        ]
        if variable_shortcuts:
            headers.append(self._name + "VariableShortcuts.h")
        if self._tracers or self.pde_terms.is_set("particle_analytical_solution"):
            headers.append("exahype2/particles/ParticleSet.h")
        if self._tracers:
            headers.append("exahype2/particles/Forces.h")
            headers.append("exahype2/plotter/ParticlePlotter.h")
            headers.append("exahype2/runner/Statistics.h")
            headers.append("tarch/accelerator/MemoryLocation.h")
        if any(tracers.records_a_database for tracers in self._tracers):
            headers.append("exahype2/particles/TrajectoryRecording.h")
            headers.append("exahype2/particles/TrajectoryDatabase.h")
        if any(tracers.reads_a_file for tracers in self._tracers):
            headers.append("exahype2/particles/FileReader.h")
        if any(tracers.error_measurement for tracers in self._tracers):
            headers.append("exahype2/ErrorMeasurement.h")
            headers.append("exahype2/particles/ParticleErrorMeasurement.h")
        headers += self._solver_family_includes() + list(self._includes)
        return sorted(set(headers))

    def _solver_family_includes(self):
        """Returns the headers of the kernels the C++ class of the solver family instantiates."""
        return []

    def statistics_metadata(self):
        return [
            ("Unknowns", self.state.unknowns),
            ("Auxiliary Variables", self.state.auxiliary_variables),
        ]

    def plotted_fields(self):
        """Returns the plotted fields as name and component pairs: The named variables, or
        every value under the solver name."""
        if self.state.has_names:
            return list(self.state.plot_description.items())
        return [(self._name + "Q", self.state.number_of_values)]

    @property
    def dictionary(self):
        """Returns the template entries of the solver, which ``build`` creates."""
        if self._dictionary is None:
            raise RuntimeError(
                "The project must build the solver {} first.".format(self._name)
            )
        return self._dictionary

    def _solver_type(self):
        """Returns the fully qualified C++ type of the solver."""
        return "::" + "::".join(self._namespace + [self._name])

    def _add_solver_family_entries(self, d):
        """Adds the template entries of a solver family to ``d``."""

    def _hooks(self):
        """Returns the static hooks of ``<Name>`` as pairs of a :class:`PDETerm` and its inline
        body. An empty body omits the hook, and ``True`` leaves its definition to the
        application."""
        return [
            (
                postprocess_updated_patch(),
                self._postprocess_updated_patch,
            )
        ]

    def _user_terms_inline(self):
        """Returns whether the application registered ``<Name>.cpph``. This file defines the
        user-defined terms and hooks inline, so the device calls them."""
        return self._name + ".cpph" in self._project.output.makefile.hfiles

    def _user_terms_in_cpp(self):
        """Returns whether the application registered ``<Name>.cpp``, which defines the
        user-defined terms and hooks."""
        return self._name + ".cpp" in self._project.output.makefile.cppfiles

    def _render_tracer_update(self, tracer, d):
        """Returns the C++ call that moves one tracer set through the solution."""
        raise NotImplementedError("A solver family must provide its tracer update.")

    def _generate_solver_family_files(self, output, dictionary):
        """Adds the generated files that the kernels of a solver family read."""

    def _render_solver_concept_assertion(self, d):
        """Returns the ``static_assert`` of the solver family concept. The concept lists the PDE
        terms and constants that the kernels of the solver family read."""
        arguments = "".join(
            ", " + d[entry] for entry in self.SOLVER_FAMILY_CONCEPT_PRECISIONS
        )
        return 'static_assert(\n  {}<{}{}>,\n  "The solver {} must define every PDE term and constant of its solver family."\n);'.format(
            self.SOLVER_FAMILY_CONCEPT,
            d["FULL_QUALIFIED_SOLVER_NAME"],
            arguments,
            d["SOLVER_NAME"],
        )

    def _fill_dictionary(self, d):
        def render_template(template):
            return render_jinja2_template(template, d, KeepUndefined)

        d["DIMENSIONS"] = self._dimensions
        d["NAMESPACE"] = self._namespace
        d["FULL_QUALIFIED_NAMESPACE"] = "::".join(self._namespace)
        d["SOLVER_NAME"] = self._name
        d["FULL_QUALIFIED_SOLVER_NAME"] = "::".join(self._namespace + [self._name])
        d["NUMBER_OF_UNKNOWNS"] = self.state.unknowns
        d["NUMBER_OF_AUXILIARY_VARIABLES"] = self.state.auxiliary_variables
        d["NUMBER_OF_DATA"] = self.state.number_of_values

        d["STORAGE_PRECISION"] = self.precision.storage
        d["COMPUTE_PRECISION"] = self.precision.compute
        d["COMPUTATION_PRECISIONS"] = self.precision.types

        d["SOLVER_FAMILY"] = self.SOLVER_FAMILY
        d["SOLVER_FAMILY_CLASS"] = self._solver_family_class()
        d["SOLVER_FAMILY_CONSTANTS"] = self._solver_family_constants()
        d["TIME_STEPPING_FACTOR"] = repr(float(self.time_stepping.factor))
        d["PLOTTED_FIELDS"] = ", ".join(
            '{"' + name + '", ' + str(count) + "}"
            for name, count in self.plotted_fields()
        )

        d.update(self.pde_terms.flags())
        d["USER_TERMS_INLINE"] = self._user_terms_inline()
        d["SOLVER_PDE_TERM_DECLARATIONS"] = self.pde_terms.render_declarations(
            d, d["USER_TERMS_INLINE"], True
        )
        d["SOLVER_HOST_PDE_TERM_DECLARATIONS"] = self.pde_terms.render_declarations(
            d, d["USER_TERMS_INLINE"], False
        )
        d["HOST_DEFINED_DEVICE_TERMS"] = self.pde_terms.get_host_defined_device_terms(
            d["USER_TERMS_INLINE"]
        )
        d["SOLVER_PDE_TERM_ASSERTIONS"] = self.pde_terms.render_assertions(d)
        d["SOLVER_CONCEPT_ASSERTION"] = self._render_solver_concept_assertion(d)
        d["SOLVER_CONCEPT_INCLUDES"] = sorted(set(self.pde_terms.concept_headers()))

        d["TRACERS"] = [
            tracers.dictionary(self._dimensions) for tracers in self._tracers
        ]
        measured = [tracers for tracers in self._tracers if tracers.error_measurement]
        d["NUMBER_OF_UNKNOWNS_PARTICLE"] = measured[0].number_of_data if measured else 0
        for tracer in d["TRACERS"]:
            tracer["UPDATE_CALL"] = self._render_tracer_update(tracer, d)
        d["TRACERS_RECORD"] = any(
            tracer["DATABASE"] or tracer["ERROR"] for tracer in d["TRACERS"]
        )

        d["ERROR_MEASUREMENT"] = self._error_measurement is not None
        d["MEASURED_UNKNOWNS"] = (
            self._error_measurement["unknowns"]
            if self._error_measurement is not None
            else []
        )
        if self._error_measurement is not None:
            d["ERROR_MEASUREMENT_FILE"] = self._error_measurement["file"]
            d["ERROR_MEASUREMENT_TIME_DELTA"] = repr(
                float(self._error_measurement["every"])
            )

        d["PERIODIC_BOUNDARIES"] = self._project.periodic_boundaries_mask
        includes = self._solver_includes(self.state.has_names)
        d["SOLVER_INCLUDES_WITHOUT_PATH"] = [
            header for header in includes if "/" not in header
        ]
        d["SOLVER_INCLUDES_WITH_PATH"] = [
            header for header in includes if "/" in header
        ]

        d["SOLVER_BASE_USER_DECLARATIONS"] = render_template(
            self._solver_base_user_declarations
        ).strip("\n")
        d["SOLVER_BASE_USER_DEFINITIONS"] = render_template(
            self._solver_base_user_definitions
        ).strip("\n")
        d["SOLVER_USER_DECLARATIONS"] = render_template(self._solver_user_declarations)
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

        self._add_solver_family_entries(d)
        precision = d["STORAGE_PRECISION"]
        hooks = [
            (term, body) for term, body in self._hooks() if body is True or body.strip()
        ]
        d["SOLVER_HOOK_DECLARATIONS"] = "\n\n".join(
            render_declaration(
                term,
                precision,
                d,
                (
                    ""
                    if body is True and not d["USER_TERMS_INLINE"]
                    else get_inline_attributes(term.gpu)
                ),
            )
            for term, body in hooks
        )
        d["SOLVER_INLINE_DEFINITIONS"] = "\n\n".join(
            [self.pde_terms.render_definitions(d, True)]
            + [
                render_definition(
                    term, precision, d, get_inline_attributes(term.gpu), body
                )
                for term, body in hooks
                if body is not True
            ]
        ).strip()
        d["SOLVER_HOST_INLINE_DEFINITIONS"] = self.pde_terms.render_definitions(
            d, False
        )
        d["SOLVER_STUB_DEFINITIONS"] = (
            ""
            if d["USER_TERMS_INLINE"] or self._user_terms_in_cpp()
            else "\n\n".join(
                self.pde_terms.render_stub_definitions(d)
                + [
                    render_stub_definition(term, precision, d)
                    for term, body in hooks
                    if body is True
                ]
            )
        )
        if d["USER_TERMS_INLINE"] and (
            d["SOLVER_INLINE_DEFINITIONS"] or d["SOLVER_HOST_INLINE_DEFINITIONS"]
        ):
            raise ValueError(
                "The solver {} defines its terms in {}.cpph. Set {} to True and define them there.".format(
                    self._name,
                    self._name,
                    ", ".join(
                        self.pde_terms.names_with_inline_bodies()
                        + [term.cpp for term, body in hooks if body is not True]
                    ),
                )
            )

    def _solver_family_class(self):
        """Returns the instantiation of the solver family's C++ solver class template."""
        raise NotImplementedError(
            "A solver family must provide its C++ solver class template."
        )

    def _solver_family_constants(self):
        """Returns the C++ constants and type aliases of the generated class. The kernels read them
        through ``Derived``."""
        return []

    def _provides_max_eigenvalue(self):
        """Returns whether the kernels compute the maximum eigenvalue for adaptive time stepping."""
        return self.pde_terms.is_set("max_eigenvalue")

    def _validate(self):
        """Checks the configuration of the solver when the project generates its code."""
        if self.time_stepping.adaptive and not self._provides_max_eigenvalue():
            raise ValueError(
                "The solver {} needs the max_eigenvalue PDE term for adaptive time stepping.".format(
                    self._name
                )
            )
        every_axis = (1 << self._dimensions) - 1
        periodic_axes = self._project.periodic_boundaries_mask & every_axis
        if periodic_axes != every_axis and not self.pde_terms.is_declared(
            "boundary_conditions"
        ):
            raise ValueError(
                "The solver {} needs the boundary_conditions PDE term on a domain that is not periodic along every axis.".format(
                    self._name
                )
            )
        if self._error_measurement is not None and not self.pde_terms.is_set(
            "analytical_solution"
        ):
            raise ValueError(
                "The solver {} needs the analytical_solution PDE term for the error measurement.".format(
                    self._name
                )
            )
        if any(
            tracers.error_measurement for tracers in self._tracers
        ) and not self.pde_terms.is_set("particle_analytical_solution"):
            raise ValueError(
                "The solver {} needs the particle_analytical_solution PDE term for the particle error measurement.".format(
                    self._name
                )
            )
        if any(
            tracers.samples_face_traces for tracers in self._tracers
        ) and self.SOLVER_FAMILY not in ("ader", "rk"):
            raise ValueError(
                "The solver {} needs a DG solver family to sample face traces.".format(
                    self._name
                )
            )
        if len({tracers.name for tracers in self._tracers}) != len(self._tracers):
            raise ValueError(
                "The solver {} traces each tracer set once only.".format(self._name)
            )

    def build(self, project):
        """Fills the dictionary of the solver and adds its C++ files to the output."""
        self._project = project
        self._dimensions = project.dimensions
        self._namespace = project.namespace
        output = project.output
        self._validate()

        dictionary = {}
        self._fill_dictionary(dictionary)
        self._dictionary = dictionary

        self.state.generate_shortcuts(dictionary, self._namespace, output)
        self._generate_solver_family_files(output, dictionary)

        template_directory = os.path.dirname(os.path.realpath(__file__))

        def template(name):
            return os.path.join(template_directory, name)

        log.debug("Solver configuration: %s.", self)
        output.add(
            exahype2.codegen.TemplatedFile(
                template("SolverBase.h.jinja2"),
                template("SolverBase.cpp.jinja2"),
                self._name + "Base",
                self._namespace,
                "",
                dictionary,
                True,
            )
        )
        solver_templates = ["Solver.h.jinja2"]
        if (
            dictionary["SOLVER_INLINE_DEFINITIONS"]
            or dictionary["SOLVER_HOST_INLINE_DEFINITIONS"]
        ):
            solver_templates.append("Solver.cpph.jinja2")
            output.makefile.add_h_file(self._name + ".cpph", generated=True)
        if dictionary["SOLVER_STUB_DEFINITIONS"]:
            solver_templates.append("Solver.cpp.jinja2")
            output.makefile.add_cpp_file(self._name + ".cpp", generated=True)
        for solver_template in solver_templates:
            output.add(
                exahype2.codegen.TemplatedFile(
                    template(solver_template),
                    None,
                    self._name,
                    self._namespace,
                    "",
                    dictionary,
                    True,
                )
            )
        output.add(
            exahype2.codegen.TemplatedFile(
                template("PDETerms.h.jinja2"),
                None,
                self._name + "PDETerms",
                self._namespace,
                "",
                dictionary,
                True,
            )
        )
        output.makefile.add_h_file(self._name + ".h", generated=True)
        output.makefile.add_h_file(self._name + "Base.h", generated=True)
        output.makefile.add_h_file(self._name + "PDETerms.h", generated=True)
        output.makefile.add_cpp_file(
            self._name + "Base.cpp", generated=True, is_gpu_kernel=True
        )
