# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .SolverCodeSnippets import SolverCodeSnippets


class LocalTimeSteppingCodeSnippets( SolverCodeSnippets ):
  """
  
  Code snippet generator for all subcycling solvers
  
  """

  def create_abstract_solver_user_declarations(self):
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


  def create_abstract_solver_user_definitions(self):
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


  def create_abstract_solver_constructor_statements(self):
    return "_maxEigenvalue = std::numeric_limits<double>::max();"
  
  
  def create_compute_time_step_size(self):
    result = """
  double timeStepSize = ::exahype2::removeTimeStepAccumulationErrorsFromCell( fineGridCell""" + solver_name + """CellLabel, fineGridFaces""" + solver_name + """FaceLabel, fineGridCell""" + solver_name + """CellLabel.getTimeStepSize() );
"""
    return result

  
  def create_compute_new_time_step_size(self):
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
  
  discretisation_steps: Integer  
    See exahype2::discretiseAndTruncateTimeStepSizes() for a description of admissible
    values and the semantics of the values.
  
    """
    determine_eigenvalue = """
    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue( maxEigenvalue );  
"""

    if avoid_staircase_effect:
      compute_time_step_sizes = """    
    double newTimeStepSize = 0.0;
    const double minGlobalTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize()<std::numeric_limits<double>::max() ? repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize() : 0.0;
    const double maxGlobalTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue()>0.0 ? """ + str(time_step_relaxation) + """ * repositories::{{SOLVER_INSTANCE}}.getMaxVolumeSize(false) / repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue() : 0.0;
    if ( tarch::la::greater( maxEigenvalue,0.0) ) {
      newTimeStepSize = ::exahype2::discretiseAndTruncateTimeStepSizes(
        """ + str(time_step_relaxation) + """ * marker.h()(0) / {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} / maxEigenvalue,
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
        """ + str(time_step_relaxation) + """ * marker.h()(0) / {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} / maxEigenvalue,
        maxGlobalTimeStepSize,
        """ + str(discretisation_steps) + """
      );
    }
    else if ( tarch::la::greater( repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue(),0.0) ) {
      newTimeStepSize = minGlobalTimeStepSize;
    }
"""    
   
    
#  set_time_step_size = """    
#    if ( tarch::la::equals(newTimeStepSize,0.0) ) {
#      logDebug( "touchCellFirstTime(...)", "can't do a time step on cell " << marker.toString() << " as global max eigenvalue=" << repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue() );
#      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(0.0);    
#    }
#    else {
#      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);    
#    }
#"""

    return determine_eigenvalue + compute_time_step_sizes


  def create_finish_time_step_implementation(self):
    """
  
    This routine is inserted after we have reduced all global quantities. These
    are the quantities with the postfix ThisTimeStep.
  
    """ 
    return """
  assertion( _minVolumeH >= 0.0 );
  assertion( MaxAdmissibleVolumeH > 0.0 );
  assertion2( _minVolumeH <= MaxAdmissibleVolumeH, _minVolumeH, MaxAdmissibleVolumeH );
"""
