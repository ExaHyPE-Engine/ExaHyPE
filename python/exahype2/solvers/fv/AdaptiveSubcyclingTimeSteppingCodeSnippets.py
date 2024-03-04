# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.AdaptiveTimeSteppingCodeSnippets


class AdaptiveSubcyclingTimeSteppingCodeSnippets( exahype2.solvers.AdaptiveTimeSteppingCodeSnippets ):
  """
  
  Code snippet generator for fixed time stepping in the Runge-Kutta schemes
  
  """
  def __init__(self, time_step_relaxation, use_enclave_tasking):
    self._time_step_relaxation = time_step_relaxation
    self._use_enclave_tasking  = use_enclave_tasking


  def create_start_time_step_implementation(self):
    predicate = """
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxVolumeH>0.0
  """
  
    if use_enclave_tasking:
      predicate += """and (_solverState == SolverState::Primary or _solverState == SolverState::PrimaryAfterGridInitialisation) """
      
    statistics = """
  if (""" + predicate + """) {
    logInfo( "startTimeStep()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "startTimeStep()", "t_{min,global}     = " << _minTimeStamp );
    logInfo( "startTimeStep()", "t_{max,global}     = " << _maxTimeStamp );
    logInfo( "startTimeStep()", "t_{min,this-step}  = " << _minTimeStampThisTimeStep );
    logInfo( "startTimeStep()", "t_{max,this-step}  = " << _maxTimeStampThisTimeStep );
    if (_minTimeStepSize > _maxTimeStepSize ) {
      logInfo( "startTimeStep()", "dt_{min} = <not yet known>" );
      logInfo( "startTimeStep()", "dt_{max} = <not yet known>" );
    }
    else {
      logInfo( "startTimeStep()", "dt_{min,this-step} = " << _minTimeStepSize );
      logInfo( "startTimeStep()", "dt_{max,this-step} = " << _maxTimeStepSize );
    }
    logInfo( "startTimeStep()", "h_{min}      = " << _minVolumeH << " (volume size)");
    logInfo( "startTimeStep()", "h_{max}      = " << _maxVolumeH << " (volume size)" );
    logInfo( "startTimeStep()", "lambda_{max} = " << _maxEigenvalue );
    logInfo( "startTimeStep()", "#updates = " << _patchUpdates << " (no of patches)" );
  }
"""
    
    if use_enclave_tasking:
      clear_max_eigenvalue = """if (_solverState == SolverState::Primary or _solverState == SolverState::PrimaryAfterGridInitialisation) {
  _maxEigenvalue = 0.0;
}"""
    else:
      clear_max_eigenvalue = """if (_solverState == SolverState::TimeStep or _solverState == SolverState::TimeStepAfterGridInitialisation) {
  _maxEigenvalue = 0.0;
}"""

    return statistics + clear_max_eigenvalue


  def create_finish_time_step_implementation(self):
    """
    
    The superclass takes the admissible cell size and divides it by the 
    maximum eigenvalue. The Finite Volume solvers however operate with 
    patches, i.e. we have to devide by the volume count per axis.
    
    """
    code_snippet = super( AdaptiveTimeSteppingCodeSnippets, self ).create_finish_time_step_implementation() + """
  if ( tarch::la::greater(_maxEigenvalue, 0.0 ) ) {
    _admissibleTimeStepSize = """ + str(self._time_step_relaxation) + """ * _minVolumeHThisTimeStep / _maxEigenvalue;
    if ( std::isnan(_admissibleTimeStepSize) or std::isinf(_admissibleTimeStepSize) ) {
      ::tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSize>0", "invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSize) );
    }
    if (tarch::la::smallerEquals(_admissibleTimeStepSize,0.0,1e-10) ) {
      logWarning( "finishTimeStep(...)", "degenerated time step size of " << std::to_string(_admissibleTimeStepSize) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug in the eigenvalue computation" );
    }
  }
"""
    if self._use_enclave_tasking:
      return """
if (_solverState==SolverState::Secondary) {
""" + code_snippet + """
}
"""
    else:
      return code_snippet

