# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .SolverCodeSnippets import SolverCodeSnippets


class AdaptiveTimeSteppingCodeSnippets( SolverCodeSnippets ):
  """
  
  Code snippet generator for all fixed time stepping solvers
  
  """

  def create_abstract_solver_user_declarations(self):
    return """
protected:
  double _maxEigenvalue;
  double _admissibleTimeStepSize;
public:
  void setMaxEigenvalue( double eigenvalue );  
  /**
   * @return Admissible time step size for the current sweep, i.e. 
   *         return _admissibleTimeStepSize. This value always refers
   *         to the minimum mesh volume size. If you use subcycling,
   *         you have to scale it for cells that are not on the finest
   *         mesh resolution. 
   */
  virtual double getAdmissibleTimeStepSize() const;  
    """  


  def create_abstract_solver_user_definitions(self):
    return """
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::setMaxEigenvalue( double eigenvalue ) {
  if ( tarch::la::greater( eigenvalue, 0.0 ) ) {
    tarch::multicore::Lock lock(_semaphore);
    _maxEigenvalue = std::max(_maxEigenvalue,eigenvalue);
  }
}    


double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getAdmissibleTimeStepSize() const {
  return _admissibleTimeStepSize;
}
    """  


  def create_abstract_solver_constructor_statements(self):
    return "_admissibleTimeStepSize = 0.0;"
  
  
  def create_compute_time_step_size(self):
    return """
  double timeStepSize = repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize();
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
    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue( maxEigenvalue );  
    
    // This is a value set for stats reasons. We'll ignore it later as we 
    // ask for a new valid time step size from getAdmissibleTimeStepSize().
    const double newTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize();
"""


  def create_finish_time_step_implementation(self):
    """
  
    This routine is inserted after we have reduced all global quantities. These
    are the quantities with the postfix ThisTimeStep.
  
    """ 
    return """
  if ( isLastGridSweepOfTimeStep() ) {
    #ifdef Parallel
    double newMaxEigenvalue = _maxEigenvalue;
    tarch::mpi::Rank::getInstance().allReduce(
        &newMaxEigenvalue,
        &_maxEigenvalue,
        1,
        MPI_DOUBLE,
        MPI_MAX, 
        [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
        );
    #endif

    if ( tarch::la::smaller(_maxEigenvalue, 0.0 ) ) {
      ::tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "_maxEigenvalue>=0", "invalid max eigenvalue: " + std::to_string(_maxEigenvalue) );
      // keep time step size invariant
      // _admissibleTimeStepSize = _admissibleTimeStepSize;
    }
    else if ( tarch::la::equals(_maxEigenvalue, 0.0 ) ) {
      logWarning( "finishTimeStep(...)", "maximum eigenvalue approaches 0.0. For nonlinear PDEs, this often means the PDE becomes stationary. It could also be a bug however" ); 
      _admissibleTimeStepSize = 0.0;
    }
  }
"""  
