# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import re

from copy import deepcopy
from enum import Enum

from exahype2.log import log

from .helper import get_jinja2_template


class Makefile(object):
    """Represents the created Makefile of a Peano 4 project"""

    class Mode(Enum):
        """The build type of a generated project; the member names are the command-line words."""

        Release = 0
        Asserts = 1
        Debug = 2

    def protected_files(self):
        """
        Returns the set of normalised relative paths the user registered as non-generated
        source files (via add_h_file/add_cpp_file/add_fortran_file).
        """
        return set(
            os.path.normpath(filename)
            for filename in (self.hfiles + self.cppfiles + self.fortranfiles)
        )

    def __init__(self):
        self.clear()

    def executable_name(self):
        return self.d["EXECUTABLENAME"]

    def dictionary(self):
        return deepcopy(self.d)

    def clear(self):
        self.d = {}
        self.d["COMPILE_DEFINITIONS"] = []
        self.d["COMPILE_OPTIONS"] = []
        self.d["INCLUDE_DIRECTORIES"] = []
        self.d["LINK_LIBRARIES"] = []
        self.d["LINK_OPTIONS"] = []
        self.d["DIMENSIONS"] = "2"
        self.d["EXECUTABLENAME"] = ""
        self.d["GPU"] = "no"
        self.d["CMAKE_BUILD_DIR"] = ""
        self.d["CMAKE_GENERATOR"] = ""
        self.d["CMAKE_GENERATOR_PLATFORM"] = ""
        self.d["CMAKE_GENERATOR_TOOLSET"] = ""
        self.d["CMAKE_C_COMPILER"] = ""
        self.d["CMAKE_CXX_COMPILER"] = ""
        self.d["CMAKE_FORTRAN_COMPILER"] = ""
        self.set_mode(Makefile.Mode.Release)
        self.available_plot_formats = {"vtu"}
        self.clear_files()

    def _append_unique(self, key, value, force=False):
        if value in self.d[key] and not force:
            log.warning(
                "%s %s is already present. Ignored as force attribute is not set",
                key,
                value,
            )
            return
        self.d[key].append(value)

    def _unique(self, items):
        result = []
        for item in items:
            if item not in result:
                result.append(item)
        return result

    def set_executable_name(self, fname):
        self.d["EXECUTABLENAME"] = fname

    def clear_files(self):
        self.generated_hfiles = []
        self.generated_cppfiles = []
        self.generated_fortranfiles = []
        self.generated_gpu_cppfiles = []
        self.clear_user_files()

    def clear_user_files(self):
        """Clears user source registrations when the solver set is replaced."""
        self.hfiles = []
        self.cppfiles = []
        self.fortranfiles = []
        self.generated_gpu_cppfiles = [
            filename
            for filename in self.generated_gpu_cppfiles
            if filename in self.generated_cppfiles
        ]

    def set_dimension(self, dimension):
        self.d["DIMENSIONS"] = str(dimension)

    def set_mode(self, mode):
        """Sets the build type, a :class:`Makefile.Mode`."""
        if not isinstance(mode, Makefile.Mode):
            raise TypeError("The build mode is a Makefile.Mode, got {!r}.".format(mode))
        self.d["MODE"] = mode.name

    def add_compile_definition(self, name, value=None, force=False):
        if value is None:
            self._append_unique("COMPILE_DEFINITIONS", name, force)
        else:
            self._append_unique("COMPILE_DEFINITIONS", name + "=" + str(value), force)

    def add_compile_option(self, value, force=False):
        self._append_unique("COMPILE_OPTIONS", value, force)

    def add_include_directory(self, value, force=False):
        self._append_unique("INCLUDE_DIRECTORIES", value, force)

    def add_link_library(self, value, force=False):
        self._append_unique("LINK_LIBRARIES", value, force)

    def add_link_option(self, value, force=False):
        self._append_unique("LINK_OPTIONS", value, force)

    def parse_configuration_outcome(self):
        """
        Locates the Peano CMake package and reads the
        configuration needed by Python code generation.
        """
        if "PEANO_SRC_ROOT_DIR" in os.environ:
            search_start = os.environ["PEANO_SRC_ROOT_DIR"]
        else:
            search_start = os.getcwd()

        self.d["CMAKE_BUILD_DIR"] = ""

        # PEANO_CMAKE_BUILD_DIR is an alias for PEANO_BUILD_DIR;
        # the latter takes precedence when both are set.
        if "PEANO_BUILD_DIR" in os.environ:
            self.d["CMAKE_BUILD_DIR"] = os.environ["PEANO_BUILD_DIR"]
        elif "PEANO_CMAKE_BUILD_DIR" in os.environ:
            self.d["CMAKE_BUILD_DIR"] = os.environ["PEANO_CMAKE_BUILD_DIR"]

        if self.d["CMAKE_BUILD_DIR"] == "":
            curr = os.path.abspath(search_start)
            while True:
                if os.path.isfile(os.path.join(curr, "PeanoConfig.cmake")):
                    self.d["CMAKE_BUILD_DIR"] = curr
                    break
                try:
                    for candidate in os.listdir(curr):
                        path = os.path.join(curr, candidate)
                        if os.path.isdir(path) and os.path.isfile(
                            os.path.join(path, "PeanoConfig.cmake")
                        ):
                            self.d["CMAKE_BUILD_DIR"] = path
                            break
                except OSError:
                    pass
                if self.d["CMAKE_BUILD_DIR"] != "":
                    break
                parent = os.path.dirname(curr)
                if parent == curr:
                    break  # Reached filesystem root
                curr = parent

        if self.d["CMAKE_BUILD_DIR"] != "":
            peano_config_file = os.path.join(
                self.d["CMAKE_BUILD_DIR"], "PeanoConfig.cmake"
            )
            if not os.path.isfile(peano_config_file):
                raise Exception(
                    "Cannot locate the PeanoConfig.cmake file: " + peano_config_file
                )

            with open(peano_config_file, "r") as input:
                for line in input:
                    # Quoted CMake values can contain spaces and closing parentheses.
                    match = re.match(
                        r'\s*set\(\s*(PEANO_[A-Za-z0-9_]+)\s+(?:"(.*)"|([^)"\s]+))\s*\)',
                        line,
                    )
                    if match:
                        key = match.group(1)
                        value = (
                            match.group(2)
                            if match.group(2) is not None
                            else match.group(3)
                        )
                        if key == "PEANO_WITH_GPU_BACKEND":
                            self.d["GPU"] = value
                        elif key == "PEANO_CMAKE_GENERATOR":
                            self.d["CMAKE_GENERATOR"] = value
                        elif key == "PEANO_CMAKE_GENERATOR_PLATFORM":
                            self.d["CMAKE_GENERATOR_PLATFORM"] = value
                        elif key == "PEANO_CMAKE_GENERATOR_TOOLSET":
                            self.d["CMAKE_GENERATOR_TOOLSET"] = value
                        elif key == "PEANO_CMAKE_C_COMPILER":
                            self.d["CMAKE_C_COMPILER"] = value
                        elif key == "PEANO_CMAKE_CXX_COMPILER":
                            self.d["CMAKE_CXX_COMPILER"] = value
                        elif key == "PEANO_CMAKE_FORTRAN_COMPILER":
                            self.d["CMAKE_FORTRAN_COMPILER"] = value
                        elif key == "PEANO_WITH_HDF5":
                            self.d["WITH_HDF5"] = value
                        elif key == "PEANO_WITH_NETCDF":
                            self.d["WITH_NETCDF"] = value

            # VTU needs no library; HDF5 and NetCDF formats need the library the build found.
            self.available_plot_formats = {"vtu"}
            for plot_format, config_key in (
                ("hdf5", "WITH_HDF5"),
                ("netcdf", "WITH_NETCDF"),
                ("vtkhdf", "WITH_HDF5"),
            ):
                value = self.d.get(config_key)
                if value is None or value.upper() != "OFF":
                    self.available_plot_formats.add(plot_format)

        else:
            raise Exception(
                """
  Error: parse_configuration_outcome() could not locate a valid CMake build directory
  (containing PeanoConfig.cmake) starting from:
  """
                + search_start
                + """
  and searching upwards through its parents.

  Run CMake first, or set PEANO_BUILD_DIR (or its alias PEANO_CMAKE_BUILD_DIR) explicitly.
  """
            )

    def add_h_file(self, filename, generated=False):
        if generated:
            if (
                self.generated_hfiles.count(filename) == 0
                and self.hfiles.count(filename) == 0
            ):
                self.generated_hfiles.append(filename)
        else:
            if self.hfiles.count(filename) == 0:
                self.hfiles.append(filename)
                if self.generated_hfiles.count(filename) > 0:
                    self.generated_hfiles.remove(filename)

    def remove_h_file(self, filename):
        if filename in self.generated_hfiles:
            self.generated_hfiles.remove(filename)
        if filename in self.hfiles:
            self.hfiles.remove(filename)

    def add_cpp_file(self, filename, generated=False, is_gpu_kernel=False):
        if generated:
            # Non-generated file takes precedence over generated files.
            if (
                self.generated_cppfiles.count(filename) == 0
                and self.cppfiles.count(filename) == 0
            ):
                self.generated_cppfiles.append(filename)
        else:
            if self.cppfiles.count(filename) == 0:
                self.cppfiles.append(filename)
                # A translation unit listed twice defines every symbol twice.
                if self.generated_cppfiles.count(filename) > 0:
                    self.generated_cppfiles.remove(filename)

        if is_gpu_kernel and self.generated_gpu_cppfiles.count(filename) == 0:
            self.generated_gpu_cppfiles.append(filename)

    def remove_cpp_file(self, filename):
        if filename in self.generated_cppfiles:
            self.generated_cppfiles.remove(filename)
        if filename in self.cppfiles:
            self.cppfiles.remove(filename)
        if filename in self.generated_gpu_cppfiles:
            self.generated_gpu_cppfiles.remove(filename)

    def add_fortran_file(self, filename, generated=False):
        """
        Adds a new Fortran file.

        Fortran needs its files in translation order.
        """
        if generated:
            if (
                self.generated_fortranfiles.count(filename) == 0
                and self.fortranfiles.count(filename) == 0
            ):
                self.generated_fortranfiles.append(filename)
        else:
            if self.fortranfiles.count(filename) == 0:
                self.fortranfiles.append(filename)
                if self.generated_fortranfiles.count(filename) > 0:
                    self.generated_fortranfiles.remove(filename)

    def remove_fortran_file(self, filename):
        if filename in self.generated_fortranfiles:
            self.generated_fortranfiles.remove(filename)
        if filename in self.fortranfiles:
            self.fortranfiles.remove(filename)

    def generate(self, directory, generated_subdir=None):
        """
        The build files .gitignore, CMakeLists.txt and Makefile are always overwritten.

        :param generated_subdir: Subfolder (relative to directory) the generated
            C++ sources live in. None keeps everything flat.
        """
        prefix = generated_subdir + "/" if generated_subdir else ""
        self.d["GENERATED_SUBDIR"] = generated_subdir or ""
        self.d["H_HEADERS"] = self.hfiles
        self.d["CXX_SOURCES"] = self.cppfiles
        self.d["FORTRAN_SOURCES"] = self.fortranfiles
        self.d["GENERATED_H_HEADERS"] = [prefix + f for f in self.generated_hfiles]
        self.d["GENERATED_CXX_SOURCES"] = [prefix + f for f in self.generated_cppfiles]
        self.d["GENERATED_FORTRAN_SOURCES"] = [
            prefix + f for f in self.generated_fortranfiles
        ]
        self.d["GENERATED_GPU_SOURCES"] = [
            prefix + filename if filename in self.generated_cppfiles else filename
            for filename in self.generated_gpu_cppfiles
        ]
        self.d["GENERATED_COMPILE_DEFINITIONS"] = self._unique(
            self.d["COMPILE_DEFINITIONS"]
        )
        self.d["GENERATED_COMPILE_OPTIONS"] = self._unique(self.d["COMPILE_OPTIONS"])
        self.d["GENERATED_INCLUDE_DIRECTORIES"] = self._unique(
            ([generated_subdir] if generated_subdir else [])
            + self.d["INCLUDE_DIRECTORIES"]
        )
        self.d["GENERATED_LINK_LIBRARIES"] = self._unique(self.d["LINK_LIBRARIES"])
        self.d["GENERATED_LINK_OPTIONS"] = self._unique(self.d["LINK_OPTIONS"])

        def generate_output_file(input_file_path, output_file_path):
            log.debug("Write " + output_file_path)
            template = get_jinja2_template(input_file_path)
            with open(output_file_path, "w") as output:
                output.write(template.render(self.d))

        file_dir = os.path.dirname(os.path.realpath(__file__))

        with open(os.path.join(directory, ".gitignore"), "w") as gitignore:
            gitignore.write(
                self.executable_name() + "\n.gitignore\nCMakeLists.txt\nMakefile\n"
            )

        generate_output_file(
            os.path.join(file_dir, "CMakeLists.jinja2"),
            os.path.join(directory, "CMakeLists.txt"),
        )
        generate_output_file(
            os.path.join(file_dir, "Makefile.jinja2"),
            os.path.join(directory, "Makefile"),
        )
