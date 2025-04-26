# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.FixedTimeSteppingCodeSnippets


class FixedTimeSteppingCodeSnippets( exahype2.solvers.FixedTimeSteppingCodeSnippets ):
  """
  Code snippet generator for fixed time stepping in the Runge-Kutta schemes
  """
  def __init__(self, normalised_time_step_size):
    self._normalised_time_step_size = normalised_time_step_size

  def create_print_time_step_implementation(self):
    return """
  if (_maxGridCellHThisTimeStep>0.0) {
    logInfo( "printTimeStep()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "printTimeStep()", "t       = " << _minTimeStampThisTimeStep );
    logInfo( "printTimeStep()", "dt      = " << getTimeStepSize() );
    logInfo( "printTimeStep()", "h_{min} = " << _minGridCellHThisTimeStep << " (individual grid cell within a patch)");
    logInfo( "printTimeStep()", "h_{max} = " << _maxGridCellHThisTimeStep << " (individual grid cell within a patch)" );
  }
"""

  def create_start_time_step_implementation(self):
    return """
"""


  def create_finish_time_step_implementation(self):
    return """
  if ( isLastGridSweepOfTimeStep() ) {
    assertion( _minGridCellH >= 0.0 );
    assertion( MaxAdmissibleGridCellH > 0.0 );
    if (_minGridCellHThisTimeStep <= MaxAdmissibleGridCellH) {
      _timeStepSize  = """ + str(self._normalised_time_step_size) + """ * _minGridCellHThisTimeStep / MaxAdmissibleGridCellH / {{RK_ORDER}} / {{RK_ORDER}};
    }
    else {
      _timeStepSize = 0.0;
    }
  }
"""


