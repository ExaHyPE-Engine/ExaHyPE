# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2
import peano4
import dastgen2


class ScalarJacobiWithRediscretisation(ActionSet):


  def construct_face_helper_data(cell_data_model: peano4.datamodel.DaStGen2):
    """
    
    We cannot access neighbouring cells within Peano. Therefore, we have
    to store helper data on the faces such that we can reconstruct the 
    value within the face-connected neighbours.
    
    cell_data_mode: 
    
    """
    result = peano4.datamodel.DaStGen2( cell_data_model.name )
    
    result.data.add_attribute(dastgen2.attributes.Double("OldAverage") )
    result.data.add_attribute(dastgen2.attributes.Double("NewAverage") )

    result.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
        "true", "true", "true" )

    result.generator.send_condition               = "true"
    result.generator.receive_and_merge_condition  = "true"

    result.peano4_mpi_and_storage_aspect.merge_implementation += """
      _NewAverage += neighbour._NewAverage;
"""
    
    return result

  
  def __init__(self, 
               cell_data: peano4.datamodel.DaStGen2, 
               face_data: peano4.datamodel.DaStGen2, 
               relaxation_coefficient, 
               rhs_expr, 
               unknown_setter="setU", 
               unknown_getter="getU",
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
    assert relaxation_coefficient>0.0, "Relaxation coefficient has to be positive"
    assert relaxation_coefficient<2.0, "Relaxation coefficient has to be smaller than 2"
    
    self.d = {}
    self.d[ "CELL_NAME" ]       = cell_data.name
    self.d[ "FACE_NAME" ]       = face_data.name
    self.d[ "UNKNOWN_SETTER" ]  = unknown_setter
    self.d[ "UNKNOWN_GETTER" ]  = unknown_getter
    self.d[ "OMEGA" ]           = relaxation_coefficient
    self.d[ "RHS" ]             = rhs_expr
    self.d[ "GUARD" ]           = guard
    self.additional_includes    = ""
    

  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n" 


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  #def get_constructor_body(self):
  #  return self.__Template_Constructor.format(**self.d)


  _Template_CreateCell= """
  fineGridCell{{CELL_NAME}}.{{UNKNOWN_SETTER}}(0.0);
"""

  _Template_CreatePersistentFace = """
  fineGridFace{{FACE_NAME}}.setNewAverage(0.0);
  fineGridFace{{FACE_NAME}}.setOldAverage(0.0);
"""

  _Template_CreateHangingFace = """
  auto coarseValue = coarseGridFaces{{FACE_NAME}}( marker.getSelectedFaceNumber() ).getOldAverage();

  fineGridFace{{FACE_NAME}}.setOldAverage( coarseValue );
  fineGridFace{{FACE_NAME}}.setNewAverage( coarseValue );
"""

  _Template_DestroyHangingFace = """
  coarseGridFaces{{FACE_NAME}}( marker.getSelectedFaceNumber() ).setNewAverage(
    coarseGridFaces{{FACE_NAME}}( marker.getSelectedFaceNumber() ).getNewAverage()
    +
    ThreePowerD/3.0 * fineGridFace{{FACE_NAME}}.getNewAverage()
  );
"""

  _Template_TouchFaceFirstTime = """
  fineGridFace{{FACE_NAME}}.setOldAverage( fineGridFace{{FACE_NAME}}.getNewAverage() );
  fineGridFace{{FACE_NAME}}.setNewAverage( 0.0 );
"""


  _Template_TouchCellFirstTime_UpdateSolution = """
  // @todo Das ist nicht schoen. Ich haette hier gerne nen allgemeinen Stencil

  if ({{GUARD}}) {  
    double diagonal = 0.0;
    double residual = {{RHS}};
  
    double centralValue = fineGridCell{{CELL_NAME}}.{{UNKNOWN_GETTER}}();
  
    for (int d=0; d<Dimensions; d++) {
      double leftNeighbour  = 2.0 * (fineGridFaces{{FACE_NAME}}(d).getOldAverage()            - 0.5 * centralValue);
      double rightNeighbour = 2.0 * (fineGridFaces{{FACE_NAME}}(d+Dimensions).getOldAverage() - 0.5 * centralValue);
    
      residual -= (-1.0 * leftNeighbour + 2.0 * centralValue - 1.0 * rightNeighbour) / marker.h()(d) / marker.h()(d);
      diagonal += (2.0) / marker.h()(d) / marker.h()(d);
    }
    
    double newValue = fineGridCell{{CELL_NAME}}.{{UNKNOWN_GETTER}}() + {{OMEGA}} / diagonal * residual;
  
    fineGridCell{{CELL_NAME}}.{{UNKNOWN_SETTER}}( newValue );
  }
"""


  _Template_TouchCellFirstTime_UpdateFaceHelperData = """
  for (int d=0; d<Dimensions; d++) {
    const double newValue = fineGridCell{{CELL_NAME}}.{{UNKNOWN_GETTER}}();
    fineGridFaces{{FACE_NAME}}(d).setNewAverage(            fineGridFaces{{FACE_NAME}}(d).getNewAverage()            + 0.5 * newValue );
    fineGridFaces{{FACE_NAME}}(d+Dimensions).setNewAverage( fineGridFaces{{FACE_NAME}}(d+Dimensions).getNewAverage() + 0.5 * newValue );
  }
"""

  
  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_CREATE_CELL:
      result = jinja2.Template(self._Template_CreateCell).render(**self.d) 
    if operation_name==ActionSet.OPERATION_CREATE_HANGING_FACE:
      result = jinja2.Template(self._Template_CreateHangingFace).render(**self.d) 
    if operation_name==ActionSet.OPERATION_CREATE_PERSISTENT_FACE:
      result = jinja2.Template(self._Template_CreatePersistentFace).render(**self.d) 
    if operation_name==ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = jinja2.Template(self._Template_TouchFaceFirstTime).render(**self.d) 
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result  = jinja2.Template(self._Template_TouchCellFirstTime_UpdateSolution).render(**self.d) 
      result += jinja2.Template(self._Template_TouchCellFirstTime_UpdateFaceHelperData).render(**self.d) 
    if operation_name==ActionSet.OPERATION_DESTROY_HANGING_FACE:
      result = jinja2.Template(self._Template_DestroyHangingFace).render(**self.d) 
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

