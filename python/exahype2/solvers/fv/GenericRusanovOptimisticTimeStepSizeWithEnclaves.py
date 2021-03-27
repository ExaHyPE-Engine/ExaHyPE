# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV                       import *
 
from .PDETerms import PDETerms

import peano4
import exahype2

import jinja2

from .GenericRusanovFixedTimeStepSize import GenericRusanovFixedTimeStepSize 
from .GenericRusanovFixedTimeStepSize import UpdateCell 
from .EnclaveTasking                  import MergeEnclaveTaskOutcome
from .EnclaveTasking                  import EnclaveTaskingFV
from .GenericRusanovAdaptiveTimeStepSizeWithEnclaves import GenericRusanovAdaptiveTimeStepSizeWithEnclaves

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor

import peano4.output.Jinja2TemplatedHeaderImplementationFilePair


class MergeEnclaveTaskOutcomeAndTriggerOptimisticTimeStep(AbstractFVActionSet):
  Template = """
  if ( marker.isEnclaveCell() and not marker.isRefined() and repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::Secondary ) {
    const int taskNumber = fineGridCell{{LABEL_NAME}}.getSemaphoreNumber();

    if ( taskNumber>=0 ) {
      ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndCopyResultOver( taskNumber, fineGridCell{{UNKNOWN_IDENTIFIER}}.value );
    }
    fineGridCell{{LABEL_NAME}}.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
      
    ::exahype2::fv::validatePatch(
      fineGridCell{{UNKNOWN_IDENTIFIER}}.value,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      0,
      std::string(__FILE__) + ": " + std::to_string(__LINE__) + "; marker=" + marker.toString()
    );
    
    if ( tarch::la::greater( {{TIME_STEP_SIZE}}, 0.0 ) ) {
      double timeStampOfOptimisticTask    = repositories::{{SOLVER_INSTANCE}}.getMaxTimeStamp() + repositories::{{SOLVER_INSTANCE}}.getMaxTimeStepSize();
      double timeStepSizeOfOptimisticTask = repositories::{{SOLVER_INSTANCE}}.getPredictedTimeStepSize();
      std::cout << std::endl << "@ " << timeStampOfOptimisticTask << " x " << timeStepSizeOfOptimisticTask << std::endl;


/*      
      
      // fineGridCell{{UNKNOWN_IDENTIFIER}}.value
      auto perCellFunctor = [&](double* reconstructedPatch, double* originalPatch, const ::peano4::datamanagement::CellMarker& marker, double t, double dt) -> void {
        {{PREPROCESS_RECONSTRUCTED_PATCH}}
      
        ::exahype2::fv::copyPatch(
          reconstructedPatch,
          originalPatch,
          {{NUMBER_OF_UNKNOWNS}},
          {{NUMBER_OF_AUXILIARY_VARIABLES}},
          {{NUMBER_OF_VOLUMES_PER_AXIS}}-2,
          1 // halo size
        );

        {% if USE_SPLIT_LOOP %}
        #if Dimensions==2
        ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
        #else
        ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
        #endif
        {% else %}
        #if Dimensions==2
        ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(
        #else
        ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(
        #endif
        {% endif %}
          [&](
            const double* __restrict__                   QL,
            const double* __restrict__                   QR,
            const tarch::la::Vector<Dimensions,double>&  x,
            double                                       dx,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double                                       FL[],
            double                                       FR[]
          ) -> void {
           ::exahype2::fv::splitRusanov1d(
            [] (
             const double * __restrict__ Q,
             const tarch::la::Vector<Dimensions,double>&  faceCentre,
             const tarch::la::Vector<Dimensions,double>&  volumeH,
             double                                       t,
             double                                       dt,
             int                                          normal,
             double * __restrict__                        F
            ) -> void {
              {% if FLUX_IMPLEMENTATION=="<none>" %}
              for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) F[i] = 0.0;
              {% else %}
              repositories::{{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, normal, F );
              {% endif %}
            },
            [] (
              const double* __restrict__                   Q,
              const double * __restrict__                  deltaQ,
              const tarch::la::Vector<Dimensions,double>&  faceCentre,
              const tarch::la::Vector<Dimensions,double>&  volumeH,
              double                                       t,
              double                                       dt,
              int                                          normal,
              double                                       BgradQ[]
            ) -> void {
              {% if NCP_IMPLEMENTATION!="<none>" %}
              repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct( Q, deltaQ, faceCentre, volumeH, t, normal, BgradQ );
              {% endif %}
            },
            [] (
              const double* __restrict__                   Q,
              const tarch::la::Vector<Dimensions,double>&  faceCentre,
              const tarch::la::Vector<Dimensions,double>&  volumeH,
              double                                       t,
              double                                       dt,
              int                                          normal
            ) -> double {
              return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
            },
            QL, QR, x, dx, t, dt, normal,
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            FL,FR,
            {% if FLUX_IMPLEMENTATION=="<none>" %}
            true,
            {% else %}
            false,
            {% endif %}
            {% if NCP_IMPLEMENTATION=="<none>" %}
            true
            {% else %}
            false
            {% endif %}
          );
        },
        [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       dx,
          double                                       t,
          double                                       dt,
          double * __restrict__                        S
        ) -> void {
          repositories::{{SOLVER_INSTANCE}}.sourceTerm(
            Q,
            x, dx, t, dt, 
            S
          );
        },  
          marker.x(),
          marker.h(),
          t,
          dt,
          {{NUMBER_OF_VOLUMES_PER_AXIS}}-2,
          {{NUMBER_OF_UNKNOWNS}},
          {{NUMBER_OF_AUXILIARY_VARIABLES}},
          reconstructedPatch,
          originalPatch
        );

        {{POSTPROCESS_UPDATED_PATCH}}
        
        double maxEigenvalue = ::exahype2::fv::maxEigenvalue_AoS(
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
          t,
          dt,
          {{NUMBER_OF_VOLUMES_PER_AXIS}}-2,
          {{NUMBER_OF_UNKNOWNS}},
          {{NUMBER_OF_AUXILIARY_VARIABLES}},
          originalPatch
        );

        repositories::{{SOLVER_INSTANCE}}.setMaximumEigenvalue( maxEigenvalue );
      };

      static int optimisticEnclaveTaskTypeId = peano4::parallel::Tasks::getTaskType("{{SOLVER_INSTANCE}}Optimistic");
      ::exahype2::EnclaveTask* newOptimisticEnclaveTask = new ::exahype2::EnclaveTask(
        optimisticEnclaveTaskTypeId,
        marker,
xxx        {{TIME_STAMP}},
xxx        {{TIME_STEP_SIZE}},
        reconstructedPatch,
        #if Dimensions==2
xxx        {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}},
        #else
xxx        {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}},
        #endif
        perCellFunctor
      );    
          
xxx    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( newEnclaveTask->getTaskId() );

      peano4::parallel::Tasks spawn( 
        newOptimisticEnclaveTask,
        peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
        peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovFixedTimeStepSizeWithEnclaves" )
      );   
      */
    }
  }
"""


  def __init__(self,solver):
    AbstractFVActionSet.__init__(self,solver)
    self.label_name = exahype2.grid.EnclaveLabels.get_attribute_name(solver._name)

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "LABEL_NAME" ] = self.label_name      
      result = jinja2.Template(self.Template).render(**d)
      pass 
    return result


class GenericRusanovOptimisticTimeStepSizeWithEnclaves( GenericRusanovAdaptiveTimeStepSizeWithEnclaves ):
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, flux=PDETerms.User_Defined_Implementation, ncp=None, plot_grid_properties=False, time_step_relaxation=0.1):
    """
    
    
    
    """
    self._time_step_relaxation                = time_step_relaxation

    GenericRusanovAdaptiveTimeStepSizeWithEnclaves.__init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties)

    self.set_implementation(flux=flux,ncp=ncp)


  def add_actions_to_perform_time_step(self, step):
    """
    
    We invoke the FV superclass' operation and add the enclave labels, but 
    we exchange the update cell thing.
 
    """
    FV.add_actions_to_perform_time_step(self,step)
    step.add_action_set( exahype2.grid.EnclaveLabels(self._name) ) 
    step.add_action_set( MergeEnclaveTaskOutcomeAndTriggerOptimisticTimeStep(self) )

