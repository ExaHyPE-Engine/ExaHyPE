# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import json
import logging
import math
import multiprocessing
import os
import re
import subprocess
import sys

import exahype2.codegen
import exahype2.particles

from .log import log
from .log import setup_logging
from .main import Main


class Project:
    """A project generates one executable from its solvers.

    The terminal times and grid levels are the defaults the generated executable takes when the
    command line names none.
    """

    _arguments = None

    def __init__(
        self,
        namespace,
        project_name,
        dimensions,
        offset,
        size,
        periodic_bc,
        executable="ExaHyPE",
        min_end_time=None,
        max_end_time=None,
        min_grid_levels=None,
        max_grid_levels=None,
        directory=".",
    ):
        arguments = Project._parse_arguments()
        setup_logging(level=logging.DEBUG if arguments.verbose else logging.INFO)
        if arguments.dimensions is not None:
            dimensions = arguments.dimensions
        if sys.version_info < (3, 11):
            log.warning(
                "Python 3.11 or higher is recommended to support all features. Current version: %s",
                sys.version,
            )

        self.namespace = list(namespace)
        self.project_name = project_name
        self.directory = directory
        self._dimensions = dimensions
        self._domain_offset = list(offset)[0:dimensions]
        self._domain_size = list(size)[0:dimensions]
        self._periodic_bc = list(periodic_bc)[0:dimensions]
        self._min_terminal_time = min_end_time
        self._max_terminal_time = max_end_time
        self._min_grid_levels = min_grid_levels
        self._max_grid_levels = max_grid_levels
        self._build_mode = (
            exahype2.codegen.Makefile.Mode[arguments.build_mode]
            if arguments.build_mode is not None
            else exahype2.codegen.Makefile.Mode.Release
        )

        self.output = exahype2.codegen.Output()
        self.main = Main(self)
        self.output.makefile.set_executable_name(executable)

        self._solvers = []
        self._couplings = []
        self._solver_set_generated = False
        self._runtime_parameters = []
        self._constants = {}
        self._makefile_configured = False
        self.is_built = False
        self.build_was_successful = False

    @staticmethod
    def _parse_arguments():
        """Parses the command-line arguments every project takes; a script reads its own
        arguments with ``parse_known_args`` next to them."""
        if Project._arguments is None:
            parser = argparse.ArgumentParser(
                allow_abbrev=False,
                description="ExaHyPE project code generation options.",
            )
            parser.add_argument(
                "-d",
                "--dimensions",
                type=int,
                choices=(2, 3),
                default=None,
                help="Code generation dimensions; overrides the value set by the application.",
            )
            parser.add_argument(
                "-m",
                "--build-mode",
                choices=[mode.name for mode in exahype2.codegen.Makefile.Mode],
                default=None,
                help="Build mode; overrides the application default (usually Release).",
            )
            parser.add_argument(
                "--generate-only",
                "--no-build",
                action="store_true",
                default=False,
                help="Generate source files but skip compilation.",
            )
            parser.add_argument(
                "-j",
                "--jobs",
                type=int,
                default=None,
                help="Parallel build jobs. Values <= 0 use the detected CPU count.",
            )
            parser.add_argument(
                "--clean-first",
                action=argparse.BooleanOptionalAction,
                default=None,
                help="Wipe CMake's incremental build state before compiling.",
            )
            parser.add_argument(
                "-v",
                "--verbose",
                action="store_true",
                default=False,
                help="Enable verbose (debug) logging during code generation.",
            )
            Project._arguments, _ = parser.parse_known_args()
        return Project._arguments

    def __str__(self):
        return "Project(name={}, solvers={})".format(
            self.project_name, [solver.name for solver in self._solvers]
        )

    @property
    def dimensions(self):
        return self._dimensions

    @property
    def domain_offset(self):
        return self._domain_offset

    @property
    def domain_size(self):
        return self._domain_size

    @property
    def periodic_bc(self):
        return self._periodic_bc

    @property
    def domain_offset_initialiser(self):
        return ", ".join(str(value) for value in self._domain_offset)

    @property
    def domain_size_initialiser(self):
        return ", ".join(str(value) for value in self._domain_size)

    @property
    def periodic_boundaries_mask(self):
        return sum(
            1 << axis for axis, periodic in enumerate(self._periodic_bc) if periodic
        )

    def add_solver(self, solver):
        """Appends a solver; a repeated solver name and a project that has already generated
        its code both start a new solver set."""
        redeclaration = self._solver_set_generated or any(
            existing.name == solver.name for existing in self._solvers
        )
        if redeclaration:
            self._solvers = []
            self.output.makefile.clear_user_files()
            self._solver_set_generated = False
        self._solvers.append(solver)
        return solver

    def add_tracer(self, name, particle_attributes):
        """Returns a tracer set; a solver moves it through ``Solver.trace``."""
        return exahype2.particles.Tracers(name, particle_attributes)

    def couple(self, first, second):
        """Couples two ADER-DG solvers of this project across the faces between the cells
        their ``restrict_to_cells`` predicates own; ``first`` drives the time steps of both.
        """
        for solver in (first, second):
            if solver not in self._solvers:
                raise ValueError(
                    "[{}] belongs to the project before it is coupled.".format(
                        solver.name
                    )
                )
            if not hasattr(solver, "coupling_matrix_from"):
                raise ValueError(
                    "[{}] is not an ADER-DG solver and cannot be coupled.".format(
                        solver.name
                    )
                )
            if not solver.owns_cells:
                raise ValueError(
                    "[{}] restricts itself to cells before it is coupled.".format(
                        solver.name
                    )
                )
        if first is second:
            raise ValueError("[{}] cannot couple with itself.".format(first.name))
        if first.state.number_of_values != second.state.number_of_values:
            raise ValueError(
                "[{}] and [{}] carry different numbers of values per node.".format(
                    first.name, second.name
                )
            )
        self._couplings.append((first, second))
        return self

    def add_constant(self, name, value, cpp_type="double"):
        """Inserts ``value`` as a C++ expression into ``Constants.h``.

        A repeated name replaces the existing constant.
        """
        self._constants[name] = {"name": name, "value": str(value), "type": cpp_type}

    def add_runtime_parameter(self, name, parameter_type, default_value):
        """Registers a parameter the executable reads from config.json and the command line. The
        generated RuntimeParameters::<name>() returns it."""
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
            raise ValueError(
                f"Invalid runtime parameter name '{name}'. Use a valid C++ identifier."
            )

        is_array = isinstance(parameter_type, (tuple, list))
        if is_array:
            if len(parameter_type) != 2:
                raise ValueError(
                    f"Array runtime parameter '{name}' must use a type specification of the form ('double', N)."
                )
            base_type = parameter_type[0]
            array_size = parameter_type[1]
            if base_type not in ("double", "int", "bool"):
                raise ValueError(
                    f"Unsupported array runtime parameter base type '{base_type}'. Supported types are: double, int, bool."
                )
            if (
                not isinstance(array_size, int)
                or isinstance(array_size, bool)
                or array_size <= 0
            ):
                raise ValueError(
                    f"Array runtime parameter '{name}' expects a positive integer size."
                )
            if (
                not isinstance(default_value, (list, tuple))
                or len(default_value) != array_size
            ):
                raise ValueError(
                    f"Array runtime parameter '{name}' expects exactly {array_size} default values."
                )
        else:
            base_type = parameter_type
            array_size = None
            if base_type not in ("double", "int", "bool", "string"):
                raise ValueError(
                    f"Unsupported runtime parameter type '{base_type}'. Supported types are: double, int, bool, string."
                )

        expected_types = {
            "double": (int, float),
            "int": (int,),
            "bool": (bool,),
            "string": (str,),
        }[base_type]
        for value in default_value if is_array else (default_value,):
            if not isinstance(value, expected_types) or (
                base_type in ("double", "int") and isinstance(value, bool)
            ):
                raise ValueError(
                    f"Runtime parameter '{name}' expects default values of type '{base_type}'."
                )
            if base_type == "double" and not math.isfinite(value):
                raise ValueError(
                    f"Runtime parameter '{name}' expects finite default values."
                )

        scalar_cpp_type = {
            "double": "double",
            "int": "int",
            "bool": "bool",
            "string": "std::string",
        }[base_type]
        if is_array:
            cpp_type = f"tarch::la::Vector<{array_size}, {scalar_cpp_type}>"
            literal = ", ".join(
                (
                    repr(float(value))
                    if base_type == "double"
                    else (
                        str(value)
                        if base_type == "int"
                        else "true" if value else "false"
                    )
                )
                for value in default_value
            )
            return_type = cpp_type
        else:
            cpp_type = scalar_cpp_type
            if base_type == "double":
                literal = repr(float(default_value))
            elif base_type == "int":
                literal = str(default_value)
            elif base_type == "bool":
                literal = "true" if default_value else "false"
            else:
                literal = json.dumps(default_value)
            return_type = cpp_type

        self._runtime_parameters = [
            parameter
            for parameter in self._runtime_parameters
            if parameter["name"] != name
        ]
        self._runtime_parameters.append(
            {
                "name": name,
                "cpp_type": cpp_type,
                "return_type": return_type,
                "is_array": is_array,
                "is_string": base_type == "string",
                "array_size": array_size,
                "default_value": literal,
            }
        )

    def _generate_constants(self):
        self.output.add(
            exahype2.codegen.TemplatedFile(
                os.path.join(
                    os.path.dirname(os.path.realpath(__file__)), "Constants.h.jinja2"
                ),
                None,
                "Constants",
                self.namespace,
                "",
                {
                    "CONSTANTS": list(self._constants.values()),
                    "DOMAIN_OFFSET": self.domain_offset_initialiser,
                    "DOMAIN_SIZE": self.domain_size_initialiser,
                    "PERIODIC_BOUNDARIES": self.periodic_boundaries_mask,
                },
                True,
            )
        )
        self.output.makefile.add_h_file("Constants.h", generated=True)

    def _generate_runtime_parameters(self):
        template_directory = os.path.dirname(os.path.realpath(__file__))
        dictionary = {
            "RUNTIME_PARAMETERS": self._runtime_parameters,
            "HAS_ARRAY_RUNTIME_PARAMETERS": any(
                parameter["is_array"] for parameter in self._runtime_parameters
            ),
        }
        self.output.add(
            exahype2.codegen.TemplatedFile(
                os.path.join(template_directory, "RuntimeParameters.h.jinja2"),
                os.path.join(template_directory, "RuntimeParameters.cpp.jinja2"),
                "RuntimeParameters",
                self.namespace,
                "",
                dictionary,
            )
        )
        self.output.add(
            exahype2.codegen.TemplatedFile(
                os.path.join(template_directory, "RuntimeParameters.cpph.jinja2"),
                None,
                "RuntimeParameters",
                self.namespace,
                "",
                dictionary,
            )
        )
        self.output.makefile.add_h_file("RuntimeParameters.h", generated=True)
        self.output.makefile.add_h_file("RuntimeParameters.cpph", generated=True)
        self.output.makefile.add_cpp_file("RuntimeParameters.cpp", generated=True)

    def _solver_repository_dictionary(self):
        return {
            "SOLVERS": [
                (
                    solver.name,
                    solver.instance_name,
                    [
                        (str(label), str(value))
                        for label, value in solver.statistics_metadata()
                    ],
                    bool(solver.dictionary["TRACERS"]),
                )
                for solver in self._solvers
            ],
            "COUPLINGS": [
                {
                    "FIRST": first.instance_name,
                    "SECOND": second.instance_name,
                    "FROM_SECOND": first.coupling_matrix_from(second),
                    "FROM_FIRST": second.coupling_matrix_from(first),
                }
                for first, second in self._couplings
                if first in self._solvers and second in self._solvers
            ],
            "SOLVER_HEADERS": sorted(
                header
                for solver in self._solvers
                for header in (solver.name + ".h", solver.name + "PDETerms.h")
            ),
        }

    def _generate_solver_repository(self):
        template_directory = os.path.dirname(os.path.realpath(__file__))
        self.output.add(
            exahype2.codegen.TemplatedFile(
                os.path.join(template_directory, "SolverRepository.h.jinja2"),
                os.path.join(template_directory, "SolverRepository.cpp.jinja2"),
                "SolverRepository",
                self.namespace,
                "",
                self._solver_repository_dictionary(),
            )
        )
        self.output.makefile.add_h_file("SolverRepository.h", generated=True)
        self.output.makefile.add_cpp_file(
            "SolverRepository.cpp", generated=True, is_gpu_kernel=True
        )

    @staticmethod
    def _optional_default(value, fallback):
        return fallback if value is None else str(value)

    def _configure_main(self):
        # A non-increasing interval uses the minimum stamp as its sole completion criterion.
        max_terminal_time = self._max_terminal_time
        if (
            max_terminal_time is not None
            and self._min_terminal_time is not None
            and max_terminal_time <= self._min_terminal_time
        ):
            max_terminal_time = None
        self.main.template_dictionary.update(
            {
                "DEFAULT_MIN_TERMINAL_TIME": self._optional_default(
                    self._min_terminal_time, "-1.0"
                ),
                "DEFAULT_MAX_TERMINAL_TIME": self._optional_default(
                    max_terminal_time, "-1.0"
                ),
                "DEFAULT_GRID_LEVELS": self._optional_default(
                    self._min_grid_levels, "-1"
                ),
                "DEFAULT_MAX_GRID_LEVELS": self._optional_default(
                    self._max_grid_levels, "-1"
                ),
            }
        )

    def _generate(self):
        """Writes every generated file; the project state is put back afterwards, so a second
        call generates the same files again."""
        if not self._solvers:
            raise ValueError("The project has no solver.")
        self._solver_set_generated = True
        self.output.makefile.parse_configuration_outcome()
        self._makefile_configured = True
        self.output.makefile.set_dimension(self._dimensions)
        self.output.makefile.set_mode(self._build_mode)

        pre_generation_artefacts = list(self.output.artefacts)
        pre_generation_generated_files = (
            list(self.output.makefile.generated_hfiles),
            list(self.output.makefile.generated_cppfiles),
            list(self.output.makefile.generated_fortranfiles),
            list(self.output.makefile.generated_gpu_cppfiles),
        )

        try:
            for solver in self._solvers:
                log.debug("Prepare solver %s.", solver.name)
                solver.build(self)

            self._generate_constants()
            self._generate_solver_repository()
            self._configure_main()
            self._generate_runtime_parameters()
            self.main.construct_output(self.output, self.project_name + "-main")

            generated_subdir = self.project_name + ".generated"
            self.output.makefile.d["BUILD_SUBDIR"] = self.project_name + ".build"
            self.output.generate(self.directory, generated_subdir)
            log.info("Generation complete (%d artefacts)", len(self.output.artefacts))
        finally:
            self.output.artefacts = pre_generation_artefacts
            (
                self.output.makefile.generated_hfiles,
                self.output.makefile.generated_cppfiles,
                self.output.makefile.generated_fortranfiles,
                self.output.makefile.generated_gpu_cppfiles,
            ) = pre_generation_generated_files

    def _run_cmake(self, command, environment, action):
        process = subprocess.Popen(
            command,
            cwd=self.directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            env=environment,
            text=True,
        )
        captured = []
        for line in process.stdout:
            sys.stdout.write(line)
            captured.append(line)
        process.stdout.close()
        if process.wait() != 0:
            raise Exception("{} failed:\n{}".format(action, "".join(captured).strip()))

    def _build(self):
        arguments = Project._parse_arguments()
        jobs = arguments.jobs
        if jobs is None or jobs <= 0:
            jobs = multiprocessing.cpu_count()
        environment = os.environ.copy()
        environment["CMAKE_BUILD_PARALLEL_LEVEL"] = str(jobs)
        build_dir = self.output.makefile.d["BUILD_SUBDIR"]
        cache = os.path.join(self.directory, build_dir, "CMakeCache.txt")
        stamp = os.path.join(self.directory, build_dir, ".configured")

        if arguments.clean_first:
            log.info("Clean up project in %s", os.path.abspath(self.directory))
            try:
                if os.path.exists(cache):
                    self._run_cmake(
                        ["cmake", "--build", build_dir, "--target", "clean"],
                        environment,
                        "CMake clean",
                    )
            except Exception as error:
                log.debug(
                    "Ignore failing CMake clean in %s: %s",
                    os.path.abspath(self.directory),
                    error,
                )
            self.is_built = False
            log.info("Clean complete")

        if self.is_built:
            log.info("Skip build because project is already marked as built")
            return
        log.info(
            "Start to compile in %s with concurrency level %d",
            os.path.abspath(self.directory),
            jobs,
        )
        try:
            makefile_dictionary = self.output.makefile.dictionary()
            configure = ["cmake", "-S", ".", "-B", build_dir]
            for key, flag in (
                ("CMAKE_GENERATOR", "-G"),
                ("CMAKE_GENERATOR_PLATFORM", "-A"),
                ("CMAKE_GENERATOR_TOOLSET", "-T"),
            ):
                if makefile_dictionary.get(key):
                    configure += [flag, makefile_dictionary[key]]

            configure_required = not os.path.exists(cache) or not os.path.exists(stamp)
            if not configure_required:
                stamp_time = os.path.getmtime(stamp)
                for dependency in ("CMakeLists.txt", "Makefile"):
                    path = os.path.join(self.directory, dependency)
                    if os.path.exists(path) and os.path.getmtime(path) > stamp_time:
                        configure_required = True
            if configure_required:
                self._run_cmake(configure, environment, "CMake configure")
                with open(stamp, "a"):
                    os.utime(stamp, None)

            self._run_cmake(
                ["cmake", "--build", build_dir, "--parallel", str(jobs)],
                environment,
                "CMake build",
            )
            log.info("Compile completed successfully")
            self.is_built = True
            self.build_was_successful = True
        except Exception as error:
            self.is_built = False
            self.build_was_successful = False
            log.error("%s", error)
            log.error("Compile was not successful")
            raise

    def generate_and_build(self):
        """Generates the project and compiles it unless the command line says generate only."""
        log.info(
            "Generate project %s in %s",
            self.project_name,
            os.path.abspath(self.directory),
        )
        self.is_built = False
        arguments = Project._parse_arguments()
        self._generate()
        if not arguments.generate_only:
            self._build()

    def run(
        self, executable, args=(), prefix=None, pipefile=None, rebuild_if_required=True
    ):
        """Runs the executable in the project directory with live terminal output.

        ``prefix`` supplies launcher arguments, such as an MPI command.
        Returns True on a zero exit code.
        """
        if rebuild_if_required and not (self.is_built and self.build_was_successful):
            self.generate_and_build()
        if rebuild_if_required and not (self.is_built and self.build_was_successful):
            log.error("Cannot run as code compilation has not been successful")
            return False

        invocation = list(prefix) if prefix else []
        invocation += ["./" + executable] + list(args)
        log.info("Run %s", " ".join(invocation))

        sink = None
        try:
            if pipefile:
                sink = open(pipefile, "w")
            with subprocess.Popen(
                invocation,
                cwd=self.directory,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            ) as process:
                for line in process.stdout:
                    sys.stdout.write(line)
                    if sink:
                        sink.write(line)
            if process.returncode != 0:
                log.error("Run failed with exit code %s", process.returncode)
                return False
            log.info(
                "Run completed without error code, but check for warnings and numerical assertions/errors"
            )
            return True
        except Exception as error:
            log.error("Run of application was not successful: %s", error)
            log.error("Invocation: %s", invocation)
            return False
        finally:
            if sink:
                sink.close()
