# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.FixedSubcyclingTimeSteppingCodeSnippets


class FixedSubcyclingTimeSteppingCodeSnippets( exahype2.solvers.FixedSubcyclingTimeSteppingCodeSnippets ):
  """
  
  Code snippet generator for fixed time stepping in the Runge-Kutta schemes
  
  """
  def __init__(self, normalised_time_step_size, use_enclave_tasking, remove_accumulation_errors):
    super( FixedSubcyclingTimeSteppingCodeSnippets, self ).__init__(remove_accumulation_errors) 
    
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
    _maxVolumeH>0.0
  """
  
    if self._use_enclave_tasking:
      predicate += """and (_solverState == SolverState::Primary or _solverState == SolverState::PrimaryAfterGridInitialisation) """
      
    return """
  if (""" + predicate + """) {
    logInfo( "step()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "step()", "t_{min,global}     = " << _minTimeStampThisTimeStep  );
    logInfo( "step()", "t_{max,global}     = " << _maxTimeStampThisTimeStep  );
    logInfo( "step()", "t_{min,this-step}  = " << _localMinTimeStampThisTimeStep );
    logInfo( "step()", "t_{max,this-step}  = " << _localMaxTimeStampThisTimeStep );
    if (_minTimeStepSize > _maxTimeStepSize ) {
      logInfo( "step()", "dt_{min} = <not yet known>" );
      logInfo( "step()", "dt_{max} = <not yet known>" );
    }
    else {
      logInfo( "step()", "dt_{min}     = " << _minTimeStepSizeThisTimeStep );
      logInfo( "step()", "dt_{max}     = " << _maxTimeStepSizeThisTimeStep );
    }
    logInfo( "step()", "h_{min}  = " << _minVolumeH << " (volume size)");
    logInfo( "step()", "h_{max}  = " << _maxVolumeH << " (volume size)" );
    logInfo( "step()", "#updates = " << _patchUpdates << " (no of patches)" );
  }
"""


  def create_finish_time_step_implementation(self):
    """
    
    The superclass takes the admissible cell size and divides it by the 
    maximum eigenvalue. The Finite Volume solvers however operate with 
    patches, i.e. we have to devide by the volume count per axis.
    
    """
    return """
  assertion( _minVolumeH >= 0.0 );
  assertion( MaxAdmissibleVolumeH > 0.0 );
  if (_minVolumeH <= MaxAdmissibleVolumeH) {
    _timeStepSize  = """ + str(self._normalised_time_step_size) + """ * _minVolumeH / MaxAdmissibleVolumeH;
  }
  else {
    _timeStepSize = 0.0;
  }
"""
