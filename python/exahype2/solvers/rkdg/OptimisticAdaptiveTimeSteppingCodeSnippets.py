# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.OptimisticAdaptiveTimeSteppingCodeSnippets


class OptimisticAdaptiveTimeSteppingCodeSnippets( exahype2.solvers.OptimisticAdaptiveTimeSteppingCodeSnippets ):
  """
  
  Code snippet generator for fixed time stepping in the Runge-Kutta schemes
  
  """
  def __init__(self, time_step_relaxation):
    self._time_step_relaxation = time_step_relaxation


  def create_start_time_step_implementation(self):
    return """
  if (
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxGridCellHThisTimeStep>0.0
    and
    isFirstGridSweepOfTimeStep()
  ) {
    logInfo( "startTimeStep()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "startTimeStep()", "t              = " << _minTimeStampThisTimeStep );
    logInfo( "startTimeStep()", "dt             = " << getAdmissibleTimeStepSize() );
    if (_cancelOptimisticTasks) {
      logInfo( "startTimeStep()", "dt_{predicted} = " << getPredictedAdmissibleTimeStepSize() << " [cancel predicted tasks]" );
    }
    else {
      logInfo( "startTimeStep()", "dt_{predicted} = " << getPredictedAdmissibleTimeStepSize() );
    }
    logInfo( "startTimeStep()", "h_{min}        = " << _minGridCellHThisTimeStep << " (individual grid cell within a patch)");
    logInfo( "startTimeStep()", "h_{max}        = " << _maxGridCellHThisTimeStep << " (individual grid cell within a patch)" );
    logInfo( "startTimeStep()", "lambda_{max}   = " << _maxEigenvalue );
  }

  if (isFirstGridSweepOfTimeStep()) {
    _maxEigenvalue         = 0.0;
  }
"""


  def create_finish_time_step_implementation(self):
    """
    
    The superclass takes the admissible cell size and divides it by the 
    maximum eigenvalue. The Finite Volume solvers however operate with 
    patches, i.e. we have to devide by the volume count per axis.
    
    """
    return super( OptimisticAdaptiveTimeSteppingCodeSnippets, self ).create_finish_time_step_implementation() + """
  if ( 
    isLastGridSweepOfTimeStep() 
    and 
    tarch::la::greater(_maxEigenvalue, 0.0 ) 
  ) {
    const double optimalTimeStepSize = """ + str(self._time_step_relaxation) + """ * _minGridCellHThisTimeStep / _maxEigenvalue / (2*{{RK_ORDER}}-1);
    
    const double Threshold = 0.95;
    
    if ( 
      tarch::la::smallerEquals(_predictedAdmissibleTimeStepSize,optimalTimeStepSize)
      and
      tarch::la::greaterEquals(_predictedAdmissibleTimeStepSize,optimalTimeStepSize*Threshold)
    ) {
      _admissibleTimeStepSize          = _predictedAdmissibleTimeStepSize;
      _predictedAdmissibleTimeStepSize = 0.5 * (optimalTimeStepSize + _predictedAdmissibleTimeStepSize);
      _cancelOptimisticTasks           = false;
    }
    else {
      _admissibleTimeStepSize          = optimalTimeStepSize;
      _predictedAdmissibleTimeStepSize = Threshold * optimalTimeStepSize;
      _cancelOptimisticTasks           = true;
    }
    
    if ( std::isnan(_admissibleTimeStepSize) or std::isinf(_admissibleTimeStepSize) ) {
      ::tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSize>0", "invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSize) );
    }
    if (tarch::la::smallerEquals(_admissibleTimeStepSize,0.0,1e-10) ) {
      logWarning( "finishTimeStep(...)", "degenerated time step size of " << std::to_string(_admissibleTimeStepSize) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug in the eigenvalue computation" );
    }
  }
"""
