// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ExecutionPolicy.h"

#if defined(WITH_GPU_HIP)

tarch::accelerator::hip::ExecutionPolicy::ExecutionPolicy():
  _state(Automatic),
  _gridSize(0),
  _blockSize(0),
  _maxBlockSize(0),
  _sharedMemBytes(0),
  _stream(nullptr) {}

tarch::accelerator::hip::ExecutionPolicy::ExecutionPolicy(int gridSize, int blockSize, std::size_t sharedMemBytes):
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

tarch::accelerator::hip::ExecutionPolicy::
  ExecutionPolicy(int gridSize, int blockSize, std::size_t sharedMemBytes, hipStream_t stream):
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

int tarch::accelerator::hip::ExecutionPolicy::getConfigState() const { return _state; }

int tarch::accelerator::hip::ExecutionPolicy::getGridSize() const { return _gridSize; }

int tarch::accelerator::hip::ExecutionPolicy::getBlockSize() const { return _blockSize; }

int tarch::accelerator::hip::ExecutionPolicy::getMaxBlockSize() const { return _maxBlockSize; }

std::size_t tarch::accelerator::hip::ExecutionPolicy::getSharedMemBytes() const { return _sharedMemBytes; }

hipStream_t tarch::accelerator::hip::ExecutionPolicy::getStream() const { return _stream; }

void tarch::accelerator::hip::ExecutionPolicy::setGridSize(int size) {
  _gridSize = size;
  if (_gridSize > 0) {
    _state |= GridSize;
  } else {
    _state &= (FullManual - GridSize);
  }
}

void tarch::accelerator::hip::ExecutionPolicy::setBlockSize(int size) {
  _blockSize = size;
  if (_blockSize > 0) {
    _state |= BlockSize;
  } else {
    _state &= (FullManual - BlockSize);
  }
}

void tarch::accelerator::hip::ExecutionPolicy::setMaxBlockSize(int size) { _maxBlockSize = size; }

void tarch::accelerator::hip::ExecutionPolicy::setSharedMemBytes(std::size_t bytes) {
  _sharedMemBytes = bytes;
  _state |= SharedMemory;
}

void tarch::accelerator::hip::ExecutionPolicy::setStream(hipStream_t stream) { _stream = stream; }

#endif
