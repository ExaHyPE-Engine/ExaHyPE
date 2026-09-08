# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
import json
import logging
import os
import re
import sys
import subprocess
import multiprocessing

import peano4.output
import peano4.datamodel
import peano4.solversteps
import peano4.plotter

from .PeanoMain import PeanoMain
from .Logger import log, setup_logging


class Project (object):
  """!
  Represents a Peano 4 project.

  namespace Sequence of strings representing the (nested) namespace. Pass in
    [ "tests", "mysolver" ] for example if you want to write a solver that
    is embedded into the namespace tests::mysolver.

  ## Global project properties

  If you want to alter some global constants, you should manipulate
  the constants attribute. It will eventually feed into the generated
  file Constants.h. See peano4.output.Constants for further info.
  """

  def __init__(self, namespace, project_name, executable = "",
               dimensions = None, offset = None, size = None, periodic_bc = None,
               grid_levels = None, build_mode = None, directory = "."):
    arguments = Project._parse_project_arguments()
    setup_logging(level=logging.DEBUG if arguments.verbose else logging.INFO)

    if arguments.dimensions is not None:
        dimensions = arguments.dimensions
    if arguments.build_mode is not None:
        build_mode = peano4.output.string_to_mode(arguments.build_mode)
    if dimensions is not None:
        if offset is not None:
            offset = list(offset)[0:dimensions]
        if size is not None:
            size = list(size)[0:dimensions]
        if periodic_bc is not None:
            periodic_bc = list(periodic_bc)[0:dimensions]

    if sys.version_info < (3, 11):
        log.warning("Python 3.11 or higher is recommended to support all features. Current version: %s", sys.version)

    self.rootnamespace = namespace.copy()
    self.namespace = namespace
    self.namespaces = []
    self.project_name = project_name
    self.directory = directory
    if self.namespace:
      if not self.namespace in self.namespaces:
        self.namespaces.append(self.namespace)

    self.output       = peano4.output.Output()
    self.datamodel    = peano4.datamodel.Model(self.namespace)

    self.solversteps  = peano4.solversteps.Steps(self)
    self._create_steps()

    self.main         = PeanoMain(self)
    self._domain_offset = list(offset) if offset is not None else None
    self._domain_size = list(size) if size is not None else None
    if periodic_bc is not None:
      self._periodic_bc = list(periodic_bc)
    elif offset is not None:
      self._periodic_bc = [False for _ in offset]
    else:
      self._periodic_bc = None
    self._default_grid_levels = grid_levels
    self._dimensions = dimensions
    self._build_mode = build_mode
    self._makefile_configured = False
    self._runtime_parameters = []

    self.is_built             = False
    self.build_was_successful = False
    self._build_make_override = None
    self._build_clean_first_override = None
    self._build_jobs_override = None

    self.constants  = peano4.output.Constants(self)

    if executable:
      self.output.makefile.set_executable_name(executable)

    self.is_extended = False

  _project_arguments = None

  @staticmethod
  def _parse_project_arguments():
    """Parse (once) the standard project code-generation command-line arguments.

    Every project uses these arguments, and subclasses inherit them. Uses
    parse_known_args() so a script may still read its own extra arguments, and -h
    prints this help and exits.
    """
    if Project._project_arguments is None:
      parser = argparse.ArgumentParser(
        allow_abbrev=False,
        description="Peano project code generation options.",
      )
      parser.add_argument("-d", "--dimensions", type=int, choices=(2, 3), default=None, help="Code generation dimensions; overrides the value set by the application.")
      parser.add_argument("-m", "--build-mode", choices=peano4.output.CompileModes, default=None, help="Build mode; overrides the application default (usually Release).")
      parser.add_argument("--generate-only", "--no-build", action="store_true", default=False, help="Generate source files but skip compilation.")
      parser.add_argument("-j", "--jobs", type=int, default=None, help="Parallel build jobs. Values <= 0 use the detected CPU count.")
      parser.add_argument("--clean-first", action=argparse.BooleanOptionalAction, default=None, help="Wipe CMake's incremental build state before compiling.")
      parser.add_argument("-v", "--verbose", action="store_true", default=False, help="Enable verbose (debug) logging during code generation.")
      parser.add_argument("--plot-format", choices=peano4.plotter.SUPPORTED_FORMATS, default=None, help="Output file format for all solver plotters; defaults to vtkhdf unless the application selects another.")
      Project._project_arguments, _ = parser.parse_known_args()
    return Project._project_arguments

  def _configure_build_from_project_arguments(self):
    project_arguments = Project._parse_project_arguments()
    self._build_make_override = False if project_arguments.generate_only else None
    self._build_jobs_override = project_arguments.jobs
    self._build_clean_first_override = project_arguments.clean_first

  def _configure_plotting_from_project_arguments(self):
    """Apply the standard --plot-format command-line option to the plotter default.

    An unset option (None) leaves the project/plotter default in place; an
    explicit one overrides it. Applied before generation so the lazily-resolved
    plotters pick up the chosen format.
    """
    project_arguments = Project._parse_project_arguments()
    if project_arguments.plot_format is not None:
      peano4.plotter.set_default_format(project_arguments.plot_format)

  @property
  def dimensions(self):
    return self._dimensions

  def __str__(self):
    return "(#steps=" + str(self.solversteps) + ",model=" + str(self.datamodel) + ")"

  def cleanup(self):
    self.output.clear()
    self.datamodel.clear()
    self.solversteps.clear()
    self._create_steps()

    self.main         = PeanoMain(self)
    self._domain_offset = None
    self._domain_size = None
    self._periodic_bc = None
    self._default_grid_levels = None
    self._runtime_parameters = []

    self.is_built     = False

    self.constants  = peano4.output.Constants(self)

  def _create_steps(self):
    self.create_grid = peano4.solversteps.Step("CreateGrid", False)
    self.solversteps.add_step(self.create_grid)

  def add_runtime_parameter(self, name, parameter_type, default_value):
    """Register a runtime parameter, read at start-up from config.json and the command line."""
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
      raise ValueError(f"Invalid runtime parameter name '{name}'. Use a valid C++ identifier.")

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
      if not isinstance(array_size, int) or array_size <= 0:
        raise ValueError(f"Array runtime parameter '{name}' expects a positive integer size.")
      if not isinstance(default_value, (list, tuple)) or len(default_value) != array_size:
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

    accessor_name = "get" + name[0].upper() + name[1:]
    scalar_cpp_type = {
      "double": "double",
      "int": "int",
      "bool": "bool",
      "string": "std::string",
    }[base_type]
    if is_array:
      cpp_type = f"tarch::la::Vector<{array_size}, {scalar_cpp_type}>"
      literal = "{" + ", ".join(
        (
          repr(float(value))
          if base_type == "double"
          else str(value)
          if base_type == "int"
          else "true" if value else "false"
        )
        for value in default_value
      ) + "}"
      getter = {
        "double": "getDoubleArray",
        "int": "getIntegerArray",
        "bool": "getBooleanArray",
      }[base_type]
      return_type = f"tarch::la::Vector<{array_size}, {scalar_cpp_type}>"
    else:
      if base_type == "double" and not isinstance(default_value, (int, float)):
        raise ValueError(f"Runtime parameter '{name}' expects a numeric default value.")
      if base_type == "int" and not isinstance(default_value, int):
        raise ValueError(f"Runtime parameter '{name}' expects an integer default value.")
      if base_type == "bool" and not isinstance(default_value, bool):
        raise ValueError(f"Runtime parameter '{name}' expects a boolean default value.")
      if base_type == "string" and not isinstance(default_value, str):
        raise ValueError(f"Runtime parameter '{name}' expects a string default value.")

      cpp_type = scalar_cpp_type
      if base_type == "double":
        literal = repr(float(default_value))
        getter = "getDouble"
      elif base_type == "int":
        literal = str(default_value)
        getter = "getInteger"
      elif base_type == "bool":
        literal = "true" if default_value else "false"
        getter = "getBoolean"
      else:
        literal = json.dumps(default_value)
        getter = "getString"
      return_type = cpp_type

    # Replace-by-name so that the generator can run several times on one
    # project without duplicating the parameter in the generated
    # RuntimeParameters class.
    self._runtime_parameters = [
      parameter for parameter in self._runtime_parameters if parameter["name"] != name
    ]
    self._runtime_parameters.append(
      {
        "name": name,
        "accessor_name": accessor_name,
        "cpp_type": cpp_type,
        "return_type": return_type,
        "is_array": is_array,
        "array_size": array_size,
        "base_type": base_type,
        "scalar_cpp_type": scalar_cpp_type,
        "default_value": literal,
        "getter": getter,
      }
    )

  def _generate_runtime_parameters(self):
    template_directory = os.path.dirname(os.path.realpath(__file__))
    dictionary = {
      "RUNTIME_PARAMETERS": self._runtime_parameters,
      "HAS_RUNTIME_PARAMETERS": len(self._runtime_parameters) > 0,
      "HAS_ARRAY_RUNTIME_PARAMETERS": any(
        parameter["is_array"] for parameter in self._runtime_parameters
      ),
      "HAS_DOUBLE_RUNTIME_PARAMETERS": any(
        parameter["base_type"] == "double" for parameter in self._runtime_parameters
      ),
      "HAS_INTEGER_RUNTIME_PARAMETERS": any(
        parameter["base_type"] == "int" for parameter in self._runtime_parameters
      ),
      "HAS_BOOLEAN_RUNTIME_PARAMETERS": any(
        parameter["base_type"] == "bool" for parameter in self._runtime_parameters
      ),
      "HAS_STRING_RUNTIME_PARAMETERS": any(
        parameter["base_type"] == "string" for parameter in self._runtime_parameters
      ),
    }
    generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      os.path.join(template_directory, "RuntimeParameters.template.h"),
      os.path.join(template_directory, "RuntimeParameters.template.cpp"),
      "RuntimeParameters",
      self.namespace,
      "",
      dictionary,
    )
    self.output.add(generated_files)
    self.output.makefile.add_h_file("RuntimeParameters.h", generated=True)
    self.output.makefile.add_cpp_file("RuntimeParameters.cpp", generated=True)

  def generate_and_build(self):
    log.info(
      "Generate project %s in %s",
      self.project_name,
      os.path.abspath(self.directory),
    )
    self.is_built = False

    self._configure_build_from_project_arguments()
    self._configure_plotting_from_project_arguments()

    # A subclass may have parsed the build configuration during code generation,
    # so the flag keeps this call from parsing it a second time.
    if self._dimensions is not None:
      if not self._makefile_configured:
        self.output.makefile.parse_configuration_outcome()
      self.output.makefile.set_dimension(self._dimensions)
      if self._build_mode is not None:
        self.output.makefile.set_mode(self._build_mode)
      self._makefile_configured = True

    if len(self.output.artefacts)>0:
      log.debug(
        "Project already holds %d pre-registered artefacts before generation",
        len(self.output.artefacts),
      )

    # The construct_output() calls below append artefacts anew on every call, so
    # the pre-registered artefacts are stored here and restored afterwards.
    pre_generation_artefacts = list(self.output.artefacts)
    pre_generation_readme_entries = list(self.output.readme._entries)

    try:
      self.output.readme.add_package_description( self.constants.readme_entry() )

      self.datamodel.construct_output(self.output)
      self.solversteps.construct_output(self.output)
      self._generate_runtime_parameters()

      self.main.construct_output(self.output,self.project_name + "-main")

      generated_subdir = self.project_name + ".generated"
      self.output.makefile.d["BUILD_SUBDIR"] = self.project_name + ".build"

      self.output.generate(self.directory, generated_subdir)
      self.constants.generate(
        os.path.join(self.directory, generated_subdir), self.output.makefile.protected_files()
      )

      log.info("Generation complete (%d artefacts)", len(self.output.artefacts))
    finally:
      self.output.artefacts = pre_generation_artefacts
      self.output.readme._entries = pre_generation_readme_entries

    make = True if self._build_make_override is None else self._build_make_override
    make_clean_first = bool(self._build_clean_first_override)
    number_of_parallel_builds = self._build_jobs_override
    if number_of_parallel_builds is None or number_of_parallel_builds <= 0:
        number_of_parallel_builds = multiprocessing.cpu_count()

    def run_cmake_command(command, build_environment, action):
      # Stream the CMake/compiler output to stdout as it is produced while also
      # capturing it, so it can be embedded in the exception if the command fails.
      process = subprocess.Popen(
        command,
        cwd=self.directory,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env=build_environment,
        text=True,
      )
      captured = []
      for line in process.stdout:
        sys.stdout.write(line)
        captured.append(line)
      process.stdout.close()
      if process.wait() != 0:
        raise Exception("{} failed:\n{}".format(action, "".join(captured).strip()))

    if make:
      build_environment = os.environ.copy()
      build_environment["CMAKE_BUILD_PARALLEL_LEVEL"] = str(number_of_parallel_builds)
      cmake_build_dir = self.output.makefile.d["BUILD_SUBDIR"]
      cmake_build_cache = os.path.join(self.directory, cmake_build_dir, "CMakeCache.txt")
      cmake_configure_stamp = os.path.join(self.directory, cmake_build_dir, ".configured")

      if make_clean_first:
        log.info("Clean up project in %s", os.path.abspath(self.directory))
        try:
          if os.path.exists(cmake_build_cache):
            run_cmake_command(
              ["cmake", "--build", cmake_build_dir, "--target", "clean"],
              build_environment,
              "CMake clean",
            )
        except Exception as e:
          log.debug("Ignore failing CMake clean in %s: %s", os.path.abspath(self.directory), e)
        self.is_built = False
        log.info("Clean complete")

      if not self.is_built:
        log.info("Start to compile in %s with concurrency level %d", os.path.abspath(self.directory), number_of_parallel_builds)
        try:
          makefile_dictionary = self.output.makefile.dictionary()
          configure_command = ["cmake", "-S", ".", "-B", cmake_build_dir]
          if makefile_dictionary.get("CMAKE_GENERATOR"):
            configure_command += ["-G", makefile_dictionary["CMAKE_GENERATOR"]]
          if makefile_dictionary.get("CMAKE_GENERATOR_PLATFORM"):
            configure_command += ["-A", makefile_dictionary["CMAKE_GENERATOR_PLATFORM"]]
          if makefile_dictionary.get("CMAKE_GENERATOR_TOOLSET"):
            configure_command += ["-T", makefile_dictionary["CMAKE_GENERATOR_TOOLSET"]]

          configure_required = not os.path.exists(cmake_build_cache) or not os.path.exists(cmake_configure_stamp)
          if not configure_required:
            configure_stamp_time = os.path.getmtime(cmake_configure_stamp)
            for dependency in ("CMakeLists.txt", "Makefile"):
              dependency_path = os.path.join(self.directory, dependency)
              if os.path.exists(dependency_path) and os.path.getmtime(dependency_path) > configure_stamp_time:
                configure_required = True

          if configure_required:
            run_cmake_command(configure_command, build_environment, "CMake configure")
            with open(cmake_configure_stamp, "a"):
              os.utime(cmake_configure_stamp, None)

          run_cmake_command(
            ["cmake", "--build", cmake_build_dir, "--parallel", str(number_of_parallel_builds)],
            build_environment,
            "CMake build",
          )
          log.info("Compile completed successfully")
          self.is_built = True
          self.build_was_successful = True
        except Exception as e:
          self.is_built = False
          self.build_was_successful = False
          log.error("%s", e)
          log.error("Compile was not successful")
          raise
      else:
        log.info("Skip build because project is already marked as built")

  def run(self, executable, args=[], prefix=None, pipefile=None, rebuild_if_required=True):
    """!
    Run the built executable with live terminal output.

    args should be a list of strings or the empty list, prefix an optional
    launcher such as ["mpirun", "-n", "2"]. Every output line is echoed to the
    terminal as it appears, so long runs stay observable; if pipefile is set,
    the output is written there as well. Returns True on a zero exit code.
    """
    if rebuild_if_required and not self.is_built and not self.build_was_successful:
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
        invocation, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
      ) as process:
        for line in process.stdout:
          sys.stdout.write(line)
          if sink:
            sink.write(line)
      if process.returncode != 0:
        log.error("Run failed with exit code %s", process.returncode)
        return False
      log.info("Run completed without error code, but check for warnings and numerical assertions/errors")
      return True
    except Exception as e:
      log.error("Run of application was not successful: %s", e)
      log.error("Invocation: %s", invocation)
      return False
    finally:
      if sink:
        sink.close()
