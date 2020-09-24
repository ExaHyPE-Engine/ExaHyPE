# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.datamodel
import peano4.output.TemplatedHeaderFile
import peano4.output.TemplatedHeaderImplementationFilePair

from .FV import FV

import jinja2


class PointWiseClawPackFVFixedTimeStepSize( FV ):
  User_Defined_Implementation = "<user-defined>"
  Empty_Implementation = "<empty>"

  __None         = "<none>"

  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties):
    """
            
    """
    super(AbstractGenericRusanovFV,self).__init__(name, patch_size, 1, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties)
   

  def get_user_includes(self):
    return """
"""    

  
  def add_entries_to_text_replacement_dictionary(self,d):
    """
    
     This routine befills a dictionary d which is in turn passed to Jinja2 to 
     befill the template for the solver.
    
    """
    d[ "TIME_STEP_SIZE" ]                     = self._time_step_size
