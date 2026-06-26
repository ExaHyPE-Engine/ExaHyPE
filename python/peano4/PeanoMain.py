# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair


class PeanoMain(object):
    def __init__(self, project):
        self.project = project
        self.d = {}
        self.is_extended = False
        # Opaque C++ snippets a higher layer (e.g. exahype2) can inject without
        # peano4 having to know what they contain.
        self.extra_includes = ""
        self.extra_repository_callbacks = ""

    def _get_header_file_template(self):
        templatefile_prefix = (
            os.path.realpath(__file__).replace(".pyc", "").replace(".py", "")
        )
        return templatefile_prefix + ".template.h"

    def _get_implementation_file_template(self):
        templatefile_prefix = (
            os.path.realpath(__file__).replace(".pyc", "").replace(".py", "")
        )
        return templatefile_prefix + ".template.cpp"

    def _step_dictionaries(self):
        # A step may carry opaque C++ snippets that wrap its traversal in the
        # generated Step body (so they run on every rank). peano4 does not know or
        # care what they are - a higher layer (e.g. exahype2) attaches them.
        result = []
        for step in self.project.solversteps._steps:
            result.append(
                {
                    "name": step.name,
                    "start_hook": getattr(step, "runner_start_hook", ""),
                    "finish_hook": getattr(step, "runner_finish_hook", ""),
                }
            )
        return result

    def _export_constants(self):
        """Export the run-invariant facts to Constants.h ONCE, so the generated
        main (and observers/action sets) reference them instead of re-emitting
        literals. ConfigurationInformation records the exact generation command."""
        constants = self.project.constants
        constants.add_include("#include <bitset>")
        constants.add_include('#include "tarch/la/Vector.h"')
        constants.add_include('#include "peano4/Globals.h"')

        offset = "{" + ", ".join(str(value) for value in self.project._domain_offset) + "}"
        size = "{" + ", ".join(str(value) for value in self.project._domain_size) + "}"
        constants.export_const_with_type("DomainOffset", offset, "tarch::la::Vector<DIMENSIONS, double>")
        constants.export_const_with_type("DomainSize", size, "tarch::la::Vector<DIMENSIONS, double>")
        constants.export_boolean_sequence("PeriodicBC", self.project._periodic_bc)
        constants.export_string("ConfigurationInformation", " ".join(["python3", *sys.argv]))

    def _configure_template_dictionary(self):
        if self.project._domain_offset is None or self.project._domain_size is None:
            raise Exception("Peano project requires a domain. Pass offset=... and size=... to peano4.Project(...).")

        self.d["MAIN_STEPS"] = self._step_dictionaries()

        if self.project._default_grid_levels is not None:
            self.d["DEFAULT_GRID_LEVELS"] = str(self.project._default_grid_levels)
        # A higher layer (exahype2) may have provided real values already; only
        # fill in generic defaults where it has not.
        self.d.setdefault("DEFAULT_GRID_LEVELS", "-1")
        self.d.setdefault("DEFAULT_MAX_GRID_LEVELS", self.d["DEFAULT_GRID_LEVELS"])
        self.d.setdefault("DEFAULT_MIN_TERMINAL_TIME", "-1.0")
        self.d.setdefault("DEFAULT_MAX_TERMINAL_TIME", "-1.0")

        if "EXECUTABLE_NAME" not in self.d:
            self.d["EXECUTABLE_NAME"] = '"{}"'.format(self.project.output.makefile.executable_name())

        self.d["EXTRA_INCLUDES"] = self.extra_includes
        self.d["EXTRA_REPOSITORY_CALLBACKS"] = self.extra_repository_callbacks

    def construct_output(self, output, main_name):
        """
        Pass in a version of output

        main_name Is the name of you main file. By default, this might be
           Main.cpp, but you might want to have different main files for
           different experiments. Please do not give the file an extension.
        """
        output.makefile.add_h_file(main_name + ".h", generated=True)
        output.makefile.add_cpp_file(main_name + ".cpp", generated=True)
        self._export_constants()
        self._configure_template_dictionary()

        header_file_template = self._get_header_file_template()
        cpp_file_template = self._get_implementation_file_template()
        self.d["MAIN_NAME"] = main_name
        self.d["HEADER_FILE_TEMPLATE"] = os.path.basename(header_file_template)
        self.d["CPP_FILE_TEMPLATE"] = os.path.basename(cpp_file_template)
        self.d["PROJECT_NAME"] = self.project.project_name

        generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            header_file_template,
            cpp_file_template,
            main_name,
            self.project.rootnamespace,
            "./",
            self.d,
        )
        output.add(generated_files)
