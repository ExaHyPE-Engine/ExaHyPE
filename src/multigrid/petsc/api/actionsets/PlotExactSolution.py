# This file is part of the Peano's PETSc extension. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class PlotExactSolution(ActionSet):
  """!

  For development purposes, we add a way to print the exact solution
  from the cells to a file.

  We do this lazily, without concerning ourselves with what indices 
  are available to us over mpi ranks. 

  We store everything in a std::vector, and print it out to file when
  we are done
  """

  templateTouchCellFirstTime = """
  if ( fineGridCell{{SOLVER_NAME}}PETScData.getType() == celldata::{{SOLVER_NAME}}PETScData::Type::Interior )
  {
    std::pair<int,int> localCellIndex = std::make_pair(_spacetreeId, fineGridCell{{SOLVER_NAME}}PETScData.getUnknownBaseNumber());
    int globalCellIndex = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localCellIndex, ::petsc::LocalToGlobalMap::Type::Cell);

    // fill the vector
    for (int i=0; i<repositories::{{SOLVER_INSTANCE}}.DoFsPerCell; i++)
    {
      // we will want this to be get exactSol
      exactSolutionAtEachCellDof[globalCellIndex+i] = fineGridCell{{SOLVER_NAME}}.getExactSol(i);
    }
  }

  """
  templateEndTraversal = """
  std::ofstream outfile("./exactSol.txt");
  for (const auto& s:exactSolutionAtEachCellDof) outfile << s << "\\n";
  outfile.close();

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
    super( PlotExactSolution, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()

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
      result = jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
      pass 

    if operation_name==peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
      result = jinja2.Template(self.templateEndTraversal).render(**self.d)
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
   
    Add some additional includes
    
    We need the solver repository here as we access hte solver object, and we
    also need access to all of Peano's d-dimensional for loops.
    
    """    
    return """
#include "repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
#include <fstream>
"""

  def get_attributes(self):
      """!

      Return attributes as copied and pasted into the generated class.

      Please note that action sets are not persistent, i.e. there is one
      object creation per grid sweep per tree.

      """
      return """
  int _spacetreeId;    
  std::vector<double> exactSolutionAtEachCellDof;
"""

  def get_constructor_body(self):
    body = """
  _spacetreeId = treeNumber;
  exactSolutionAtEachCellDof.resize( 
    repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getTotalNumberOfCells()
  );

"""

    return jinja2.Template(body).render(**self.d)