// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ExecutionPolicy.h"

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)

tarch::accelerator::cuda::ExecutionPolicy::ExecutionPolicy() :
  _state(Automatic),
  _gridSize(0),
  _blockSize(0),
  _maxBlockSize(0),
  _sharedMemBytes(0),
  _stream(nullptr) {}

tarch::accelerator::cuda::ExecutionPolicy::ExecutionPolicy(int gridSize, int blockSize, std::size_t sharedMemBytes) :
  _state(0),
  _gridSize(0),
  _blockSize(0),
  _maxBlockSize(0),
  _sharedMemBytes(0),
  _stream(nullptr) {
  setGridSize(gridSize);
  setBlockSize(blockSize);
  setSharedMemBytes(sharedMemBytes);
}

tarch::accelerator::cuda::ExecutionPolicy::ExecutionPolicy(
  int          gridSize,
  int          blockSize,
  std::size_t  sharedMemBytes,
  cudaStream_t stream
) :
  _state(0),
  _gridSize(0),
  _blockSize(0),
  _maxBlockSize(0),
  _sharedMemBytes(0),
  _stream(nullptr) {
  setGridSize(gridSize);
  setBlockSize(blockSize);
  setSharedMemBytes(sharedMemBytes);
  setStream(stream);
}

int
tarch::accelerator::cuda::ExecutionPolicy::getConfigState() const {
  return _state;
}

int
tarch::accelerator::cuda::ExecutionPolicy::getGridSize() const {
  return _gridSize;
}

int
tarch::accelerator::cuda::ExecutionPolicy::getBlockSize() const {
  return _blockSize;
}

int
tarch::accelerator::cuda::ExecutionPolicy::getMaxBlockSize() const {
  return _maxBlockSize;
}

std::size_t
tarch::accelerator::cuda::ExecutionPolicy::getSharedMemBytes() const {
  return _sharedMemBytes;
}

cudaStream_t
tarch::accelerator::cuda::ExecutionPolicy::getStream() const {
  return _stream;
}

std::size_t
tarch::accelerator::cuda::ExecutionPolicy::getAvailableSharedBytesPerBlock(
  std::size_t sharedMemPerMultiprocessor,
  std::size_t sharedSizeBytesStatic,
  int         blocksPerSM,
  int         sharedMemAllocationUnit
) {
  std::size_t bytes =
    __occRoundUp(sharedMemPerMultiprocessor / blocksPerSM, sharedMemAllocationUnit) - sharedMemAllocationUnit;
  return bytes - sharedSizeBytesStatic;
}

void
tarch::accelerator::cuda::ExecutionPolicy::setGridSize(int size) {
  _gridSize = size;
  if (_gridSize > 0) {
    _state |= GridSize;
  } else {
    _state &= (FullManual - GridSize);
  }
}

void
tarch::accelerator::cuda::ExecutionPolicy::setBlockSize(int size) {
  _blockSize = size;
  if (_blockSize > 0) {
    _state |= BlockSize;
  } else {
    _state &= (FullManual - BlockSize);
  }
}

void
tarch::accelerator::cuda::ExecutionPolicy::setMaxBlockSize(int size) {
  _maxBlockSize = size;
}

void
tarch::accelerator::cuda::ExecutionPolicy::setSharedMemBytes(std::size_t bytes) {
  _sharedMemBytes = bytes;
  _state |= SharedMemory;
}

void
tarch::accelerator::cuda::ExecutionPolicy::setStream(cudaStream_t stream) {
  _stream = stream;
}

#endif
