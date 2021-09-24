# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org


def create_preprocess_reconstructed_patch_throughout_sweep_kernel_for_fixed_time_stepping( time_step_size ):
  return """
  cellTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getTimeStepSize();
  cellTimeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
"""


def create_finish_time_step_implementation_for_fixed_time_stepping(normalised_time_step_size):
  return """
  assertion( _minVolumeH >= 0.0 );
  assertion( MaxAdmissibleVolumeH > 0.0 );
  assertion( _minVolumeH <= MaxAdmissibleVolumeH );
  _timeStepSize  = """ + str(normalised_time_step_size) + """ * _minVolumeH / MaxAdmissibleVolumeH;
"""


def create_empty_source_term_kernel():
  return """
  std::fill_n(S,{{NUMBER_OF_UNKNOWNS}},0.0);
"""



def create_user_defined_source_term_kernel():
  return """
  repositories::{{SOLVER_INSTANCE}}.sourceTerm(
    Q,
    x, dx, t, dt,
    S
  );
"""


def create_abstract_solver_user_declarations_for_fixed_time_stepping():
  return """
private:
  double _timeStepSize;
public:
  double getTimeStepSize() const;  
    """  


def create_abstract_solver_user_definitions_for_fixed_time_stepping():
  return """
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getTimeStepSize() const {
  return _timeStepSize;
}
    """  


def create_start_time_step_implementation_for_fixed_time_stepping(use_enclave_tasking):
  predicate = """
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxVolumeH>0.0
  """
  
  if use_enclave_tasking:
    predicate += """and (_solverState == SolverState::Primary or _solverState == SolverState::PrimaryAfterGridInitialisation) """
      
  return """
  if (""" + predicate + """) {
    logInfo( "step()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "step()", "t       = " << _minTimeStamp );
    logInfo( "step()", "dt      = " << getTimeStepSize() );
    logInfo( "step()", "h_{min} = " << _minVolumeH << " (volume size)");
    logInfo( "step()", "h_{max} = " << _maxVolumeH << " (volume size)" );
  }
"""

def create_start_time_step_implementation_for_fixed_time_stepping_with_subcycling(use_enclave_tasking):
  predicate = """
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxVolumeH>0.0
  """
  
  if use_enclave_tasking:
    predicate += """and (_solverState == SolverState::Primary or _solverState == SolverState::PrimaryAfterGridInitialisation) """
      
  return """
  if (""" + predicate + """) {
    logInfo( "step()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "step()", "t_{min}  = " << _minTimeStamp );
    logInfo( "step()", "t_{max}  = " << _maxTimeStamp );
    if (_minTimeStepSize > _maxTimeStepSize ) {
      logInfo( "step()", "dt_{min} = <not yet known>" );
      logInfo( "step()", "dt_{max} = <not yet known>" );
    }
    else {
      logInfo( "step()", "dt_{min} = " << _minTimeStepSize );
      logInfo( "step()", "dt_{max} = " << _maxTimeStepSize );
    }
    logInfo( "step()", "h_{min}  = " << _minVolumeH << " (volume size)");
    logInfo( "step()", "h_{max}  = " << _maxVolumeH << " (volume size)" );
    logInfo( "step()", "#updates = " << _patchUpdates << " (no of patches)" );
  }
"""
