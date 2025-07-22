// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "repositories/SolverRepository.h"

#include "kernels/{{SOLVER_NAME}}/ReconstructPatch.h"
#include "kernels/{{SOLVER_NAME}}/RiemannSolver.h"

#include "exahype2/CellData.h"
#include "exahype2/fv/PatchUtils.h"

#include "peano4/parallel/parallel.h"

#include <algorithm>
#include <cstring>

tarch::logging::Log {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log("{{NAMESPACE | join("::")}}::{{CLASSNAME}}");
int                 {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_enclaveTaskTypeId(peano4::parallel::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}"));

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getEnclaveTaskTypeId() {
  return _enclaveTaskTypeId;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::applyKernelToPatch(
  const peano4::datamanagement::CellMarker&       marker,
  const double                                    timeStamp,
  const double                                    timeStepSize,
  {{STORAGE_PRECISION}}* NOALIAS                  QIn,
  {{STORAGE_PRECISION}}* NOALIAS const            faces[1][TwoTimesD],
  {{STORAGE_PRECISION}}* NOALIAS                  QOut
) {
  std::swap(QIn, QOut);
  kernels::{{SOLVER_NAME}}::reconstructPatch(QIn, faces[0], QOut);
  std::swap(QIn, QOut);

#if PeanoDebug > 0
  ::exahype2::fv::validatePatch(
    QIn,
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    {{HALO_SIZE}}, // Halo size
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + ")"
  ); // Previous time step has to be valid
#endif

  {{PREPROCESS_RECONSTRUCTED_PATCH}}
  {% if COMPUTE_MAX_EIGENVALUE %}
  const auto maxEigenvalue = {{COMPUTE_KERNEL_CALL}}
  {% else %}
  const auto _ = {{COMPUTE_KERNEL_CALL}}
  {% endif %}
  {{POSTPROCESS_UPDATED_PATCH}}

#if PeanoDebug > 0
  ::exahype2::fv::validatePatch(
    QOut,
    {{NUMBER_OF_UNKNOWNS}},
    {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {{NUMBER_OF_VOLUMES_PER_AXIS}},
    0, // Halo size
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + ")"
  ); // Outcome has to be valid
#endif

  {{COMPUTE_NEW_TIME_STEP_SIZE}}
  repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, timeStamp + timeStepSize, tarch::la::min(marker.h()), tarch::la::max(marker.h()));
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(
  const peano4::datamanagement::CellMarker&       marker,
  const double                                    timeStamp,
  const double                                    timeStepSize,
  {{STORAGE_PRECISION}}* NOALIAS                  QIn,
  {{STORAGE_PRECISION}}* NOALIAS const            faces[1][TwoTimesD],
  {{STORAGE_PRECISION}}* NOALIAS                  QOut
):
  ::exahype2::EnclaveTask<{{STORAGE_PRECISION}}>(
    _enclaveTaskTypeId,
    marker,
    timeStamp,
    timeStepSize,
    QIn,
    faces,
    QOut,
    [&]() -> void {
      applyKernelToPatch(
        _marker,
        _t,
        _dt,
        _QIn,
        _faces,
        _QOut
      );
    }
  )
{
  setPriority({{ENCLAVE_TASK_PRIORITY}});
}

{% if CAN_FUSE %}

#include "kernels/{{SOLVER_NAME}}/ReconstructPatchFused.h"
#include "kernels/{{SOLVER_NAME}}/RiemannSolverFused.h"

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::fuse(const std::list<Task*>& otherTasks) {
  const int numberOfPatches = otherTasks.size() + 1;

  logDebug("fuse()", "Rank " << tarch::mpi::Rank::getInstance().getRank()
    << " asked to fuse " << numberOfPatches << " patches of type " << getTaskType() << " into "
    << {{NUMBER_OF_ENCLAVE_TASKS}} << " asynchronous enclave tasks on target device " << TARGET_DEVICE
  );

  ::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}, 1> patchData(
    numberOfPatches,
    tarch::MemoryLocation::Pinned,
    TARGET_DEVICE
  );

  int patchIndex = 0;
  for (auto& otherTask: otherTasks) {
    patchData.QIn[patchIndex]               = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_QIn;
    for (int f = 0; f < TwoTimesD; ++f) {
      patchData.faces[0][patchIndex][f]     = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_faces[0][f];
    }
    patchData.QOut[patchIndex]              = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_QOut;
    patchData.cellSize[patchIndex]          = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_marker.h();
    patchData.cellCentre[patchIndex]        = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_marker.x();
    patchData.maxEigenvalue[patchIndex]     = 0.0;
    delete otherTask;
    patchIndex++;
  }
  patchData.t                             = _t;
  patchData.dt                            = _dt;
  patchData.QIn[patchIndex]               = _QIn;
  for (int f = 0; f < TwoTimesD; ++f) {
    patchData.faces[0][patchIndex][f]     = _faces[0][f];
  }
  patchData.QOut[patchIndex]              = _QOut;
  patchData.cellCentre[patchIndex]        = _marker.x();
  patchData.cellSize[patchIndex]          = _marker.h();
  patchData.maxEigenvalue[patchIndex]     = 0.0;

  patchData.QInFlattened = tarch::accelerator::MemoryManager::getInstance().allocate<{{STORAGE_PRECISION}}>(
    numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %},
    tarch::MemoryLocation::Pinned,
    TARGET_DEVICE
  );
  patchData.facesFlattened = tarch::accelerator::MemoryManager::getInstance().allocate<{{STORAGE_PRECISION}}>(
    numberOfPatches * TwoTimesD * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %},
    tarch::MemoryLocation::Pinned,
    TARGET_DEVICE
  );
  patchData.QOutFlattened = tarch::accelerator::MemoryManager::getInstance().allocate<{{STORAGE_PRECISION}}>(
    numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %},
    tarch::MemoryLocation::Pinned,
    TARGET_DEVICE
  );

  // Before swap: QIn is with halo, QOut is without halo
  // After swap: QIn is without halo, QOut is with halo (that is reconstructed as next step)
  std::swap(patchData.QInFlattened, patchData.QOutFlattened);
  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    std::swap(patchData.QIn[patchIndex], patchData.QOut[patchIndex]);

    std::copy_n(
      patchData.QIn[patchIndex],
      {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %},
      &patchData.QInFlattened[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}]
    );

    for (int f = 0; f < TwoTimesD; ++f) {
      std::copy_n(
        patchData.faces[0][patchIndex][f],
        {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %},
        &patchData.facesFlattened[
          patchIndex * TwoTimesD * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %} // Skip whole patches
          + f * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %} // Move to the f-th face inside the patch
        ]
      );
    }
  }

  kernels::{{SOLVER_NAME}}::reconstructPatchFused(patchData, TARGET_DEVICE);

  // QOut of patch reconstruction is now QIn for the Riemann solver
  // QIn for patch reconstruction is now QOut of the Riemann solver
  std::swap(patchData.QInFlattened, patchData.QOutFlattened);

  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    auto* const QIn = &patchData.QInFlattened[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}];

#if PeanoDebug > 0
    ::exahype2::fv::validatePatch(
      QIn,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{HALO_SIZE}}, // Halo size
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + ")"
    ); // Previous time step has to be valid
#endif

    {{PREPROCESS_RECONSTRUCTED_PATCH}}
  }

  const auto maxEigenvalue = {{FUSED_COMPUTE_KERNEL_CALL}}

  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    std::swap(patchData.QIn[patchIndex], patchData.QOut[patchIndex]);
    std::copy_n(
      &patchData.QOutFlattened[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}],
      {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %},
      patchData.QOut[patchIndex]
    );

    auto* const QOut = patchData.QOut[patchIndex];

    {{POSTPROCESS_UPDATED_PATCH}}

#if PeanoDebug > 0
    ::exahype2::fv::validatePatch(
      QOut,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      0, // Halo size
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + ")"
    ); // Outcome has to be valid
#endif
  }

  tarch::accelerator::MemoryManager::getInstance().free(patchData.QInFlattened, TARGET_DEVICE);
  tarch::accelerator::MemoryManager::getInstance().free(patchData.facesFlattened, TARGET_DEVICE);
  tarch::accelerator::MemoryManager::getInstance().free(patchData.QOutFlattened, TARGET_DEVICE);

  {{COMPUTE_NEW_TIME_STEP_SIZE}}
  repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, _t + _dt, tarch::la::min(_marker.h()), tarch::la::max(_marker.h()));
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::fuse(::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}>& patchData) {
  // Before swap: QIn is with halo, QOut is without halo
  // After swap: QIn is without halo, QOut is with halo (that is reconstructed as next step)
  std::swap(patchData.QInFlattened, patchData.QOutFlattened);
  kernels::{{SOLVER_NAME}}::reconstructPatchFused(patchData, TARGET_DEVICE);
  // QOut of patch reconstruction is now QIn for the Riemann solver
  // QIn for patch reconstruction is now QOut of the Riemann solver
  std::swap(patchData.QInFlattened, patchData.QOutFlattened);

  const auto maxEigenvalue = {{FUSED_COMPUTE_KERNEL_CALL}}

  {{COMPUTE_NEW_TIME_STEP_SIZE}}
  repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, patchData.t + patchData.dt, tarch::la::min(patchData.cellSize[0]), tarch::la::max(patchData.cellSize[0]));
}
{% endif %}
