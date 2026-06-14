// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "repositories/SolverRepository.h"

#include "kernels/{{SOLVER_NAME}}/ProjectPatch.h"
#include "kernels/{{SOLVER_NAME}}/ReconstructPatch.h"
#include "kernels/{{SOLVER_NAME}}/RiemannSolver.h"

#include "exahype2/CellData.h"
#include "exahype2/RuntimeParameters.h"
#include "exahype2/fv/PatchUtils.h"

#include "peano4/parallel/parallel.h"

#include "tarch/instrumentation/Trace.h"

#include <algorithm>
#include <cstring>
#include <unordered_map>

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
  traceEnter("Finite volume enclave task fusion");

  const int numberOfPatches = otherTasks.size() + 1;
  const int targetDevice    = ::exahype2::RuntimeParameters::getInstance().getTargetDevice();

  logDebug("Rank " << tarch::mpi::Rank::getInstance().getRank()
    << " asked to fuse " << numberOfPatches << " patches of type " << getTaskType() << " into "
    << {{NUMBER_OF_ENCLAVE_TASKS}} << " asynchronous enclave tasks on target device " << targetDevice);

  ::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}, 3> patchData(
    numberOfPatches,
    tarch::MemoryLocation::Pinned,
    targetDevice
  );

  std::vector<double> hostFacesOld;
  std::vector<double> hostFacesUpdate;
  std::vector<double> hostFacesNew;
  hostFacesOld.reserve(numberOfPatches * TWO_TIMES_D * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %});
  hostFacesUpdate.reserve(numberOfPatches * TWO_TIMES_D * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %});
  hostFacesNew.reserve(numberOfPatches * TWO_TIMES_D * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %});

  std::unordered_map<{{STORAGE_PRECISION}}*, int>                    faceMapHostToDevice;
  std::unordered_map<{{STORAGE_PRECISION}}*, {{STORAGE_PRECISION}}*> facesOldDeviceToHost;
  std::unordered_map<{{STORAGE_PRECISION}}*, {{STORAGE_PRECISION}}*> facesUpdateDeviceToHost;
  std::unordered_map<{{STORAGE_PRECISION}}*, {{STORAGE_PRECISION}}*> facesNewDeviceToHost;

  auto processTaskFaces = [&](Task* task) {
    auto* typedTask = static_cast<{{ENCLAVE_TASK_TYPE}}*>(task);
    for (int f = 0; f < TWO_TIMES_D; ++f) {
      {{STORAGE_PRECISION}}* faceKeyOld    = typedTask->_faces[0][f];
      {{STORAGE_PRECISION}}* faceKeyUpdate = typedTask->_faces[1][f];
      {{STORAGE_PRECISION}}* faceKeyNew    = typedTask->_faces[2][f];

      if (faceMapHostToDevice.find(faceKeyNew) == faceMapHostToDevice.end()) {
        faceMapHostToDevice[faceKeyNew] = static_cast<int>(faceMapHostToDevice.size());
        hostFacesNew.insert(hostFacesNew.end(), faceKeyNew, faceKeyNew + {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %});
        hostFacesUpdate.insert(hostFacesUpdate.end(), faceKeyUpdate, faceKeyUpdate + {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %});
        hostFacesOld.insert(hostFacesOld.end(), faceKeyOld, faceKeyOld + {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %});
      }
    }
  };

  for (auto& otherTask: otherTasks) {
    processTaskFaces(otherTask);
  }
  processTaskFaces(this);

  {{STORAGE_PRECISION}}* deviceFacesOld    = tarch::allocateMemory<{{STORAGE_PRECISION}}>(hostFacesOld.size(), tarch::MemoryLocation::Pinned, targetDevice);
  {{STORAGE_PRECISION}}* deviceFacesUpdate = tarch::allocateMemory<{{STORAGE_PRECISION}}>(hostFacesUpdate.size(), tarch::MemoryLocation::Pinned, targetDevice);
  {{STORAGE_PRECISION}}* deviceFacesNew    = tarch::allocateMemory<{{STORAGE_PRECISION}}>(hostFacesNew.size(), tarch::MemoryLocation::Pinned, targetDevice);
  tarch::copyTo(deviceFacesOld, hostFacesOld.data(), hostFacesOld.size(), tarch::MemoryLocation::Pinned, targetDevice);
  tarch::copyTo(deviceFacesUpdate, hostFacesUpdate.data(), hostFacesUpdate.size(), tarch::MemoryLocation::Pinned, targetDevice);
  tarch::copyTo(deviceFacesNew, hostFacesNew.data(), hostFacesNew.size(), tarch::MemoryLocation::Pinned, targetDevice);
  hostFacesOld.clear();
  hostFacesUpdate.clear();
  hostFacesNew.clear();

  std::vector<std::uint64_t> hostFaceIndices;
  std::vector<std::uint64_t> patchIndicesOfBoundaryFaces;
  std::vector<std::uint8_t>  boundaryFaceNumbers;
  std::vector<std::uint64_t> boundaryFaceIndices;

  int patchIndex = 0;
  for (auto& otherTask: otherTasks) {
    patchData.QIn[patchIndex]                   = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_QIn;
    patchData.QOut[patchIndex]                  = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_QOut;
    patchData.cellSize[patchIndex]              = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_marker.getH();
    patchData.cellCentre[patchIndex]            = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_marker.getX();
    patchData.maxEigenvalue[patchIndex]         = 0.0;

    for (int f = 0; f < TWO_TIMES_D; ++f) {
      auto* faceKeyOld                          = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_faces[0][f];
      auto* faceKeyUpdate                       = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_faces[1][f];
      auto* faceKeyNew                          = static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_faces[2][f];
      auto faceIndex                            = faceMapHostToDevice.at(faceKeyNew);

      // pointer based setup for each faceType
      auto* deviceFaceOld                       = deviceFacesOld + faceIndex * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};
      patchData.faces[patchIndex][0][f]         = deviceFaceOld;
      facesOldDeviceToHost[deviceFaceOld]       = faceKeyOld;

      auto* deviceFaceUpdate                    = deviceFacesUpdate + faceIndex * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};
      patchData.faces[patchIndex][1][f]         = deviceFaceUpdate;
      facesUpdateDeviceToHost[deviceFaceUpdate] = faceKeyUpdate;

      auto* deviceFaceNew                       = deviceFacesNew + faceIndex * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};
      patchData.faces[patchIndex][2][f]         = deviceFaceNew;
      facesNewDeviceToHost[deviceFaceNew]       = faceKeyNew;

      hostFaceIndices.push_back(faceIndex);

      if (static_cast<{{ENCLAVE_TASK_TYPE}}*>(otherTask)->_isBoundary[f]) {
        patchIndicesOfBoundaryFaces.push_back(patchIndex);
        boundaryFaceNumbers.push_back(f);
        boundaryFaceIndices.push_back(faceIndex);
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
    auto* faceKeyUpdate                       = _faces[1][f];
    auto* faceKeyNew                          = _faces[2][f];
    auto faceIndex                            = faceMapHostToDevice.at(faceKeyNew);

    // pointer based setup for each faceType
    auto* deviceFaceOld                       = deviceFacesOld + faceIndex * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};
    patchData.faces[patchIndex][0][f]         = deviceFaceOld;
    facesOldDeviceToHost[deviceFaceOld]       = faceKeyOld;

    auto* deviceFaceUpdate                    = deviceFacesUpdate + faceIndex * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};
    patchData.faces[patchIndex][1][f]         = deviceFaceUpdate;
    facesUpdateDeviceToHost[deviceFaceUpdate] = faceKeyUpdate;

    auto* deviceFaceNew                       = deviceFacesNew + faceIndex * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};
    patchData.faces[patchIndex][2][f]         = deviceFaceNew;
    facesNewDeviceToHost[deviceFaceNew]       = faceKeyNew;

    // Index into pool (index is same for every face type)
    hostFaceIndices.push_back(faceIndex);

    if (_isBoundary[f]) {
      patchIndicesOfBoundaryFaces.push_back(patchIndex);
      boundaryFaceNumbers.push_back(f);
      boundaryFaceIndices.push_back(faceIndex);
    }
  }
  patchData.numberOfBoundaryFaces = boundaryFaceNumbers.size();
  if (patchData.numberOfBoundaryFaces > 0) {
    patchData.patchIndicesOfBoundaryFaces = tarch::allocateMemory<std::uint64_t>(patchIndicesOfBoundaryFaces.size(), tarch::MemoryLocation::Pinned, targetDevice);
    patchData.boundaryFaceNumbers         = tarch::allocateMemory<std::uint8_t>(boundaryFaceNumbers.size(), tarch::MemoryLocation::Pinned, targetDevice);
    patchData.boundaryFaceIndices         = tarch::allocateMemory<std::uint64_t>(boundaryFaceIndices.size(), tarch::MemoryLocation::Pinned, targetDevice);

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
    std::copy_n(
      boundaryFaceIndices.data(),
      boundaryFaceIndices.size(),
      patchData.boundaryFaceIndices
    );
    patchIndicesOfBoundaryFaces.clear();
    boundaryFaceNumbers.clear();
    boundaryFaceIndices.clear();
  }
  patchData.QInFlattened = tarch::allocateMemory<{{STORAGE_PRECISION}}>(
    numberOfPatches * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_WITH_HALO_3D}} {% else %} {{PATCH_CARDINALITY_WITH_HALO_2D}} {% endif %},
    tarch::MemoryLocation::Pinned,
    targetDevice
  );
  patchData.QOutFlattened = tarch::allocateMemory<{{STORAGE_PRECISION}}>(
    numberOfPatches * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %},
    tarch::MemoryLocation::Pinned,
    targetDevice
  );
  patchData.facesFlattened = deviceFacesNew;
  patchData.faceIndices = tarch::allocateMemory<std::uint64_t>(
    hostFaceIndices.size(),
    tarch::MemoryLocation::Pinned,
    targetDevice
  );
  std::copy_n(
    hostFaceIndices.data(),
    hostFaceIndices.size(),
    patchData.faceIndices
  );

  if (patchData.numberOfBoundaryFaces > 0) {
    traceEnter("Handle boundary");
    kernels::{{SOLVER_NAME}}::handleBoundaryFused(repositories::{{SOLVER_INSTANCE}}, patchData, targetDevice);
    traceLeave("Handle boundary");
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

  traceEnter("Patch reconstruction");
  kernels::{{SOLVER_NAME}}::reconstructPatchFused(patchData, targetDevice);
  traceLeave("Patch reconstruction");

  // QOut of patch reconstruction is now QIn for the Riemann solver
  // QIn for patch reconstruction is now QOut of the Riemann solver
  std::swap(patchData.QInFlattened, patchData.QOutFlattened);

  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    auto* const QIn = &patchData.QInFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %}];

#if PEANO_DEBUG
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

  traceEnter("Riemann solver");
  const auto maxEigenvalue = {{FUSED_COMPUTE_KERNEL_CALL}}
  traceLeave("Riemann solver");

  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    std::swap(patchData.QIn[patchIndex], patchData.QOut[patchIndex]);
    std::copy_n(
      &patchData.QOutFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %}],
      {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %},
      patchData.QOut[patchIndex]
    );

    auto* const QOut = patchData.QOut[patchIndex];

    {{POSTPROCESS_UPDATED_PATCH}}

#if PEANO_DEBUG
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
  traceEnter("Patch projection");
  kernels::{{SOLVER_NAME}}::projectPatchFused(patchData, targetDevice);
  traceLeave("Patch projection");
  std::swap(patchData.QInFlattened, patchData.QOutFlattened);

  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    for (int f = 0; f < TWO_TIMES_D; f++) {
      auto* deviceFaceOld     = patchData.faces[patchIndex][0][f];
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
        deviceFaceNew,
        hostFaceNew
      );
    }
  }

  tarch::freeMemory(patchData.QInFlattened, tarch::MemoryLocation::Pinned, targetDevice);
  tarch::freeMemory(patchData.QOutFlattened, tarch::MemoryLocation::Pinned, targetDevice);
  tarch::freeMemory(patchData.faceIndices, tarch::MemoryLocation::Pinned, targetDevice);
  if (patchData.numberOfBoundaryFaces > 0) {
    tarch::freeMemory(patchData.patchIndicesOfBoundaryFaces, tarch::MemoryLocation::Pinned, targetDevice);
    tarch::freeMemory(patchData.boundaryFaceNumbers, tarch::MemoryLocation::Pinned, targetDevice);
    tarch::freeMemory(patchData.boundaryFaceIndices, tarch::MemoryLocation::Pinned, targetDevice);
  }
  tarch::freeMemory(deviceFacesOld, tarch::MemoryLocation::Pinned, targetDevice);
  tarch::freeMemory(deviceFacesUpdate, tarch::MemoryLocation::Pinned, targetDevice);
  tarch::freeMemory(deviceFacesNew, tarch::MemoryLocation::Pinned, targetDevice);

  {{COMPUTE_NEW_TIME_STEP_SIZE}}
  repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, _t + _dt, tarch::la::min(_marker.getH()), tarch::la::max(_marker.getH()));
  traceLeave("Finite volume enclave task fusion");
}
