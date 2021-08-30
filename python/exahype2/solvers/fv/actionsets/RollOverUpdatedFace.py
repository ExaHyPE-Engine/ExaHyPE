# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractFVActionSet import AbstractFVActionSet

import peano4
import jinja2

class RollOverUpdatedFace(AbstractFVActionSet):
  """
  
    The global periodic boundary conditions are set in the Constants.h. 
   
  """
  TemplateHandleFace = jinja2.Template( """
    if ( {{GUARD}} ) {
      logTraceInWith2Arguments( "touchFaceLastTime(...)---RollOverUpdatedFace", fineGridFace{{SOLVER_NAME}}FaceLabel.toString(), marker.toString() );
      
      const int normal = marker.getSelectedFaceNumber() % Dimensions;
      // Left half
      if ({{FACE_METADATA_ACCESSOR}}.getUpdated(0)) {
        dfore(k,{{DOFS_PER_AXIS}},normal,0) {
          for (int i=0; i<{{OVERLAP}}; i++) {
            tarch::la::Vector<Dimensions,int> overlapCell = k;
            overlapCell(normal) = i;
            const int index = toolbox::blockstructured::serialisePatchIndexInOverlap(overlapCell,{{DOFS_PER_AXIS}},{{OVERLAP}},normal);
            logDebug( "touchFaceLastTime(...)", "normal=" << normal << ",{{NEW_ACCESSOR}}[" << index << "]->{{OLD_ACCESSOR}}[" << index << "]" );
            for (int j=0; j<{{UNKNOWNS}}; j++) {
              {{OLD_ACCESSOR}}.value[index*{{UNKNOWNS}}+j] = {{NEW_ACCESSOR}}.value[index*{{UNKNOWNS}}+j]; 
              {{NEW_ACCESSOR}}.value[index*{{UNKNOWNS}}+j] = {{UPDATE_ACCESSOR}}.value[index*{{UNKNOWNS}}+j]; 
            }
          }
        }
      }
      // Right half
      if ({{FACE_METADATA_ACCESSOR}}.getUpdated(1)) {
        dfore(k,{{DOFS_PER_AXIS}},normal,0) {
          for (int i={{OVERLAP}}; i<2*{{OVERLAP}}; i++) {
            tarch::la::Vector<Dimensions,int> overlapCell = k;
            overlapCell(normal) = i;
            const int index = toolbox::blockstructured::serialisePatchIndexInOverlap(overlapCell,{{DOFS_PER_AXIS}},{{OVERLAP}},normal);
            logDebug( "touchFaceLastTime(...)", "normal=" << normal << ",{{NEW_ACCESSOR}}[" << index << "]->{{OLD_ACCESSOR}}[" << index << "]" );
            for (int j=0; j<{{UNKNOWNS}}; j++) {
              {{OLD_ACCESSOR}}.value[index*{{UNKNOWNS}}+j] = {{NEW_ACCESSOR}}.value[index*{{UNKNOWNS}}+j]; 
              {{NEW_ACCESSOR}}.value[index*{{UNKNOWNS}}+j] = {{UPDATE_ACCESSOR}}.value[index*{{UNKNOWNS}}+j]; 
            }
          }
        }
      }
      
      int index = 0;
      logTraceOut( "touchFaceLastTime(...)---RollOverUpdatedFace" );
    }
""" )
  
  
  def __init__(self,solver,guard):
    AbstractFVActionSet.__init__(self,solver)
    self.d = {}
    self.d[ "GUARD" ]           = guard
    self.d[ "UNKNOWNS" ]        = str(solver._patch_overlap_update.no_of_unknowns)
    self.d[ "DOFS_PER_AXIS" ]   = str(solver._patch.dim[0])
    self.d[ "OVERLAP" ]         = str(int(solver._patch_overlap_update.dim[0]/2))
    self.d[ "UPDATE_ACCESSOR" ] = "fineGridFace" + solver._patch_overlap_update.name
    self.d[ "OLD_ACCESSOR" ]    = "fineGridFace" + solver._patch_overlap_old.name
    self.d[ "NEW_ACCESSOR" ]    = "fineGridFace" + solver._patch_overlap_new.name
    self.d[ "FACE_METADATA_ACCESSOR" ] = "fineGridFace"  + solver._face_label.name


  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_LAST_TIME:
      self._solver._init_dictionary_with_default_parameters(self.d)
      self._solver.add_entries_to_text_replacement_dictionary(self.d)
      result = self.TemplateHandleFace.render(**self.d)
      pass 
    return result


  def get_includes(self):
    return """
#include "exahype2/PatchUtils.h"
#include "peano4/utils/Loop.h"
#include "toolbox/blockstructured/Enumeration.h"
""" + AbstractFVActionSet.get_includes(self) 


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
