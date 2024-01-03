// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <cstddef>

#include "MemoryLocation.h"
#include "MemoryManager.h"
#include "MemoryUsageFormat.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/Device.h"

namespace tarch { // TODO: namespace tarch::memory
  /**
   * Method for getting the application's memory footprint
   *
   * We use the exact version of KiloByte with 1024 bytes.
   *
   * The implementation is based on the C-version of Michael Moltenbrey.
   */
  int getMemoryUsage(MemoryUsageFormat format);
  int getTotalMemory(MemoryUsageFormat format);
  int getFreeMemory(MemoryUsageFormat format);

  int convertMemorySize(long long value, tarch::memory::MemoryUsageFormat format);

  template <class T = double>
  T* allocateMemory(std::size_t size, MemoryLocation location, int device = tarch::accelerator::Host) {
    device = tarch::accelerator::Device::getInstance().getRankLocalDeviceId();
    // return reinterpret_cast<T*>(MemoryManager::getInstance().allocate(size * sizeof(T), location, device));
    return reinterpret_cast<T*>(tarch::accelerator::allocateRawData(size * sizeof(T), location, device));
  }

  void freeMemory(void* pointer, MemoryLocation location, int device = tarch::accelerator::Host);
} // namespace tarch
