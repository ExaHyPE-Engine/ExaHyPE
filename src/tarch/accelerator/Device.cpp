// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Device.h"

#include "accelerator.h"

#include "tarch/multicore/Lock.h"

tarch::logging::Log tarch::accelerator::Device::_log(
  "tarch::accelerator::Device"
);

tarch::accelerator::Device::~Device() {
  shutdown();
}

tarch::accelerator::Device& tarch::accelerator::Device::getInstance() {
  static Device instance;
  return instance;
}

std::map<void*, std::size_t>& tarch::accelerator::Device::getMemoryBlockTable() {
  return _memoryBlockTable;
}

const std::map<void*, std::size_t>& tarch::accelerator::Device::getMemoryBlockTable() const {
  return _memoryBlockTable;
}

tarch::multicore::BooleanSemaphore& tarch::accelerator::Device::getMemoryBlockSemaphore() {
  return _memoryBlockSemaphore;
}

const tarch::multicore::BooleanSemaphore& tarch::accelerator::Device::getMemoryBlockSemaphore() const {
  return _memoryBlockSemaphore;
}

void tarch::accelerator::Device::checkForMemoryLeaks() {
  tarch::multicore::Lock lock(_memoryBlockSemaphore);

  if (!_memoryBlockTable.empty()) {
    logWarning("checkForMemoryLeaks()", "WARNING: Detected " << _memoryBlockTable.size() << " memory leaks!");
  }

  auto itr = _memoryBlockTable.begin();
  auto end = _memoryBlockTable.end();

#if PeanoDebug > 0
  for (; itr != end; ++itr) {
    logWarning("checkForMemoryLeaks()", "Block @" << itr->first << " size: " << itr->second);
  }
#endif
}

#if defined(GPUOffloadingOff)

tarch::accelerator::Device::Device() {}

void tarch::accelerator::Device::configure(const std::set<int>&) {}

void tarch::accelerator::Device::shutdown() {
  checkForMemoryLeaks();
  tarch::multicore::Lock lock(_memoryBlockSemaphore);
  _memoryBlockTable.clear();
}

bool tarch::accelerator::Device::isInitialised() const { return true; }

int tarch::accelerator::Device::getNumberOfDevices() const { return 0; }

#endif // defined(GPUOffloadingOff)
