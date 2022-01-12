# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractFVActionSet import AbstractFVActionSet

import peano4
import jinja2

class EmptyPostprocessSolution(AbstractFVActionSet):
  """
  
  PostprocessSolution differs from other action sets, as I only create it once. See 
  FV.create_action_sets(). Once the instance does exist, you can tailor it. But you
  can also completely overwrite it.
  
  """
  def __init__(self,solver):
    AbstractFVActionSet.__init__(self,solver)
    # Is there for compatibility reasons
    self.guard               = "true"
    

  def get_body_of_operation(self,operation_name):
    result = ""
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")





class VolumeWisePostprocessSolution(AbstractFVActionSet):
  """
  
  PostprocessSolution differs from other action sets, as I only create it once. See 
  FV.create_action_sets(). Once the instance does exist, you can tailor it. But you
  can also completely overwrite it.
  
  """
  def __init__(self,solver):
    AbstractFVActionSet.__init__(self,solver)
    self.guard               = "true"
    self._compute_kernel     = ""


  def add_postprocessing_kernel(self, operation_per_point):
    """
    
    Add a code snippet which is applied to each and every point. You have the following
    variables which are well-defined:
    
    - value: Is a pointer to the current finite volume's data
    - volumeX: A tarch::la::Vector over doubles
    - volumeH: A tarch::la::Vector over doubles
    
    
    operation_per_point: String
      C/C++ code snippet
    
    """
    self._compute_kernel += """
  if (
    not marker.hasBeenRefined() 
    and 
    {{PREDICATE}}
  ) { 
    logTraceIn( "touchCellFirstTime(...)" );
    int index = 0;
    dfor( volume, {{NUMBER_OF_VOLUMES_PER_AXIS}} ) {
      double* value   = fineGridCell{{UNKNOWN_IDENTIFIER}}.value + index;
      auto    volumeX = ::exahype2::getVolumeCentre( marker.x(), marker.h(), {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume);
      auto    volumeH = ::exahype2::getVolumeSize( marker.h(), {{NUMBER_OF_VOLUMES_PER_AXIS}});
      
      """ + operation_per_point + """
      
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
    logTraceOut( "touchCellFirstTime(...)" );
  } 
"""
    print( "\n\n\n\n\n CALLED THIS ONE \n\n\n\n " + self._compute_kernel )

    

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]           = self.guard      
      result = jinja2.Template(self._compute_kernel).render(**d)
      print( "\n\n\n\n\n WAS HERE \n\n\n\n " + self._compute_kernel + " -> " + result)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
