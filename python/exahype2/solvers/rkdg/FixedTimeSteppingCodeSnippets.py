# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.FixedTimeSteppingCodeSnippets


class FixedTimeSteppingCodeSnippets( exahype2.solvers.FixedTimeSteppingCodeSnippets ):
  """
  
  Code snippet generator for fixed time stepping in the Runge-Kutta schemes
  
  """
  def __init__(self, normalised_time_step_size, use_enclave_tasking):
    self._normalised_time_step_size = normalised_time_step_size
    self._use_enclave_tasking       = use_enclave_tasking


  def create_start_time_step_implementation(self):
    """
  
    The outcome is used before we actually roll over the accumulation variables
    and other stuff. So we assume that the state is already updated, but the
    other data is not yet rolled-over.
    
    I don't want to plot the very first time stamp, where I don't know the 
    cell size yet, so I check _maxCellHThisTimeStep. If we don't know the cell
    size yet, we haven't done a single grid sweep. In this case, I don't plot
    anything. The other check isFirstGridSweepOfTimeStep() ensures that I only
    plot once prior to the first Runge-Kutta step. 
  
    """
    predicate = """
      tarch::mpi::Rank::getInstance().isGlobalMaster() 
      and
      _maxCellHThisTimeStep>0.0
      and
      isFirstGridSweepOfTimeStep() 
    """
  
    if self._use_enclave_tasking:
      predicate += """and (_solverState == SolverState::Primary or _solverState == SolverState::PrimaryAfterGridInitialisation) """
      
    return """
  if (""" + predicate + """) {
    logInfo( "startTimeStep()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "startTimeStep()", "t       = " << _minTimeStampThisTimeStep );
    logInfo( "startTimeStep()", "dt      = " << getTimeStepSize() );
    logInfo( "startTimeStep()", "h_{min} = " << _minCellHThisTimeStep );
    logInfo( "startTimeStep()", "h_{max} = " << _maxCellHThisTimeStep );
  }
"""


  def create_finish_time_step_implementation(self):
    return """
  if ( isLastGridSweepOfTimeStep() ) {
    assertion( _minCellH >= 0.0 );
    assertion( MaxAdmissibleCellH > 0.0 );
    if (_minCellHThisTimeStep <= MaxAdmissibleCellH) {
      _timeStepSize  = """ + str(self._normalised_time_step_size) + """ * _minCellHThisTimeStep / {{RK_ORDER}} / {{RK_ORDER}};
    }
    else {
      _timeStepSize = 0.0;
    }
  }
"""


