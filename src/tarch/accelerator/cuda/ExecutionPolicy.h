// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)

#include "tarch/accelerator/cuda/DevicePropertiesCache.h"
#include "tarch/accelerator/cuda/ErrorCheck.h"

#include <cuda_occupancy.h>

namespace tarch::accelerator::cuda {
  class ExecutionPolicy;
} // namespace tarch::accelerator::cuda

class tarch::accelerator::cuda::ExecutionPolicy {
public:
  enum ConfigurationState {
    Automatic    = 0,
    SharedMemory = 1,
    BlockSize    = 2,
    GridSize     = 4,
    FullManual   = GridSize | BlockSize | SharedMemory
  };

  ExecutionPolicy();
  ExecutionPolicy(int gridSize, int blockSize, std::size_t sharedMemBytes);
  ExecutionPolicy(int gridSize, int blockSize, std::size_t sharedMemBytes, cudaStream_t stream);
  ~ExecutionPolicy() = default;

  int getConfigState() const;
  int getGridSize() const;
  int getBlockSize() const;
  int getMaxBlockSize() const;
  std::size_t getSharedMemBytes() const;
  cudaStream_t getStream() const;

  void setGridSize(int size);
  void setBlockSize(int size);
  void setMaxBlockSize(int size);
  void setSharedMemBytes(std::size_t bytes);
  void setStream(cudaStream_t stream);

  static std::size_t getAvailableSharedBytesPerBlock(
    std::size_t sharedMemPerMultiprocessor,
    std::size_t sharedSizeBytesStatic,
    int         blocksPerSM,
    int         sharedMemAllocationUnit
  );

  template <class KernelFunc>
  cudaError_t configureGrid(KernelFunc kernel) {
    const int configurationState = getConfigState();

    if (configurationState == ExecutionPolicy::FullManual) {
      return cudaSuccess;
    }

    const cudaDeviceProp* properties = &DevicePropertiesCache::getProperties();

    cudaFuncAttributes attributes;
    cudaOccDeviceProp  occupancyProperties(*properties);

    CHECK_CUDA_ERROR(cudaFuncGetAttributes(&attributes, kernel));
    cudaOccFuncAttributes occupancyAttributes(attributes);

    cudaFuncCache cacheConfiguration;
    CHECK_CUDA_ERROR(cudaDeviceGetCacheConfig(&cacheConfiguration));
    cudaOccDeviceState occupancyState;
    occupancyState.cacheConfig = static_cast<cudaOccCacheConfig>(cacheConfiguration);

    const int numberOfMultiprocessors = properties->multiProcessorCount;

    if ((configurationState & ExecutionPolicy::BlockSize) == 0) {
      int                blockSize          = 0;
      int                minimumGridSize    = 0;
      const cudaOccError blockSizeOccupancy = cudaOccMaxPotentialOccupancyBlockSize(
        &minimumGridSize,
        &blockSize,
        &occupancyProperties,
        &occupancyAttributes,
        &occupancyState,
        getSharedMemBytes()
      );
      if (blockSizeOccupancy != CUDA_OCC_SUCCESS or blockSize < 0) {
        return cudaErrorInvalidConfiguration;
      }
      setBlockSize(blockSize);
    }

    if ((configurationState & ExecutionPolicy::GridSize) == 0) {
      cudaOccResult      result;
      const cudaOccError blockCountOccupancy = cudaOccMaxActiveBlocksPerMultiprocessor(
        &result,
        &occupancyProperties,
        &occupancyAttributes,
        &occupancyState,
        getBlockSize(),
        getSharedMemBytes()
      );
      if (blockCountOccupancy != CUDA_OCC_SUCCESS) {
        return cudaErrorInvalidConfiguration;
      }
      setGridSize(result.activeBlocksPerMultiprocessor * numberOfMultiprocessors);
      if (getGridSize() < numberOfMultiprocessors) {
        return cudaErrorInvalidConfiguration;
      }
    }

    if ((configurationState & ExecutionPolicy::SharedMemory) == 0) {
      int                granularity          = 0;
      const cudaOccError granularityOccupancy = cudaOccSMemAllocationGranularity(&granularity, &occupancyProperties);
      if (granularityOccupancy != CUDA_OCC_SUCCESS) {
        return cudaErrorInvalidConfiguration;
      }
      const std::size_t bytesPerBlock = getAvailableSharedBytesPerBlock(
        properties->sharedMemPerBlock,
        attributes.sharedSizeBytes,
        __occDivideRoundUp(getGridSize(), numberOfMultiprocessors),
        granularity
      );
      setSharedMemBytes(bytesPerBlock);
    }

    return cudaSuccess;
  }

private:
  int          _state;
  int          _gridSize;
  int          _blockSize;
  int          _maxBlockSize;
  std::size_t  _sharedMemBytes;
  cudaStream_t _stream;
};

#endif
