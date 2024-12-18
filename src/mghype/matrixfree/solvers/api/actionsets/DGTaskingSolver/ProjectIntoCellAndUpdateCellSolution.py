from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2

class ProjectIntoCellAndUpdateCellSolution(ActionSet):
  templateTouchCellFirstTime="""
  if ( fineGridCell{{SOLVER_NAME}}.getType() != celldata::{{SOLVER_NAME}}::Type::Coarse ) {

    // wait for task assigned to this cell - already done this in begin traversal for the face update
    tarch::multicore::waitForTask( fineGridCell{{SOLVER_NAME}}.getTaskNumber( 1 ) );

    // This vector faceSol will contain: soln on face 0 | soln on face 1 | soln on face 2 | .....
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution*TwoTimesD, double > faceSol;
    for (int f=0; f<TwoTimesD; f++)
    for (int s=0; s<repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution; s++)
      faceSol( f*repositories::{{SOLVER_INSTANCE}}.FaceUnknownsSolution + s ) = fineGridFaces{{SOLVER_NAME}}Solution(f).get(s);

    // next, we need to project solution on face onto the cell...
    auto projection = repositories::{{SOLVER_INSTANCE}}.getCellFromFaceMatrix(marker.x(), marker.h()) * faceSol;
    // loop over each face, finish updating residual as we go
    for (int f=0; f<repositories::{{SOLVER_INSTANCE}}.CellUnknowns; f++)
    {
      // project the face solution onto the cell; add to r
      fineGridCell{{SOLVER_NAME}}Residual.set(f,
        fineGridCell{{SOLVER_NAME}}Residual.get(f)- projection(f));
    }

    // multiply by "smoother" / preconditioner
    fineGridCell{{SOLVER_NAME}}Residual.set(
      repositories::{{SOLVER_INSTANCE}}.getInvertedApproxSystemMatrix(marker.x(),marker.h()) 
      * fineGridCell{{SOLVER_NAME}}Residual.get()
    );

    for (int i=0; i<fineGridCell{{SOLVER_NAME}}Solution.get().size(); i++) {
      double res = fineGridCell{{SOLVER_NAME}}Residual.get( i );
      double du  = repositories::{{SOLVER_INSTANCE}}.OmegaCell * res;
      fineGridCell{{SOLVER_NAME}}Solution.set( i,
        fineGridCell{{SOLVER_NAME}}Solution.get( i ) + du
      );
      repositories::{{SOLVER_INSTANCE}}.updateGlobalResidual(res, marker.h());
      repositories::{{SOLVER_INSTANCE}}.updateGlobalSolutionUpdates(du, marker.h()(0));
    }

    // now time to spawn the tasks for the final two stages

    auto solutionPtr = fineGridCell{{SOLVER_NAME}}Solution.value;
    auto rhsPtr      = fineGridCell{{SOLVER_NAME}}Rhs.value;
    auto residualPtr = fineGridCell{{SOLVER_NAME}}Residual.value;

    // need to capture each of the pointers for the faces...
    std::vector< double* > faceProjectionPointers;
    for (int f=0; f<TwoTimesD; f++) {
      if ( f<Dimensions )
        faceProjectionPointers.push_back( fineGridFaces{{SOLVER_NAME}}RProjection(f).value );
      else
        faceProjectionPointers.push_back( fineGridFaces{{SOLVER_NAME}}LProjection(f).value );

    }

    const int residualVecSize = repositories::{{SOLVER_INSTANCE}}.CellUnknowns;

    std::function<bool()> projectOntoFacesFuntor = [solutionPtr, faceProjectionPointers, marker, residualVecSize]() {
      // no need to test for whether we are in Coarse cell or not, since we spawn from within
      // one and capture pointers

      // apply cell to face projection, store it in faceProjections
      // no tricky indexing needed here, since the whole of the cell
      // solution vector is in scope here.
      tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection * TwoTimesD, double > faceProjections = 
        repositories::{{SOLVER_INSTANCE}}. getFaceFromCellMatrix(marker.x(), marker.h()) * tarch::la::Vector< residualVecSize, double >( solutionPtr );

      
      // write these values into the faces themselves
      for (int f=0; f<TwoTimesD; f++)
      {
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
          // double*                   index                        locally calculated value
          (faceProjectionPointers[f])[ p ] = faceProjections( f*repositories::{{SOLVER_INSTANCE}}.FaceUnknownsProjection + p + startIndexProjection );
        }
        
      }

      // no need to rerun
      return false;
    };

    std::function<bool()> updateResidualFunctor = [solutionPtr, rhsPtr, marker, residualPtr, residualVecSize]() {
      // no need to test for whether we are in Coarse cell or not, since we spawn from within
      // one and capture everything by ref

      // set residual = Mb - A^cc u^C
      tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double > r =
        repositories::{{SOLVER_INSTANCE}}.getMassMatrix( marker.x(), marker.h() )        * tarch::la::Vector< residualVecSize, double >( rhsPtr )
      - repositories::{{SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()) * tarch::la::Vector< residualVecSize, double >( solutionPtr );

      for (int i=0; i<residualVecSize; i++)
        residualPtr[i] = r[i];

      // no need to rerun
      return false;

    };

    tarch::multicore::TaskWithCopyOfFunctor* task0 = new tarch::multicore::TaskWithCopyOfFunctor(
      tarch::multicore::Task::DontFuse,
      tarch::multicore::Task::DefaultPriority,
      projectOntoFacesFuntor
    );  
    
    tarch::multicore::TaskWithCopyOfFunctor* task1 = new tarch::multicore::TaskWithCopyOfFunctor(
      tarch::multicore::Task::DontFuse,
      tarch::multicore::Task::DefaultPriority,
      updateResidualFunctor
    );

    tarch::multicore::spawnTask(
      task0,
      tarch::multicore::NoInDependencies,
      fineGridCell{{SOLVER_NAME}}.getTaskNumber(0)
    );

    tarch::multicore::spawnTask(
      task1,
      tarch::multicore::NoInDependencies,
      fineGridCell{{SOLVER_NAME}}.getTaskNumber(1)
    );
  }
  """

  templateEndTraversal = """
  tarch::multicore::waitForAllTasks();
  """

  def __init__(self,
               solver,
               descend_invocation_order=0,
               parallel=True):
    super( ProjectIntoCellAndUpdateCellSolution, self ).__init__(
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
    if operation_name==peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
      result = jinja2.Template(self.templateEndTraversal).render(**self.d)
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
    return __name__.replace(".py", "").replace(".", "_") + "_ProjectIntoCellAndUpdateCellSolution"
    
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
#include "tarch/multicore/Task.h"
"""


