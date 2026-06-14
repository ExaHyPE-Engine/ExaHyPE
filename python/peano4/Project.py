# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import gc
import os
import re
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

  def __init__(self, namespace, project_name, directory = ".", subdirectory = "", executable = ""):
    setup_logging()

    if sys.version_info < (3, 11):
        log.warning("Python 3.11 or higher is recommended to support all features. Current version: %s", sys.version)

    if subdirectory and not os.path.exists(subdirectory):
      os.mkdir(subdirectory)

    self.rootnamespace = namespace.copy()
    self.namespace = namespace
    self.namespaces = []
    self.subdirectories = []
    self.project_name = project_name
    self.directory = directory
    self._subdirectory = subdirectory
    if subdirectory:
      self.namespace += [ subdirectory ]
    if self.namespace:
      if not self.namespace in self.namespaces:
        self.namespaces.append(self.namespace)
      if not self.subdirectory in self.subdirectories:
        self.subdirectories.append(self.subdirectory)

    #
    # Create default output model, i.e. those parts that have to be there
    # always
    #
    self.output       = peano4.output.Output()

    #
    # Empty model by default
    #
    self.datamodel    = peano4.datamodel.Model(self.namespace, self._subdirectory)

    self.solversteps  = peano4.solversteps.Steps(self)

    self.main         = PeanoMain(self)

    self.is_generated         = False
    self.is_built             = False
    self.build_was_successful = False
    self._build_make_override = None
    self._build_clean_first_override = None
    self._build_jobs_override = None

    self.constants  = peano4.output.Constants(self)
    self.subconstants = []

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
    self.subconstants = []

  def generate(self,
               overwrite=peano4.output.Overwrite.Default,
               throw_away_data_after_generation=False):
    """!
    Generate all code. If you add stuff to your project after a
    build, you have to (re-)generate the code. If you compile only
    once, you don't have to invoke this routine explicitly. It is
    lazily called by the other project routines - the latest before
    you run the code.

    It is important that I reset the output after each generate
    call before you change parameter settings and call generate
    again. To do so, invoke cleanup(). If you forget this, two
    subsequent generate calls enrich the output twice.

    throw_away_data_after_generation: Bool
      The Peano 4 memory footprint can become quite substantial
      effectively reducing the translation capabilities (as compilers
      tend to require a lot of memory, too). So pass in True if you
      want the script to throw away the data structures (and run a
      garbage collection) after all files have been generated. Please
      note that it invalidates both this object (and maybe another
      object that you've used to generate the present one - such as
      ExaHyPE). It really strips everything down to the stuff you
      absolutely need to translate and run the code.
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
    self.output.generate(overwrite, self.directory, self.subdirectories)
    if self.subconstants:
      for i in range(len(self.subconstants)):
        self.subconstants[i].generate(overwrite, self.directory, self.subdirectories[i])
    else:
      self.constants.generate(overwrite, self.directory)

    log.info("Generation complete (%d artefacts)", len(self.output.artefacts))

    if throw_away_data_after_generation:
      self.datamodel = None
      self.solversteps = None
      self.output      = None
      gc.collect()
      log.info("Threw away all data and ran garbage collection")

  def build(self,
            make=True,
            make_clean_first=True,
            throw_away_data_after_build=False,
            number_of_parallel_builds=-1):
    """!
      Invokes the underlying make build mechanism on the project.

      number_of_parallel_builds: int
        This is mapped onto make -jnumber_of_parallel_builds, i.e., it
        determines how many parallel make instances the code spawns.
        Usually, a lot of the generated code is quite lengthy. Therefore
        compile time can be high. If you pass something smaller or equal
        to zero, it will use the core count as guideline how to compile.

      throw_away_data_after_build: Bool
        see generate()
    """
    if self._build_make_override is not None:
      make = self._build_make_override
    if self._build_clean_first_override is not None:
      make_clean_first = self._build_clean_first_override
    if self._build_jobs_override is not None:
      number_of_parallel_builds = self._build_jobs_override

    if number_of_parallel_builds <= 0:
        number_of_parallel_builds = multiprocessing.cpu_count()

    if not self.is_generated:
      self.generate()

    if make:
      if make_clean_first:
        log.info("Clean up project in %s", os.path.abspath(self.directory))
        try:
          subprocess.check_call(["make", "clean"], cwd=self.directory, stderr=subprocess.DEVNULL)
        except Exception as e:
          log.debug("Ignore failing make clean in %s: %s", os.path.abspath(self.directory), e)
        self.is_built = False
        log.info("Clean complete")

      if not self.is_built:
        log.info("Start to compile in %s with concurrency level %d", os.path.abspath(self.directory), number_of_parallel_builds)
        try:
          process = subprocess.run(["make", "-j"+str(number_of_parallel_builds)], cwd=self.directory, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
          # Extract warnings and errors from error message
          error_message = process.stderr.decode().strip()
          warnings = re.findall(r"(?i)Warning:.*", error_message)
          errors = re.findall(r"(?i)Error:.*", error_message)
          # Print warnings into the terminal
          # if warnings:
          #   log.warning("\n".join(warnings))
          if errors:
            error_message = "\n".join(errors)
          # Check the return code for linker errors
          # Raise exception with errors
          if process.returncode != 0:
            linker_error_message = process.stderr.decode().strip() + "\n" + error_message
            raise Exception(linker_error_message)
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

    if throw_away_data_after_build:
      self.cleanup()
      self.datamodel    = None
      self.solversteps  = None
      self.output       = None
      gc.collect()
      log.info("Threw away all data and ran garbage collection")

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
      self.build()

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

  # TODO: Remove
  @property
  def subdirectory(self):
      subdirectory = self._subdirectory
      if subdirectory:
          subdirectory += "/"
      return subdirectory
