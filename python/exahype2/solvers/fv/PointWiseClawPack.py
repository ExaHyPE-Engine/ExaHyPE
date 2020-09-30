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
  
  
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties):
    super(AbstractGenericRusanovFV,self).__init__(name, patch_size, 1, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties)
    
    #self._eigenvalues_implementation                = self.User_Defined_Implementation
    #self._boundary_conditions_implementation        = self.User_Defined_Implementation
    #self._refinement_criterion_implementation       = self.User_Defined_Implementation
    #self._initial_conditions_implementation         = self.User_Defined_Implementation

  
  def set_implementation(self,boundary_conditions=User_Defined_Implementation,refinement_criterion=User_Defined_Implementation,initial_conditions=User_Defined_Implementation):
    """
      If you pass in User_Defined, then the generator will create C++ stubs 
      that you have to befill manually. If you pass in Empty_Implementation, it 
      will create nop, i.e. no implementation or defaults. Any other string
      is copied 1:1 into the implementation. 
    """
    #self._flux_implementation        = flux
    #self._ncp_implementation         = ncp
    #self._eigenvalues_implementation = eigenvalues
    #self._boundary_conditions_implementation        = boundary_conditions
    #self._refinement_criterion_implementation       = refinement_criterion
    #self._initial_conditions_implementation         = initial_conditions
    pass
    

  def get_user_includes(self):
    return """
#include "exahype2/fv/Rusanov.h"
"""    
