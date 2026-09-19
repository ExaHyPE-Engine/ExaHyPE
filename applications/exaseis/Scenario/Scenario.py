# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os, shutil, jinja2


class Scenario:
    domain_offset = [1.0, 1.0, 1.0]
    domain_size = [1.0, 1.0, 1.0]

    end_time = 0.0

    tracer_coordinates = []

    def domain_perturbations(self):
        return ""

    def initial_conditions(self):
        return True

    def generate_required_files(self):
        return

    def generate_file_from_template(
        self, template_file, full_qualified_filename, dictionary
    ):
        template_loader = jinja2.FileSystemLoader(
            searchpath=os.path.split(template_file)[0]
        )
        templateEnv = jinja2.Environment(
            loader=template_loader, undefined=jinja2.StrictUndefined
        )
        template = templateEnv.get_template(os.path.split(template_file)[1])

        with open(full_qualified_filename, "w") as output:
            output.write(template.render(dictionary))

        return

    def copy_file_to_current_folder(self, current_filename, full_qualified_filename):
        shutil.copyfile(current_filename, full_qualified_filename)
        return
