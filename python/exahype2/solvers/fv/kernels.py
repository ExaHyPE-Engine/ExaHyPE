# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.fv.PDETerms  import PDETerms

import jinja2


def create_compute_time_step_size_for_fixed_time_stepping( time_step_size ):
  """
  
  If you work with fixed time stepping, the computation of the next
  time step size per cell or patch is trivial: You simply read out 
  the solver. Therefore, this computation can trivially be a generic
  one which is the same for all solver flavours.
  
  """
  return """
  const double timeStepSize = repositories::{{SOLVER_INSTANCE}}.getTimeStepSize();
"""


def create_finish_time_step_implementation_for_fixed_time_stepping(normalised_time_step_size):
  return """
  assertion( _minVolumeH >= 0.0 );
  assertion( MaxAdmissibleVolumeH > 0.0 );
  if (_minVolumeH <= MaxAdmissibleVolumeH) {
    _timeStepSize  = """ + str(normalised_time_step_size) + """ * _minVolumeH / MaxAdmissibleVolumeH;
  }
  else {
    _timeStepSize = 0.0;
  }
"""


def create_finish_time_step_implementation_for_local_time_stepping():
  return """
  assertion( _minVolumeH >= 0.0 );
  assertion( MaxAdmissibleVolumeH > 0.0 );
  assertion2( _minVolumeH <= MaxAdmissibleVolumeH, _minVolumeH, MaxAdmissibleVolumeH );
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


def create_start_time_step_implementation_for_fixed_time_stepping(use_enclave_tasking):
  """
  
  The outcome is used before we actually roll over the accumulation variables
  and other stuff.
  
  """
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
    logInfo( "step()", "t       = " << _minTimeStampThisTimeStep );
    logInfo( "step()", "dt      = " << getTimeStepSize() );
    logInfo( "step()", "h_{min} = " << _minVolumeHThisTimeStep << " (volume size)");
    logInfo( "step()", "h_{max} = " << _maxVolumeHThisTimeStep << " (volume size)" );
  }
"""


def create_start_time_step_implementation_for_local_time_stepping(use_enclave_tasking):
  return create_start_time_step_implementation_for_adaptive_time_stepping_with_subcycling(use_enclave_tasking)
  

def create_start_time_step_implementation_for_fixed_time_stepping_with_subcycling(use_enclave_tasking):
  """
  
  The outcome is used before we actually roll over the accumulation variables
  and other stuff.
  
  """
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


def create_start_time_step_implementation_for_adaptive_time_stepping_with_subcycling(use_enclave_tasking):
  predicate = """
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxVolumeH>0.0
  """
  
  if use_enclave_tasking:
    predicate += """and (_solverState == SolverState::Primary or _solverState == SolverState::PrimaryAfterGridInitialisation) """
      
  statistics = """
  if (""" + predicate + """) {
    logInfo( "startTimeStep()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "startTimeStep()", "t_{min,global}     = " << _minTimeStamp );
    logInfo( "startTimeStep()", "t_{max,global}     = " << _maxTimeStamp );
    logInfo( "startTimeStep()", "t_{min,this-step}  = " << _minTimeStampThisTimeStep );
    logInfo( "startTimeStep()", "t_{max,this-step}  = " << _maxTimeStampThisTimeStep );
    if (_minTimeStepSize > _maxTimeStepSize ) {
      logInfo( "startTimeStep()", "dt_{min} = <not yet known>" );
      logInfo( "startTimeStep()", "dt_{max} = <not yet known>" );
    }
    else {
      logInfo( "startTimeStep()", "dt_{min,this-step} = " << _minTimeStepSize );
      logInfo( "startTimeStep()", "dt_{max,this-step} = " << _maxTimeStepSize );
    }
    logInfo( "startTimeStep()", "h_{min}      = " << _minVolumeH << " (volume size)");
    logInfo( "startTimeStep()", "h_{max}      = " << _maxVolumeH << " (volume size)" );
    logInfo( "startTimeStep()", "lambda_{max} = " << _maxEigenvalue );
    logInfo( "startTimeStep()", "#updates = " << _patchUpdates << " (no of patches)" );
  }
"""
    
  if use_enclave_tasking:
    clear_max_eigenvalue = """if (_solverState == SolverState::Primary or _solverState == SolverState::PrimaryAfterGridInitialisation) {
  _maxEigenvalue = 0.0;
}"""
  else:
    clear_max_eigenvalue = """if (_solverState == SolverState::TimeStep or _solverState == SolverState::TimeStepAfterGridInitialisation) {
  _maxEigenvalue = 0.0;
}"""

  return statistics + clear_max_eigenvalue
  

def create_start_time_step_implementation_for_adaptive_time_stepping(use_enclave_tasking):
  predicate = """
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxVolumeH>0.0
  """
  
  if use_enclave_tasking:
    predicate += """and (_solverState == SolverState::Primary or _solverState == SolverState::PrimaryAfterGridInitialisation) """
      
  statistics = """
  if (""" + predicate + """) {
    logInfo( "startTimeStep()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "startTimeStep()", "t            = " << _minTimeStampThisTimeStep );
    logInfo( "startTimeStep()", "dt           = " << getAdmissibleTimeStepSize() );
    logInfo( "startTimeStep()", "h_{min}      = " << _minVolumeHThisTimeStep << " (volume size)");
    logInfo( "startTimeStep()", "h_{max}      = " << _maxVolumeHThisTimeStep << " (volume size)" );
    logInfo( "startTimeStep()", "lambda_{max} = " << _maxEigenvalue );
  }
"""
    
  if use_enclave_tasking:
    clear_max_eigenvalue = """if (_solverState == SolverState::Primary or _solverState == SolverState::PrimaryAfterGridInitialisation) {
  _maxEigenvalue = 0.0;
}"""
  else:
    clear_max_eigenvalue = """if (_solverState == SolverState::TimeStep or _solverState == SolverState::TimeStepAfterGridInitialisation) {
  _maxEigenvalue = 0.0;
}"""

  return statistics + clear_max_eigenvalue


def create_halo_layer_construction_with_interpolation_for_reconstructed_patch(solver):
  """
  
  This is a straightforward modification of ReconstructPatchAndApplyFunctor._Template_TouchCellFirstTime_Fill_Halos
  in the blockstructured toolkit.
  
  """
  return """
    //
    // Bring in the auxiliary patches, i.e. befill halo
    //
    for(int d=0; d<Dimensions; d++) {{
      logTraceInWith1Argument( "touchCellFirstTime(...)::loopOverFace", d );
      //
      // d-loop over all dimensions except d. The vector k's entry d is set
      // to 0. We start with the left/bottom face, i.e. the one closer to the 
      // coordinate system's origin.
      //
      dfore(k,{DOFS_PER_AXIS},d,0) {{
        std::pair<double,double> oldNewWeightsLeft  = ::exahype2::getInterpolationWeights( 
          fineGridFaces""" + solver + """FaceLabel(d).getOldTimeStamp(0),
          fineGridFaces""" + solver + """FaceLabel(d).getNewTimeStamp(0),
          fineGridCell""" + solver + """CellLabel.getTimeStamp()
        );
        std::pair<double,double> oldNewWeightsRight = ::exahype2::getInterpolationWeights(
          fineGridFaces""" + solver + """FaceLabel(d+Dimensions).getOldTimeStamp(1),
          fineGridFaces""" + solver + """FaceLabel(d+Dimensions).getNewTimeStamp(1),
          fineGridCell""" + solver + """CellLabel.getTimeStamp()
        );
      
        for (int i=0; i<{OVERLAP}; i++) {{
          tarch::la::Vector<Dimensions,int> destinationCell = k + tarch::la::Vector<Dimensions,int>({OVERLAP});
          tarch::la::Vector<Dimensions,int> sourceCell      = k;
          destinationCell(d) = i;
          sourceCell(d)      = i;
          
          int destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,{DOFS_PER_AXIS} + 2*{OVERLAP});
          int sourceCellSerialised        = serialisePatchIndex(sourceCell,d);

          for (int j=0; j<{UNKNOWNS}; j++) {{
            reconstructedPatch[ destinationCellSerialised*{UNKNOWNS}+j ] 
              = oldNewWeightsLeft.first  * fineGridFaces""" + solver + """QOld(d).value[ sourceCellSerialised*{UNKNOWNS}+j ]
              + oldNewWeightsLeft.second * fineGridFaces""" + solver + """QNew(d).value[ sourceCellSerialised*{UNKNOWNS}+j ];
            {ASSERTION_WITH_7_ARGUMENTS}( 
              {ASSERTION_PREFIX_FOR_HALO} or 
              reconstructedPatch[ destinationCellSerialised*{UNKNOWNS}+j ]==reconstructedPatch[ destinationCellSerialised*{UNKNOWNS}+j ], 
              sourceCell, destinationCell, j, d, marker.toString(), _treeNumber, marker.toString()
            );
          }}

          destinationCell(d) = i+{DOFS_PER_AXIS}+{OVERLAP};
          sourceCell(d)      = i+{OVERLAP};

          destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,{DOFS_PER_AXIS} + 2*{OVERLAP});
          sourceCellSerialised        = serialisePatchIndex(sourceCell,d);
          for (int j=0; j<{UNKNOWNS}; j++) {{
            reconstructedPatch[ destinationCellSerialised*{UNKNOWNS}+j ] 
              = oldNewWeightsLeft.first  * fineGridFaces""" + solver + """QOld(d+Dimensions).value[ sourceCellSerialised*{UNKNOWNS}+j ]
              + oldNewWeightsLeft.second * fineGridFaces""" + solver + """QNew(d+Dimensions).value[ sourceCellSerialised*{UNKNOWNS}+j ];
            {ASSERTION_WITH_7_ARGUMENTS}( 
              {ASSERTION_PREFIX_FOR_HALO} or 
              reconstructedPatch[ destinationCellSerialised*{UNKNOWNS}+j ]==reconstructedPatch[ destinationCellSerialised*{UNKNOWNS}+j ], 
              sourceCell, destinationCell, j, d, marker.toString(), _treeNumber, marker.toString()
            );
          }}
        }}
      }}
      logTraceOut( "touchCellFirstTime(...)::loopOverFace" );
    }}
"""

def create_compute_time_step_size_for_fixed_time_stepping_with_subcycling( time_step_size, solver_name, remove_accumulation_errors=True ):
  if remove_accumulation_errors:
    return """
  // The fixed solver's _timeStepSize scales with min volume h, i.e. it is 
  // always chosen such that the finest grid does something meaningful.
  const double originalStepSize = repositories::{{SOLVER_INSTANCE}}.getMinPatchSize(false)>0.0 ?
    repositories::{{SOLVER_INSTANCE}}.getTimeStepSize() * marker.h()(0) / repositories::{{SOLVER_INSTANCE}}.getMinPatchSize(false) :
    0.0;
  const double timeStepSize = ::exahype2::removeTimeStepAccumulationErrorsFromCell( fineGridCell""" + solver_name + """CellLabel, fineGridFaces""" + solver_name + """FaceLabel, originalStepSize);
"""  
  else:
    return """
  // The fixed solver's _timeStepSize scales with min volume h, i.e. it is 
  // always chosen such that the finest grid does something meaningful.
  const double timeStepSize = repositories::{{SOLVER_INSTANCE}}.getMinPatchSize(false)>0.0 ?
    repositories::{{SOLVER_INSTANCE}}.getTimeStepSize() * marker.h()(0) / repositories::{{SOLVER_INSTANCE}}.getMinPatchSize(false) :
    0.0;
"""  


def create_compute_time_step_size_kernel_for_adaptive_time_stepping_with_subcycling( solver_name, remove_accumulation_errors=True ):
  if remove_accumulation_errors:
    return """
  // The fixed solver's _timeStepSize scales with min volume h, i.e. it is 
  // always chosen such that the finest grid does something meaningful.
  const double originalTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getMinPatchSize(false)>0.0 ?
    repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize() * marker.h()(0) / repositories::{{SOLVER_INSTANCE}}.getMinPatchSize(false) :
    0.0;
  const double timeStepSize = ::exahype2::removeTimeStepAccumulationErrorsFromCell( fineGridCell""" + solver_name + """CellLabel, fineGridFaces""" + solver_name + """FaceLabel, originalTimeStepSize);
"""
  else:
    return """
  // The fixed solver's _timeStepSize scales with min volume h, i.e. it is 
  // always chosen such that the finest grid does something meaningful.
  const double timeStepSize = repositories::{{SOLVER_INSTANCE}}.getMinPatchSize(false)>0.0 ?
    repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize() * marker.h()(0) / repositories::{{SOLVER_INSTANCE}}.getMinPatchSize(false) :
    0.0;
"""


def create_compute_time_step_size_kernel_for_local_time_stepping( solver_name, time_step_relaxation ):
  result = """
  const double timeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();  
  const double timeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();  

  const double timeStepSize = ::exahype2::removeTimeStepAccumulationErrorsFromCell( fineGridCell""" + solver_name + """CellLabel, fineGridFaces""" + solver_name + """FaceLabel, timeStepSize);
"""

  return result


def create_compute_time_step_size_kernel_for_adaptive_time_stepping():
  return """
  const double timeStepSize = repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize();
"""


def create_postprocess_updated_patch_for_local_time_stepping(time_step_relaxation, avoid_staircase_effect, discretisation_steps):
  """
  
  :: Zero eigenvalues
  
  If you solve non-linear problems, cells can have a zero eigenvalue. It means that 
  nothing happens within this cell. There are two options on the table: You can take
  the biggest global eigenvalue and march forward using this value. In ExaHyPE 2, 
  this yields a staircase effect if you have larger, regular region, then we have 
  something similar to many stencil codes which then update the cells in the middle
  again, and then those in the middle again, and so forth.
  
  We can avoid this by making a cell march if an only if one neighbour has advanced.
  In this case large global areas where nothing happens lag behind. 
  
  
  avoid_staircase_effect: Boolean
  
  """
  if discretisation_steps<=1.0:
    raise Exception( "invalid discretisation step size for local time step (val=" + str(discretisation_steps) + ")" )

  determine_eigenvalue = """
    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue( maxEigenvalue );  
"""

  if avoid_staircase_effect:
    compute_time_step_sizes = """    
    double newTimeStepSize = 0.0;
    //const double minGlobalTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue()>0.0 ? """ + str(time_step_relaxation) + """ * repositories::{{SOLVER_INSTANCE}}.getMinVolumeSize(false) / repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue() : 0.0;
    const double minGlobalTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize()<std::numeric_limits<double>::max() ? repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize() : 0.0;
    const double maxGlobalTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue()>0.0 ? """ + str(time_step_relaxation) + """ * repositories::{{SOLVER_INSTANCE}}.getMaxVolumeSize(false) / repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue() : 0.0;
    if ( tarch::la::greater( maxEigenvalue,0.0) ) {
      newTimeStepSize = ::exahype2::discretiseAndTruncateTimeStepSizes(
        """ + str(time_step_relaxation) + """ * marker.h()(0) / {{NUMBER_OF_VOLUMES_PER_AXIS}} / maxEigenvalue,
        minGlobalTimeStepSize,
        maxGlobalTimeStepSize,
        """ + str(discretisation_steps) + """
      );
    }
    else {
      const double minTimeStampOfNeighbours = ::exahype2::getMinTimeStampOfNeighboursAhead(fineGridCell{{SOLVER_NAME}}CellLabel, fineGridFaces{{SOLVER_NAME}}FaceLabel);

      newTimeStepSize = minTimeStampOfNeighbours - fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    }
"""    
  else:
    compute_time_step_sizes = """    
    double       newTimeStepSize       = 0.0;
    //const double minGlobalTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue()>0.0 ? """ + str(time_step_relaxation) + """ * repositories::{{SOLVER_INSTANCE}}.getMinVolumeSize(false) / repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue() : 0.0;
    const double minGlobalTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize()<std::numeric_limits<double>::max() ? repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize() : 0.0;
    const double maxGlobalTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue()>0.0 ? """ + str(time_step_relaxation) + """ * repositories::{{SOLVER_INSTANCE}}.getMaxVolumeSize(false) / repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue() : 0.0;
    if ( tarch::la::greater( maxEigenvalue,0.0) ) {
      newTimeStepSize = ::exahype2::discretiseAndTruncateTimeStepSizes(
        """ + str(time_step_relaxation) + """ * marker.h()(0) / {{NUMBER_OF_VOLUMES_PER_AXIS}} / maxEigenvalue,
        minGlobalTimeStepSize,
        maxGlobalTimeStepSize,
        """ + str(discretisation_steps) + """
      );
    }
    else if ( tarch::la::greater( repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue(),0.0) ) {
      newTimeStepSize = minGlobalTimeStepSize;
    }
"""    
   
    
  set_time_step_size = """    
    if ( tarch::la::equals(newTimeStepSize,0.0) ) {
      logDebug( "touchCellFirstTime(...)", "can't do a time step on cell " << marker.toString() << " as global max eigenvalue=" << repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue() );
      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(0.0);    
    }
    else {
      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);    
    }
"""

  return determine_eigenvalue + compute_time_step_sizes + set_time_step_size
    
    
def create_compute_new_time_step_size_kernel_for_adaptive_time_stepping():
  return """
    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue( maxEigenvalue );  
    
    // This is a dummy value. We'll ignore it later anyway as we ask for a 
    // new valid time step size from getAdmissibleTimeStepSize().
    const double newTimeStepSize = 0.0;
"""


def create_constructor_implementation_for_adaptive_time_stepping():
  return "_admissibleTimeStepSize = 0.0;"


def create_constructor_implementation_for_local_time_stepping():
  return "_maxEigenvalue = std::numeric_limits<double>::max();"
  

def create_abstract_solver_user_declarations_for_adaptive_time_stepping():
  return """
private:
  double _maxEigenvalue;
  double _admissibleTimeStepSize;
public:
  void setMaxEigenvalue( double eigenvalue );  
  /**
   * @return Admissible time step size for the current sweep, i.e. 
   *         return _admissibleTimeStepSize. This value always refers
   *         to the minimum mesh volume size. If you use subcycling,
   *         you have to scale it for cells that are not on the finest
   *         mesh resolution. 
   */
  double getAdmissibleTimeStepSize() const;  
    """  
    
    
def create_abstract_solver_user_declarations_for_local_time_stepping():
  return """
private:
  double _maxEigenvalue;
  double _maxEigenvalueOfPreviousSweep;
public:
  /**
   * @return Minimum non-zero eigenvalue. Keep in mind that the per-cell 
   *         eigenvalues can become zero for some non-linear problems (if 
   *         nothing happens), so it is important to neglect those when we
   *         take the minimum.
   */
  double getMaxEigenvalue() const;  

  void setMaxEigenvalue( double eigenvalue );
    """  
    
    
def create_abstract_solver_user_definitions_for_adaptive_time_stepping():
  return """
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::setMaxEigenvalue( double eigenvalue ) {
  if ( tarch::la::greater( eigenvalue, 0.0 ) ) {
    tarch::multicore::Lock lock(_semaphore);
    _maxEigenvalue = std::max(_maxEigenvalue,eigenvalue);
  }
}    


double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getAdmissibleTimeStepSize() const {
  return _admissibleTimeStepSize;
}
    """  
    
    
def create_abstract_solver_user_definitions_for_local_time_stepping():
  return """
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::setMaxEigenvalue( double eigenvalue ) {
  if ( tarch::la::greater( eigenvalue, 0.0 ) ) {
    tarch::multicore::Lock lock(_semaphore);
    _maxEigenvalue = std::max(_maxEigenvalue,eigenvalue);
  }
}    


double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getMaxEigenvalue() const {
  return _maxEigenvalueOfPreviousSweep;
}
    """  


def create_finish_time_step_implementation_for_local_time_stepping():
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
    
    
def create_finish_time_step_implementation_for_adaptive_time_stepping(time_step_relaxation):
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

  if ( tarch::la::smaller(_maxEigenvalue, 0.0 ) ) {
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_maxEigenvalue>=0", "invalid max eigenvalue: " + std::to_string(_maxEigenvalue) );
    // keep time step size invariant
    // _admissibleTimeStepSize = _admissibleTimeStepSize;
  }
  else if ( tarch::la::equals(_maxEigenvalue, 0.0 ) ) {
    logWarning( "finishTimeStep(...)", "maximum eigenvalue approaches 0.0. For nonlinear PDEs, this often means the PDE becomes stationary. It could also be a bug however" ); 
    _admissibleTimeStepSize = 0.0;
  }
  else {
    const double minVolumeSize = _minVolumeHThisTimeStep;
    _admissibleTimeStepSize = """ + str(time_step_relaxation) + """ * minVolumeSize / _maxEigenvalue;
    if ( std::isnan(_admissibleTimeStepSize) or std::isinf(_admissibleTimeStepSize) ) {
      ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSize>0", "invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSize) );
    }
    if (tarch::la::smallerEquals(_admissibleTimeStepSize,0.0,1e-10) ) {
      logWarning( "finishTimeStep(...)", "degenerated time step size of " << std::to_string(_admissibleTimeStepSize) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug (h_volume=" << minVolumeSize << ")" );
    }
  }
"""


def create_source_term_kernel(source_term_implementation):
  if source_term_implementation==PDETerms.None_Implementation:
    return create_empty_source_term_kernel()
  else:
    return create_user_defined_source_term_kernel()
  
    
def create_optimised_Riemann_kernel_template_arguments(flux_implementation, 
                                                       ncp_implementation, 
                                                       source_term_implementation, 
                                                       compute_new_time_step_size):
  Template = jinja2.Template( """
  <{{NUMBER_OF_VOLUMES_PER_AXIS}},{{NUMBER_OF_UNKNOWNS}},{{NUMBER_OF_AUXILIARY_VARIABLES}},{{SOLVER_NAME}},{{SKIP_FLUX}},{{SKIP_NCP}},{{SKIP_SOURCE_TERM}},{{SKIP_COMPUTATION_OF_NEW_TIME_STEP_SIZE}}>
""", undefined=jinja2.DebugUndefined)
  
  d= {}
  if flux_implementation==PDETerms.None_Implementation or flux_implementation==PDETerms.Empty_Implementation:
    d[ "SKIP_FLUX"] = "true"
  else:
    d[ "SKIP_FLUX"] = "false"
  if ncp_implementation==PDETerms.None_Implementation or ncp_implementation==PDETerms.Empty_Implementation:
    d[ "SKIP_NCP"]  = "true"
  else:
    d[ "SKIP_NCP"] = "false"
  if source_term_implementation==PDETerms.None_Implementation or source_term_implementation==PDETerms.Empty_Implementation:
    d[ "SKIP_SOURCE_TERM"]  = "true"
  else:
    d[ "SKIP_SOURCE_TERM"] = "false"
  if compute_new_time_step_size:
    d[ "SKIP_COMPUTATION_OF_NEW_TIME_STEP_SIZE"] = "false"
  else:
    d[ "SKIP_COMPUTATION_OF_NEW_TIME_STEP_SIZE"] = "true"
  return Template.render(**d)



def create_abstract_solver_declarations(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
  public:
    {% if EIGENVALUES_IMPLEMENTATION=="<none>" %}
    #error eigenvalue implementation cannot be none
    {% endif %}
    
    {% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it 
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}
    
    /**
     * Determine max eigenvalue over Jacobian in a given point with solution values
     * (states) Q. All parameters are in.
     *
     * @return Max eigenvalue. Result has to be positive, so we are actually speaking
     *   about the maximum absolute eigenvalue.
     */
    virtual double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal
    ) {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};


    {% if FLUX_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it 
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__                        F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}
    
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if FLUX_IMPLEMENTATION=="<user-defined>" %}=0{% else %} final {% endif %};
    {% endif %}


    {% if NCP_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it 
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static void nonconservativeProduct(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__                        BgradQ, // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}
    
    {% if NCP_IMPLEMENTATION!="<none>" %}
    virtual void nonconservativeProduct(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__ BgradQ // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
    ) {% if NCP_IMPLEMENTATION=="<user-defined>" %}=0{% endif %};
    {% endif %}
    

    {% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it 
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static void sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      double * __restrict__ S,
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}
    
    {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
    virtual void sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      double * __restrict__ S
    ) {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final {% endif %};
    {% endif %}

""", undefined=jinja2.DebugUndefined)

  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "STATELESS_PDE_TERMS"]                 = pde_terms_without_state
  return Template.render(**d)


def create_abstract_solver_definitions(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and EIGENVALUES_IMPLEMENTATION!="<none>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  {{EIGENVALUES_IMPLEMENTATION}}
}
{% endif %}


{% if FLUX_IMPLEMENTATION!="<none>" and FLUX_IMPLEMENTATION!="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
 const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
 const tarch::la::Vector<Dimensions,double>&  faceCentre,
 const tarch::la::Vector<Dimensions,double>&  volumeH,
 double                                       t,
 int                                          normal,
 double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{FLUX_IMPLEMENTATION}}
}
{% endif %}


{% if NCP_IMPLEMENTATION!="<none>" and NCP_IMPLEMENTATION!="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
) {
  {{NCP_IMPLEMENTATION}}
}
{% endif %}


{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and SOURCE_TERM_IMPLEMENTATION!="<none>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S
) {
  {% if SOURCE_TERM_IMPLEMENTATION!="<empty>" %}
  {{SOURCE_TERM_IMPLEMENTATION}}
  {% else %}
  std::fill_n(S,{{NUMBER_OF_UNKNOWNS}},0.0);
  {% endif %}
}
{% endif %}



{% if EIGENVALUES_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      Offloadable
) {
  {{EIGENVALUES_IMPLEMENTATION}};
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif
{% endif %}


{% if FLUX_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__                        F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
) {
  {% if FLUX_IMPLEMENTATION=="<none>" %}
  abort();
  {% else %}
  {{FLUX_IMPLEMENTATION}}
  {% endif %}
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif
{% endif %}


{% if NCP_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__                        BgradQ, // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
) {
  {% if NCP_IMPLEMENTATION=="<none>" %}
  abort();
  {% else %}
  {{NCP_IMPLEMENTATION}}
  {% endif %}
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif
{% endif %}


{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(OpenMPGPUOffloading)
#pragma omp declare target
#endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      double * __restrict__ S,
      Offloadable
) {
  {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %}
  abort();
  {% else %}
  {{SOURCE_TERM_IMPLEMENTATION}}
  {% endif %}
}
#if defined(OpenMPGPUOffloading)
#pragma omp end declare target
#endif
{% endif %}
""", undefined=jinja2.DebugUndefined)

  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "STATELESS_PDE_TERMS"]                             = pde_terms_without_state  
  return Template.render(**d)


def create_solver_declarations(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
  public:
    {% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    void sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      double * __restrict__ S
    ) override;
    {% endif %}

    {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
    /**
     * Determine max eigenvalue over Jacobian in a given point with solution values
     * (states) Q. All parameters are in.
     *
     * @return Max eigenvalue. Result has to be positive, so we are actually speaking
     *   about the maximum absolute eigenvalue.
     */
    virtual double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal
    ) override;
    {% endif %}


    {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
    virtual void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}
     
     
    {% if NCP_IMPLEMENTATION=="<user-defined>" %}
    virtual void nonconservativeProduct(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__ BgradQ // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
    ) override;
    {% endif %}
    
   
    {% if STATELESS_PDE_TERMS  and SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
    /**
     * To obtain the best performance, I recommend to man inline command to 
     * this signature and to copy the implementation into the header. So it would
     * read 
     *
     * static inline void sourceTerm( ... ) {
     *  code here
     * }
     *
     * The GPU offloading requires static functions. As we cannot overload the
     * original (virtual) function with a static alternative, we do the
     * TBB trick and overload by adding an additional enum. It has no semantics
     * but helps the compiler to distinguish the different function variants.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
     static void sourceTerm(
       const double * __restrict__ Q,
       const tarch::la::Vector<Dimensions,double>&  volumeCentre,
       const tarch::la::Vector<Dimensions,double>&  volumeH,
       double                                       t,
       double                                       dt,
       double * __restrict__ S,
       Offloadable
     );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}


    {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * To obtain the best performance, I recommend to man inline command to 
     * this signature and to copy the implementation into the header. So it would
     * read 
     *
     * static inline double maxEigenvalue( ... ) {
     *  code here
     * }
     *
     * The GPU offloading requires static functions. As we cannot overload the
     * original (virtual) function with a static alternative, we do the
     * TBB trick and overload by adding an additional enum. It has no semantics
     * but helps the compiler to distinguish the different function variants.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}


    {% if STATELESS_PDE_TERMS  and FLUX_IMPLEMENTATION=="<user-defined>" %}
    /**
     * To obtain the best performance, I recommend to man inline command to 
     * this signature and to copy the implementation into the header. So it would
     * read 
     *
     * static inline void flux( ... ) {
     *  code here
     * }
     *
     * The GPU offloading requires static functions. As we cannot overload the
     * original (virtual) function with a static alternative, we do the
     * TBB trick and overload by adding an additional enum. It has no semantics
     * but helps the compiler to distinguish the different function variants.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__                        F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}
     
   
    {% if STATELESS_PDE_TERMS  and NCP_IMPLEMENTATION=="<user-defined>" %}
    /**
     * To obtain the best performance, I recommend to man inline command to 
     * this signature and to copy the implementation into the header. So it would
     * read 
     *
     * static inline void nonconservativeProduct( ... ) {
     *  code here
     * }
     *
     * The GPU offloading requires static functions. As we cannot overload the
     * original (virtual) function with a static alternative, we do the
     * TBB trick and overload by adding an additional enum. It has no semantics
     * but helps the compiler to distinguish the different function variants.
     */
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
    static void nonconservativeProduct(
      const double * __restrict__                  Q,         // Q[5+0],
      const double * __restrict__                  deltaQ,    // [5+0]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      double * __restrict__                        BgradQ,     // BgradQ[5]
      Offloadable
    );
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
    {% endif %}    
""", undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "STATELESS_PDE_TERMS"]                             = pde_terms_without_state  
  return Template.render(**d)


def create_solver_definitions(flux_implementation, ncp_implementation, eigenvalues_implementation, source_term_implementation, pde_terms_without_state):
  Template = jinja2.Template( """
{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
)  {
  logTraceInWith4Arguments( "maxEigenvalue(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "maxEigenvalue(...)" );
}
{% endif %}


{% if FLUX_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ F // F[{{NUMBER_OF_UNKNOWNS}}]
)  {
  logTraceInWith4Arguments( "flux(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "flux(...)" );
}
{% endif %}


{% if NCP_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
)  {
  logTraceInWith4Arguments( "nonconservativeProduct(...)", faceCentre, volumeH, t, normal );
  // @todo implement
  logTraceOut( "nonconservativeProduct(...)" );
}
{% endif %}


{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" %}
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__                  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  volumeX,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S  // S[{{NUMBER_OF_UNKNOWNS}}
) {
  logTraceInWith4Arguments( "sourceTerm(...)", volumeX, volumeH, t, dt );
  // @todo implement
  logTraceOut( "sourceTerm(...)" );
}
{% endif %}





{% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  Offloadable
)  {
  // @todo implement
}
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
{% endif %}


{% if FLUX_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ F,
  Offloadable
)  {
  // @todo implement
}
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
{% endif %}


{% if NCP_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  double * __restrict__ BgradQ,
  Offloadable
)  {
  // @todo implement
}
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
{% endif %}



{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(OpenMPGPUOffloading)
    #pragma omp declare target
    #endif
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
  const double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  double * __restrict__ S,
  Offloadable
) {
  // @todo implement
}
    #if defined(OpenMPGPUOffloading)
    #pragma omp end declare target
    #endif
{% endif %}


""", undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION"]                 = flux_implementation
  d[ "NCP_IMPLEMENTATION"]                  = ncp_implementation
  d[ "EIGENVALUES_IMPLEMENTATION"]          = eigenvalues_implementation
  d[ "SOURCE_TERM_IMPLEMENTATION"]          = source_term_implementation
  d[ "STATELESS_PDE_TERMS"]                             = pde_terms_without_state  
  return Template.render(**d)


