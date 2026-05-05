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
    return """
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

  def create_print_time_step_implementation(self):
    return """
  logDebug("logTimeStep()", "Phase  = " << toString(_solverState));
  logInfo("logTimeStep()", "t       = " << getMinTimeStamp() << " / " << MinTerminalTime);
  logInfo("logTimeStep()", "dt      = " << (getMinTimeStepSize() < std::numeric_limits<double>::max() / 10.0 ? getMinTimeStepSize() : getTimeStepSize()));
  logInfo("logTimeStep()", "h_{min} = " << getMinCellSize() << " (patch size)");
  logInfo("logTimeStep()", "h_{max} = " << getMaxCellSize() << " (patch size)");
"""
