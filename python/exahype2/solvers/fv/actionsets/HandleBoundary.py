# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractFVActionSet import AbstractFVActionSet

import peano4
import jinja2

class HandleBoundary(AbstractFVActionSet):
  """
  
    The global periodic boundary conditions are set in the Constants.h. 
   
  """
  TemplateHandleBoundary = """
    if (
      {{PREDICATE}}
      and
      not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[marker.getSelectedFaceNumber()%Dimensions]
      and
      not marker.isRefined() 
      and 
      fineGridFace{{SOLVER_NAME}}FaceLabel.getBoundary()
    ) {
      logTraceInWith3Arguments( "touchFaceFirstTime(...)---HandleBoundary", fineGridFace{{SOLVER_NAME}}FaceLabel.toString(), (repositories::{{SOLVER_INSTANCE}}.PeriodicBC[marker.getSelectedFaceNumber()%Dimensions]), marker.toString() );
      ::exahype2::fv::applyBoundaryConditions(
        [&](
          const double * __restrict__                  Qinside,
          double * __restrict__                        Qoutside,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  volumeH,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> void {
          repositories::{{SOLVER_INSTANCE}}.boundaryConditions( Qinside, Qoutside, faceCentre, volumeH, t, normal );
        },  
        marker.x(),
        marker.h(),
        repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp(),
        repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
        marker.getSelectedFaceNumber(),
        fineGridFace{{UNKNOWN_IDENTIFIER}}.value
      );
      logTraceOut( "touchFaceFirstTime(...)---HandleBoundary" );
    }
"""
  def __init__(self,solver,predicate):
    AbstractFVActionSet.__init__(self,solver)
    self.predicate = predicate


  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ] = self.predicate      
      result = jinja2.Template(self.TemplateHandleBoundary).render(**d)
      pass 
    return result


  def get_includes(self):
    return """
#include "exahype2/PatchUtils.h"
#include "exahype2/fv/BoundaryConditions.h"
""" + AbstractFVActionSet.get_includes(self) 
