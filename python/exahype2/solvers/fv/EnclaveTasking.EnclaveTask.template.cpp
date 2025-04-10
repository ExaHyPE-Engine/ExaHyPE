// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

{{SOLVER_INCLUDES}}

#include "config.h"

#include "exahype2/CellData.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"

#include "peano4/parallel/parallel.h"

#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/mpi/DoubleMessage.h"
#include "tarch/mpi/IntegerMessage.h"
#include "tarch/timing/Measurement.h"

#include <string.h>
#include <memory>

tarch::logging::Log {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}" );
int                 {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_enclaveTaskTypeId(peano4::parallel::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}"));

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getEnclaveTaskTypeId() {
  return _enclaveTaskTypeId;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyKernelToCell(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      timeStamp,
  double                                      timeStepSize,
  {{STORAGE_PRECISION}}* __restrict__         oldQWithHalo,
  {{STORAGE_PRECISION}}* __restrict__         newQ
) {
  {% if COMPUTE_MAX_EIGENVALUE %}
  const double maxEigenvalue = {{COMPUTE_KERNEL_CALL}}
  {% else %}
  const double _ = {{COMPUTE_KERNEL_CALL}}
  {% endif %}

  ::tarch::freeMemory(oldQWithHalo, ::tarch::MemoryLocation::Heap);

  {% if COMPUTE_MAX_EIGENVALUE %}
  return maxEigenvalue;
  {% else %}
  return 0.0;
  {% endif %}
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(
  const ::peano4::datamanagement::CellMarker& marker,
  double                                      timeStamp,
  double                                      timeStepSize,
  {{STORAGE_PRECISION}}* __restrict__         oldQWithHalo,
  {{STORAGE_PRECISION}}* __restrict__         newQ
):
  ::exahype2::EnclaveTask<{{STORAGE_PRECISION}}>(
    _enclaveTaskTypeId,
    marker,
    timeStamp,
    timeStepSize,
    oldQWithHalo,
    newQ,
    #if Dimensions==2
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}},
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}},
    #else
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}},
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}},
    #endif
    [&]() -> void {
      _maxEigenvalue = applyKernelToCell(
        _marker,
        _t,
        _dt,
        _inputValues,
        _outputValues
      );
    }
  )
{
  setPriority({{ENCLAVE_TASK_PRIORITY}});
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::canFuse() const {
  {% if STATELESS_PDE_TERMS %}
  return true;
  {% else %}
  return false;
  {% endif %}
}

{% if STATELESS_PDE_TERMS %}

/**
 * Also merge the current task
 */
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::fuse(const std::list<Task*>& otherTasks, int targetDevice) {
  logDebug("fuse()", "Rank " << tarch::mpi::Rank::getInstance().getRank()
    << " asked to fuse " << (otherTasks.size() + 1) << " tasks of type " << getTaskType() << " into one large enclave task on device " << targetDevice);

  ::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}> patchData(otherTasks.size() + 1, ::tarch::MemoryLocation::Heap, targetDevice);

  int currentTask = 0;
  for (auto& p: otherTasks) {
    patchData.QIn[currentTask]         = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_inputValues;
    patchData.cellCentre[currentTask]  = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_marker.x();
    patchData.cellSize[currentTask]    = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_marker.h();
    patchData.t[currentTask]           = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_t;
    patchData.dt[currentTask]          = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_dt;
    patchData.id[currentTask]          = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_taskNumber;
    patchData.QOut[currentTask]        = ::tarch::allocateMemory<{{STORAGE_PRECISION}}>(_numberOfResultValues, ::tarch::MemoryLocation::Heap, targetDevice);
    //patchData.QOut[currentTask]        = reinterpret_cast<{{STORAGE_PRECISION}}*>(::tarch::accelerator::MemoryManager::getInstance().allocate(_numberOfResultValues * sizeof({{STORAGE_PRECISION}}), ::tarch::MemoryLocation::Heap, targetDevice));
    delete p;
    currentTask++;
  }
  patchData.QIn[currentTask]          = _inputValues;
  patchData.cellCentre[currentTask]   = _marker.x();
  patchData.cellSize[currentTask]     = _marker.h();
  patchData.t[currentTask]            = _t;
  patchData.dt[currentTask]           = _dt;
  patchData.id[currentTask]           = _taskNumber;
  patchData.QOut[currentTask]         = ::tarch::allocateMemory<{{STORAGE_PRECISION}}>(_numberOfResultValues, ::tarch::MemoryLocation::Heap, targetDevice);
  //patchData.QOut[currentTask]         = reinterpret_cast<{{STORAGE_PRECISION}}*>(::tarch::accelerator::MemoryManager::getInstance().allocate(_numberOfResultValues * sizeof({{STORAGE_PRECISION}}), ::tarch::MemoryLocation::Heap, targetDevice));

  const double maxEigenvalue = {{FUSED_COMPUTE_KERNEL_CALL}}

  //
  // ==================
  // Bring back results
  // ==================
  //
  for (int i = 0; i < patchData.numberOfCells; i++) {
    // Delete reconstructed data which has been allocated before spawning the tasks.
    ::tarch::freeMemory(patchData.QIn[i], ::tarch::MemoryLocation::Heap, targetDevice);
    ::exahype2::EnclaveBookkeeping<{{STORAGE_PRECISION}}>::getInstance().bookmarkFinishedTask(patchData.id[i], _numberOfResultValues, patchData.QOut[i], maxEigenvalue);
  }

  // Don't rerun, so return false.
  return false;
}

{% endif %}
