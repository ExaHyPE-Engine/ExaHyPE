from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2

class UpdateCell(ActionSet):
  """!
  This is for DG solver.
  """
  templateTCFT="""
  if ( fineGridCell{{SOLVER_NAME}}.getType() != celldata::{{SOLVER_NAME}}::Type::Coarse )
  {
    logTraceIn("UpdateCell");

    // create a temporary array r on the heap
    tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double >* r = new tarch::la::Vector< repositories::{{SOLVER_INSTANCE}}.CellUnknowns, double >();
    // write Mb into it, i.e. mass matrix * rhs
    *r =  repositories::{{SOLVER_INSTANCE}}.getMassMatrix( marker.x(), marker.h() ) * fineGridCell{{SOLVER_NAME}}.getRhs();

    // next, we need to project solution on face onto the cell and add it on 
    // loop over each face
    for (int f=0; f<TwoTimesD; f++)
    {
      // project the face solution onto the cell; add to r
      //*r += repositories::{{SOLVER_INSTANCE}}.getFaceToCellMatrix() * fineGridFaces{{SOLVER_NAME}}(f).getSolution();
    }

    // fetch inverted assembly matrix from abstract solver class. left-multiply r.
    *r = repositories::{{SOLVER_INSTANCE}}.getInvertedAssemblyMatrix() * ( *r );

    // scale current entries in solution vector by (1-OmegaCell)
    for (int i=0; i<fineGridCell{{SOLVER_NAME}}.getSolution().size(); i++)
    {
      double sol = fineGridCell{{SOLVER_NAME}}.getSolution(i) * (1-repositories::{{SOLVER_INSTANCE}}.OmegaCell);

      // add on r values whilst we are here
      fineGridCell{{SOLVER_NAME}}.setSolution(i, sol + (*r)(i) );
    }


    delete r;
    logTraceOut("UpdateCell");
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
    # if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
    #   result = jinja2.Template(self.templateTVFT).render(**self.d)
    #   pass 
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTCFT).render(**self.d)
      pass
    # if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
    #   result = jinja2.Template(self.templateTVLT).render(**self.d)
    #   pass
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
"""
