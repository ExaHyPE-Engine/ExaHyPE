# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .SolverCodeSnippets import SolverCodeSnippets


class FixedTimeSteppingCodeSnippets( SolverCodeSnippets ):
  """
  
  Code snippet generator for all fixed time stepping solvers
  
  """
      

  def create_abstract_solver_user_declarations(self):
    return """
private:
  double _timeStepSize;
public:
  double getTimeStepSize() const;  
    """  


  def create_abstract_solver_user_definitions(self):
    return """
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getTimeStepSize() const {
  return _timeStepSize;
}
    """  


  def create_abstract_solver_constructor_statements(self):
    return """
  _timeStepSize = 0.0;
"""    
  
  
  def create_compute_time_step_size(self):
    return """
  const double timeStepSize = repositories::{{SOLVER_INSTANCE}}.getTimeStepSize();
"""

  
  def create_compute_new_time_step_size(self):
    """
    
    This is global, fixed time stepping, i.e. the new time step size will likely
    be the same as the previous one, unless the mesh changes, as we work with 
    normalised time step sizes, i.e. in this case the time step size might change.
    Anyway, the new time step size is only for stats anyway, as we'll pick a 
    global one when we determine timeStepSize the next time step.
    
    """
    return """
  const double newTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getTimeStepSize();
"""

