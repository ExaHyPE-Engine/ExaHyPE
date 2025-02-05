// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "config.h"

#if defined(GPUOffloadingCUDA)

#include <cuda_occupancy.h>

#include "ErrorCheck.h"
#include "DevicePropertiesCache.h"

namespace tarch::accelerator::cuda {
  class ExecutionPolicy;
} // namespace tarch::accelerator::cuda

class tarch::accelerator::cuda::ExecutionPolicy {
private:
  int          _state;
  int          _gridSize;
  int          _blockSize;
  int          _maxBlockSize;
  std::size_t  _sharedMemBytes;
  cudaStream_t _stream;

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
  ExecutionPolicy(
    int          gridSize,
    int          blockSize,
    std::size_t  sharedMemBytes,
    cudaStream_t stream
  );
  ~ExecutionPolicy() = default;

  int          getConfigState() const;
  int          getGridSize() const;
  int          getBlockSize() const;
  int          getMaxBlockSize() const;
  std::size_t  getSharedMemBytes() const;
  cudaStream_t getStream() const;

  void setGridSize(int size);
  void setBlockSize(int size);
  void setMaxBlockSize(int size);
  void setSharedMemBytes(std::size_t bytes);
  void setStream(cudaStream_t stream);

  static std::size_t availableSharedBytesPerBlock(
    std::size_t sharedMemPerMultiprocessor,
    std::size_t sharedSizeBytesStatic,
    int         blocksPerSM,
    int         sharedMemAllocationUnit
  );

  template <class KernelFunc>
  cudaError_t configureGrid(KernelFunc k) {
    int configState = getConfigState();

    if (configState == ExecutionPolicy::FullManual) {
      return cudaSuccess;
    }

    cudaDeviceProp* prop = &DevicePropertiesCache::get();

    cudaFuncAttributes attributes;
    cudaOccDeviceProp  occProp(*prop);

    CHECK_CUDA_ERROR(cudaFuncGetAttributes(&attributes, k));
    cudaOccFuncAttributes occAttrib(attributes);

    cudaFuncCache cacheConfig;
    CHECK_CUDA_ERROR(cudaDeviceGetCacheConfig(&cacheConfig));
    cudaOccDeviceState occState;
    occState.cacheConfig = (cudaOccCacheConfig)cacheConfig;

    int numSMs = prop->multiProcessorCount;

    if ((configState & ExecutionPolicy::BlockSize) == 0) {
      int          blockSize = 0, minGridSize = 0;
      cudaOccError occErr = cudaOccMaxPotentialOccupancyBlockSize(
        &minGridSize,
        &blockSize,
        &occProp,
        &occAttrib,
        &occState,
        getSharedMemBytes()
      );
      if (occErr != CUDA_OCC_SUCCESS || blockSize < 0) {
        return cudaErrorInvalidConfiguration;
      }
      setBlockSize(blockSize);
    }

    if ((configState & ExecutionPolicy::GridSize) == 0) {
      cudaOccResult result;
      cudaOccError  occErr = cudaOccMaxActiveBlocksPerMultiprocessor(
        &result,
        &occProp,
        &occAttrib,
        &occState,
        getBlockSize(),
        getSharedMemBytes()
      );
      if (occErr != CUDA_OCC_SUCCESS) {
        return cudaErrorInvalidConfiguration;
      }
      setGridSize(result.activeBlocksPerMultiprocessor * numSMs);
      if (getGridSize() < numSMs) {
        return cudaErrorInvalidConfiguration;
      }
    }

    if ((configState & ExecutionPolicy::SharedMemory) == 0) {
      int          smemGranularity = 0;
      cudaOccError occErr          = cudaOccSMemAllocationGranularity(
        &smemGranularity,
        &occProp
      );
      if (occErr != CUDA_OCC_SUCCESS) {
        return cudaErrorInvalidConfiguration;
      }
      std::size_t sbytes = availableSharedBytesPerBlock(
        prop->sharedMemPerBlock,
        attributes.sharedSizeBytes,
        __occDivideRoundUp(getGridSize(), numSMs),
        smemGranularity
      );
      setSharedMemBytes(sbytes);
    }

    return cudaSuccess;
  }
};

#endif
