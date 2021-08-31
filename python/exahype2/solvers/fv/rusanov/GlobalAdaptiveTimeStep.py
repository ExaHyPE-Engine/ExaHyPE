# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.fv.SingleSweep import SingleSweep
from exahype2.solvers.fv.PDETerms    import PDETerms

import jinja2

from .kernels import create_source_term_kernel_for_Rusanov
from .kernels import create_compute_Riemann_kernel_for_Rusanov_fixed_timestepping
from .kernels import create_abstract_solver_declarations
from .kernels import create_abstract_solver_definitions
from .kernels import create_solver_declarations
from .kernels import create_solver_definitions



class GlobalAdaptiveTimeStep( SingleSweep ):
  def __init__(self, 
    name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_relaxation,
    flux=PDETerms.User_Defined_Implementation, 
    ncp=None, 
    eigenvalues=PDETerms.User_Defined_Implementation, 
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    plot_grid_properties=False
  ):
    super(GlobalAdaptiveTimeStep,self).__init__(name, patch_size, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties) 
    
    self._time_step_relaxation = time_step_relaxation

    self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.None_Implementation
    self._source_term_implementation          = PDETerms.None_Implementation
    
    self._preprocess_reconstructed_patch      = """
  cellTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize();
  cellTimeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
"""
    self._postprocess_updated_patch      = """
    const double maxEigenvalue = ::exahype2::fv::maxEigenvalue_AoS(
      [] (
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) -> double {
        return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
      },
      marker.x(),
      marker.h(),
      cellTimeStamp,
      cellTimeStepSize,
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      targetPatch
    );
    
    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue( maxEigenvalue );  
"""

    self.set_implementation(flux=flux, 
      ncp=ncp, 
      eigenvalues=eigenvalues, 
      boundary_conditions=boundary_conditions, 
      refinement_criterion=refinement_criterion, 
      initial_conditions=initial_conditions, 
      source_term=source_term )


  def set_implementation(self,
    flux=None,ncp=None,
    eigenvalues=None,
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    memory_location         = None,
    use_split_loop          = False
  ):
    """
      If you pass in User_Defined, then the generator will create C++ stubs
      that you have to befill manually. If you pass in None_Implementation, it
      will create nop, i.e. no implementation or defaults. Any other string
      is copied 1:1 into the implementation. If you pass in None, then the
      set value so far won't be overwritten.

      Please note that not all options are supported by all solvers. You
      cannot set ncp and fluxes for the ClawPack Riemann solvers, e.g.

      This routine should be the very last invoked by the constructor.
    """
    if flux                 is not None:  self._flux_implementation                       = flux
    if ncp                  is not None:  self._ncp_implementation                        = ncp
    if eigenvalues          is not None:  self._eigenvalues_implementation                = eigenvalues
    if source_term          is not None:  self._source_term_implementation                = source_term

    self._source_term_call    = create_source_term_kernel_for_Rusanov(self._source_term_implementation, False)
    self._Riemann_solver_call = create_compute_Riemann_kernel_for_Rusanov_fixed_timestepping(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, False)

    self._abstract_solver_user_declarations = create_abstract_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    self._abstract_solver_user_definitions  = create_abstract_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    self._solver_user_declarations          = create_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    self._solver_user_definitions           = create_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)

    self._constructor_implementation = "_admissibleTimeStepSize = 0.0;"
    
    self._abstract_solver_user_declarations += """
private:
  double _maxEigenvalue;
  double _admissibleTimeStepSize;
public:
  void setMaxEigenvalue( double eigenvalue );  
  double getAdmissibleTimeStepSize() const;  
    """  

    self._abstract_solver_user_definitions += """
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
    
    self._start_time_step_implementation          = """
  _maxEigenvalue = 0.0;
"""

    self._finish_time_step_implementation          = """
  #ifdef Parallel
  double newMaxEigenvalue = _maxEigenvalue;
  tarch::mpi::Rank::getInstance().allReduce(
        &_maxEigenvalue,
        &newMaxEigenvalue,
        1,
        MPI_DOUBLE,
        MPI_MAX, 
        [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
        );
  #endif

  _admissibleTimeStepSize = """ + str(self._time_step_relaxation) + """ * _minH / _maxEigenvalue / 3.0;
  if ( std::isnan(_admissibleTimeStepSize) or std::isinf(_admissibleTimeStepSize) ) {
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSize>0", "invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSize) );
  }
  if (tarch::la::smallerEquals(_admissibleTimeStepSize,0.0,1e-10) ) {
    ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSize>0", "degenerated time step size of " + std::to_string(_admissibleTimeStepSize) );
  }
  
  _maxTimeStepSize  = _admissibleTimeStepSize; // for plotting reasons
  _minTimeStepSize  = std::min( _minTimeStepSize, _admissibleTimeStepSize );
"""
    
    SingleSweep.set_implementation(self, boundary_conditions, refinement_criterion, initial_conditions, memory_location, use_split_loop)

