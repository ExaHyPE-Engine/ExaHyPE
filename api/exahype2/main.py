# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import exahype2.codegen


class Main:
    """Writes ``<project>-main.{h,cpp}``, whose main function passes the domain and the
    solver repository callbacks to the runner."""

    def __init__(self, project):
        self.project = project
        self.template_dictionary = {}

    def _configure_template_dictionary(self):
        self.template_dictionary["DOMAIN_OFFSET"] = (
            self.project.domain_offset_initialiser
        )
        self.template_dictionary["DOMAIN_SIZE"] = self.project.domain_size_initialiser
        self.template_dictionary["PERIODIC_BOUNDARIES"] = (
            self.project.periodic_boundaries_mask
        )
        self.template_dictionary.setdefault("DEFAULT_GRID_LEVELS", "-1")
        self.template_dictionary.setdefault(
            "DEFAULT_MAX_GRID_LEVELS", self.template_dictionary["DEFAULT_GRID_LEVELS"]
        )
        self.template_dictionary.setdefault("DEFAULT_MIN_TERMINAL_TIME", "-1.0")
        self.template_dictionary.setdefault("DEFAULT_MAX_TERMINAL_TIME", "-1.0")

    def construct_output(self, output, main_name):
        """Adds the header and the source file of the main function to ``output``.
        ``main_name`` is their path without a file extension."""
        output.makefile.add_h_file(main_name + ".h", generated=True)
        output.makefile.add_cpp_file(main_name + ".cpp", generated=True)
        self._configure_template_dictionary()

        template_directory = os.path.dirname(os.path.realpath(__file__))
        self.template_dictionary["MAIN_NAME"] = main_name
        self.template_dictionary["PROJECT_NAME"] = self.project.project_name

        output.add(
            exahype2.codegen.TemplatedFile(
                os.path.join(template_directory, "Main.h.jinja2"),
                os.path.join(template_directory, "Main.cpp.jinja2"),
                main_name,
                self.project.namespace,
                "./",
                self.template_dictionary,
            )
        )
