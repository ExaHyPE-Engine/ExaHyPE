# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractFVActionSet import AbstractFVActionSet

import peano4
import jinja2

class InitialCondition(AbstractFVActionSet):
  TemplateInitialCondition = """
  if ({{PREDICATE}}) { 
    int index = 0;
    dfor( volume, {{NUMBER_OF_VOLUMES_PER_AXIS}} ) {
      repositories::{{SOLVER_INSTANCE}}.initialCondition(
        fineGridCell{{UNKNOWN_IDENTIFIER}}.value + index,
        ::exahype2::getVolumeCentre( marker.x(), marker.h(), {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume), 
        ::exahype2::getVolumeSize( marker.h(), {{NUMBER_OF_VOLUMES_PER_AXIS}} ),
        {{GRID_IS_CONSTRUCTED}}
      );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  } 
"""
  
  def __init__(self,solver,predicate,grid_is_constructed):
    AbstractFVActionSet.__init__(self,solver)
    self.predicate           = predicate
    self.grid_is_constructed = grid_is_constructed


  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]           = self.predicate      
      d[ "GRID_IS_CONSTRUCTED" ] = self.grid_is_constructed      
      result = jinja2.Template(self.TemplateInitialCondition).render(**d)
      pass 
    return result
