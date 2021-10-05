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


def create_finish_time_step_implementation_for_local_time_stepping():
  return """
  assertion( _minVolumeH >= 0.0 );
  assertion( MaxAdmissibleVolumeH > 0.0 );
  assertion( _minVolumeH <= MaxAdmissibleVolumeH );
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


def create_start_time_step_implementation_for_local_time_stepping(use_enclave_tasking):
  return create_start_time_step_implementation_for_adaptive_time_stepping_with_subcycling(use_enclave_tasking)
  

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
    logInfo( "step()", "t_{min,global}     = " << _minTimeStamp );
    logInfo( "step()", "t_{max,global}     = " << _maxTimeStamp );
    logInfo( "step()", "t_{min,this-step}  = " << _minTimeStampThisTimeStep );
    logInfo( "step()", "t_{max,this-step}  = " << _maxTimeStampThisTimeStep );
    if (_minTimeStepSize > _maxTimeStepSize ) {
      logInfo( "step()", "dt_{min} = <not yet known>" );
      logInfo( "step()", "dt_{max} = <not yet known>" );
    }
    else {
      logInfo( "step()", "dt_{min,this-step} = " << _minTimeStepSize );
      logInfo( "step()", "dt_{max,this-step} = " << _maxTimeStepSize );
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
    predicate += """and _solverState == SolverState::Secondary """
      
  statistics = """
  if (""" + predicate + """) {
    logInfo( "step()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "step()", "t_{min,global}     = " << _minTimeStamp );
    logInfo( "step()", "t_{max,global}     = " << _maxTimeStamp );
    logInfo( "step()", "t_{min,this-step}  = " << _minTimeStampThisTimeStep );
    logInfo( "step()", "t_{max,this-step}  = " << _maxTimeStampThisTimeStep );
    if (_minTimeStepSize > _maxTimeStepSize ) {
      logInfo( "step()", "dt_{min} = <not yet known>" );
      logInfo( "step()", "dt_{max} = <not yet known>" );
    }
    else {
      logInfo( "step()", "dt_{min,this-step} = " << _minTimeStepSize );
      logInfo( "step()", "dt_{max,this-step} = " << _maxTimeStepSize );
    }
    logInfo( "step()", "h_{min}      = " << _minVolumeH << " (volume size)");
    logInfo( "step()", "h_{max}      = " << _maxVolumeH << " (volume size)" );
    logInfo( "step()", "lambda_{max} = " << _maxEigenvalue );
    logInfo( "step()", "#updates = " << _patchUpdates << " (no of patches)" );
  }
"""
    
  return statistics + """
  _maxEigenvalue = 0.0;
"""

def create_start_time_step_implementation_for_adaptive_time_stepping(use_enclave_tasking):
  predicate = """
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxVolumeH>0.0
  """
  
  if use_enclave_tasking:
    predicate += """and _solverState == SolverState::Secondary """
      
  statistics = """
  if (""" + predicate + """) {
    logInfo( "step()", "Solver {{SOLVER_NAME}}:" );
    logInfo( "step()", "t            = " << _minTimeStamp );
    logInfo( "step()", "dt           = " << getAdmissibleTimeStepSize() );
    logInfo( "step()", "h_{min}      = " << _minVolumeH << " (volume size)");
    logInfo( "step()", "h_{max}      = " << _maxVolumeH << " (volume size)" );
    logInfo( "step()", "lambda_{max} = " << _maxEigenvalue );
  }
"""
    
  return statistics + """
  _maxEigenvalue = 0.0;
"""


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
              + oldNewWeightsLeft.second * fineGridFaces""" + solver + """QNew(d).value[ sourceCellSerialised*{UNKNOWNS}+j ]
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
              + oldNewWeightsLeft.second * fineGridFaces""" + solver + """QNew(d+Dimensions).value[ sourceCellSerialised*{UNKNOWNS}+j ]
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
