// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <list>
#include <map>
#include <vector>

#include "tarch/logging/Log.h"
#include "tarch/memory/MemoryLocation.h"
#include "tarch/memory/MemoryUsageFormat.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/multicore.h"

namespace tarch::memory {
  class MemoryManager;
} // namespace tarch::memory

/**
 * Singleton that manages persistent memory on different accelerators
 *
 * The idea is that this is basically a heap rep for accelerators and that code
 * requiring heap memory does not allocate this memory but instead grabs it
 * from here. As it is not freed afterwards, we avoid overheads that arise
 * from frequent re-allocation.
 *
 */
class tarch::memory::MemoryManager {
private:
  static tarch::logging::Log _log;

  tarch::multicore::BooleanSemaphore _semaphore;

  /**
   * Map of pages (memory blocks) per size.
   */
  typedef std::map<std::size_t, std::vector<std::pair<void*, MemoryLocation>>> FreePages;

  /**
   * Counterpart of FreePages, i.e., map of memory blocks (addresses) onto
   * sizes.
   */
  typedef std::map<void*, std::pair<std::size_t, MemoryLocation>> UsedPages;

  struct MemoryPages {
    FreePages freePages;
    UsedPages usedPages;
  };

  /**
   * Map from device number onto memory
   */
  std::map<int, MemoryPages> _memoryPages;

  MemoryManager() = default;

  void clearWithoutLock();

public:
  MemoryManager(const MemoryManager& copy) = delete;

  /**
   * Invoke clear().
   */
  virtual ~MemoryManager();

  static MemoryManager& getInstance();

  /**
   * Return a pointer to a new memory block of size bytes on device.
   *
   * This operation is thread-safe. If tries to recycle data that has been
   * allocated before. If this is not successful, it acquires new data on
   * the device and then hands out this new data.
   *
   * Before we return the address of the new data, we memorise it as booked
   * data and store the size corresponding to this device. This is important:
   * When we return the data through free(), we have to look up what the
   * underlying size is and bucket sort the pointer for reusage
   * appropriately.
   *
   * The routine could, in theory, hand out a pointer to a memory block which
   * points to a chunk of memory that is bigger than size. In the context of
   * Peano this would however be stupid: Most Peano codes work with rather
   * regular chunks of data and if we use big chunks to host smaller data
   * footprint, we actually run risk to fragment the device memory.
   *
   * The routine takes a memory location as input argument where the new
   * memory block shall be placed. Then, it forwards the actual memory allocation
   * to the respective accelerator.
   */
  void* allocate(std::size_t size, MemoryLocation location, int device);

  /**
   * Free does not actually free data, but instead hands the memory block
   * back to the singleton, such that the next allocate() call can
   * reuse the memory region.
   */
  void free(void* pointer, int device);

  /**
   * Idempotent function
   *
   * Run over _memoryPages, i.e., handle device by device. If a device still
   * has used pages, the user code has not properly freed data. We issue an
   * error.
   *
   * This operation is thread-safe.
   */
  void clear();

  const std::map<int, MemoryPages>& getMemoryPages() const;

  int getNumberOfFreePages() const;
  int getNumberOfUsedPages() const;

  int getNumberOfFreePages(int device) const;
  int getNumberOfUsedPages(int device) const;

  int getFreeAllocatedMemory(MemoryUsageFormat format) const;
  int getUsedAllocatedMemory(MemoryUsageFormat format) const;

  int getFreeAllocatedMemory(MemoryUsageFormat format, int device) const;
  int getUsedAllocatedMemory(MemoryUsageFormat format, int device) const;

  std::size_t getFreeAllocatedMemoryInBytes() const;
  std::size_t getUsedAllocatedMemoryInBytes() const;

  std::size_t getFreeAllocatedMemoryInBytes(int device) const;
  std::size_t getUsedAllocatedMemoryInBytes(int device) const;
};
