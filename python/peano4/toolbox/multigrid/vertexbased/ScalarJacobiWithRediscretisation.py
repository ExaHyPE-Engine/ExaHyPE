# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


class ScalarJacobiWithRediscretisation(ActionSet):

  
  def __init__(self, 
               vertex_data, 
               relaxation_coefficient, 
               rhs_expr, 
               unknown_setter="setU", 
               unknown_getter="getU",
               residual_setter="setResidual",
               residual_getter="getResidual",
               store_residual_persistently_and_update_in_touch_first = True,
               guard="true"
               ):
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
    self.d[ "UNKNOWN_SETTER" ]  = unknown_setter
    self.d[ "UNKNOWN_GETTER" ]  = unknown_getter
    self.d[ "RESIDUAL_SETTER" ] = residual_setter
    self.d[ "RESIDUAL_GETTER" ] = residual_getter
    self.d[ "OMEGA" ]           = relaxation_coefficient
    self.d[ "RHS" ]             = rhs_expr
    self.d[ "GUARD" ]           = guard
    self.additional_includes    = ""
    self._store_residual_persistently_and_update_in_touch_first = store_residual_persistently_and_update_in_touch_first
    

  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n" 


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  __Template_Constructor = jinja2.Template("""
  _cellStiffnessMatrix = toolbox::finiteelements::StencilFactory::getLaplacian( 0.0, 1.0 );
""")
  
  
  #def get_constructor_body(self):
  #  return self.__Template_Constructor.format(**self.d)


  Template_ResetResidual = """ 
  fineGridVertex{{UNKNOWN}}.{{RESIDUAL_SETTER}}(0.0);
"""

  
  """
  
  This is where you should interpolate
  
  @todo Einbauen! Obwohl wir wissen, dass es ueberschrieben wird
  
  """
  Template_CreateHangingVertex = """
  fineGridVertex{{UNKNOWN}}.{{RESIDUAL_SETTER}}(0.0);
"""  

  
  Template_TouchCellFirstTime = """
  if ({{GUARD}}) {
    const tarch::la::Vector<ThreePowerD,double> stencil = toolbox::finiteelements::getLaplacian(
      marker.h(), 1.0
    );
    const toolbox::finiteelements::ElementWiseAssemblyMatrix A = toolbox::finiteelements::getElementWiseAssemblyMatrix(
      stencil
    );

    tarch::la::Vector<TwoPowerD,double> u;
    for (int i=0; i<TwoPowerD; i++) {
      u(i) = fineGridVertices{{UNKNOWN}}(i).{{UNKNOWN_GETTER}}();
    }

    tarch::la::Vector<TwoPowerD,double> r = A * u;
  
    for (int i=0; i<TwoPowerD; i++) {
      fineGridVertices{{UNKNOWN}}(i).{{RESIDUAL_SETTER}}(
        fineGridVertices{{UNKNOWN}}(i).{{RESIDUAL_GETTER}}() + r(i)
      );
    }
  }
"""  


  Template_UpdateValue = """
  const tarch::la::Vector<ThreePowerD,double> stencil = toolbox::finiteelements::getLaplacian(
    marker.h(), 1.0
  );
  const double diag     = stencil(ThreePowerD/2);
  const double residual = {{RHS}} * tarch::la::volume(marker.h()) - fineGridVertex{{UNKNOWN}}.{{RESIDUAL_GETTER}}();
  fineGridVertex{{UNKNOWN}}.{{UNKNOWN_SETTER}}(
    fineGridVertex{{UNKNOWN}}.{{UNKNOWN_GETTER}}()
    + 
    {{OMEGA}} * residual / diag
  );  
  
  if ( marker.coincidesWithCoarseGridVertex() ) {
    tarch::la::Vector<Dimensions,int> parent = marker.getRelativePositionWithinFatherCell() / 3;
    coarseGridVertices{{UNKNOWN}}( peano4::utils::dLinearised(parent,2) ).{{UNKNOWN_SETTER}}(
      fineGridVertex{{UNKNOWN}}.{{UNKNOWN_GETTER}}()
    );
  }
"""
  

  Template_CreatePersistentVertex = """
"""  
  
  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = """
logTraceIn( "touchVertexFirstTime()" );
"""
      if self._store_residual_persistently_and_update_in_touch_first:  
        result += jinja2.Template(self.Template_UpdateValue).render(**self.d) 
      result += jinja2.Template(self.Template_ResetResidual).render(**self.d) 
      result += """
logTraceOut( "touchVertexFirstTime()" );
"""
    if operation_name==ActionSet.OPERATION_CREATE_HANGING_VERTEX:
      result = """
logTraceIn( "createHangingVertex()" );
"""
      result += jinja2.Template(self.Template_CreateHangingVertex).render(**self.d) 
      result += """
logTraceOut( "createHangingVertex()" );
"""
    if operation_name==ActionSet.OPERATION_CREATE_PERSISTENT_VERTEX:
      result = """
logTraceIn( "createPersistentVertex()" );
"""
      result += jinja2.Template(self.Template_CreatePersistentVertex).render(**self.d) 
      result += """
logTraceOut( "createPersistentVertex()" );
"""
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = """
logTraceIn( "touchCellFirstTime()" );
"""
      result += jinja2.Template(self.Template_TouchCellFirstTime).render(**self.d) 
      result += """
logTraceOut( "touchCellFirstTime()" );
"""
    if operation_name==ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
      result = """
logTraceIn( "touchVertexLastTime()" );
"""
      if not self._store_residual_persistently_and_update_in_touch_first:  
        result += jinja2.Template(self.Template_UpdateValue).render(**self.d) 
      result += """
logTraceOut( "touchVertexLastTime()" );
"""
    return result


  def get_attributes(self):
    return """
    toolbox::finiteelements::ElementWiseAssemblyMatrix  _cellStiffnessMatrix;
"""


  def get_includes(self):
    return """
#include "toolbox/finiteelements/ElementMatrix.h"
#include "toolbox/finiteelements/StencilFactory.h"

#include "peano4/utils/Loop.h"
""" + self.additional_includes