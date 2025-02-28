// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
{% import 'CustomRiemann.template.h' as riemann with context %}{# get template macros #}
{% else %}
{% import 'GenericRusanov.template.h' as riemann with context %}{# get template macros #}
{% endif %}

{% import 'Loop.template.h' as loop with context %}{# get template macros #}

{% macro loopOverVolumes() %}
  {% if DIMENSIONS == 3 %}
  for (int z = threadIdx.z; z < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; z += 8) {
    for (int y = threadIdx.y; y < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; y += 8) {
      for (int x = threadIdx.x; x < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; x += 8) {
        const auto volume = ::exahype2::volumeIndex(x, y, z);
        const auto volumeSize = ::exahype2::fv::getVolumeSize(patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}});
        const auto volumeCentre = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume);

        {{ riemann.createMemoryOnStack() | indent(4) }}
        {{ riemann.gatherQ() | indent(4) }}

        {{ loop.loopOverDimensions() | indent(4) }}

        {% if COMPUTE_MAX_EIGENVALUE==True %}
        {{ riemann.computeMaxEigenvalueAfterTimeStep() | indent(4) }};
        {% endif %}

        {{ riemann.scatterQ() | indent(4) }}
      } // x
    } // y
  } // z
  {% else %}
  for (int y =  threadIdx.y; y < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; y += 16) {
    for (int x = threadIdx.x; x < {{ NUMBER_OF_VOLUMES_PER_AXIS }}; x += 16) {
      const auto volume = ::exahype2::volumeIndex(x, y);
      const auto volumeSize = ::exahype2::fv::getVolumeSize(patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}});
      const auto volumeCentre = ::exahype2::fv::getVolumeCentre(patchCentre, patchSize, {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume);

      {{ riemann.createMemoryOnStack() | indent(4) }}
      {{ riemann.gatherQ() | indent(4) }}

      {{ loop.loopOverDimensions() | indent(4) }}

      {% if COMPUTE_MAX_EIGENVALUE==True %}
      {{ riemann.computeMaxEigenvalueAfterTimeStep() | indent(4) }};
      {% endif %}

      {{ riemann.scatterQ() | indent(4) }}
    } // x
  } // y
  {% endif %}
{% endmacro %}

{% macro defineGlobalKernel() %}
  __global__ static void
  {% if DIMENSIONS==3 %} __launch_bounds__(512) {% else %} __launch_bounds__(256) {% endif %}
  kernel(
    const double* const __restrict__ QIns,
    double* const __restrict__       QOuts,
    const double* const __restrict__ patchCentres,
    const double* const __restrict__ patchSizes,
    const double* const __restrict__ timeStamps,
    const double* const __restrict__ timeStepSizes
    {% if COMPUTE_MAX_EIGENVALUE==True %}
    ,
    double* const __restrict__     maxEigenvalues
    {% endif %}
  ) {
    const int patchIndex    = blockIdx.x;
    const double* const QIn = &QIns[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %}];
    double* const QOut      = &QOuts[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}];

    const tarch::la::Vector<Dimensions, double> patchCentre {
      patchCentres[patchIndex * {{DIMENSIONS}}],
      patchCentres[patchIndex * {{DIMENSIONS}} + 1]
      {% if DIMENSIONS==3 %}
      ,
      patchCentres[patchIndex * {{DIMENSIONS}} + 2]
      {% endif %}
    };
    const tarch::la::Vector<Dimensions, double> patchSize {
      patchSizes[patchIndex * {{DIMENSIONS}}],
      patchSizes[patchIndex * {{DIMENSIONS}} + 1]
      {% if DIMENSIONS==3 %}
      ,
      patchSizes[patchIndex * {{DIMENSIONS}} + 2]
      {% endif %}
    };

    const double timeStamp    = timeStamps[patchIndex];
    const double timeStepSize = timeStepSizes[patchIndex];

    {% if COMPUTE_MAX_EIGENVALUE==True %}
    double maxEigenvalueAfterTimeStep = 0.0;
    {% endif %}

    {{ loopOverVolumes() | indent(2) }}

    {% if COMPUTE_MAX_EIGENVALUE==True %}
    __shared__ double sdata[{% if DIMENSIONS==3 %} 512 {% else %} 256 {% endif %}];
    {% if DIMENSIONS==3 %}
    const int tid = threadIdx.z * (blockDim.x * blockDim.y) + threadIdx.y * blockDim.x + threadIdx.x;
    {% else %}
    const int tid = threadIdx.y * blockDim.x + threadIdx.x;
    {% endif %}
    sdata[tid] = maxEigenvalueAfterTimeStep;
    __syncthreads();
    for (unsigned int s = {% if DIMENSIONS==3 %} blockDim.x * blockDim.y * blockDim.z / 2 {% else %} blockDim.x * blockDim.y / 2 {% endif %}; s > 0; s >>= 1) {
      if (tid < s)
        sdata[tid] = fmax(sdata[tid], sdata[tid + s]);
      __syncthreads();
    }
    if (tid == 0) {
      maxEigenvalues[patchIndex] = sdata[0];
    }
    {% endif %}
  }
{% endmacro %}

{% macro loopOverPatches() %}
  const int numberOfPatches = patchData.numberOfCells;

  // TODO: We need to get rid of these memory allocations and the memory copies below.
  double* QIns = tarch::accelerator::MemoryManager::getInstance().allocate<double>(
    numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %},
    tarch::MemoryLocation::DeviceHeap,
    targetDevice
  );
  double* QOuts = tarch::accelerator::MemoryManager::getInstance().allocate<double>(
    numberOfPatches * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %},
    tarch::MemoryLocation::DeviceHeap,
    targetDevice
  );
  double* patchCentres = tarch::accelerator::MemoryManager::getInstance().allocate<double>(
    numberOfPatches * {{DIMENSIONS}},
    tarch::MemoryLocation::DeviceHeap,
    targetDevice
  );
  double* patchSizes = tarch::accelerator::MemoryManager::getInstance().allocate<double>(
    numberOfPatches * {{DIMENSIONS}},
    tarch::MemoryLocation::DeviceHeap,
    targetDevice
  );
  double* timeStamps = tarch::accelerator::MemoryManager::getInstance().allocate<double>(
    numberOfPatches,
    tarch::MemoryLocation::DeviceHeap,
    targetDevice
  );
  double* timeStepSizes = tarch::accelerator::MemoryManager::getInstance().allocate<double>(
    numberOfPatches,
    tarch::MemoryLocation::DeviceHeap,
    targetDevice
  );
  {% if COMPUTE_MAX_EIGENVALUE==True %}
  double* maxEigenvalues = tarch::accelerator::MemoryManager::getInstance().allocate<double>(
    numberOfPatches,
    tarch::MemoryLocation::DeviceHeap,
    targetDevice
  );
  {% endif %}

  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    tarch::copyTo(
      &QIns[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %}],
      patchData.QIn[patchIndex],
      {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D}} {% endif %},
      tarch::MemoryLocation::DeviceHeap,
      targetDevice
    );
  }

  tarch::copyTo(
    patchCentres,
    patchData.cellCentre[0].data(),
    numberOfPatches * {{DIMENSIONS}},
    tarch::MemoryLocation::DeviceHeap,
    targetDevice
  );
  tarch::copyTo(
    patchSizes,
    patchData.cellSize[0].data(),
    numberOfPatches * {{DIMENSIONS}},
    tarch::MemoryLocation::DeviceHeap,
    targetDevice
  );
  tarch::copyTo(
    timeStamps,
    patchData.t,
    numberOfPatches,
    tarch::MemoryLocation::DeviceHeap,
    targetDevice
  );
  tarch::copyTo(
    timeStepSizes,
    patchData.dt,
    numberOfPatches,
    tarch::MemoryLocation::DeviceHeap,
    targetDevice
  );

  {% if DIMENSIONS==3 %} const dim3 dimBlock(8, 8, 8); {% else %} const dim3 dimBlock(16, 16, 1); {% endif %}
  const dim3 dimGrid(numberOfPatches, 1, 1);
  CHECK_CUDA_ERROR(cudaSetDevice(patchData.targetDevice));

  kernel<<<dimGrid, dimBlock, 0, tarch::accelerator::deviceStreamGet()>>>(
    QIns,
    QOuts,
    patchCentres,
    patchSizes,
    timeStamps,
    timeStepSizes
    {% if COMPUTE_MAX_EIGENVALUE==True %}
    , maxEigenvalues
    {% endif %}
  );

  CHECK_CUDA_ERROR(cudaDeviceSynchronize());

  for (int patchIndex = 0; patchIndex < numberOfPatches; patchIndex++) {
    tarch::copyFrom(
      &QOuts[patchIndex * {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %}],
      patchData.QOut[patchIndex],
      {% if DIMENSIONS==3 %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}} {% else %} {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}} {% endif %},
      tarch::MemoryLocation::DeviceHeap,
      targetDevice
    );
  }

  {% if COMPUTE_MAX_EIGENVALUE==True %}
  double* cubMaxEigenvalue = tarch::accelerator::MemoryManager::getInstance().allocate<double>(
    1,
    tarch::MemoryLocation::DeviceHeap,
    targetDevice
  );
  void* cubTempStorage = nullptr;
  std::size_t cubTempStorageBytes = 0;
  cub::DeviceReduce::Max(
    cubTempStorage,
    cubTempStorageBytes,
    maxEigenvalues,
    cubMaxEigenvalue,
    numberOfPatches
  );
  cubTempStorage = tarch::accelerator::MemoryManager::getInstance().allocate<char>(
    cubTempStorageBytes,
    tarch::MemoryLocation::DeviceHeap,
    targetDevice
  );
  cub::DeviceReduce::Max(
    cubTempStorage,
    cubTempStorageBytes,
    maxEigenvalues,
    cubMaxEigenvalue,
    numberOfPatches
  );
  tarch::copyFrom(
    cubMaxEigenvalue,
    &maxEigenvalueAfterTimeStep,
    1,
    tarch::MemoryLocation::DeviceHeap,
    targetDevice
  );
  {% endif %}

  tarch::accelerator::MemoryManager::getInstance().free(QIns, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(QOuts, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(patchCentres, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(patchSizes, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(timeStamps, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(timeStepSizes, targetDevice);
  {% if COMPUTE_MAX_EIGENVALUE==True %}
  tarch::accelerator::MemoryManager::getInstance().free(maxEigenvalues, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(cubMaxEigenvalue, targetDevice);
  tarch::accelerator::MemoryManager::getInstance().free(cubTempStorage, targetDevice);
  {% endif %}

  CHECK_CUDA_ERROR(cudaDeviceSynchronize());
{% endmacro %}
