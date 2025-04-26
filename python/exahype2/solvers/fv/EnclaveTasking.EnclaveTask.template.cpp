// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "config.h"

{{SOLVER_INCLUDES}}

#include "exahype2/CellData.h"
#include "exahype2/EnclaveTask.h"
#include "exahype2/fv/PatchUtils.h"

#include "peano4/parallel/parallel.h"

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/mpi/DoubleMessage.h"
#include "tarch/mpi/IntegerMessage.h"
#include "tarch/timing/Measurement.h"

#include <algorithm>
#include <cstring>

tarch::logging::Log {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log("{{NAMESPACE | join("::")}}::{{CLASSNAME}}");
int                 {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_enclaveTaskTypeId(peano4::parallel::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}"));

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getEnclaveTaskTypeId() {
  return _enclaveTaskTypeId;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyKernelToPatch(
  const peano4::datamanagement::CellMarker&       marker,
  double                                          timeStamp,
  double                                          timeStepSize,
  /*const*/ {{STORAGE_PRECISION}}* noalias const oldQWithHalo,
  {{STORAGE_PRECISION}}* noalias const       newQ
) {
  {{PREPROCESS_RECONSTRUCTED_PATCH}}
  {% if COMPUTE_MAX_EIGENVALUE %}
  const double maxEigenvalue = {{COMPUTE_KERNEL_CALL}}
  {% else %}
  const double _ = {{COMPUTE_KERNEL_CALL}}
  {% endif %}
  {{POSTPROCESS_UPDATED_PATCH}}

  #if PeanoDebug > 0
  ::exahype2::fv::validatePatch(
    newQ,
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    0, // Halo size
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + ")"
  ); // Outcome has to be valid
  #endif

  {{COMPUTE_NEW_TIME_STEP_SIZE}}
  repositories::{{SOLVER_INSTANCE}}.update(
    newTimeStepSize,
    timeStamp + timeStepSize,
    marker.h()(0)
  );
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(
  const peano4::datamanagement::CellMarker&       marker,
  double                                          timeStamp,
  double                                          timeStepSize,
  /*const*/ {{STORAGE_PRECISION}}* noalias const oldQWithHalo,
  {{STORAGE_PRECISION}}* noalias const       newQ
):
  ::exahype2::EnclaveTask<{{STORAGE_PRECISION}}>(
    _enclaveTaskTypeId,
    marker,
    timeStamp,
    timeStepSize,
    oldQWithHalo,
    newQ,
    {% if DIMENSIONS == 2 %}
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}},
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}},
    {% else %}
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}},
    {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}},
    {% endif %}
    [&]() -> void {
      applyKernelToPatch(
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
  {% if CAN_FUSE %}
  return true;
  {% else %}
  return false;
  {% endif %}
}

{% if CAN_FUSE %}
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::fuse(const std::list<Task*>& otherTasks) {
  const int numberOfPatches = otherTasks.size() + 1;

  logDebug("fuse()", "Rank " << tarch::mpi::Rank::getInstance().getRank()
    << " asked to fuse " << numberOfPatches << " patches of type " << getTaskType() << " into one large enclave task on device " << TARGET_DEVICE);

  ::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}> patchData(
    numberOfPatches,
    tarch::MemoryLocation::Pinned,
    TARGET_DEVICE
  );

  int patchIndex = 0;
  for (auto& p: otherTasks) {
    patchData.QIn[patchIndex]          = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_inputValues;
    patchData.QOut[patchIndex]         = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_outputValues;
    patchData.cellCentre[patchIndex]   = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_marker.x();
    patchData.cellSize[patchIndex]     = static_cast<{{NAMESPACE | join("::")}}::{{CLASSNAME}}*>(p)->_marker.h();
    delete p;
    patchIndex++;
  }
  patchData.t = _t;
  patchData.dt = _dt;
  patchData.QIn[patchIndex]            = _inputValues;
  patchData.QOut[patchIndex]           = _outputValues;
  patchData.cellCentre[patchIndex]     = _marker.x();
  patchData.cellSize[patchIndex]       = _marker.h();

  patchData.QIn1D = tarch::accelerator::MemoryManager::getInstance().allocate<{{STORAGE_PRECISION}}>(
    numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %},
    tarch::MemoryLocation::Pinned,
    TARGET_DEVICE
  );
  patchData.QOut1D = tarch::accelerator::MemoryManager::getInstance().allocate<{{STORAGE_PRECISION}}>(
    numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %},
    tarch::MemoryLocation::Pinned,
    TARGET_DEVICE
  );
  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    auto* const oldQWithHalo = patchData.QOut[patchIndex];

    {{PREPROCESS_RECONSTRUCTED_PATCH}}

    std::copy_n(
      patchData.QIn[patchIndex],
      {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %},
      &patchData.QIn1D[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %}]
    );
  }

  const auto maxEigenvalue = {{FUSED_COMPUTE_KERNEL_CALL}}

  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    std::copy_n(
      &patchData.QOut1D[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}],
      {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %},
      patchData.QOut[patchIndex]
    );

    auto* const newQ = patchData.QOut[patchIndex];
    {{POSTPROCESS_UPDATED_PATCH}}

    #if PeanoDebug > 0
    ::exahype2::fv::validatePatch(
      newQ,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      0, // Halo size
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + ")"
    ); // Outcome has to be valid
    #endif
  }

  {{COMPUTE_NEW_TIME_STEP_SIZE}}
  repositories::{{SOLVER_INSTANCE}}.update(
    newTimeStepSize,
    _t + _dt,
    _marker.h()(0)
  );

  tarch::accelerator::MemoryManager::getInstance().free(patchData.QIn1D, TARGET_DEVICE);
  tarch::accelerator::MemoryManager::getInstance().free(patchData.QOut1D, TARGET_DEVICE);

  return false;
}
{% endif %}
