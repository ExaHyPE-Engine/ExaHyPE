// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "repositories/SolverRepository.h"

#include "kernels/{{SOLVER_NAME}}/ProjectPatch.h"
#include "kernels/{{SOLVER_NAME}}/ReconstructPatch.h"
#include "kernels/{{SOLVER_NAME}}/RiemannSolver.h"

#include "exahype2/CellData.h"
#include "exahype2/fv/PatchUtils.h"

#include "peano4/parallel/parallel.h"

#include <algorithm>
#include <cstring>
#include <unordered_map>

tarch::logging::Log {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log("{{NAMESPACE | join("::")}}::{{CLASSNAME}}");
int                 {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_enclaveTaskTypeId(peano4::parallel::getTaskType("{{NAMESPACE | join("::")}}::{{CLASSNAME}}"));

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getEnclaveTaskTypeId() {
  return _enclaveTaskTypeId;
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}(
  const peano4::datamanagement::CellMarker&       marker,
  const double                                    timeStamp,
  const double                                    timeStepSize,
  {{STORAGE_PRECISION}}* NOALIAS                  QIn,
  {{STORAGE_PRECISION}}* NOALIAS const            faces[3][TWO_TIMES_D],
  bool                                            isBoundary[TWO_TIMES_D],
  {{STORAGE_PRECISION}}* NOALIAS                  QOut
):
  ::exahype2::EnclaveTask<{{STORAGE_PRECISION}}, 3>(
    _enclaveTaskTypeId,
    marker,
    timeStamp,
    timeStepSize,
    QIn,
    faces,
    isBoundary,
    QOut,
    [&]() -> void {
      throw std::runtime_error("Launching individual enclave tasks is not supported anymore");
    }
  )
{
  setPriority({{ENCLAVE_TASK_PRIORITY}});
}

#include "kernels/{{SOLVER_NAME}}/HandleBoundaryFused.h"
#include "kernels/{{SOLVER_NAME}}/ProjectPatchFused.h"
#include "kernels/{{SOLVER_NAME}}/ReconstructPatchFused.h"
#include "kernels/{{SOLVER_NAME}}/RiemannSolverFused.h"

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::fuse(const std::list<Task*>& otherTasks) {
  const int numberOfPatches = otherTasks.size() + 1;

  logDebug("fuse()", "Rank " << tarch::mpi::Rank::getInstance().getRank()
    << " asked to fuse " << numberOfPatches << " patches of type " << getTaskType() << " into "
    << {{NUMBER_OF_ENCLAVE_TASKS}} << " asynchronous enclave tasks on target device " << TARGET_DEVICE
  );

  ::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}, 3> patchData(
    numberOfPatches,
    tarch::MemoryLocation::Pinned,
    TARGET_DEVICE
  );

  std::vector<double> hostFacesOld;
  std::vector<double> hostFacesUpdate;
  std::vector<double> hostFacesNew;
  hostFacesOld.reserve(numberOfPatches * TWO_TIMES_D * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %});
  hostFacesUpdate.reserve(numberOfPatches * TWO_TIMES_D * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %});
  hostFacesNew.reserve(numberOfPatches * TWO_TIMES_D * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %});

  std::unordered_map<{{STORAGE_PRECISION}}*, int>                    facesOldHostToDevice;
  std::unordered_map<{{STORAGE_PRECISION}}*, {{STORAGE_PRECISION}}*> facesOldDeviceToHost;

  std::unordered_map<{{STORAGE_PRECISION}}*, int>                    facesUpdateHostToDevice;
  std::unordered_map<{{STORAGE_PRECISION}}*, {{STORAGE_PRECISION}}*> facesUpdateDeviceToHost;

  std::unordered_map<{{STORAGE_PRECISION}}*, int>                    facesNewHostToDevice;
  std::unordered_map<{{STORAGE_PRECISION}}*, {{STORAGE_PRECISION}}*> facesNewDeviceToHost;

  auto processTaskFaces = [&](Task* task) {
    auto* typedTask = static_cast<{{ENCLAVE_TASK_TYPE}}*>(task);
    for (int f = 0; f < TWO_TIMES_D; ++f) {
      {{STORAGE_PRECISION}}* faceKeyOld = typedTask->_faces[0][f];
      if (facesOldHostToDevice.find(faceKeyOld) == facesOldHostToDevice.end()) {
        facesOldHostToDevice[faceKeyOld] = static_cast<int>(facesOldHostToDevice.size());
        hostFacesOld.insert(hostFacesOld.end(), faceKeyOld, faceKeyOld + {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %});
      }

      {{STORAGE_PRECISION}}* faceKeyUpdate = typedTask->_faces[1][f];
      if (facesUpdateHostToDevice.find(faceKeyUpdate) == facesUpdateHostToDevice.end()) {
        facesUpdateHostToDevice[faceKeyUpdate] = static_cast<int>(facesUpdateHostToDevice.size());
        hostFacesUpdate.insert(hostFacesUpdate.end(), faceKeyUpdate, faceKeyUpdate + {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %});
      }

      {{STORAGE_PRECISION}}* faceKeyNew = typedTask->_faces[2][f];
      if (facesNewHostToDevice.find(faceKeyNew) == facesNewHostToDevice.end()) {
        facesNewHostToDevice[faceKeyNew] = static_cast<int>(facesNewHostToDevice.size());
        hostFacesNew.insert(hostFacesNew.end(), faceKeyNew, faceKeyNew + {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %});
      }
    }
  };

  for (auto& otherTask: otherTasks) {
    processTaskFaces(otherTask);
  }
  processTaskFaces(this);

  {{STORAGE_PRECISION}}* deviceFacesOld    = tarch::accelerator::MemoryManager::getInstance().allocate<{{STORAGE_PRECISION}}>(hostFacesOld.size(), tarch::MemoryLocation::Pinned, TARGET_DEVICE);
  {{STORAGE_PRECISION}}* deviceFacesUpdate = tarch::accelerator::MemoryManager::getInstance().allocate<{{STORAGE_PRECISION}}>(hostFacesUpdate.size(), tarch::MemoryLocation::Pinned, TARGET_DEVICE);
  {{STORAGE_PRECISION}}* deviceFacesNew    = tarch::accelerator::MemoryManager::getInstance().allocate<{{STORAGE_PRECISION}}>(hostFacesNew.size(), tarch::MemoryLocation::Pinned, TARGET_DEVICE);
  tarch::copyTo(deviceFacesOld, hostFacesOld.data(), hostFacesOld.size(), tarch::MemoryLocation::Pinned, TARGET_DEVICE);
  tarch::copyTo(deviceFacesUpdate, hostFacesUpdate.data(), hostFacesUpdate.size(), tarch::MemoryLocation::Pinned, TARGET_DEVICE);
  tarch::copyTo(deviceFacesNew, hostFacesNew.data(), hostFacesNew.size(), tarch::MemoryLocation::Pinned, TARGET_DEVICE);
  hostFacesOld.clear();
  hostFacesUpdate.clear();
  hostFacesNew.clear();

  std::vector<uint64_t> patchIndicesOfBoundaryFaces;
  std::vector<uint8_t>  boundaryFaceNumbers;

  int patchIndex = 0;
  for (auto& otherTask: otherTasks) {
    patchData.QIn[patchIndex]                   = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_QIn;
    patchData.QOut[patchIndex]                  = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_QOut;
    patchData.cellSize[patchIndex]              = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_marker.getH();
    patchData.cellCentre[patchIndex]            = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_marker.getX();
    patchData.maxEigenvalue[patchIndex]         = 0.0;

    for (int f = 0; f < TWO_TIMES_D; ++f) {
      auto* faceKeyOld                          = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_faces[0][f];
      auto deviceIndexOld                       = facesOldHostToDevice.at(faceKeyOld);
      auto* deviceFaceOld                       = deviceFacesOld + deviceIndexOld * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};
      patchData.faces[patchIndex][0][f]         = deviceFaceOld;
      facesOldDeviceToHost[deviceFaceOld]       = faceKeyOld;
      auto* faceKeyUpdate                       = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_faces[1][f];
      auto deviceIndexUpdate                    = facesUpdateHostToDevice.at(faceKeyUpdate);
      auto* deviceFaceUpdate                    = deviceFacesUpdate + deviceIndexUpdate * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};
      patchData.faces[patchIndex][1][f]         = deviceFaceUpdate;
      facesUpdateDeviceToHost[deviceFaceUpdate] = faceKeyUpdate;
      auto* faceKeyNew                          = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_faces[2][f];
      auto deviceIndexNew                       = facesNewHostToDevice.at(faceKeyNew);
      auto* deviceFaceNew                       = deviceFacesNew + deviceIndexNew * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};
      patchData.faces[patchIndex][2][f]         = deviceFaceNew;
      facesNewDeviceToHost[deviceFaceNew]       = faceKeyNew;

      if (static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_isBoundary[f]) {
        patchIndicesOfBoundaryFaces.push_back(patchIndex);
        boundaryFaceNumbers.push_back(f);
      }
    }

    delete otherTask;
    patchIndex++;
  }
  patchData.t                                 = _t;
  patchData.dt                                = _dt;
  patchData.QIn[patchIndex]                   = _QIn;
  patchData.QOut[patchIndex]                  = _QOut;
  patchData.cellCentre[patchIndex]            = _marker.getX();
  patchData.cellSize[patchIndex]              = _marker.getH();
  patchData.maxEigenvalue[patchIndex]         = 0.0;

  for (int f = 0; f < TWO_TIMES_D; ++f) {
    auto* faceKeyOld                          = _faces[0][f];
    auto deviceIndexOld                       = facesOldHostToDevice.at(faceKeyOld);
    auto* deviceFaceOld                       = deviceFacesOld + deviceIndexOld * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};
    patchData.faces[patchIndex][0][f]         = deviceFaceOld;
    facesOldDeviceToHost[deviceFaceOld]       = faceKeyOld;
    auto* faceKeyUpdate                       = _faces[1][f];
    auto deviceIndexUpdate                    = facesUpdateHostToDevice.at(faceKeyUpdate);
    auto* deviceFaceUpdate                    = deviceFacesUpdate + deviceIndexUpdate * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};
    patchData.faces[patchIndex][1][f]         = deviceFaceUpdate;
    facesUpdateDeviceToHost[deviceFaceUpdate] = faceKeyUpdate;
    auto* faceKeyNew                          = _faces[2][f];
    auto deviceIndexNew                       = facesNewHostToDevice.at(faceKeyNew);
    auto* deviceFaceNew                       = deviceFacesNew + deviceIndexNew * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};
    patchData.faces[patchIndex][2][f]         = deviceFaceNew;
    facesNewDeviceToHost[deviceFaceNew]       = faceKeyNew;

    if (_isBoundary[f]) {
        patchIndicesOfBoundaryFaces.push_back(patchIndex);
        boundaryFaceNumbers.push_back(f);
      }
  }
  patchData.numberOfBoundaryFaces = boundaryFaceNumbers.size();
  if (patchData.numberOfBoundaryFaces > 0) {
    patchData.patchIndicesOfBoundaryFaces   = tarch::accelerator::MemoryManager::getInstance().allocate<uint64_t>(patchIndicesOfBoundaryFaces.size(), tarch::MemoryLocation::Pinned, TARGET_DEVICE);
    patchData.boundaryFaceNumbers   = tarch::accelerator::MemoryManager::getInstance().allocate<uint8_t>(boundaryFaceNumbers.size(), tarch::MemoryLocation::Pinned, TARGET_DEVICE);

    std::copy_n(
      patchIndicesOfBoundaryFaces.data(),
      patchIndicesOfBoundaryFaces.size(),
      patchData.patchIndicesOfBoundaryFaces
    );
    std::copy_n(
      boundaryFaceNumbers.data(),
      boundaryFaceNumbers.size(),
      patchData.boundaryFaceNumbers
    );
    patchIndicesOfBoundaryFaces.clear();
    boundaryFaceNumbers.clear();
  }
  patchData.QInFlattened = tarch::accelerator::MemoryManager::getInstance().allocate<{{STORAGE_PRECISION}}>(
    numberOfPatches * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_WITH_HALO_3D}} {% else %} {{PATCH_CARDINALITY_WITH_HALO_2D}} {% endif %},
    tarch::MemoryLocation::Pinned,
    TARGET_DEVICE
  );
  patchData.QOutFlattened = tarch::accelerator::MemoryManager::getInstance().allocate<{{STORAGE_PRECISION}}>(
    numberOfPatches * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %},
    tarch::MemoryLocation::Pinned,
    TARGET_DEVICE
  );

  if (patchData.numberOfBoundaryFaces > 0) {
    kernels::{{SOLVER_NAME}}::handleBoundaryFused(repositories::{{SOLVER_INSTANCE}}, patchData, TARGET_DEVICE);
  }

  // Before swap: QIn is with halo, QOut is without halo
  // After swap: QIn is without halo, QOut is with halo (that is reconstructed as next step)
  std::swap(patchData.QInFlattened, patchData.QOutFlattened);
  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    std::swap(patchData.QIn[patchIndex], patchData.QOut[patchIndex]);

    std::copy_n(
      patchData.QIn[patchIndex],
      {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %},
      &patchData.QInFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %}]
    );
  }

  kernels::{{SOLVER_NAME}}::reconstructPatchFused(patchData, TARGET_DEVICE);

  // QOut of patch reconstruction is now QIn for the Riemann solver
  // QIn for patch reconstruction is now QOut of the Riemann solver
  std::swap(patchData.QInFlattened, patchData.QOutFlattened);

  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    auto* const QIn = &patchData.QInFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %}];

#if PEANO_DEBUG > 0
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
      &patchData.QOutFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %}],
      {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %},
      patchData.QOut[patchIndex]
    );

    auto* const QOut = patchData.QOut[patchIndex];

    {{POSTPROCESS_UPDATED_PATCH}}

#if PEANO_DEBUG > 0
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

  // QIn is no longer needed
  // QOut of Riemann solver is now QIn for the patch reconstruction
  std::swap(patchData.QInFlattened, patchData.QOutFlattened);
  kernels::{{SOLVER_NAME}}::projectPatchFused(patchData, TARGET_DEVICE);
  std::swap(patchData.QInFlattened, patchData.QOutFlattened);

  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    for (int f = 0; f < TWO_TIMES_D; f++) {
      auto* deviceFaceOld     = patchData.faces[patchIndex][0][f];
      auto* deviceFaceUpdate  = patchData.faces[patchIndex][1][f];
      auto* deviceFaceNew     = patchData.faces[patchIndex][2][f];
      auto* hostFaceOld       = facesOldDeviceToHost[deviceFaceOld];
      auto* hostFaceNew       = facesNewDeviceToHost[deviceFaceNew];

      ::exahype2::fv::copyHalfOfHalo(
        {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        {{HALO_SIZE}},
        f % DIMENSIONS,
        f < DIMENSIONS,
        deviceFaceNew,
        hostFaceOld
      );
      ::exahype2::fv::copyHalfOfHalo(
        {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        {{HALO_SIZE}},
        f % DIMENSIONS,
        f < DIMENSIONS,
        deviceFaceUpdate,
        hostFaceNew
      );
    }
  }

  tarch::accelerator::MemoryManager::getInstance().free(patchData.QInFlattened, TARGET_DEVICE);
  tarch::accelerator::MemoryManager::getInstance().free(patchData.QOutFlattened, TARGET_DEVICE);
  if (patchData.numberOfBoundaryFaces > 0) {
    tarch::accelerator::MemoryManager::getInstance().free(patchData.patchIndicesOfBoundaryFaces, TARGET_DEVICE);
    tarch::accelerator::MemoryManager::getInstance().free(patchData.boundaryFaceNumbers, TARGET_DEVICE);
  }
  tarch::accelerator::MemoryManager::getInstance().free(deviceFacesOld, TARGET_DEVICE);
  tarch::accelerator::MemoryManager::getInstance().free(deviceFacesUpdate, TARGET_DEVICE);
  tarch::accelerator::MemoryManager::getInstance().free(deviceFacesNew, TARGET_DEVICE);

  {{COMPUTE_NEW_TIME_STEP_SIZE}}
  repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, _t + _dt, tarch::la::min(_marker.getH()), tarch::la::max(_marker.getH()));
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::fuse(::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}, 3>& patchData) {
  // Before swap: QIn is with halo, QOut is without halo
  // After swap: QIn is without halo, QOut is with halo (that is reconstructed as next step)
  std::swap(patchData.QInFlattened, patchData.QOutFlattened);
  kernels::{{SOLVER_NAME}}::reconstructPatchFused(patchData, TARGET_DEVICE);
  // QOut of patch reconstruction is now QIn for the Riemann solver
  // QIn for patch reconstruction is now QOut of the Riemann solver
  std::swap(patchData.QInFlattened, patchData.QOutFlattened);

  const auto maxEigenvalue = {{FUSED_COMPUTE_KERNEL_CALL}}

  std::swap(patchData.QInFlattened, patchData.QOutFlattened);
  kernels::{{SOLVER_NAME}}::projectPatchFused(patchData, TARGET_DEVICE);
  std::swap(patchData.QInFlattened, patchData.QOutFlattened);

  {{COMPUTE_NEW_TIME_STEP_SIZE}}
  repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, patchData.t + patchData.dt, tarch::la::min(patchData.cellSize[0]), tarch::la::max(patchData.cellSize[0]));
}
