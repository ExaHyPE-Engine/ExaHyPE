# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.fv.PDETerms  import PDETerms

import jinja2
from enum import Enum


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
  const double timeStepSize = ::exahype2::removeTimeStepAccumulationErrorsFromCell( fineGridCell""" + solver_name + """CellLabel, fineGridFaces""" + solver_name + """FaceLabel, fineGridCell""" + solver_name + """CellLabel.getTimeStepSize() );
"""

  return result


def create_compute_time_step_size_kernel_for_adaptive_time_stepping():
  return """
  const double timeStepSize = repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize();
"""


def create_compute_new_time_step_size_kernel_for_local_time_stepping(time_step_relaxation, avoid_staircase_effect, discretisation_steps):
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
        """ + str(time_step_relaxation) + """ * marker.h()(0) / {{NUMBER_OF_VOLUMES_PER_AXIS}} / maxEigenvalue,
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
  #+ set_time_step_size
    
    
def create_compute_new_time_step_size_kernel_for_adaptive_time_stepping():
  return """
    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue( maxEigenvalue );  
    
    // This is a value set for stats reasons. We'll ignore it later as we 
    // ask for a new valid time step size from getAdmissibleTimeStepSize().
    //const double newTimeStepSize = 0.0;
    const double newTimeStepSize =  repositories::{{SOLVER_INSTANCE}}.getMinPatchSize(false)>0.0 ?
      repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize() * marker.h()(0) / repositories::{{SOLVER_INSTANCE}}.getMinPatchSize(false) :
      0.0;

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


class SolverVariant(Enum):
  WithVirtualFunctions = 0,
  Stateless            = 1,
  Accelerator          = 2


class RiemannKernelVariant(Enum):
  PatchWiseAoSStack                = 0
  PatchWiseAoSoAStack              = 1
  BatchedAoSStack                  = 2
  BatchedAoSoAStack                = 3
  PatchWiseSplitTermsAoSStack      = 4
  PatchWiseSplitTermsAoSoAStack    = 5
  PatchWiseAoSHeap                 = 6
  PatchWiseAoSoAHeap               = 7
  BatchedAoSHeap                   = 8
  BatchedAoSoAHeap                 = 9
  PatchWiseSplitTermsAoSHeap       = 10
  PatchWiseSplitTermsAoSoAHeap     = 11
  PatchWiseInSitu                  = 12
  BatchedInSitu                    = 13
    

def create_compute_Riemann_kernel_for_Rusanov(flux_implementation, 
                                              ncp_implementation, 
                                              source_implementation, 
                                              compute_max_eigenvalue_of_next_time_step, 
                                              solver_variant,
                                              riemann_kernel_variant):
  """
  
  I return only the unqualified function call, i.e. without any namespaces.
  So by setting the right namespace as prefix, you can direct it to particular
  implementations.
  
  solver_variant: SolverVariant
  
  riemann_kernel_variant: RiemannKernelVariant
  
  """  
  RiemannKernelCalls = {
    RiemannKernelVariant.PatchWiseAoSStack:              "timeStepWithRusanov_overlap1_patchwise_call_stack",
    RiemannKernelVariant.PatchWiseAoSoAStack:            "timeStepWithRusanov_overlap1_patchwise_call_stack",
    RiemannKernelVariant.BatchedAoSStack:                "timeStepWithRusanov_overlap1_batched_call_stack",
    RiemannKernelVariant.BatchedAoSoAStack:              "timeStepWithRusanov_overlap1_batched_call_stack",
    RiemannKernelVariant.PatchWiseSplitTermsAoSStack:    "timeStepWithRusanov_overlap1_patchwise_split_terms_call_stack",
    RiemannKernelVariant.PatchWiseSplitTermsAoSoAStack:  "timeStepWithRusanov_overlap1_patchwise_split_terms_call_stack",
    RiemannKernelVariant.PatchWiseAoSHeap:               "timeStepWithRusanov_overlap1_patchwise_heap",
    RiemannKernelVariant.PatchWiseAoSoAHeap:             "timeStepWithRusanov_overlap1_patchwise_heap",
    RiemannKernelVariant.BatchedAoSHeap:                 "timeStepWithRusanov_overlap1_batched_heap",
    RiemannKernelVariant.BatchedAoSoAHeap:               "timeStepWithRusanov_overlap1_batched_heap",
    RiemannKernelVariant.PatchWiseSplitTermsAoSHeap:     "timeStepWithRusanov_overlap1_patchwise_split_terms_heap",
    RiemannKernelVariant.PatchWiseSplitTermsAoSoAHeap:   "timeStepWithRusanov_overlap1_patchwise_split_terms_heap",
    RiemannKernelVariant.PatchWiseInSitu:                "timeStepWithRusanov_overlap1_patchwise_in_situ",
    RiemannKernelVariant.BatchedInSitu:                  "timeStepWithRusanov_overlap1_patchwise_in_situ"
  }
  
  EnumeratorTemplateTypes = {
    RiemannKernelVariant.PatchWiseAoSStack:              "::exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator",
    RiemannKernelVariant.PatchWiseAoSoAStack:            "::exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator",
    RiemannKernelVariant.BatchedAoSStack:                "::exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator",
    RiemannKernelVariant.BatchedAoSoAStack:              "::exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator",
    RiemannKernelVariant.PatchWiseSplitTermsAoSStack:    "::exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator",
    RiemannKernelVariant.PatchWiseSplitTermsAoSoAStack:  "::exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator",
    RiemannKernelVariant.PatchWiseAoSHeap:              "::exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator",
    RiemannKernelVariant.PatchWiseAoSoAHeap:            "::exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator",
    RiemannKernelVariant.BatchedAoSHeap:                "::exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator",
    RiemannKernelVariant.BatchedAoSoAHeap:              "::exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator",
    RiemannKernelVariant.PatchWiseSplitTermsAoSHeap:    "::exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator",
    RiemannKernelVariant.PatchWiseSplitTermsAoSoAHeap:  "::exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator",
    RiemannKernelVariant.PatchWiseInSitu:           None,
    RiemannKernelVariant.BatchedInSitu:             None
  }

  template  = RiemannKernelCalls[riemann_kernel_variant]
  
  if solver_variant == SolverVariant.WithVirtualFunctions:
    template += """_functors(
  patchData,
  {{NUMBER_OF_VOLUMES_PER_AXIS}},
  {{NUMBER_OF_UNKNOWNS}},
  {{NUMBER_OF_AUXILIARY_VARIABLES}},
  {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
  {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
  {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
  {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %},
  [&](
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double * __restrict__ F
  )->void {
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, dt, normal, F );
    {% endif %}
  },
  [&](
    const double * __restrict__                  Q,
    const double * __restrict__     deltaQ,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double * __restrict__ BgradQ
  )->void {
    {% if NCP_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, deltaQ, faceCentre, volumeH, t, dt, normal, BgradQ );
    {% endif %}
  },
  [&](
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  volumeX,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    double * __restrict__ S
  )->void {
    {% if SOURCE_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.sourceTerm( Q, volumeX, volumeH, t, dt, S );
    {% endif %}
  },
  [&](
    const double * __restrict__ Q,
    const tarch::la::Vector<Dimensions,double>&  faceCentre,
    const tarch::la::Vector<Dimensions,double>&  volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal
  )->double {
    return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, dt, normal);
  }
);
  """
  if solver_variant == SolverVariant.Stateless and EnumeratorTemplateTypes[riemann_kernel_variant]==None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_VOLUMES_PER_AXIS}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}>(
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    );
  """
  if solver_variant == SolverVariant.Accelerator and EnumeratorTemplateTypes[riemann_kernel_variant]==None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_VOLUMES_PER_AXIS}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}>(
      targetDevice,
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    );
  """
  if solver_variant == SolverVariant.Stateless and EnumeratorTemplateTypes[riemann_kernel_variant]!=None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_VOLUMES_PER_AXIS}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, {{TEMP_DATA_ENUMERATOR}}>(
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    );
  """
  if solver_variant == SolverVariant.Accelerator and EnumeratorTemplateTypes[riemann_kernel_variant]!=None:
    template += """_static_calls<{{SOLVER_NAME}},{{NUMBER_OF_VOLUMES_PER_AXIS}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}, {{TEMP_DATA_ENUMERATOR}}>(
      targetDevice,
      patchData,
      {% if FLUX_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if NCP_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if SOURCE_IMPLEMENTATION=="<none>" %} false {% else %} true {% endif %},
      {% if COMPUTE_MAX_EIGENVALUE==False %} false {% else %} true {% endif %}
    );
  """
        

  result = jinja2.Template( template, undefined=jinja2.DebugUndefined)
  d= {}
  d[ "FLUX_IMPLEMENTATION" ]         = flux_implementation
  d[ "NCP_IMPLEMENTATION" ]          = ncp_implementation
  d[ "SOURCE_IMPLEMENTATION" ]       = source_implementation
  d[ "COMPUTE_MAX_EIGENVALUE" ]      = compute_max_eigenvalue_of_next_time_step
  #if EnumeratorTemplateTypes[riemann_kernel_variant]==None:
  #  d[ "TEMP_DATA_ENUMERATOR" ]      = "::exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator"
  #else:
  d[ "TEMP_DATA_ENUMERATOR" ]      = EnumeratorTemplateTypes[riemann_kernel_variant]
  return result.render(**d)


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
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
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
      double                                       dt,
      int                                          normal
    ) {% if EIGENVALUES_IMPLEMENTATION=="<user-defined>" %}= 0{% else %} final{% endif %};


    {% if FLUX_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
    /**
     * Depending on the implementation, this variant might be slow as it 
     * lacks an inline define. Also, if you don't want to use ipo aggressively,
     * it might be clever to put the implementation into the header.
     */
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
    {% endif %}
    
    {% if FLUX_IMPLEMENTATION!="<none>" %}
    virtual void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
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
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void nonconservativeProduct(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        BgradQ, // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
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
      double                                       dt,
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
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      double * __restrict__ S,
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
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
  double                                       dt,
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
 double                                       dt,
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
  double                                       dt,
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
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      Offloadable
) {
  {{EIGENVALUES_IMPLEMENTATION}};
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}


{% if FLUX_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
) {
  {% if FLUX_IMPLEMENTATION=="<none>" %}
  tarch::gpuAbort();
  {% else %}
  {{FLUX_IMPLEMENTATION}}
  {% endif %}
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}


{% if NCP_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        BgradQ, // BgradQ[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
) {
  {% if NCP_IMPLEMENTATION=="<none>" %}
  tarch::gpuAbort();
  {% else %}
  {{NCP_IMPLEMENTATION}}
  {% endif %}
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
{% endif %}


{% if SOURCE_TERM_IMPLEMENTATION!="<user-defined>" and STATELESS_PDE_TERMS %}
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
      const double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  volumeCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      double * __restrict__ S,
      Offloadable
) {
  {% if SOURCE_TERM_IMPLEMENTATION=="<none>" %}
  tarch::gpuAbort();
  {% else %}
  {{SOURCE_TERM_IMPLEMENTATION}}
  {% endif %}
}
#if defined(GPUOffloadingOMP)
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
      double                                       dt,
      int                                          normal
    ) override;
    {% endif %}


    {% if FLUX_IMPLEMENTATION=="<user-defined>" %}
    virtual void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
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
      double                                       dt,
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
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void sourceTerm(
       const double * __restrict__ Q,
       const tarch::la::Vector<Dimensions,double>&  volumeCentre,
       const tarch::la::Vector<Dimensions,double>&  volumeH,
       double                                       t,
       double                                       dt,
       double * __restrict__ S,
       Offloadable
     );
    #if defined(GPUOffloadingOMP)
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
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod double maxEigenvalue(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      int                                          normal,
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
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
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void flux(
      const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        F, // F[{{NUMBER_OF_UNKNOWNS}}]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
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
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
    static GPUCallableMethod void nonconservativeProduct(
      const double * __restrict__                  Q,         // Q[5+0],
      const double * __restrict__                  deltaQ,    // [5+0]
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double * __restrict__                        BgradQ,     // BgradQ[5]
      Offloadable
    );
    #if defined(GPUOffloadingOMP)
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
  double                                       dt,
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
  double                                       dt,
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
  double                                       dt,
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
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
GPUCallableMethod double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::maxEigenvalue(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal,
  Offloadable
)  {
  // @todo implement
}
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
{% endif %}


{% if FLUX_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::flux(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ F,
  Offloadable
)  {
  // @todo implement
}
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
{% endif %}


{% if NCP_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::nonconservativeProduct(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const double * __restrict__             deltaQ, // [{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ BgradQ,
  Offloadable
)  {
  // @todo implement
}
    #if defined(GPUOffloadingOMP)
    #pragma omp end declare target
    #endif
{% endif %}



{% if SOURCE_TERM_IMPLEMENTATION=="<user-defined>" and STATELESS_PDE_TERMS %}
    #if defined(GPUOffloadingOMP)
    #pragma omp declare target
    #endif
GPUCallableMethod void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::sourceTerm(
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
    #if defined(GPUOffloadingOMP)
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


