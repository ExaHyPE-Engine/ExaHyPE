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
    const double maxAdmissiblePatchSize = ::exahype2::RuntimeParameters::getInstance().getMaxAdmissiblePatchSize(DomainSize);
    assertion( _minCellH >= 0.0 );
    assertion( maxAdmissiblePatchSize > 0.0 );
    if (_minCellHThisTimeStep <= maxAdmissiblePatchSize) {
      _timeStepSize  = """ + str(self._normalised_time_step_size) + """ * _minCellHThisTimeStep / {{RK_ORDER}} / {{RK_ORDER}};
    }
    else {
      _timeStepSize = 0.0;
    }
    if (tarch::la::greater(_timeStepSize, 0.0)) {
      if (_minTimeStepSizeThisTimeStep >= std::numeric_limits<double>::max() / 10.0) {
        _minTimeStepSizeThisTimeStep = _timeStepSize;
      }
      if (_maxTimeStepSizeThisTimeStep <= std::numeric_limits<double>::min() * 10.0) {
        _maxTimeStepSizeThisTimeStep = _timeStepSize;
      }
    }
  }
"""
