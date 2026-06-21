# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys
import subprocess
import multiprocessing

import peano4.output
import peano4.datamodel
import peano4.solversteps

from .PeanoMain import PeanoMain
from .Logger import log, setup_logging

class Project (object):
  """!
  Represents a Peano 4 project.

  namespace Sequence of strings representing the (nested) namespace. Pass in
    [ "tests", "exahype2" ] for example if you wanna write a solver that
    is embedded into the namespace tests::exahype2.


  ## Global project properties

  If you want to alter some global constants, you should manipulate
  the constants attribute. It will eventually feed into the generated
  file Constants.h. See peano4.output.Constants for further info.
  """

  def __init__(self, namespace, project_name, directory = ".", executable = ""):
    setup_logging()

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

    #
    # Create default output model, i.e. those parts that have to be there
    # always
    #
    self.output       = peano4.output.Output()

    #
    # Empty model by default
    #
    self.datamodel    = peano4.datamodel.Model(self.namespace)

    self.solversteps  = peano4.solversteps.Steps(self)

    self.main         = PeanoMain(self)

    self.is_generated         = False
    self.is_built             = False
    self.build_was_successful = False
    self._build_make_override = None
    self._build_clean_first_override = None
    self._build_jobs_override = None

    self.constants  = peano4.output.Constants(self)

    if executable:
      self.output.makefile.set_executable_name(executable)

    self.is_extended = False


  def __str__(self):
    return "(#steps=" + str(self.solversteps) + ",model=" + str(self.datamodel) + ")"

  def cleanup(self):
    """!
      This routine has to be called after you've generated your code.
    """
    self.output.clear()
    self.datamodel.clear()
    self.solversteps.clear()

    self.main         = PeanoMain(self)

    self.is_generated = False
    self.is_built     = False

    self.constants  = peano4.output.Constants(self)

  def generate_and_build(self):
    """!
    Generate the application's source files and, unless code generation only is
    requested, configure and (incrementally) compile it via CMake.

    All behaviour is controlled through the project-internal command-line
    arguments (see exahype2.Project):

    - --generate-only / --no-build : generate the sources but skip compilation
    - -j / --jobs N                : build parallelism (default: all cores)
    - --clean-first                : wipe CMake's incremental state first
    - -v / --verbose               : enable debug logging

    The CMake/compiler output is streamed to stdout as the build progresses.

    Reset the output (via cleanup()) before changing parameters and calling this
    again; otherwise two subsequent calls enrich the output twice.
    """
    log.info(
      "Generate project %s in %s",
      self.project_name,
      os.path.abspath(self.directory),
    )
    self.is_generated = True
    self.is_built = False
    if len(self.output.artefacts)>0:
      log.debug(
        "Project already holds %d pre-registered artefacts before generation",
        len(self.output.artefacts),
      )

    self.output.readme.add_package_description( self.constants.readme_entry() )

    self.datamodel.construct_output(self.output)
    self.solversteps.construct_output(self.output)

    self.main.construct_output(self.output,self.project_name + "-main")
    self.output.generate(self.directory)
    self.constants.generate(self.directory, self.output.makefile.protected_files())

    log.info("Generation complete (%d artefacts)", len(self.output.artefacts))

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
      cmake_build_dir = "build." + self.output.makefile.executable_name()
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
          self.is_built = True
          self.build_was_successful = False
          log.error("%s", e)
          log.error("Compile was not successful")
          sys.exit(1)
      else:
        log.info("Skip build because project is already marked as built")

  def run(self, executable, args=[], prefix=None, pipefile=None, rebuild_if_required=True):
    """!
    Runs the code. args should be a list of strings or the empty list.
    prefix is an array, too. A typical invocation looks alike
    project.run( ["16.0", "32.0"], ["/opt/mpi/mpirun", "-n", "1"] )
    The operation returns True if the run had been successful

    pipefile: string or None
    """
    success = False
    if rebuild_if_required and not self.is_built and not self.build_was_successful:
      self.generate_and_build()

    if not rebuild_if_required or (self.is_built and self.build_was_successful):
      log.info("Run executable %s", executable)

      invocation  = []
      if prefix!=None:
        invocation += prefix
      invocation += ["./" + executable]
      invocation += args

      try:
        result = None
        if pipefile==None:
          result = subprocess.run( invocation, stdout=subprocess.PIPE, stderr=subprocess.PIPE )
        else:
          result = subprocess.run( invocation, stdout=open( pipefile, "w" ), stderr=subprocess.PIPE )
        if result.returncode==0:
          if pipefile==None:
            stdout = result.stdout.decode("utf-8")
            if stdout:
              log.info("%s", stdout)
          if result.stderr!=None:
            stderr = result.stderr.decode("utf-8")
            if stderr:
              log.warning("%s", stderr)
          log.info("Run completed without error code, but check for warnings and numerical assertions/errors")
          success = True
        else:
          log.error("%s", result.stderr.decode("utf-8"))
          log.error("Run failed")
      except Exception as e:
        log.error("Run of application was not successful: %s", e)
        log.error("Invocation: %s", invocation)
    else:
      log.error("Cannot run as code compilation has not been successful")
    return success
