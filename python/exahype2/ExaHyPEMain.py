# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import peano4.PeanoMain


class ExaHyPEMain(peano4.PeanoMain):
    def __init__(self, peano_project):
        """
        project: peano4.Project
        """
        super(ExaHyPEMain, self).__init__(peano_project)

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
