# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


class MatrixFreeJacobi(ActionSet):

  
  
  def __init__(self, vertex_data, relaxation_coefficient, kappa_expr, rhs_expr):
    """
      
      :Attibutes:
      
      vertex_data: DaStGen object
        This object must have at least two attributes: A double value u and a double
        value residual. 
        
      kappa_expr: C++ code (string)
        Material parameter. Pass in 1.0 if there's no jump anywhere.
      
          
    """
    self.d = {}
    self.d[ "UNKNOWN" ] = vertex_data.name
    

  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n" 


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  __Template_Constructor = jinja2.Template("""
  _cellStiffnessMatrix = toolbox::finiteelements::StencilFactory::getLaplacian( 0.0, 1.0 );
""")
  
  
  def get_constructor_body(self):
    return self.__Template_Constructor.format(**self.d)


  __Template_TouchVertexFirstTime = """ 
  fineGridVertex{{UNKNOWN}}.setResidual(0.0);
//     tarch::la::Vector<ThreePowerD,double>     
"""

  
  """
  
  This is where you should interpolate
  
  @todo Einbauen! Obwohl wir wissen, dass es ueberschrieben wird
  
  """
  Template_CreateHangingVertex = """
"""  

  
  Template_TouchCellFirstTime = """
"""  


  Template_TouchVertexLastTime = """
"""
  

  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = jinja2.Template(self.Template_TouchVertexFirstTime).render(**self.d) 
    if operation_name==ActionSet.OPERATION_CREATE_HANGING_VERTEX:
      result = jinja2.Template(self.Template_CreateHangingVertex).render(**self.d) 
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.__Template_TouchCellFirstTime).render(**self.d) 
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
      result = jinja2.Template(self.__Template_TouchVertexLastTime).render(**self.d) 
    return result


  def get_attributes(self):
    return """
    toolbox::finiteelements::ElementWiseAssemblyMatrix  _cellStiffnessMatrix;
"""


  def get_includes(self):
    return """
#include "toolbox/finiteelements/ElementMatrix.h"
#include "toolbox/finiteelements/StencilFactory.h"
"""