# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRKFDActionSet import AbstractRKFDActionSet

import peano4
import jinja2


from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor


class EmptyPreprocessSolution(AbstractRKFDActionSet):
  """!
  
  PreprocessSolution differs from other action sets, as I only create it once. See 
  FV.create_action_sets(). Once the instance does exist, you can tailor it. But you
  can also completely overwrite it.
  
  """
  def __init__(self,solver):
    AbstractRKFDActionSet.__init__(self,solver)
    # Is there for compatibility reasons
    self.guard               = "true"
    self.descend_invocation_order            = 1 


  def get_body_of_operation(self,operation_name):
    result = ""
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")



class PreprocessReconstructedSolutionWithHalo(ReconstructPatchAndApplyFunctor):
  """!
  
  Preprocess the solution and reconstruct halo for this
  
  This action set preprocesses the solution. By default, it only plugs into the 
  very first grid sweep of a time step. That is, if you have a Runge-Kutta solver
  and/or enclave solver, these guys usually need multiple sweeps. We only plug 
  into the very first one.
  
  If it becomes active, it first of all creates an array called oldQWithHalo. It
  is held on the heap and will be destroyed automatically later on. This array
  is properly befilled. You can postprocess it, but if you alter entries 
  therein, these alterations will be lost after the preprocessing step.
  
  ## Enclave solver variant
  
  The action set can work in an enclave solver style or as plain invocation. 
  Plain invocation means we just reconstruct the patch plus halo, invoke a 
  compute kernel on it, and then free this reconstruction data structure 
  again.
  
  Enclave-style means that we do the reconstruction, but then embed the actual
  operation and the freeing into a task. We use the actual enclave task number
  as task number. Peano does not allow multiple tasks in the system with the 
  same number, so if an enclave task is spawned afterwards, we have a read-write-read
  dependency.
  
  @todo Das ist falsch
  
  
  """  
  def __init__(self,
               solver,
               compute_kernel_implementation,
               enclave_task_cell_label = None
               ):
    """!
    
    Construct the preprocessing
    
    If we work with an enclave task, the modelling is pretty straightforward. 
    However, we have to explicitly delete the reconstructed data structure
    oldQWithHalo, as we tell the parent class not to delete stuff. If they 
    deleted the oldQWithHalo, it would be at the end of the function. At this
    point our task might not have finished yet. We need the delete within the 
    task.
    
    @param enclave_task_cell_label: String or None
      If this field is none, we do not use any tasking. If it is a string, we assume
      it denotes a cell label (usually it is something similar to 
      fineGridCellMySolver_FD4CellLabel). In this case, we spawn a separate task as
      enclave task.
    
    """  
    if enclave_task_cell_label==None:
        functor_implementation = compute_kernel_implementation
        reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarch
    else:
        functor_implementation = """
            auto taskBody = [=,this]() -> bool { """ + compute_kernel_implementation  + """
                ::tarch::freeMemory(oldQWithHalo, tarch::MemoryLocation::Heap );
                return false;
            };

            if (
              marker.willBeEnclaveCell() 
              or 
              marker.hasBeenEnclaveCell() 
            ) {
              taskBody();
            }
            else {
              assertion( """ + enclave_task_cell_label + """.getSemaphoreNumber()<0 );
              
              tarch::multicore::Task* newTask = new tarch::multicore::TaskWithCopyOfFunctor (
                tarch::multicore::Task::DontFuse,
                tarch::multicore::Task::DefaultPriority,
                taskBody
              );

              int newTaskNumber = ::exahype2::EnclaveTask::reserveTaskNumber();
            
              tarch::multicore::spawnTask( 
                newTask, 
                tarch::multicore::NoInDependencies, 
                newTaskNumber
              );
              
              """ + enclave_task_cell_label + """.setSemaphoreNumber( newTaskNumber );
            }
            """
        reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete
        
    super(PreprocessReconstructedSolutionWithHalo,self).__init__(patch = solver._patch,
                                             patch_overlap                       = solver._patch_overlap_new,
                                             functor_implementation              = functor_implementation,
                                             reconstructed_array_memory_location = reconstructed_array_memory_location, 
                                             guard                               = "repositories::instanceOf" + solver._name + ".isFirstGridSweepOfTimeStep()",
                                             add_assertions_to_halo_exchange     = False
                                             )
    self._solver = solver

  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def get_includes(self):
    return super(PreprocessReconstructedSolutionWithHalo,self).get_includes() + """
#include <functional>
#include "exahype2/fd/PatchUtils.h"
#include "exahype2/EnclaveTask.h"
#include "tarch/multicore/Tasks.h"
""" + self._solver._get_default_includes() + self._solver.user_action_set_includes 


class CellWisePreprocessSolution(AbstractRKFDActionSet):
  """!
  
  Preprocess solution mesh cell by mesh cell without mesh cell interactions
  
  PreprocessSolution differs from other action sets, as I only create it once. See 
  FV.create_action_sets(). Once the instance does exist, you can tailor it. But you
  can also completely overwrite it.
  
  The postprocessing plugs into touch cell last time. It is the last action set added
  by the default implementation. Therefore, it is the first action set of which 
  touchCellLastTime() is called. The reason why I plug into the leaving of the cell
  is that some solvers may add further action sets to the solve. Enclave tasking for
  example adds the merger as additional action set. These action sets plug into 
  touchCellFirstTime() - and consequently their first time is called after the
  postprocessing's first time. By making the postprocessing use touchCellLastTime(),
  I ensure that any additional action set added by a subclass can still precede the 
  postprocessing.
  
  
  """
  def __init__(self,solver):
    AbstractRKFDActionSet.__init__(self,solver)
    self.guard               = "true"
    self._compute_kernel     = ""
    self.descend_invocation_order            = 2


  def add_postprocessing_kernel(self, operation_per_point):
    """
    
    Add a code snippet which is applied to each and every point. You have the following
    variables which are well-defined:
    
    - value: Is a pointer to the current finite volume's data
    - volumeX: A tarch::la::Vector over doubles
    - volumeH: A tarch::la::Vector over doubles
    
    
    operation_per_point: String
      C/C++ code snippet
    
    """
    self._compute_kernel += """
  if (
    not marker.hasBeenRefined() 
    and 
    {{PREDICATE}}
  ) { 
    logTraceIn( "touchCellFirstTime(...)" );
    int index = 0;
    dfor( volume, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} ) {
      double* value   = fineGridCell{{UNKNOWN_IDENTIFIER}}.value + index;
      auto    volumeX = ::exahype2::fv::getVolumeCentre( marker.x(), marker.h(), {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}, volume);
      auto    volumeH = ::exahype2::fv::getVolumeSize( marker.h(), {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}});
      
      """ + operation_per_point + """
      
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
    logTraceOut( "touchCellFirstTime(...)" );
  } 
"""
    

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]           = jinja2.Template(self.guard, undefined=jinja2.DebugUndefined).render(**d)
      result = jinja2.Template(self._compute_kernel, undefined=jinja2.DebugUndefined).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
