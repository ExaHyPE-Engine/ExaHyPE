# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import jinja2


def create_preprocess_reconstructed_patch_throughout_sweep_kernel_for_fixed_time_stepping( time_step_size ):
  return """
  cellTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getTimeStepSize();
  cellTimeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
"""


def create_compute_Riemann_kernel_for_ClawPack():
  Template = jinja2.Template( """
        double wave[{{NUMBER_OF_UNKNOWNS}}][{{NUMBER_OF_UNKNOWNS}}];
        double speed[{{NUMBER_OF_UNKNOWNS}}];

        int num_eqn   = {{NUMBER_OF_UNKNOWNS}};
        int num_aux   = {{NUMBER_OF_AUXILIARY_VARIABLES}};
        int num_waves = {{NUMBER_OF_UNKNOWNS}};

        {{CLAWPACK_RIEMANN_SOLVER}}_(
          {%if DISCRIMINATE_NORMAL %}
            &normal,
          {% endif %}
          &num_eqn,
          &num_aux,
          &num_waves,
          QL,                                 // double* q_l
          QR,                                 // double* q_r
          QL+{{NUMBER_OF_UNKNOWNS}},          // double* aux_l
          QR+{{NUMBER_OF_UNKNOWNS}},          // double* aux_r
          wave,
          speed,
          FL,                                 // double* amdq
          FR                                  // double* apdq
        );

        for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) {
          FL[i] = -FL[i];
        }
""", undefined=jinja2.DebugUndefined)
  
  d= {}
  return Template.render(**d)


# @todo replication of Rusanov. Have to consolidate!
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

# @todo replication of Rusanov. Have to consolidate!
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


# @todo replication of Rusanov. Have to consolidate!
def create_finish_time_step_implementation_for_fixed_time_stepping(normalised_time_step_size):
  return """
  assertion( _minVolumeH > 0.0 );
  assertion( MaxAdmissibleVolumeH > 0.0 );
  assertion( _minVolumeH <= MaxAdmissibleVolumeH );
  _timeStepSize  = """ + str(normalised_time_step_size) + """ * _minVolumeH / MaxAdmissibleVolumeH;
"""


def create_empty_source_term_kernel(source_term_implementation):
  return """
  std::fill_n(S,{{NUMBER_OF_UNKNOWNS}},0.0);
"""
