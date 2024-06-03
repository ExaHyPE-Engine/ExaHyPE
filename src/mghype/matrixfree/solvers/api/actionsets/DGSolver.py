from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2

class UpdateCell(ActionSet):
  """!

  This is for DG solver
  
  @todo Documntation missing
  
  """
  templateTouchCellFirstTime="""
  if ( 
    fineGridCell{{SOLVER_NAME}}.getType() != celldata::{{SOLVER_NAME}}::Type::Coarse 
    and
    repositories::{{SOLVER_INSTANCE}}.updateCell()
  ) {
    logTraceInWith4Arguments("touchCellFirstTime", 
      fineGridCell{{SOLVER_NAME}}.getSolution(),
      fineGridCell{{SOLVER_NAME}}.getRhs(),
      marker.toString(),
      repositories::{{SOLVER_INSTANCE}}.getMassMatrix( marker.x(), marker.h() )
    );

    // write Mb into it, i.e. mass matrix * rhs
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > r =
      repositories::{{SOLVER_INSTANCE}}.getMassMatrix( marker.x(), marker.h() ) * fineGridCell{{SOLVER_NAME}}.getRhs();

    // This vector faceSol will contain: soln on face 0 | soln on face 1 | soln on face 2 | .....
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution*TwoTimesD, double > faceSol;
    for (int f=0; f<TwoTimesD; f++)
    for (int s=0; s<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; s++)
      faceSol( f*repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution + s ) = fineGridFaces{{SOLVER_NAME}}(f).getSolution(s);
  
    logTraceInWith1Argument("UpdateCell::Projection", faceSol);
    // next, we need to project solution on face onto the cell...
    auto projection = repositories::{{SOLVER_INSTANCE}}.getCellFromFaceMatrix(marker.x(), marker.h()) * faceSol;
    // loop over each face
    for (int f=0; f<repositories::{{SOLVER_INSTANCE}}.CellUnknowns; f++)
    {
      // project the face solution onto the cell; add to r
      r(f) -= projection(f);
    }

    // also subtract A^cc u^c
    r = r - repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()) * fineGridCell{{SOLVER_NAME}}.getSolution();

    // residual is now complete

    // compute preconditioned max norm; now it's not used in the stopping criterion. todo: add it to updateGlobalResidual()
    //double precMaxNorm = tarch::la::max( repositories::{{SOLVER_INSTANCE}}.OmegaCell * repositories::{{SOLVER_INSTANCE}}.getInvertedApproxSystemMatrix(marker.x(),marker.h()) * r );

    repositories::{{SOLVER_INSTANCE}}.updateGlobalResidual(
      tarch::la::max( tarch::la::abs(r) ),
      tarch::la::volume(marker.h()) * tarch::la::max(tarch::la::abs(r)),
      tarch::la::norm2Squared(tarch::la::abs(r)),
      marker.h()(0)
    );

    logTraceOutWith2Arguments("UpdateCell::Projection", r, projection);

    // fetch inverted assembly matrix from abstract solver class. left-multiply r.
    r = repositories::{{SOLVER_INSTANCE}}.getInvertedApproxSystemMatrix(marker.x(),marker.h()) * r;

    logTraceInWith1Argument("UpdateCell::UpdateSol", fineGridCell{{SOLVER_NAME}}.getSolution());
    // scale current entries in solution vector by OmegaCell
    for (int i=0; i<fineGridCell{{SOLVER_NAME}}.getSolution().size(); i++)
    {
      double sol = fineGridCell{{SOLVER_NAME}}.getSolution(i);

      // add on r values whilst we are here
      fineGridCell{{SOLVER_NAME}}.setSolution(i, sol + repositories::{{SOLVER_INSTANCE}}.OmegaCell * r(i) );
    }
    logTraceOutWith1Argument("UpdateCell::UpdateSol", fineGridCell{{SOLVER_NAME}}.getSolution());

    logTraceOut("UpdateCell::touchCellFirstTime");
  }
  """


  def __init__(self,
               solver,
               descend_invocation_order=0,
               parallel=True):
    super( UpdateCell, self ).__init__(
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
    return __name__.replace(".py", "").replace(".", "_") + "_UpdateCell"
    
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
#include "repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
"""


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
#include "repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
"""


class UpdateFaceSolution(ActionSet):
  """!
  This is for DG solver.
  
  @todo Docu missing
  
  """


  templatetouchFaceFirstTime="""
  if ( 
    fineGridFace{{SOLVER_NAME}}.getType() == facedata::{{SOLVER_NAME}}::Type::Interior 
    and
    repositories::{{SOLVER_INSTANCE}}.updateFace()
  ) {
    // project the u^\pm into a temporary vector
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution, double > projection = 
      repositories::{{SOLVER_INSTANCE}}.getRiemannMatrix() * fineGridFace{{SOLVER_NAME}}.getProjection();

    for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; i++)
      fineGridFace{{SOLVER_NAME}}.setSolution(
        i,
        (1-repositories::{{SOLVER_INSTANCE}}.OmegaFace)*fineGridFace{{SOLVER_NAME}}.getSolution(i)
         + repositories::{{SOLVER_INSTANCE}}.OmegaFace * projection(i)
      );
  }

  else if ( fineGridFace{{SOLVER_NAME}}.getType() == facedata::{{SOLVER_NAME}}::Type::Boundary )
  {
    logTraceInWith1Argument( "InteriorPenalty", fineGridFace{{SOLVER_NAME}}.getProjection() );
    // Essentially what we do here is fix the solution on the boundary to be 
    // the inner-side projection.

    // skip halfway along the projection vector if we are on face 0 or 1
    int startIndexProjection = 
      marker.getSelectedFaceNumber() < Dimensions ?
      repositories::{{SOLVER_INSTANCE}}.NodesPerFace * repositories::{{SOLVER_INSTANCE}}.ProjectionsPerFaceNode :
      0;

    auto boundaryMatrix = repositories::{{SOLVER_INSTANCE}}.getBoundaryConditionMatrix();

    /*
    Here we do slightly messy matrix multiplication. We would just multiply the boundary matrix
    by the projection vector, and place that into the solution, but here we only want to capture
    half of the projection vector; the other half lies outside the computational domain and
    should be fixed to 0.
    */
    for (int row=0; row<boundaryMatrix.rows(); row++)
    {
      double rowSolution = 0;
      for (int col=0; col<boundaryMatrix.cols(); col++)
      {
        rowSolution += boundaryMatrix( row, col ) * fineGridFace{{SOLVER_NAME}}.getProjection( col + startIndexProjection );
      }
      // place this solution into the face
      fineGridFace{{SOLVER_NAME}}.setSolution( row, rowSolution );
    }

    logTraceOut( "InteriorPenalty");
  }


"""


  def __init__(self,
               solver,
               descend_invocation_order=0,
               parallel=True):
    super( UpdateFaceSolution, self ).__init__(
      descend_invocation_order,
      parallel
    )
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = jinja2.Template(self.templatetouchFaceFirstTime).render(**self.d)
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
    return __name__.replace(".py", "").replace(".", "_") + "_UpdateFaceSolution"
    
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
#include "repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
"""
