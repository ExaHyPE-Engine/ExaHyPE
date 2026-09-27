// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if defined(PEANO_WITH_GPU_BACKEND_HIP)

#include "tarch/accelerator/hip/DevicePropertiesCache.h"
#include "tarch/accelerator/hip/ErrorCheck.h"

#include <hip/hip_runtime.h>

namespace tarch::accelerator::hip {
  class ExecutionPolicy;
} // namespace tarch::accelerator::hip

class tarch::accelerator::hip::ExecutionPolicy {
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

  int getConfigState() const;
  int getGridSize() const;
  int getBlockSize() const;
  int getMaxBlockSize() const;
  std::size_t getSharedMemBytes() const;
  hipStream_t getStream() const;

  void setGridSize(int size);
  void setBlockSize(int size);
  void setMaxBlockSize(int size);
  void setSharedMemBytes(std::size_t bytes);
  void setStream(hipStream_t stream);

  /**
   * Reads the grid size and the block size from the HIP occupancy interface and stores them, unless
   * the caller has set every field by hand.
   */
  template <class KernelFunc>
  hipError_t configureGrid(KernelFunc kernel) {
    const int configurationState = getConfigState();

    if (configurationState == ExecutionPolicy::FullManual) {
      return hipSuccess;
    }

    const hipDeviceProp_t* properties              = &DevicePropertiesCache::getProperties();
    const int              numberOfMultiprocessors = properties->multiProcessorCount;

    if ((configurationState & ExecutionPolicy::BlockSize) == 0) {
      int              blockSize       = 0;
      int              minimumGridSize = 0;
      const hipError_t blockSizeError =
        hipOccupancyMaxPotentialBlockSize(&minimumGridSize, &blockSize, kernel, getSharedMemBytes(), 0);
      if (blockSizeError != hipSuccess or blockSize < 0) {
        return hipErrorInvalidConfiguration;
      }
      setBlockSize(blockSize);
    }

    if ((configurationState & ExecutionPolicy::GridSize) == 0) {
      int              blocksPerMultiprocessor = 0;
      const hipError_t blockCountError         = hipOccupancyMaxActiveBlocksPerMultiprocessor(
        &blocksPerMultiprocessor,
        kernel,
        getBlockSize(),
        getSharedMemBytes()
      );
      if (blockCountError != hipSuccess) {
        return hipErrorInvalidConfiguration;
      }
      setGridSize(blocksPerMultiprocessor * numberOfMultiprocessors);
      if (getGridSize() < numberOfMultiprocessors) {
        return hipErrorInvalidConfiguration;
      }
    }

    return hipSuccess;
  }

private:
  int         _state;
  int         _gridSize;
  int         _blockSize;
  int         _maxBlockSize;
  std::size_t _sharedMemBytes;
  hipStream_t _stream;
};

#endif
