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
  const double* const NOALIAS                patchCentres,
  const double* const NOALIAS                patchSizes,
  const double                               timeStamp,
  const double                               timeStepSize
  {% if COMPUTE_MAX_EIGENVALUE %} , {{COMPUTE_PRECISION}}* const NOALIAS maxEigenvalues {% endif %}
) {
  const int patchIndex                   = blockIdx.x;
  const {{COMPUTE_PRECISION}}* const QIn = &QInFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_WITH_HALO_3D}} {% else %} {{PATCH_CARDINALITY_WITH_HALO_2D}} {% endif %}];
  {{COMPUTE_PRECISION}}* const QOut      = &QOutFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %}];

  const tarch::la::Vector<DIMENSIONS, double> patchCentre{patchCentres + patchIndex * {{DIMENSIONS}}};
  const tarch::la::Vector<DIMENSIONS, double> patchSize{patchSizes + patchIndex * {{DIMENSIONS}}};

{% if COMPUTE_MAX_EIGENVALUE %}
    double maxEigenvalueAfterTimeStep = 0.0;
{% endif %}

{% if DIMENSIONS==3 %}
  for (int z = threadIdx.z; z < {{NUMBER_OF_VOLUMES_PER_AXIS}}; z += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
{% endif %}
    for (int y = threadIdx.y; y < {{NUMBER_OF_VOLUMES_PER_AXIS}}; y += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
      for (int x = threadIdx.x; x < {{NUMBER_OF_VOLUMES_PER_AXIS}}; x += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
        {{ ComputeFluxesAndPerformTimeStep() | indent(8)}}
      } // x
    } // y
{% if DIMENSIONS==3 %}  } // z {% endif %}

{% if COMPUTE_MAX_EIGENVALUE %}
  __shared__ {{COMPUTE_PRECISION}} sdata[{% if DIMENSIONS==3 %} 512 {% else %} 256 {% endif %}];
{% if DIMENSIONS==3 %}
  const int tid = threadIdx.z * (blockDim.x * blockDim.y) + threadIdx.y * blockDim.x + threadIdx.x;
{% else %}
  const int tid = threadIdx.y * blockDim.x + threadIdx.x;
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
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&                       solver,
  ::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}, 3>& patchData,
  [[maybe_unused]] const int                                             targetDevice
) {
  return riemannSolverFused(
	solver,
	patchData.QInFlattened,
	patchData.QOutFlattened,
	patchData.t,
	patchData.dt,
	patchData.cellCentre[0].data(),
	patchData.cellSize[0].data(),
	patchData.numberOfCells,
	targetDevice
  );
}

double {{FULL_QUALIFIED_NAMESPACE}}::riemannSolverFused(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&    solver,
  const {{COMPUTE_PRECISION}}* const                  QInFlattened,
  {{COMPUTE_PRECISION}}* const                        QOutFlattened,
  const double                                        timeStamp,
  const double                                        timeStepSize,
  const double* const                                 patchCentres,
  const double* const                                 patchSizes,
  const int                                           numberOfPatches,
  [[maybe_unused]] const int                          targetDevice
) {
{% if COMPUTE_MAX_EIGENVALUE %}
  {{COMPUTE_PRECISION}}* maxEigenvalues = tarch::accelerator::MemoryManager::getInstance().allocate<{{COMPUTE_PRECISION}}>(
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
    patchCentres,
    patchSizes,
    timeStamp,
    timeStepSize
    {% if COMPUTE_MAX_EIGENVALUE %} , maxEigenvalues {% endif %}
  );

{% if COMPUTE_MAX_EIGENVALUE %}
  {{COMPUTE_PRECISION}}* cubMaxEigenvalue = nullptr;
  CHECK_CUDA_ERROR(cudaMalloc(&cubMaxEigenvalue, sizeof(double)));
  void* cubTempStorage = nullptr;
  std::size_t cubTempStorageBytes = 0;
  cub::DeviceReduce::Max(
    cubTempStorage,
    cubTempStorageBytes,
    maxEigenvalues,
    cubMaxEigenvalue,
    numberOfPatches
  );
  CHECK_CUDA_ERROR(cudaMalloc(&cubTempStorage, cubTempStorageBytes));
  cub::DeviceReduce::Max(
    cubTempStorage,
    cubTempStorageBytes,
    maxEigenvalues,
    cubMaxEigenvalue,
    numberOfPatches
  );
  double maxEigenvalueAfterTimeStep = 0.0;
  CHECK_CUDA_ERROR(cudaMemcpyAsync(
    &maxEigenvalueAfterTimeStep,
    cubMaxEigenvalue,
    sizeof({{COMPUTE_PRECISION}}),
    cudaMemcpyDeviceToHost,
    tarch::accelerator::deviceStreamGet()
  ));
  CHECK_CUDA_ERROR(cudaFree(cubTempStorage));
  CHECK_CUDA_ERROR(cudaFree(cubMaxEigenvalue));
  tarch::accelerator::MemoryManager::getInstance().free(maxEigenvalues, targetDevice);
{% endif %}

  CHECK_CUDA_ERROR(cudaStreamSynchronize(tarch::accelerator::deviceStreamGet()));

{% if COMPUTE_MAX_EIGENVALUE %}
  return maxEigenvalueAfterTimeStep;
{% else %}
  return 0.0;
{% endif %}
}
