// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_HIP)

#include "DevicePropertiesCache.h"
#include "ErrorCheck.h"

#include <hip/hip_runtime.h>

namespace tarch::accelerator::hip {
  class ExecutionPolicy;
} // namespace tarch::accelerator::hip

class tarch::accelerator::hip::ExecutionPolicy {
private:
  int         _state;
  int         _gridSize;
  int         _blockSize;
  int         _maxBlockSize;
  std::size_t _sharedMemBytes;
  hipStream_t _stream;

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
  ExecutionPolicy(int gridSize, int blockSize, std::size_t sharedMemBytes, hipStream_t stream);
  ~ExecutionPolicy() = default;

  int         getConfigState() const;
  int         getGridSize() const;
  int         getBlockSize() const;
  int         getMaxBlockSize() const;
  std::size_t getSharedMemBytes() const;
  hipStream_t getStream() const;

  void setGridSize(int size);
  void setBlockSize(int size);
  void setMaxBlockSize(int size);
  void setSharedMemBytes(std::size_t bytes);
  void setStream(hipStream_t stream);

  /**
   * Auto-configure grid dimensions using HIP occupancy API.
   * Uses hipOccupancyMaxPotentialBlockSize for automatic block size selection.
   */
  template <class KernelFunc>
  hipError_t configureGrid(KernelFunc k) {
    int configState = getConfigState();

    if (configState == ExecutionPolicy::FullManual) {
      return hipSuccess;
    }

    hipDeviceProp_t* prop   = &DevicePropertiesCache::get();
    int              numSMs = prop->multiProcessorCount;

    if ((configState & ExecutionPolicy::BlockSize) == 0) {
      int        blockSize = 0, minGridSize = 0;
      hipError_t err = hipOccupancyMaxPotentialBlockSize(
        &minGridSize,
        &blockSize,
        k,
        getSharedMemBytes(),
        0 // no block size limit
      );
      if (err != hipSuccess || blockSize < 0) {
        return hipErrorInvalidConfiguration;
      }
      setBlockSize(blockSize);
    }

    if ((configState & ExecutionPolicy::GridSize) == 0) {
      int        blocksPerSM = 0;
      hipError_t err         = hipOccupancyMaxActiveBlocksPerMultiprocessor(
        &blocksPerSM,
        k,
        getBlockSize(),
        getSharedMemBytes()
      );
      if (err != hipSuccess) {
        return hipErrorInvalidConfiguration;
      }
      setGridSize(blocksPerSM * numSMs);
      if (getGridSize() < numSMs) {
        return hipErrorInvalidConfiguration;
      }
    }

    return hipSuccess;
  }
};

#endif
