# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms import PDETerms

import os, shutil, jinja2

class Scenario:
    domain_offset   = [1., 1., 1.]
    domain_size     = [1., 1., 1.]

    end_time = 0.0

    tracer_coordinates = []

    def initial_conditions(self):
        return PDETerms.User_Defined_Implementation
    
    def generate_required_files(self, order):
        return
    
    def generate_file_from_template(self, template_file, full_qualified_filename, dictionary):
        print(os.path.split(template_file)[0])
        template_loader = jinja2.FileSystemLoader(searchpath=os.path.split(template_file)[0])
        templateEnv = jinja2.Environment(loader=template_loader, undefined=jinja2.DebugUndefined)
        template = templateEnv.get_template( os.path.split(template_file)[1] )

        with open( full_qualified_filename, "w" ) as output:
            output.write( template.render(dictionary) )

        return
    
    def copy_file_to_current_folder(self, current_filename, full_qualified_filename):
        shutil.copyfile(current_filename, full_qualified_filename)
        return
