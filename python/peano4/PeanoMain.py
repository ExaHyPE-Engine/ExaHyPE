# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair


class PeanoMain(object):
    def __init__(self, project):
        self.project = project
        self.overwrite = False
        self.d = {}
        self.is_extended = False

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

    def construct_output(self, output, main_name):
        """
        Pass in a version of output

        main_name Is the name of you main file. By default, this might be
           Main.cpp, but you might want to have different main files for
           different experiments. Please do not give the file an extension.
        """
        output.makefile.add_h_file(main_name + ".h", generated=True)
        output.makefile.add_cpp_file(main_name + ".cpp", generated=True)

        header_file_template = self._get_header_file_template()
        cpp_file_template = self._get_implementation_file_template()
        self.d["MAIN_NAME"] = main_name
        self.d["HEADER_FILE_TEMPLATE"] = os.path.basename(header_file_template)
        self.d["CPP_FILE_TEMPLATE"] = os.path.basename(cpp_file_template)
        self.d["PROJECT_NAME"] = self.project.project_name
        self.d["SUBNAMESPACE"] = ""
        if self.project._subdirectory:
            self.d["SUBNAMESPACE"] = self.project._subdirectory + "::"
        self.d["SUBDIRECTORY"] = self.project.subdirectory
        self.d["SUBDIRECTORIES"] = []
        if self.project.subdirectories:
            for subdirectory in self.project.subdirectories:
                self.d["SUBDIRECTORIES"].append(subdirectory)

        generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            header_file_template,
            cpp_file_template,
            main_name,
            self.project.rootnamespace,
            "./",
            self.d,
            self.overwrite,
        )
        output.add(generated_files)

    def merge(self, main):
        """
        Make subproject's data a part of the project
        (will be extended)
        """

        self.is_extended = True
