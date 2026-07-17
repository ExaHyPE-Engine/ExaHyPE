# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4.output.Jinja2TemplatedHeaderImplementationFilePair


class PeanoMain:
    def __init__(self, project):
        self.project = project
        self.template_dictionary = {}
        self.additional_includes = ""
        self.additional_repository_callbacks = ""

    def _get_header_file_template(self):
        template_file_prefix = (
            os.path.realpath(__file__).replace(".pyc", "").replace(".py", "")
        )
        return template_file_prefix + ".template.h"

    def _get_implementation_file_template(self):
        template_file_prefix = (
            os.path.realpath(__file__).replace(".pyc", "").replace(".py", "")
        )
        return template_file_prefix + ".template.cpp"

    def _step_dictionaries(self):
        """Describe opaque C++ hooks placed around each generated traversal.

        Higher layers attach the hooks; Peano emits them into every rank's
        step body.
        """
        result = []
        for step in self.project.solversteps._steps:
            result.append(
                {
                    "name": step.name,
                    "start_hooks": getattr(step, "runner_start_hook", "").splitlines(),
                    "finish_hooks": getattr(
                        step, "runner_finish_hook", ""
                    ).splitlines(),
                }
            )
        return result

    def _export_constants(self):
        constants = self.project.constants
        constants.add_include("#include <bitset>")
        constants.add_include('#include "tarch/la/Vector.h"')
        constants.add_include('#include "peano4/Globals.h"')

        offset = (
            "{" + ", ".join(str(value) for value in self.project._domain_offset) + "}"
        )
        size = "{" + ", ".join(str(value) for value in self.project._domain_size) + "}"
        constants.export_const_with_type(
            "DomainOffset", offset, "tarch::la::Vector<DIMENSIONS, double>"
        )
        constants.export_const_with_type(
            "DomainSize", size, "tarch::la::Vector<DIMENSIONS, double>"
        )
        constants.export_boolean_sequence("PeriodicBC", self.project._periodic_bc)

    def _configure_template_dictionary(self):
        if self.project._domain_offset is None or self.project._domain_size is None:
            raise ValueError(
                "Peano project requires a domain. Pass offset=... and size=... to peano4.Project(...)."
            )

        main_steps = self._step_dictionaries()
        self.template_dictionary["MAIN_STEPS"] = main_steps

        self.template_dictionary["ADDITIONAL_INCLUDES"] = [
            include.strip()
            for include in self.additional_includes.splitlines()
            if include.strip()
        ]

        if self.project._default_grid_levels is not None:
            self.template_dictionary["DEFAULT_GRID_LEVELS"] = str(
                self.project._default_grid_levels
            )
        # Higher layers may prepopulate runtime defaults, so Peano fills only missing entries.
        self.template_dictionary.setdefault("DEFAULT_GRID_LEVELS", "-1")
        self.template_dictionary.setdefault(
            "DEFAULT_MAX_GRID_LEVELS",
            self.template_dictionary["DEFAULT_GRID_LEVELS"],
        )
        self.template_dictionary.setdefault("DEFAULT_MIN_TERMINAL_TIME", "-1.0")
        self.template_dictionary.setdefault("DEFAULT_MAX_TERMINAL_TIME", "-1.0")

        self.template_dictionary["ADDITIONAL_REPOSITORY_CALLBACKS"] = (
            self.additional_repository_callbacks
        )

    def construct_output(self, output, main_name):
        """Generate the main files without appending a file extension to the name."""
        output.makefile.add_h_file(main_name + ".h", generated=True)
        output.makefile.add_cpp_file(main_name + ".cpp", generated=True)
        self._export_constants()
        self._configure_template_dictionary()

        header_file_template = self._get_header_file_template()
        cpp_file_template = self._get_implementation_file_template()
        self.template_dictionary["MAIN_NAME"] = main_name
        self.template_dictionary["HEADER_FILE_TEMPLATE"] = os.path.basename(
            header_file_template
        )
        self.template_dictionary["CPP_FILE_TEMPLATE"] = os.path.basename(
            cpp_file_template
        )
        self.template_dictionary["PROJECT_NAME"] = self.project.project_name

        generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            header_file_template,
            cpp_file_template,
            main_name,
            self.project.rootnamespace,
            "./",
            self.template_dictionary,
        )
        output.add(generated_files)
