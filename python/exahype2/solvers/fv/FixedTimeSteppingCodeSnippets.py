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
    and other stuff.
  
    """
    predicate = """
      tarch::mpi::Rank::getInstance().isGlobalMaster() 
      and
      _maxVolumeHThisTimeStep>0.0
      and
      isFirstGridSweepOfTimeStep() 
    """
  
    if self._use_enclave_tasking:
      predicate += """and (_solverState == SolverState::Primary or _solverState == SolverState::PrimaryAfterGridInitialisation) """
      
    return """
  if (""" + predicate + """) {
    logInfo( "step()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "step()", "t       = " << _minTimeStampThisTimeStep );
    logInfo( "step()", "dt      = " << getTimeStepSize() );
    logInfo( "step()", "h_{min} = " << _minVolumeHThisTimeStep << " (volume size)");
    logInfo( "step()", "h_{max} = " << _maxVolumeHThisTimeStep << " (volume size)" );
  }
"""


  def create_finish_time_step_implementation(self):
    return """
  if ( isLastGridSweepOfTimeStep() ) {
    assertion( _minVolumeH >= 0.0 );
    assertion( MaxAdmissibleVolumeH > 0.0 );
    if (_minVolumeHThisTimeStep <= MaxAdmissibleVolumeH) {
      _timeStepSize  = """ + str(self._normalised_time_step_size) + """ * _minVolumeHThisTimeStep / MaxAdmissibleVolumeH;
    }
    else {
      _timeStepSize = 0.0;
    }
  }
"""


