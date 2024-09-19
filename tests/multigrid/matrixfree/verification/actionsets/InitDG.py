from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2
import mghype

class InitDofsDGTest1(ActionSet):
  """!
  Redoing this class for discontinuous galerkin solver.
  
  This time, we store values only in the cells and just 
  determine the type of vertex we see.
  """
  templateTouchVertexFirstTime = """

  //logTraceInWith3Arguments("InitDofsDG::touchVertexFirstTime", marker.toString(), isOnBoundary(marker.x()), fineGridVertex{{SOLVER_NAME}}.toString());

  vertexdata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getVertexDoFType(marker.x(),marker.h());

  switch(dofType)
  {

    case vertexdata::{{SOLVER_NAME}}::Type::Interior:
    {
      if ( marker.willBeRefined() )
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Coarse );
      }

      else
      {
        fineGridVertex{{SOLVER_NAME}}.setType( vertexdata::{{SOLVER_NAME}}::Type::Interior );
      }
    }
    break;

    case vertexdata::{{SOLVER_NAME}}::Type::Coarse:
      assertionMsg(false, "should not be returned by user" );
      break;

    case vertexdata::{{SOLVER_NAME}}::Type::Undefined:
      assertionMsg(false, "should not be returned by user" );
      break;
  }


  //logTraceOutWith2Arguments("InitDofsDG::touchVertexFirstTime", marker.toString(), fineGridVertex{{SOLVER_NAME}}.toString());
   """

  templateTouchCellFirstTime = """
  logTraceInWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());

  auto insertIfNotPresent = []( std::vector<int>& v, int i ) {
    if( std::find( v.begin(), v.end(), i ) == v.end() ) v.push_back(i);
  };

  // anonymous functions to calculate cell indices which are on the boundary
  std::vector<int> indicesOnBoundary;

  for (int f=0; f<TwoTimesD; f++) {
    if( fineGridFaces{{SOLVER_NAME}}(f).getType() == facedata::{{SOLVER_NAME}}::Type::Boundary ) {
      // this face is on the boundary. Gather the cell dof indices this corresponds to

      switch(f){
        case 0: {
          for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.PolyDegree+1; i++) insertIfNotPresent( indicesOnBoundary, (repositories::{{SOLVER_INSTANCE}}.PolyDegree+1 + 1)*i );
        } break;

        case 1: {
          for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.PolyDegree+1; i++) insertIfNotPresent( indicesOnBoundary, i );
        } break;

        case 2: {
          for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.PolyDegree+1; i++) insertIfNotPresent( indicesOnBoundary, repositories::{{SOLVER_INSTANCE}}.PolyDegree + (repositories::{{SOLVER_INSTANCE}}.PolyDegree+1 + 1)*i );
        } break;

        case 3: {
          for (int i=(repositories::{{SOLVER_INSTANCE}}.PolyDegree+1)*repositories::{{SOLVER_INSTANCE}}.PolyDegree; i<(repositories::{{SOLVER_INSTANCE}}.PolyDegree+1)*(repositories::{{SOLVER_INSTANCE}}.PolyDegree+1); i++) insertIfNotPresent( indicesOnBoundary, i );
        } break;

      }
    
    }
  }

  auto indexIsOnBoundary = [&indicesOnBoundary](int i) -> bool { return not( indicesOnBoundary.empty() ) and std::find( indicesOnBoundary.begin(), indicesOnBoundary.end(), i ) != indicesOnBoundary.end() ; };

  celldata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getCellDoFType(marker.x(),marker.h());
  
  /*
  extremely hacky static random number generator
  */
  static std::mt19937 rng(0); // seed with 0 for reproducibility
  static std::uniform_real_distribution<> dis(0.0, 1.0);

  switch(dofType)
  {
    case celldata::{{SOLVER_NAME}}::Type::Outside:
    {
      if ( marker.willBeRefined() ) // make it coarse
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Outside );
      }
    }
    break;

    case celldata::{{SOLVER_NAME}}::Type::Interior:
    {
      if ( marker.willBeRefined() )
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else 
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Interior );

        // create vectors to send to implementation
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > solution;
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > rhs;
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > residual;

        // first - fill the solution vector with randoms
        for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.CellUnknowns; i++) {
          if ( indexIsOnBoundary(i) ) {
            // ...
          }
          else {
            // ...
          }
          solution[i] = dis(rng);

          // and residual to 0...
          residual[i] = 0;
        }

        // set rhs and update when we have all the projections...
        rhs =  repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()) * solution;

        // send to mesh...
        fineGridCell{{SOLVER_NAME}}.setSolution( solution );
        fineGridCell{{SOLVER_NAME}}.setRhs( rhs );
        fineGridCell{{SOLVER_NAME}}.setResidual( residual );
        fineGridCell{{SOLVER_NAME}}.setRand( solution );

        // now we need to compute all projections...
        auto faceProjections = repositories::{{SOLVER_INSTANCE}}. getFaceFromCellMatrix(marker.x(), marker.h()) * solution;

        for (int f=0; f<TwoTimesD; f++) {
          int startIndexProjection = 
            f < Dimensions ?
            repositories::{{SOLVER_INSTANCE}}.NodesPerFace * repositories::{{SOLVER_INSTANCE}}.ProjectionsPerFaceNode :
            0;
          for (int p=0; p<repositories::{{SOLVER_INSTANCE}}.NodesPerFace * repositories::{{SOLVER_INSTANCE}}.ProjectionsPerFaceNode; p++) {
            fineGridFaces{{SOLVER_NAME}}(f).setProjection(
              p + startIndexProjection,
              faceProjections( f*repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection + p + startIndexProjection )
            );

          }
          
        }

      }
    }
    break;

  }
  
  logTraceOutWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
   """

  templateTouchFaceFirstTime = """
  logTraceInWith2Arguments("InitDofs::touchFaceFirstTime", marker.toString(), fineGridFace{{SOLVER_NAME}}.toString());

  facedata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getFaceDoFType(marker.x(),marker.h());

  switch(dofType)
  {
    case facedata::{{SOLVER_NAME}}::Type::Outside:
    {
      if ( marker.willBeRefined() ) // make it coarse
      {
        fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Outside );
      }
    } break;

    case facedata::{{SOLVER_NAME}}::Type::Interior:
    {
      if ( marker.willBeRefined() )
      {
        fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else 
      {
        fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Interior );

        // vectors to be passed into face
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution,   double > sol;
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection, double > projection;

        // set all of these values to 0
        for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; i++)
          sol[i] = 0;

        for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection; i++)
          projection[i] = 0;
        

        // put into mesh
        fineGridFace{{SOLVER_NAME}}.setSolution(sol);
        fineGridFace{{SOLVER_NAME}}.setProjection(projection);

      }
    } break;

    case facedata::{{SOLVER_NAME}}::Type::Boundary:
    {
      if ( marker.willBeRefined() )
      {
        fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else 
      {
        fineGridFace{{SOLVER_NAME}}.setType( facedata::{{SOLVER_NAME}}::Type::Boundary );

        // vectors to be passed into face
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution,   double > sol;
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection, double > projection;

        // set all of these values to 0
        for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; i++)
          sol[i] = 0;

        for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection; i++)
          projection[i] = 0;

        // put into mesh
        fineGridFace{{SOLVER_NAME}}.setSolution(sol);
        fineGridFace{{SOLVER_NAME}}.setProjection(projection);
      }
    } break;
    

  }
  
  logTraceOutWith2Arguments("InitDofs::touchFaceFirstTime", marker.toString(), fineGridFace{{SOLVER_NAME}}.toString());
   """
  
  def __init__(self,
               solver):
    super( InitDofsDGTest1, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    # self.d["VERTEX_CARDINALITY"] = solver._unknowns_per_vertex

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = jinja2.Template(self.templateTouchVertexFirstTime).render(**self.d)
      pass 
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
      pass
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = jinja2.Template(self.templateTouchFaceFirstTime).render(**self.d)
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
    return __name__.replace(".py", "").replace(".", "_")
    
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
#include<numeric> // for std::iota
#include <random>
"""

class InitDofsIntermediatePhaseTest1(ActionSet):
  """!
  Perform some intermediate actions in test 1, namely:
    - Fix the face solution, with projections that we get from the init phase
    - Fix the rhs, using new face solution projected back into cell

  This action set will run first, so we can fix the face solution (perhaps redundantly,
  as the first TFFT in the solver will do this anyway), and then we fix the rhs during 
  the TCFT of this action set, which will be run first.

  We only want to do this once, so we have a static int to keep count...
  """

  templateEndTraversal="""
  Counter++;
  """

  templateTouchFaceFirstTime="""
  if ( 
    fineGridFace{{SOLVER_NAME}}.getType() == facedata::{{SOLVER_NAME}}::Type::Interior
    and
    Counter==0
  ) {
    logTraceInWith2Arguments( "updateFaceSolution::Interior", fineGridFace{{SOLVER_NAME}}.getSolution(), marker.toString() );

    // project the u^\pm into a temporary vector
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution, double > projection = 
      repositories::{{SOLVER_INSTANCE}}.getRiemannMatrix() * fineGridFace{{SOLVER_NAME}}.getProjection();

    for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; i++)
      fineGridFace{{SOLVER_NAME}}.setSolution(
        i,
        repositories::{{SOLVER_INSTANCE}}.OmegaFace * projection(i)
      );
    logTraceOutWith1Argument( "updateFaceSolution::Interior", fineGridFace{{SOLVER_NAME}}.getSolution() );
  }

  else if (
    fineGridFace{{SOLVER_NAME}}.getType() == facedata::{{SOLVER_NAME}}::Type::Boundary
    and
    Counter==0
  ) {
    logTraceInWith1Argument( "updateFaceSolution::InteriorPenalty", fineGridFace{{SOLVER_NAME}}.getProjection() );
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

    logTraceOut( "updateFaceSolution::InteriorPenalty");
  }


"""

  templateTouchCellFirstTime="""
    if ( 
    fineGridCell{{SOLVER_NAME}}.getType() != celldata::{{SOLVER_NAME}}::Type::Coarse
    and
    Counter==0
  ) {
    // now let's finish updating rhs
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution*TwoTimesD, double > faceSol;
    for (int f=0; f<TwoTimesD; f++)
    for (int s=0; s<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; s++)
      faceSol( f*repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution + s ) = fineGridFaces{{SOLVER_NAME}}(f).getSolution(s);

    // next, we need to project solution on face onto the cell...
    auto projection = repositories::{{SOLVER_INSTANCE}}.getCellFromFaceMatrix(marker.x(), marker.h()) * faceSol;

    fineGridCell{{SOLVER_NAME}}.setRhs( fineGridCell{{SOLVER_NAME}}.getRhs( ) + projection );

  }
  """

  def __init__(self, solver):
    super(InitDofsIntermediatePhaseTest1, self).__init__(0, False)
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()

  def get_attributes(self):
    return """
    static int Counter;
  """

  def get_static_initialisations(self, full_qualified_classname):
    return f"""
  int {full_qualified_classname}::Counter = 0;
    """
  
  def get_body_of_operation(self, operation_name):
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      return jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      return jinja2.Template(self.templateTouchFaceFirstTime).render(**self.d)
    if operation_name==peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
      return jinja2.Template(self.templateEndTraversal).render(**self.d)
    return ""
  
  def get_action_set_name(self):
    """!
    
    Configure name of generated C++ action set
    
    This action set will end up in the directory observers with a name that
    reflects how the observer (initialisation) is mapped onto this action 
    set. The name pattern is ObserverName2ActionSetIdentifier where this
    routine co-determines the ActionSetIdentifier. We make is reflect the
    Python class name.
     
    """
    return __name__.replace(".py", "").replace(".", "_") + "IntermediateSolve"
  
  def user_should_modify_template(self):
    return False
  
  def get_includes(self):
    """!
   
    """    
    return """
#include "repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
"""


class InitDofsDGTest6(InitDofsDGTest1):
  """!
  Inherit from class above and modify one template.
  The only difference is that here we set the initial guess to 0.
  """

  def __init__(self,
               solver):
    super( InitDofsDGTest6, self ).__init__(solver)

  templateTouchCellFirstTime = """
  logTraceInWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());

    auto insertIfNotPresent = []( std::vector<int>& v, int i ) {
    if( std::find( v.begin(), v.end(), i ) == v.end() ) v.push_back(i);
  };

  // anonymous functions to calculate cell indices which are on the boundary
  std::vector<int> indicesOnBoundary;

  for (int f=0; f<TwoTimesD; f++) {
    if( fineGridFaces{{SOLVER_NAME}}(f).getType() == facedata::{{SOLVER_NAME}}::Type::Boundary ) {
      // this face is on the boundary. Gather the cell dof indices this corresponds to

      switch(f){
        case 0: {
          for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.PolyDegree+1; i++) insertIfNotPresent( indicesOnBoundary, (repositories::{{SOLVER_INSTANCE}}.PolyDegree + 1)*i );
        } break;

        case 1: {
          for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.PolyDegree+1; i++) insertIfNotPresent( indicesOnBoundary, i );
        } break;

        case 2: {
          for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.PolyDegree+1; i++) insertIfNotPresent( indicesOnBoundary, repositories::{{SOLVER_INSTANCE}}.PolyDegree + (repositories::{{SOLVER_INSTANCE}}.PolyDegree + 1)*i );
        } break;

        case 3: {
          for (int i=(repositories::{{SOLVER_INSTANCE}}.PolyDegree+1)*repositories::{{SOLVER_INSTANCE}}.PolyDegree; i<(repositories::{{SOLVER_INSTANCE}}.PolyDegree+1)*(repositories::{{SOLVER_INSTANCE}}.PolyDegree+1); i++) insertIfNotPresent( indicesOnBoundary, i );
        } break;

      }
    
    }
  }

  auto indexIsOnBoundary = [&indicesOnBoundary](int i) -> bool { return not( indicesOnBoundary.empty() ) and std::find( indicesOnBoundary.begin(), indicesOnBoundary.end(), i ) != indicesOnBoundary.end() ; };

  celldata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getCellDoFType(marker.x(),marker.h());
  
  /*
  extremely hacky static random number generator
  */
  static std::mt19937 rng(0); // seed with 0 for reproducibility
  static std::uniform_real_distribution<> dis(0.0, 1.0);

  switch(dofType)
  {
    case celldata::{{SOLVER_NAME}}::Type::Outside:
    {
      if ( marker.willBeRefined() ) // make it coarse
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Outside );
      }
    }
    break;

    case celldata::{{SOLVER_NAME}}::Type::Interior:
    {
      if ( marker.willBeRefined() )
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else 
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Interior );

        // create vectors to send to implementation
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > solution;
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > rhs;
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > residual;
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > rand;

        // first - fill the solution vector with randoms
        for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.CellUnknowns; i++) {
          if ( indexIsOnBoundary(i) ) {
            rand[i] = 0.0; // to satisfy the boundary conditions
          }
          else {
            rand[i] = dis(rng);
            // solution[i] = std::sin( 2*tarch::la::PI * marker.x()(0) ) * std::sin( 2*tarch::la::PI * marker.x()(1) );
          }

          // and residual to 0...
          residual[i] = 0;
          solution[i] = 0;
        }

        // save the random field
        fineGridCell{{SOLVER_NAME}}.setRand( rand );

        // set rhs...
        rhs = repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()) * rand;

        // now we need to compute all projections...
        auto faceProjections = repositories::{{SOLVER_INSTANCE}}. getFaceFromCellMatrix(marker.x(), marker.h()) * rand;

        for (int f=0; f<TwoTimesD; f++) {
          int startIndexProjection = 
            f < Dimensions ?
            repositories::{{SOLVER_INSTANCE}}.NodesPerFace * repositories::{{SOLVER_INSTANCE}}.ProjectionsPerFaceNode :
            0;
          for (int p=0; p<repositories::{{SOLVER_INSTANCE}}.NodesPerFace * repositories::{{SOLVER_INSTANCE}}.ProjectionsPerFaceNode; p++) {
            fineGridFaces{{SOLVER_NAME}}(f).setRandProjection(
              p + startIndexProjection,
              faceProjections( f*repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection + p + startIndexProjection )
            );
            }

          }

        // send to mesh...
        fineGridCell{{SOLVER_NAME}}.setSolution( solution );
        fineGridCell{{SOLVER_NAME}}.setRhs( rhs );
        fineGridCell{{SOLVER_NAME}}.setResidual( residual );

      }
    }
    break;

  }
  
  logTraceOutWith2Arguments("InitDofs::touchCellFirstTime", marker.toString(), fineGridCell{{SOLVER_NAME}}.toString());
   """

class InitDofsIntermediatePhaseTest6(InitDofsIntermediatePhaseTest1):
  """
  Inherit from the class for test1.
  """
  templateTouchCellFirstTime="""
    if ( 
    fineGridCell{{SOLVER_NAME}}.getType() != celldata::{{SOLVER_NAME}}::Type::Coarse
    and
    Counter==0
  ) {
    // now let's finish updating rhs
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution*TwoTimesD, double > faceSol;
    for (int f=0; f<TwoTimesD; f++)
    for (int s=0; s<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; s++)
      faceSol( f*repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution + s ) = fineGridFaces{{SOLVER_NAME}}(f).getRandSolution(s);

    // next, we need to project solution on face onto the cell...
    auto projection = repositories::{{SOLVER_INSTANCE}}.getCellFromFaceMatrix(marker.x(), marker.h()) * faceSol;

    fineGridCell{{SOLVER_NAME}}.setRhs( fineGridCell{{SOLVER_NAME}}.getRhs( ) + projection );
  }
  """

  templateTouchFaceFirstTime="""
  if ( 
    fineGridFace{{SOLVER_NAME}}.getType() == facedata::{{SOLVER_NAME}}::Type::Interior
    and
    Counter==0
  ) {

    // project the u^\pm into a temporary vector
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution, double > projection = 
      repositories::{{SOLVER_INSTANCE}}.getRiemannMatrix() * fineGridFace{{SOLVER_NAME}}.getRandProjection();

    for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; i++)
      fineGridFace{{SOLVER_NAME}}.setRandSolution(
        i,
        repositories::{{SOLVER_INSTANCE}}.OmegaFace * projection(i)
      );
  }

  else if (
    fineGridFace{{SOLVER_NAME}}.getType() == facedata::{{SOLVER_NAME}}::Type::Boundary
    and
    Counter==0
  ) {
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
        rowSolution += boundaryMatrix( row, col ) * fineGridFace{{SOLVER_NAME}}.getRandProjection( col + startIndexProjection );
      }
      // place this solution into the face
      fineGridFace{{SOLVER_NAME}}.setRandSolution( row, rowSolution );
    }

  }

  """

class InitDofsDGTest4(InitDofsDGTest1):
  def __init__(self,
               solver):
    super( InitDofsDGTest4, self ).__init__(solver)

    
  templateTouchCellFirstTime="""
  celldata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getCellDoFType(marker.x(),marker.h());
  
  /*
  extremely hacky static random number generator
  */
  static std::mt19937 rng(1); // seed with 1 for reproducibility. make sure it's different to the numbers we put in the vertices
  static std::uniform_real_distribution<> dis(0.0, 1.0);

  switch(dofType)
  {
    case celldata::{{SOLVER_NAME}}::Type::Outside:
    {
      if ( marker.willBeRefined() ) // make it coarse
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Outside );
      }
    }
    break;

    case celldata::{{SOLVER_NAME}}::Type::Interior:
    {
      if ( marker.willBeRefined() )
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else 
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Interior );

        // create vectors to send to implementation
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > solution;
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > rhs;
        tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > residual;

        // first - fill the solution vector with randoms
        for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.CellUnknowns; i++) {
          solution[i] = dis(rng);

          // and residual to 0...
          residual[i] = 0;
        }

        // set rhs...
        rhs =  repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()) * solution;

        // send to mesh...
        fineGridCell{{SOLVER_NAME}}.setSolution( solution );
        fineGridCell{{SOLVER_NAME}}.setRhs( rhs );
        fineGridCell{{SOLVER_NAME}}.setResidual( residual );

      }
    }
    break;

  }
  
  """

class InitDofsDGTest5(InitDofsDGTest1):

  templateTouchCellFirstTime="""
  celldata::{{SOLVER_NAME}}::Type dofType = repositories::{{SOLVER_INSTANCE}}.getCellDoFType(marker.x(),marker.h());
  
  switch(dofType)
  {
    case celldata::{{SOLVER_NAME}}::Type::Outside:
    {
      if ( marker.willBeRefined() ) // make it coarse
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Outside );
      }
    }
    break;

    case celldata::{{SOLVER_NAME}}::Type::Interior:
    {
      if ( marker.willBeRefined() )
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else 
      {
        fineGridCell{{SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Interior );

      }
    }
    break;

  }
  
  """
  def __init__(self,
               solver):
    super( InitDofsDGTest5, self ).__init__(solver)

class InitDofsDGTest7(InitDofsDGTest1):

  templateTouchCellFirstTime="""
  constexpr int Cols = TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns;
  constexpr int Rows = {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode;

  static const tarch::la::Matrix< Rows, Cols, double > prolongationMatrix = 
    {
      {{PROLONGATION_MATRIX[0]| join(", ")}}
        {% for ROW in PROLONGATION_MATRIX[1:] %}
        ,{{ROW | join(", ")}}
        {% endfor %}
    };

  celldata::{{DG_SOLVER_NAME}}::Type dofType = repositories::{{DG_SOLVER_INSTANCE}}.getCellDoFType(marker.x(),marker.h());
  
  switch(dofType)
  {
    case celldata::{{DG_SOLVER_NAME}}::Type::Outside:
    {
      if ( marker.willBeRefined() ) // make it coarse
      {
        fineGridCell{{DG_SOLVER_NAME}}.setType( celldata::{{DG_SOLVER_NAME}}::Type::Coarse );
      }
      else
      {
        fineGridCell{{DG_SOLVER_NAME}}.setType( celldata::{{DG_SOLVER_NAME}}::Type::Outside );
      }
    }
    break;

    case celldata::{{SOLVER_NAME}}::Type::Interior:
    {
      if ( marker.willBeRefined() )
      {
        fineGridCell{{DG_SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Coarse );
      }
      else 
      {
        fineGridCell{{DG_SOLVER_NAME}}.setType( celldata::{{SOLVER_NAME}}::Type::Interior );

        // also gather the cg values
        tarch::la::Vector<TwoPowerD, double> u_cg_random;
        for (int i=0; i<TwoPowerD; i++) u_cg_random(i) = fineGridVertices{{CG_SOLVER_NAME}}(i).getRand(0);
        auto v_cell = prolongationMatrix * u_cg_random;

        // next, project v_cell onto each of the faces
        tarch::la::Vector< TwoTimesD * repositories::{{DG_SOLVER_INSTANCE}}.FaceUnknownsProjection, double > faceProjections = repositories::{{SOLVER_INSTANCE}}. getFaceFromCellMatrix(marker.x(), marker.h()) * v_cell;
        for (int f=0; f<TwoTimesD; f++) {
          int startIndexProjection = 
            f < Dimensions ?
            repositories::{{DG_SOLVER_INSTANCE}}.NodesPerFace * repositories::{{DG_SOLVER_INSTANCE}}.ProjectionsPerFaceNode :
            0;
          for (int p=0; p<repositories::{{DG_SOLVER_INSTANCE}}.NodesPerFace * repositories::{{DG_SOLVER_INSTANCE}}.ProjectionsPerFaceNode; p++) {
            fineGridFaces{{DG_SOLVER_NAME}}(f).setRandProjections(
              p + startIndexProjection,
              faceProjections( f*repositories::{{DG_SOLVER_INSTANCE}}.FaceUnknownsProjection + p + startIndexProjection )
            );

          }
        }

        // also begin to set rhs
        fineGridCell{{DG_SOLVER_NAME}}.setRhs(
          repositories::{{DG_SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()) * v_cell
        );

        // set the initial guess to 0
        fineGridCell{{DG_SOLVER_NAME}}.setSolution( 
          0.0
        );

      }
    }
    break;

  }
  
  """

  templateTouchFaceLastTime="""
  // perform averaging on the faces
  if (
    fineGridFace{{DG_SOLVER_NAME}}.getType() == facedata::{{DG_SOLVER_NAME}}::Type::Interior
  ) {
    // project the u^\pm into a temporary vector
    tarch::la::Vector< repositories::{{DG_SOLVER_INSTANCE}}.FaceUnknownsSolution, double > projection = 
      repositories::{{DG_SOLVER_INSTANCE}}.getRiemannMatrix() * fineGridFace{{DG_SOLVER_NAME}}.getRandProjections();

    for (int i=0; i<repositories::{{DG_SOLVER_INSTANCE}}.FaceUnknownsSolution; i++)
      fineGridFace{{DG_SOLVER_NAME}}.setRandSolutions(
        i,
        projection(i)
      );
  }

  else if (
    fineGridFace{{DG_SOLVER_NAME}}.getType() == facedata::{{DG_SOLVER_NAME}}::Type::Boundary
  ) {
    // Essentially what we do here is fix the solution on the boundary to be 
    // the inner-side projection.

    // skip halfway along the projection vector if we are on face 0 or 1
    int startIndexProjection = 
      marker.getSelectedFaceNumber() < Dimensions ?
      repositories::{{DG_SOLVER_INSTANCE}}.NodesPerFace * repositories::{{DG_SOLVER_INSTANCE}}.ProjectionsPerFaceNode :
      0;

    auto boundaryMatrix = repositories::{{DG_SOLVER_INSTANCE}}.getBoundaryConditionMatrix();

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
        rowSolution += boundaryMatrix( row, col ) * fineGridFace{{DG_SOLVER_NAME}}.getRandProjections( col + startIndexProjection );
      }
      // place this solution into the face
      fineGridFace{{DG_SOLVER_NAME}}.setRandSolutions( row, rowSolution );
    }

  }

  """

  def get_prolongation_matrix(self):
    intergrid_operators = mghype.api.matrixgenerators.blockmatrix.IntergridOperators(
      dim = 2,
      coarse_order = 1,
      fine_order = self.dg_solver._poly_degree
      )
    return intergrid_operators.prolongation()

  def __init__(self,
               dg_solver,
               ):
    super( InitDofsDGTest7, self ).__init__(dg_solver)
    self.dg_solver = dg_solver
    self.d["DG_SOLVER_INSTANCE"]    = dg_solver.instance_name()
    self.d["DG_SOLVER_NAME"]        = dg_solver.typename()
    self.d["CG_SOLVER_INSTANCE"]    = "instanceOfCollocatedPoisson"
    self.d["CG_SOLVER_NAME"]        = "CollocatedPoisson"
    self.d["PROLONGATION_MATRIX"]   = self.get_prolongation_matrix()

  def get_body_of_operation(self,operation_name):
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_LAST_TIME:
      return jinja2.Template(self.templateTouchFaceLastTime).render(**self.d)
    else:
      return super().get_body_of_operation(operation_name)
