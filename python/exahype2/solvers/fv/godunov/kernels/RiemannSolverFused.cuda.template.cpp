// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolverFused.h"

#include "exahype2/fv/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"

#include "tarch/accelerator/accelerator.h"

#include <cub/cub.cuh>

{% from 'ComputeFluxesAndPerformTimeStep.macro.h' import ComputeFluxesAndPerformTimeStep with context %}

__global__ static void
{% if DIMENSIONS==3 %} __launch_bounds__(512) {% else %} __launch_bounds__(256) {% endif %}
kernel(
  const {{STORAGE_PRECISION}}* const NOALIAS QInFlattened,
  {{STORAGE_PRECISION}}* const NOALIAS       QOutFlattened,
  const {{STORAGE_PRECISION}}* const NOALIAS QInFaces,
  {{STORAGE_PRECISION}}* const NOALIAS       QOutFaces,
  const std::uint64_t* const NOALIAS         faceIndices,
  const double* const NOALIAS                patchCentres,
  const double* const NOALIAS                patchSizes,
  const double                               timeStamp,
  const double                               timeStepSize
  {% if COMPUTE_MAX_EIGENVALUE %} , {{COMPUTE_PRECISION}}* const NOALIAS maxEigenvalues {% endif %}
) {
  constexpr int FaceCardinality = {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};

  const auto patchIndex                  = blockIdx.x;
  const {{STORAGE_PRECISION}}* const QIn = &QInFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %}];
  {{STORAGE_PRECISION}}* const QOut      = &QOutFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %}];

  const tarch::la::Vector<DIMENSIONS, double> patchCentre{patchCentres + patchIndex * {{DIMENSIONS}}};
  const tarch::la::Vector<DIMENSIONS, double> patchSize{patchSizes + patchIndex * {{DIMENSIONS}}};

  const {{STORAGE_PRECISION}}* faceQIn[TWO_TIMES_D];
  {{STORAGE_PRECISION}}* faceQOut[TWO_TIMES_D];
  for (int f = 0; f < TWO_TIMES_D; f++) {
    auto faceIndex = faceIndices[patchIndex * TWO_TIMES_D + f];
    faceQIn[f]  = &QInFaces[faceIndex * FaceCardinality];
    faceQOut[f] = &QOutFaces[faceIndex * FaceCardinality];
  }

{% if COMPUTE_MAX_EIGENVALUE %}
  double maxEigenvalueAfterTimeStep = 0.0;
{% endif %}

{% if DIMENSIONS==3 %}
  for (int z = threadIdx.z; z < {{NUMBER_OF_VOLUMES_PER_AXIS}}; z += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
{% endif %}
    for (int y = threadIdx.y; y < {{NUMBER_OF_VOLUMES_PER_AXIS}}; y += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
      for (int x = threadIdx.x; x < {{NUMBER_OF_VOLUMES_PER_AXIS}}; x += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
        {{ ComputeFluxesAndPerformTimeStep(False) | indent(8)}}
      } // x
    } // y
{% if DIMENSIONS==3 %}  } // z {% endif %}

{% if COMPUTE_MAX_EIGENVALUE %}
  __shared__ {{COMPUTE_PRECISION}} sdata[{% if DIMENSIONS==3 %} 512 {% else %} 256 {% endif %}];
{% if DIMENSIONS==3 %}
  const auto tid = threadIdx.z * (blockDim.x * blockDim.y) + threadIdx.y * blockDim.x + threadIdx.x;
{% else %}
  const auto tid = threadIdx.y * blockDim.x + threadIdx.x;
{% endif %}
  sdata[tid] = maxEigenvalueAfterTimeStep;
  __syncthreads();
  for (unsigned int s = {% if DIMENSIONS==3 %} blockDim.x * blockDim.y * blockDim.z / 2 {% else %} blockDim.x * blockDim.y / 2 {% endif %}; s > 0; s >>= 1) {
    if (tid < s) {
      sdata[tid] = fmax(sdata[tid], sdata[tid + s]);
    }
    __syncthreads();
  }
  if (tid == 0) {
    maxEigenvalues[patchIndex] = sdata[0];
  }
{% endif %}
}

double {{FULL_QUALIFIED_NAMESPACE}}::riemannSolverFused(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&    solver,
  const {{STORAGE_PRECISION}}* const                  QInFlattened,
  {{STORAGE_PRECISION}}* const                        QOutFlattened,
  const {{STORAGE_PRECISION}}* const                  QInFaces,
  {{STORAGE_PRECISION}}* const                        QOutFaces,
  const std::uint64_t*                                faceIndices,
  const double                                        timeStamp,
  const double                                        timeStepSize,
  const double* const                                 patchCentres,
  const double* const                                 patchSizes,
  const int                                           numberOfPatches,
  [[maybe_unused]] const int                          targetDevice
) {
{% if COMPUTE_MAX_EIGENVALUE %}
  {{COMPUTE_PRECISION}}* maxEigenvalues = tarch::allocateMemory<{{COMPUTE_PRECISION}}>(
    numberOfPatches,
    tarch::MemoryLocation::Device,
    targetDevice
  );
{% endif %}

  {% if DIMENSIONS==3 %} const dim3 dimBlock(8, 8, 8); {% else %} const dim3 dimBlock(16, 16, 1); {% endif %}
  const dim3 dimGrid(numberOfPatches, 1, 1);
  CHECK_CUDA_ERROR(cudaSetDevice(targetDevice));

  kernel<<<dimGrid, dimBlock, 0, tarch::accelerator::deviceStreamGet()>>>(
    QInFlattened,
    QOutFlattened,
    QInFaces,
    QOutFaces,
    faceIndices,
    patchCentres,
    patchSizes,
    timeStamp,
    timeStepSize
    {% if COMPUTE_MAX_EIGENVALUE %} , maxEigenvalues {% endif %}
  );

{% if COMPUTE_MAX_EIGENVALUE %}
  {{COMPUTE_PRECISION}}* cubMaxEigenvalue = nullptr;
  CHECK_CUDA_ERROR(cudaMalloc(&cubMaxEigenvalue, sizeof({{COMPUTE_PRECISION}})));
  void* cubTempStorage = nullptr;
  std::size_t cubTempStorageBytes = 0;
  CHECK_CUDA_ERROR(cub::DeviceReduce::Max(
    cubTempStorage,
    cubTempStorageBytes,
    maxEigenvalues,
    cubMaxEigenvalue,
    numberOfPatches
  ));
  CHECK_CUDA_ERROR(cudaMalloc(&cubTempStorage, cubTempStorageBytes));
  CHECK_CUDA_ERROR(cub::DeviceReduce::Max(
    cubTempStorage,
    cubTempStorageBytes,
    maxEigenvalues,
    cubMaxEigenvalue,
    numberOfPatches
  ));
  {{COMPUTE_PRECISION}} maxEigenvalueAfterTimeStep = 0.0;
  CHECK_CUDA_ERROR(cudaMemcpyAsync(
    &maxEigenvalueAfterTimeStep,
    cubMaxEigenvalue,
    sizeof({{COMPUTE_PRECISION}}),
    cudaMemcpyDeviceToHost,
    tarch::accelerator::deviceStreamGet()
  ));
  CHECK_CUDA_ERROR(cudaFree(cubTempStorage));
  CHECK_CUDA_ERROR(cudaFree(cubMaxEigenvalue));
  tarch::freeMemory(maxEigenvalues, tarch::MemoryLocation::Device, targetDevice);
{% endif %}

  CHECK_CUDA_ERROR(cudaStreamSynchronize(tarch::accelerator::deviceStreamGet()));

{% if COMPUTE_MAX_EIGENVALUE %}
  return static_cast<double>(maxEigenvalueAfterTimeStep);
{% else %}
  return 0.0;
{% endif %}
}
