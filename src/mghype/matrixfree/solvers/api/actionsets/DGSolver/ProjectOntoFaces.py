from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class ProjectOntoFaces(ActionSet):
  """!
  
  This is for DG solver
  
  @todo There's documentation missing
  
  """

  templateTouchCellFirstTime="""
  /*
  all we need to do here is project updated solution u^c onto the faces
  */

  if ( 
    fineGridCell{{SOLVER_NAME}}.getType() != celldata::{{SOLVER_NAME}}::Type::Coarse 
    and
    repositories::{{SOLVER_INSTANCE}}.projectOntoFaces()
  ) {
    // apply cell to face projection, store it in faceProjections
    // no tricky indexing needed here, since the whole of the cell
    // solution vector is in scope here.
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection * TwoTimesD, double > faceProjections = 
      repositories::{{SOLVER_INSTANCE}}. getFaceFromCellMatrix(marker.x(), marker.h()) * fineGridCell{{SOLVER_NAME}}.getSolution();

    logTraceInWith3Arguments("ProjectOntoFace", marker.toString(), faceProjections, fineGridCell{{SOLVER_NAME}}.getSolution());
    
    // write these values into the faces themselves
    for (int f=0; f<TwoTimesD; f++)
    {
      logTraceInWith2Arguments("ProjectOntoFaces::FaceLoop", f, fineGridFaces{{SOLVER_NAME}}(f).getProjection())
      /*
      This loop is crucial - we have a lot of redundant zeros and we don't wanna overwrite eg negative
      projections when we are dealing with face 0. So, some tricky indexing needed
      */
      // skip halfway along the projection vector if we are on face 0 or 1
      int startIndexProjection = 
        f < Dimensions ?
        repositories::{{SOLVER_INSTANCE}}.NodesPerFace * repositories::{{SOLVER_INSTANCE}}.ProjectionsPerFaceNode :
        0;

      // previously, this loop would go up to total number of projections.
      // but we only want half this number, since we skip the projections
      // that should be written to by the other face
      for (int p=0; p<repositories::{{SOLVER_INSTANCE}}.NodesPerFace * repositories::{{SOLVER_INSTANCE}}.ProjectionsPerFaceNode; p++)
      {
        fineGridFaces{{SOLVER_NAME}}(f).setProjection(
          p + startIndexProjection,
          faceProjections( f*repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection + p + startIndexProjection )
        );
      }
      logTraceOutWith2Arguments("ProjectOntoFaces::FaceLoop", f, fineGridFaces{{SOLVER_NAME}}(f).getProjection())
      
    }

    logTraceOut("ProjectOntoFace");
  }
"""


  def __init__(self,
               solver,
               descend_invocation_order=0,
               parallel=True):
    super( ProjectOntoFaces, self ).__init__(
      descend_invocation_order,
      parallel
    )
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
      pass
    return result

  def get_action_set_name(self):
    """!
    
    Configure name of generated C++ action set
    
    This action set will end up in the directory observers with a name that
    reflects how the observer (initialisation) is mapped onto this action 
    set. The name pattern is ObserverName2ActionSetIdentifier where this
    routine co-determines the ActionSetIdentifier. We make is reflect the
    Python class name.
     
    """
    return __name__.replace(".py", "").replace(".", "_") + "_ProjectOntoFaces"
    
  def user_should_modify_template(self):
    """!
    
    The action set that Peano will generate that corresponds to this class
    should not be modified by users and can safely be overwritten every time
    we run the Python toolkit.
    
    """
    return False

  def get_includes(self):
    """!
   
    We need the solver repository in this action set, as we directly access
    the solver object. We also need access to Peano's d-dimensional loops.
         
    """    
    return """
#include "../repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
"""
