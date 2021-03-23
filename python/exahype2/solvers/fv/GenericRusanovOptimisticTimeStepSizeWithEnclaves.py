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

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor

import peano4.output.Jinja2TemplatedHeaderImplementationFilePair


class UpdateCellWithEnclaves(ReconstructPatchAndApplyFunctor):
  TemplateUpdateCell = """
  #if Dimensions==2
  const int NumberOfBackedUpEntries = ({{NUMBER_OF_VOLUMES_PER_AXIS}}+2)*({{NUMBER_OF_VOLUMES_PER_AXIS}}+2)*( {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}} );
  #else
  const int NumberOfBackedUpEntries = ({{NUMBER_OF_VOLUMES_PER_AXIS}}+2)*({{NUMBER_OF_VOLUMES_PER_AXIS}}+2)*({{NUMBER_OF_VOLUMES_PER_AXIS}}+2)*( {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}} );
  #endif
  
  ::exahype2::fv::validatePatch(
      reconstructedPatch,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      1, // halo
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  ); // previous time step has to be valid
    
  if (marker.isSkeletonCell()) {
    {{PREPROCESS_RECONSTRUCTED_PATCH}}
    
    ::exahype2::fv::copyPatch(
      reconstructedPatch,
      originalPatch,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      1 // halo size
    );
    
    tarch::multicore::BooleanSemaphore maxEigenvalueSemaphore;
    double maxPatchEigenvalue = 0.0;

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
          [&] (
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
          [&] (
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
          [&] (
            const double* __restrict__                   Q,
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal
          ) -> double {
            const double result = repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
            
            tarch::multicore::Lock lock(maxEigenvalueSemaphore);
            maxPatchEigenvalue = std::max( maxPatchEigenvalue, result );
           
            return result;
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
      {{TIME_STAMP}},
      {{TIME_STEP_SIZE}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      reconstructedPatch,
      originalPatch
    );
    
    repositories::{{SOLVER_INSTANCE}}.setMaximumEigenvalue( maxPatchEigenvalue );
    {{FREE_SKELETON_MEMORY}}
    {{POSTPROCESS_UPDATED_PATCH}}
  }
  else { // is an enclave cell
    auto perCellFunctor = [&](double* reconstructedPatch, double* originalPatch, const ::peano4::datamanagement::CellMarker& marker, double t, double dt) -> void {
      {{PREPROCESS_RECONSTRUCTED_PATCH}}
      
      ::exahype2::fv::copyPatch(
        reconstructedPatch,
        originalPatch,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        1 // halo size
      );

      tarch::multicore::BooleanSemaphore maxEigenvalueSemaphore;
      double maxPatchEigenvalue = 0.0;

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
          [&] (
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
          [&] (
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
          [&] (
            const double* __restrict__                   Q,
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal
          ) -> double {
            const double result = repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
            tarch::multicore::Lock lock(maxEigenvalueSemaphore);
            maxPatchEigenvalue = std::max( maxPatchEigenvalue, result );
            return result;
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
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        reconstructedPatch,
        originalPatch
      );

      repositories::{{SOLVER_INSTANCE}}.setMaximumEigenvalue( maxPatchEigenvalue );
      
      {{POSTPROCESS_UPDATED_PATCH}}
    };

    #if defined(UseSmartMPI)
    ::exahype2::SmartEnclaveTask* newEnclaveTask = new ::exahype2::SmartEnclaveTask(
      marker,
      {{TIME_STAMP}},
      {{TIME_STEP_SIZE}},
      reconstructedPatch,
      #if Dimensions==2
      {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}},
      #else
      {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}},
      #endif
      perCellFunctor,
      {{SOLVER_NUMBER}}
    );    
    #else
    ::exahype2::EnclaveTask* newEnclaveTask = new ::exahype2::EnclaveTask(
      marker,
      {{TIME_STAMP}},
      {{TIME_STEP_SIZE}},
      reconstructedPatch,
      #if Dimensions==2
      {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}},
      #else
      {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}},
      #endif
      perCellFunctor
    );    
    #endif
          
    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( newEnclaveTask->getTaskId() );


    #if defined(UseSmartMPI)
    smartmpi::spawn( newEnclaveTask );
    #else
    peano4::parallel::Tasks spawn( 
      newEnclaveTask,
      peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
      peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovFixedTimeStepSizeWithEnclaves" )
    );   
    #endif   
  }
  """      
    
    
  def __init__(self,solver,use_split_loop=False):
    d = {}
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)
    d["USE_SPLIT_LOOP"] = use_split_loop      
    
    ReconstructPatchAndApplyFunctor.__init__(self,
      solver._patch,
      solver._patch_overlap,
      jinja2.Template( self.TemplateUpdateCell ).render(**d),
      solver._reconstructed_array_memory_location,
      "not marker.isRefined() and (" + \
      "repositories::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::Primary or " + \
      "repositories::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::PrimaryAfterGridInitialisation" + \
      ")"
    )
    self.label_name = exahype2.grid.EnclaveLabels.get_attribute_name(solver._name)

    self._solver    = solver



  def get_includes(self):
    return ReconstructPatchAndApplyFunctor.get_includes(self) + """
#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"
#include "exahype2/SmartEnclaveTask.h"

#include "peano4/parallel/Tasks.h"

#include "repositories/SolverRepository.h"

#include <algorithm>

#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Lock.h"
""" + self._solver._get_default_includes() + self._solver.get_user_includes() 


class GenericRusanovOptimisticTimeStepSizeWithEnclaves( EnclaveTaskingFV ):
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, flux=PDETerms.User_Defined_Implementation, ncp=None, plot_grid_properties=False, time_step_relaxation=0.1):
    """
    
    A fixed time stepping scheme with enclave tasking
    
    This is a simple implementation of a FV scheme using a generic 
    Rusanov solver. It applies the concept of enclave tasking and 
    thus exhibits a higher concurrency level than a plain FV 
    counterpart. The price to pay is a higher memory pressure and 
    further admin overhead.
    
    Algorithmic workflow:
    
    The enclave tasking variant here is simpler than the original
    enclave tasking as proposed by Charrier et al. The reason that 
    we keep it simpler is that the baseline code scales better.
    Therefore, it is reasonable to keep the enclave complexity and
    overhead down more aggressively.
    
    The basic idea behind enclave tasking is that each time step 
    consists of two grid sweeps and that we distinguish enclave 
    tasks from skeleton cells. Skeleton cells are cells that are
    adjacent to a resolution transition or adjacent to a domain 
    boundary. The FV steps are distributed among these two sweeps
    as follows:
    
    image:: GenericRusanovFixedTimeStepSizeWithEnclaves_state-transitions.svg
    
    The variant of enclave tasking as it is discussed here has nothing
    in common with the fused tasking as discussed by Charrier and 
    Weinzierl.
    
    
    
    Methods:
  
    We extend the superclass' add_actions_to_perform_time_step(), 
    add_to_Peano4_datamodel() and add_use_data_statements_to_Peano4_solver_step().
    For the actions, I add a further action which administers the task
    spawning over the enclaves. I plug into the data model routines to 
    add the marker to the cell which holds the semaphore/cell number.      
    
    """
    self._time_step_relaxation                = time_step_relaxation

    EnclaveTaskingFV.__init__(self, name, patch_size, 1, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties)
   
    self.set_implementation(flux=flux,ncp=ncp)


  def add_entries_to_text_replacement_dictionary(self,d):
    """
     
     d: Dictionary of string to string
        in/out argument
    
    """
    EnclaveTaskingFV.add_entries_to_text_replacement_dictionary(self,d)
    d[ "TIME_STEP_SIZE" ]                     = "repositories::"+d[ "SOLVER_INSTANCE" ] + ".getMinTimeStepSize()"
    d[ "TIME_STAMP" ]                         = "repositories::"+d[ "SOLVER_INSTANCE" ] + ".getMinTimeStamp()"
    d[ "TIME_STEP_RELAXATION" ]               = self._time_step_relaxation
   

  def create_action_sets(self):
    EnclaveTaskingFV.create_action_sets(self)

    self._action_set_project_patch_onto_faces            = ProjectPatchOntoFaces(self, 
      self._store_cell_data_default_predicate() + " and (" + \
         "(repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary                         and marker.isSkeletonCell() ) " + \
      "or (repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation  and marker.isSkeletonCell() ) " + \
      "or (repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary                       and marker.isEnclaveCell() ) " + \
      "or (repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation )" + \
      ")"
    )
    self._action_set_update_cell = UpdateCellWithEnclaves(self)


  #def create_data_structures(self):
  #  """
  #  
  #  See EnclaveTasking.create_data_structures()
  #  
  #  """    
  #  self._initialisation_sweep_predicate = "(" + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" + \
  #    ")"
  #    
  #  self._first_iteration_after_initialisation_predicate = "(" + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation or " + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingInitialCondition" + \
  #  ")"
  #
  #  self._primary_sweep_predicate = "(" + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryWithRollback or " + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation" + \
  #    ")"

  #  self._primary_sweep_or_plot_predicate = "(" + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryWithRollback or " + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation or " + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingInitialCondition or " + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Plotting " + \
  #    ")"

  #  self._primary_or_initialisation_sweep_predicate= "(" + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation or " + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryWithRollback or " + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation" + \
  #    ")"

  #  self._secondary_sweep_predicate = "(" + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary" + \
  #    ")"

  #  self._secondary_sweep_or_grid_construction_predicate = "(" + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary or " + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridConstruction" + \
  #    ")"

  #  self._secondary_sweep_or_grid_initialisation_predicate = "(" + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary or " + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" + \
  #    ")"

  #  self._secondary_sweep_or_grid_initialisation_or_plot_predicate = "(" + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary or " + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation or " + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingInitialCondition or " + \
  #    "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Plotting " + \
  #    ")"

  #  EnclaveTaskingFV.create_data_structures(self)


  #def add_to_Peano4_datamodel( self, datamodel, verbose ):
  #  EnclaveTaskingFV.add_to_Peano4_datamodel(self,datamodel, verbose)
  #  datamodel.add_cell(self._reconstructed_patch_backup)

 
  #def add_use_data_statements_to_Peano4_solver_step(self, step):
  #  EnclaveTaskingFV.add_use_data_statements_to_Peano4_solver_step(self,step)
  #  step.use_cell(self._reconstructed_patch_backup)
