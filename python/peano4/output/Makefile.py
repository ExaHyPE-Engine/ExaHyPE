# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import re

from copy import deepcopy

from .Helper import get_jinja2_template

from .CompileMode import CompileMode

from peano4.Logger import log


class Makefile(object):
    """Represents the created Makefile of a Peano 4 project"""

    def protected_files(self):
        """
        Set of normalised relative paths the user registered as non-generated
        source files (via add_h_file/add_cpp_file/add_Fortran_file). The code
        generator must never overwrite these.
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
        self.d["SIMDLEN"] = "1"
        self.d["GPU"] = "no"
        self.d["CMAKE_BUILD_DIR"] = ""
        self.d["CMAKE_GENERATOR"] = ""
        self.d["CMAKE_GENERATOR_PLATFORM"] = ""
        self.d["CMAKE_GENERATOR_TOOLSET"] = ""
        self.d["CMAKE_C_COMPILER"] = ""
        self.d["CMAKE_CXX_COMPILER"] = ""
        self.d["CMAKE_FORTRAN_COMPILER"] = ""
        self.set_mode(CompileMode.Release)
        self.clear_files()

    def _append_unique(self, key, value, force=False):
        if value in self.d[key] and not force:
            log.warning("%s %s is already present. Ignored as force attribute is not set", key, value)
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
        self.clear_user_files()
        self.generated_hfiles = []
        self.generated_cppfiles = []
        self.generated_fortranfiles = []
        # Generated sources holding GPU kernels, compiled with the GPU language (CUDA/HIP). The
        # kernel generators flag these via add_cpp_file(..., is_gpu_kernel=True).
        self.generated_gpu_cppfiles = []

    def clear_user_files(self):
        """Forget the non-generated source files the user registered.

        Used when the user re-declares the solver set (e.g. a configuration
        that toggles a limiter on/off is re-executed): the implementation
        files belong to the solvers, so they are re-registered along with
        them.
        """
        self.hfiles = []
        self.cppfiles = []
        self.fortranfiles = []

    def set_dimension(self, dimension):
        self.d["DIMENSIONS"] = str(dimension)

    def set_mode(self, mode):
        """
        mode should be of type CompileMode. Pass in

        peano4.output.CompileMode.Release

        for example. Release is the default.
        """
        if mode == CompileMode.Debug:
            self.d["MODE"] = "Debug"
        elif mode == CompileMode.Asserts:
            self.d["MODE"] = "Asserts"
        elif mode == CompileMode.Trace:
            self.d["MODE"] = "Trace"
        elif mode == CompileMode.Release:
            self.d["MODE"] = "Release"
        else:
            assert False

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
        Locate the Peano CMake package and read the small amount of core
        configuration still needed by Python code generation.
        """
        if "PEANO_SRC_ROOT_DIR" in os.environ:
            search_start = os.environ["PEANO_SRC_ROOT_DIR"]
        else:
            search_start = os.getcwd()

        self.d["CMAKE_BUILD_DIR"] = ""

        if "PEANO_CMAKE_BUILD_DIR" in os.environ:
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
                    pass  # Skip unreadable directories
                if self.d["CMAKE_BUILD_DIR"] != "":
                    break
                # Move up one level
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
                    # Values are emitted by PeanoConfig.cmake double-quoted.
                    # Capture the full quoted string (which may legitimately
                    # contain spaces or ')', e.g. a compiler path under
                    # "Program Files (x86)") and fall back to a bare token for
                    # any unquoted value.
                    match = re.match(
                        r'\s*set\(\s*(PEANO_[A-Za-z0-9_]+)\s+(?:"(.*)"|([^)"\s]+))\s*\)',
                        line,
                    )
                    if match:
                        key = match.group(1)
                        value = match.group(2) if match.group(2) is not None else match.group(3)
                        if key == "PEANO_WITH_SIMDLEN":
                            self.d["SIMDLEN"] = value
                        elif key == "PEANO_WITH_GPU_BACKEND":
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

            # Inform the plotter which output formats this build actually provides.
            # VTU has no external dependency and is always available; HDF5/NetCDF
            # are only available when CMake found and enabled the libraries. A
            # plotter asking for a missing format then falls back to VTU rather
            # than failing to link.
            available_formats = {"vtu"}
            for plot_format, config_key in (("hdf5", "WITH_HDF5"), ("netcdf", "WITH_NETCDF"), ("vtkhdf", "WITH_HDF5")):
                value = self.d.get(config_key)
                if value is None or value.upper() != "OFF":
                    available_formats.add(plot_format)
            import peano4.plotter

            peano4.plotter.set_available_formats(available_formats)

        else:
            raise Exception(
                """
  Error: parse_configuration_outcome() could not locate a valid CMake build directory
  (containing PeanoConfig.cmake) starting from:
  """
                + search_start
                + """
  and searching upwards through its parents.

  Please ensure CMake has run, or set PEANO_CMAKE_BUILD_DIR explicitly.
  """
            )

    def add_h_file(self, filename, generated=False):
        """
        Add a new header filename.
        This is actually not needed for compilation,
        but is useful to have for IDEs where the header files can be displayed
        in the project.

        filename: String
           Filename of a C/C++ header file. They usually should have the .h/.hpp extension.
        generated: Bool
           Use this flag for generated files which can be tracked
           by the cleaning routines 'distclean' or 'maintainer-clean' of the Makefile.
        """
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
        """
        Add a new cpp filename. This behaves like a set: adding the same file
        multiple times inserts it only once. This matters because multiple steps
        can hold the same action, which would otherwise add the file repeatedly.

        All the standard Peano 4 routines rely on this function to add their
        generated files to the build environment. Nothing stops you however to
        add more files yourself.

        filename: String
           Filename of a C++ file. They usually should have the .cpp/.cxx extension.
        generated: Bool
           Use this flag for generated files which can be tracked
           by the cleaning routines 'distclean' or 'maintainer-clean' of the Makefile.
        """
        if generated:
            # Non-generated file takes precedence over generated files.
            # We only add a generated file if has not been added yet as a non-generated file instead.
            if (
                self.generated_cppfiles.count(filename) == 0
                and self.cppfiles.count(filename) == 0
            ):
                self.generated_cppfiles.append(filename)
        else:
            # If a file is a non-generated file we definitely want to add it.
            if self.cppfiles.count(filename) == 0:
                self.cppfiles.append(filename)
                # We ensure that we only add a .cpp file once.
                # Otherwise we would get duplicated symbols errors.
                # But we never delete a non-generated file.
                if self.generated_cppfiles.count(filename) > 0:
                    self.generated_cppfiles.remove(filename)

        # Record GPU kernels so the generated CMakeLists can pass the list to peano_enable_gpu_language().
        if is_gpu_kernel and self.generated_gpu_cppfiles.count(filename) == 0:
            self.generated_gpu_cppfiles.append(filename)

    def remove_cpp_file(self, filename):
        if filename in self.generated_cppfiles:
            self.generated_cppfiles.remove(filename)
        if filename in self.cppfiles:
            self.cppfiles.remove(filename)

    def add_Fortran_file(self, filename, generated=False):
        """
        Add a new Fortran file.

        All the standard Peano 4 routines rely on this function to add their
        generated files to the build environment. Nothing stops you however to
        add more files yourself. Don't add a file multiple times. This might
        break the compiler.

        Fortran is really picky about the translation order. So you have to add
        the stuff in the right order. Otherwise, Fortran might complain. This is
        your responsibility.

        filename: String
           Filename of a Fortran file. They usually should have the .f90 extension.
        generated: Bool
           Use this flag for generated files which can be tracked
           by the cleaning routines 'distclean' or 'maintainer-clean' of the Makefile.
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

    def remove_Fortran_file(self, filename):
        if filename in self.generated_fortranfiles:
            self.generated_fortranfiles.remove(filename)
        if filename in self.fortranfiles:
            self.fortranfiles.remove(filename)

    def generate(self, directory, generated_subdir=None):
        """
        Generates build files and other project-related files based on templates.
        These are always (re)generated build artefacts (.gitignore, CMakeLists.txt,
        Makefile), so they are unconditionally overwritten.

        :param directory: The directory where the generated files will be placed.
        :type directory: str
        :param generated_subdir: Subfolder (relative to directory) the generated
            C++ sources live in. Their paths are prefixed accordingly and the
            subfolder is added as an include directory. None keeps everything flat.
        """
        prefix = generated_subdir + "/" if generated_subdir else ""
        self.d["GENERATED_SUBDIR"] = generated_subdir or ""
        self.d["H_HEADERS"] = self.hfiles
        self.d["CXX_SOURCES"] = self.cppfiles
        self.d["FORTRAN_SOURCES"] = self.fortranfiles
        self.d["GENERATED_H_HEADERS"] = [prefix + f for f in self.generated_hfiles]
        self.d["GENERATED_CXX_SOURCES"] = [prefix + f for f in self.generated_cppfiles]
        self.d["GENERATED_FORTRAN_SOURCES"] = [prefix + f for f in self.generated_fortranfiles]
        self.d["GENERATED_GPU_SOURCES"] = [prefix + f for f in self.generated_gpu_cppfiles]
        self.d["GENERATED_COMPILE_DEFINITIONS"] = self._unique(
            self.d["COMPILE_DEFINITIONS"]
        )
        self.d["GENERATED_COMPILE_OPTIONS"] = self._unique(
            self.d["COMPILE_OPTIONS"]
        )
        self.d["GENERATED_INCLUDE_DIRECTORIES"] = self._unique(
            ([generated_subdir] if generated_subdir else []) + self.d["INCLUDE_DIRECTORIES"]
        )
        self.d["GENERATED_LINK_LIBRARIES"] = self._unique(
            self.d["LINK_LIBRARIES"]
        )
        self.d["GENERATED_LINK_OPTIONS"] = self._unique(
            self.d["LINK_OPTIONS"]
        )

        # Encapsulate file generation as a function. The build files are pure
        # generator output and are always overwritten.
        def generate_output_file(input_file_path, output_file_path):
            log.debug("Write " + output_file_path)
            template = get_jinja2_template(input_file_path)
            with open(output_file_path, "w") as output:
                output.write(template.render(self.d))

        file_dir = os.path.dirname(os.path.realpath(__file__))

        with open(os.path.join(directory, ".gitignore"), "w") as gitignore:
            gitignore.write(self.executable_name() + "\n.gitignore\nCMakeLists.txt\nMakefile\n")

        generate_output_file(
            os.path.join(file_dir, "CMakeLists.jinja2"),
            os.path.join(directory, "CMakeLists.txt"),
        )
        generate_output_file(
            os.path.join(file_dir, "Makefile.jinja2"),
            os.path.join(directory, "Makefile"),
        )
