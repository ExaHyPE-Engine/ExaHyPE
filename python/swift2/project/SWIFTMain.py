# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4.runner.DefaultSequence
import os


class SWIFTMain(peano4.runner.DefaultSequence):
  def __init__(self,project,template_file):
    """
    
     project: swift2.project.Project
     
    """
    super(SWIFTMain,self).__init__(project)
    self.overwrite     = False
    self.template_file = template_file


  def _get_header_file_template(self):
    return os.path.dirname(__file__) + "/" + self.template_file+".h"
    
    
  def _get_implementation_file_template(self):
    return os.path.dirname(__file__) + "/" + self.template_file+".cpp"

