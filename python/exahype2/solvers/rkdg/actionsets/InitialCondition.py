# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

import peano4
import jinja2

class InitialCondition(AbstractRungeKuttaDGActionSet):
  TemplateInitialCondition = """
  if ({{PREDICATE}}) { 
    logTraceIn( "touchCellFirstTime(...)---InitialCondition" );
    int linearisedIndex = 0;
    dfor( index, {{DG_ORDER}}+1 ) {
      repositories::{{SOLVER_INSTANCE}}.initialCondition(
        fineGridCell{{UNKNOWN_IDENTIFIER}}.value + linearisedIndex,
        ::exahype2::dg::getQuadraturePoint(
          marker.x(), marker.h(), index, repositories::{{SOLVER_INSTANCE}}.DGOrder, repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d
        ),
        marker.h(),
        index,
        {{GRID_IS_CONSTRUCTED}}
      );
      linearisedIndex += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
    
    // relevant for tracing et al which kicks in if and only if cell has been
    // updated
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);

    logTraceOut( "touchCellFirstTime(...)---InitialCondition" );
  } 
"""
  
  def __init__(self,solver,guard,grid_is_constructed):
    super(InitialCondition,self).__init__(solver)
    self.guard               = guard
    self.grid_is_constructed = grid_is_constructed


  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]           = self.guard      
      d[ "GRID_IS_CONSTRUCTED" ] = self.grid_is_constructed      
      result = jinja2.Template(self.TemplateInitialCondition).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
