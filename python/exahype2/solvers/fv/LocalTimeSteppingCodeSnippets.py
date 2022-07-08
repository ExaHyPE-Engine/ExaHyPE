# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.LocalTimeSteppingCodeSnippets


class LocalTimeSteppingCodeSnippets( exahype2.solvers.LocalTimeSteppingCodeSnippets ):
  """
  
  Code snippet generator for fixed time stepping in the Runge-Kutta schemes
  
  """
  def __init__(self, time_step_relaxation, use_enclave_tasking):
    self._time_step_relaxation = time_step_relaxation
    self._use_enclave_tasking  = use_enclave_tasking


  def create_start_time_step_implementation(self):
    return create_start_time_step_implementation_for_adaptive_time_stepping_with_subcycling(use_enclave_tasking)


  def create_finish_time_step_implementation(self):
    """
    
     This routine is inserted after we have reduced all global quantities. These
  are the quantities with the postfix ThisTimeStep.
  
    """ 
    return """
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
  
  if ( _solverState == SolverState::Secondary ) {
    _maxEigenvalueOfPreviousSweep = _maxEigenvalue;
  }
"""

