# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4.runner.DefaultSequence
import os


class SWIFTMain(peano4.runner.DefaultSequence):
  def __init__(self,project,initialisation_steps,solver_steps,overwrite=False):
    """
    
     solversteps: [peano4.solversteps.Steps]
     
     overwrite: Boolean
       Shall I keep an existing main, or shall I overwrite it. You can reset this
       behaviour after you've created the main object.
     
    """
    super(SWIFTMain,self).__init__(project)
    self.overwrite               = overwrite
    self.d[ "SOLVERSTEP_NAMES" ]         = [x.name for x in solver_steps]
    self.d[ "INITIALISATIONSTEP_NAMES" ] = [x.name for x in initialisation_steps]


  def _get_header_file_template(self):
    return os.path.dirname(__file__) + "/SWIFTMain.template.h"
    
    
  def _get_implementation_file_template(self):
    return os.path.dirname(__file__) + "/SWIFTMain.template.cpp"

