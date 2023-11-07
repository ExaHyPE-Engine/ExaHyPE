# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import dastgen2
import peano4.datamodel.DaStGen2
import jinja2


class UpdateFaceLabel(ActionSet):
  def get_attribute_name(solver_name):
    return solver_name + "FaceLabel"

  
  """
  
   SetLabels is an action set which is automatically merged into all ExaHyPE2
   steps by the project. There's nothing for the user solvers to be done here.
   
   There are four different fields per face:
   
   - Updated: Boolean[2]
     Shows if the corresponding adjacent cell (left vs right) has performed a
     time step.
   - UpdatedTimeStamp: Double[2]
     Used only by the Finite Volumes solver so far to store a new time stamp
     temporarily which is later on rolled over into the real time stamp.
   - NewTimeStamp: Double[2]
     New (and also current time stamp). This time stamp determines for example
     if you want to do a local time step or not.
   - OldTimeStamp: Double[2]
     Time stamp of previous time step. The old time stamp is required for the
     in-time interpolation, e.g.
     
   ## Initialisation
   
   The initialisation sets the boundary flag if the face is on the boundary.
   It also sets the time stamps all to zero. This is actually a tricky thing
   to do, so I'm not sure if it is a good idea: the DG solvers for example
   require the face time stamps to be set properly. The Finite Volumes solver
   in return benefits if it is not (properly) initialised, as it then is 
   simpler to spot inconsistencies and missing data updates.
   
  
  """
  def __init__(self, solver_name):
    super(UpdateFaceLabel,self).__init__(descend_invocation_order=1,parallel=False)
    self._solver_name = solver_name
    pass


  def get_constructor_body(self):
    return ""

    
  def get_destructor_body(self):
    return ""


  _Template_TouchFaceFirstTime = """
  fineGridFace{{FACE_LABEL_NAME}}.setUpdated( false );
"""


  _Template_CreateFace = """
  logTraceInWith1Argument( "createPersistentFace(...)", marker.toString() );
      
  tarch::la::Vector<Dimensions, double> offset(DomainOffset);
  tarch::la::Vector<Dimensions, double> size(DomainSize);
  bool isBoundary = false;
  for (int d=0; d<Dimensions; d++) {
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) );
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) + size(d) );
  }
  fineGridFace{{FACE_LABEL_NAME}}.setBoundary( isBoundary );
  fineGridFace{{FACE_LABEL_NAME}}.setIsHanging( false );
  fineGridFace{{FACE_LABEL_NAME}}.setAboveHanging( false );
  fineGridFace{{FACE_LABEL_NAME}}.setUpdatedTimeStamp( 0.0 );
  fineGridFace{{FACE_LABEL_NAME}}.setNewTimeStamp( 0.0 );
  fineGridFace{{FACE_LABEL_NAME}}.setOldTimeStamp( 0.0 );

  logTraceOutWith1Argument( "createPersistentFace(...)", fineGridFace{{FACE_LABEL_NAME}}.toString() );
"""


  _Template_CreateHangingFace = """
  logTraceInWith1Argument( "createHangingFace(...)", marker.toString() );
  fineGridFace{{FACE_LABEL_NAME}}.setIsHanging( true );
  coarseGridFaces{{FACE_LABEL_NAME}}(marker.getSelectedFaceNumber()).setAboveHanging( true );
  logTraceOutWith1Argument( "createHangingFace(...)", fineGridFace{{FACE_LABEL_NAME}}.toString() );
"""  
  
  
  _Template_DestroyHangingFace = """
"""  


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  def get_body_of_operation(self,operation_name):
    result = "\n"
    
    d = {}
    d[ "FACE_LABEL_NAME" ] = UpdateFaceLabel.get_attribute_name(self._solver_name)
    
    if operation_name==ActionSet.OPERATION_CREATE_PERSISTENT_FACE or operation_name==ActionSet.OPERATION_CREATE_HANGING_FACE:
      result = jinja2.Template( self._Template_CreateFace ).render( **d )
    if operation_name==ActionSet.OPERATION_CREATE_HANGING_FACE:
      result = jinja2.Template( self._Template_CreateHangingFace ).render( **d )
    if operation_name==ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = jinja2.Template( self._Template_TouchFaceFirstTime ).render( **d )
    if operation_name==ActionSet.OPERATION_DESTROY_HANGING_FACE:
      result = jinja2.Template( self._Template_DestroyHangingFace ).render( **d )
      
    return result


  def get_attributes(self):
    return ""


  def get_includes(self):
    return """
#include "Constants.h"
"""    


def create_face_label(solver_name):
  """

   Build up the DaStGen2 data structure that we need per face to maintain
   per-face data per solver.
     
   solver_name: string
     Name of the solver
     
  """
  result = peano4.datamodel.DaStGen2( UpdateFaceLabel.get_attribute_name( solver_name ) )

  result.data.add_attribute( dastgen2.attributes.Boolean("Boundary") )
  result.data.add_attribute( dastgen2.attributes.Boolean("IsHanging") )
  result.data.add_attribute( dastgen2.attributes.Boolean("AboveHanging") )
  result.data.add_attribute( dastgen2.attributes.BooleanArray("Updated","2",compress=True) )
  result.data.add_attribute( peano4.dastgen2.Peano4DoubleArray("UpdatedTimeStamp","2") )
  result.data.add_attribute( peano4.dastgen2.Peano4DoubleArray("NewTimeStamp","2") )
  result.data.add_attribute( peano4.dastgen2.Peano4DoubleArray("OldTimeStamp","2") )
  
  result.peano4_mpi_and_storage_aspect.merge_implementation = """
  switch (context) {
    case ::peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange:
    case ::peano4::grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap:
      {
        _Boundary = _Boundary or neighbour._Boundary;
  
        const int normal         = marker.getSelectedFaceNumber() % Dimensions;
        const int neighbourEntry = marker.outerNormal()(normal)<0.0 ? 0 : 1;
  
        _Updated[ neighbourEntry ]          = neighbour._Updated[ neighbourEntry ];
        _UpdatedTimeStamp( neighbourEntry ) = neighbour._UpdatedTimeStamp( neighbourEntry );
        _NewTimeStamp( neighbourEntry )     = neighbour._NewTimeStamp( neighbourEntry );
        _OldTimeStamp( neighbourEntry )     = neighbour._OldTimeStamp( neighbourEntry );
      }
      break;
    default:
      assertionMsg( false, "not implemented yet" );
  }
"""

  return result

