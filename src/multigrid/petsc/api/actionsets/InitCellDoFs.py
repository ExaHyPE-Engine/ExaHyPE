# This file is part of the Peano's PETSc extension. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class InitCellDoFs(ActionSet):
  """!

Initialise degrees of freedom associated with cells
      
  """
  TemplateInitCell = """
  if ( fineGridCell{{SOLVER_NAME}}PETScData.getType() == celldata::{{SOLVER_NAME}}PETScData::Type::Interior ) {
    int dof = 0;
    dfor( k, {{POLYNOMIAL_DEGREE+1}} ) {
      {% if CELL_UNKNOWNS>1 %}
      xxxx
      {% else %}
      double value, rhs, exactSol;
      tarch::la::Vector<Dimensions,double> x = marker.getOffset();
      for (int d=0; d<Dimensions; d++) {
        x(d) += repositories::{{SOLVER_INSTANCE}}.QuadraturePointsInUnitInterval[ k(d) ] * marker.h()(d);
      }
      repositories::{{SOLVER_INSTANCE}}.initNode(
        x,
        marker.h(),
        value,
        rhs,
        exactSol
      );
      fineGridCell{{SOLVER_NAME}}.setValue(dof,value);
      fineGridCell{{SOLVER_NAME}}.setRhs(dof,rhs);
      fineGridCell{{SOLVER_NAME}}.setExactSol(dof,exactSol);
      dof++;
      {% endif %}
    }
  }
"""
  
  def __init__(self,
               solver):
    """!
    
Initialise vertex-associated degrees of freedom
    
The initialisation requires a solver object, as we have to know what C++
object this solver will produce.

solver: petsc.solvers.CollocatedLowOrderDiscretisation or similar solver where
  degrees of freedom are assigned exclusively to the vertices.
    
    """
    super( InitCellDoFs, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["CELL_UNKNOWNS"]      = solver.cell_unknowns
    self.d["POLYNOMIAL_DEGREE"]  = solver.polynomial_degree


  def get_body_of_operation(self,operation_name):
    """!

Provide C++ code snippet for peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME  
  
Only touchVertexFirstTime is an event where this action set actually
does something: It inserts the template TemplateInitVertex and 
replaces it with entries from the dictionary. The latter is befilled
in init().
    
    """
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.TemplateInitCell).render(**self.d)
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
"""

