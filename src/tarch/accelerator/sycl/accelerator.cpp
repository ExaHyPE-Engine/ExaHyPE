// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(WITH_GPU_SYCL)

#include "Device.h"
#include "tarch/Assertions.h"
#include "tarch/accelerator/MemoryBlocks.h"

#include <cstring>
#include <sycl/sycl.hpp>

namespace {
  tarch::logging::Log _log("tarch::accelerator::sycl");
} // namespace

void* tarch::accelerator::allocateRawData(std::size_t sizeInBytes, MemoryLocation location, int device) {
  void* data = nullptr;
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    data = std::aligned_alloc(ALIGNMENT, tarch::padSizeToAlignment(sizeInBytes, ALIGNMENT));
    break;
  case MemoryLocation::Device: {
    assertion1(device >= 0, device);
    sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
#ifdef WITH_USM
    data = sycl::malloc_shared(sizeInBytes, queue);
#else
    data = sycl::malloc_device(sizeInBytes, queue);
#endif
    break;
  }
  case MemoryLocation::Pinned: {
    assertion1(device >= 0, device);
    sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
#ifdef WITH_USM
    data = sycl::malloc_shared(sizeInBytes, queue);
#else
    data = sycl::malloc_host(sizeInBytes, queue);
#endif
    break;
  }
  }

  assertion(data);

  MemoryBlocks::getInstance().bookmarkMemoryBlock(data, sizeInBytes);

  return data;
}

void tarch::accelerator::freeRawData(void* data, MemoryLocation location, int device) {
  if (!data) {
    return;
  }

  MemoryBlocks::getInstance().removeMemoryBlock(data);

  switch (location) {
  case MemoryLocation::Heap:
    std::free(data);
    break;
  case MemoryLocation::Device:
  case MemoryLocation::Pinned:
    assertion1(device >= 0, device);
    sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
    sycl::free(data, queue);
    break;
  }
}

void tarch::accelerator::
  copyTo(void* target, const void* source, std::size_t sizeInBytes, MemoryLocation location, int device) {
  assertion(source);
  assertion(target);
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to host");
    std::memcpy(target, source, sizeInBytes);
    break;
  case MemoryLocation::Device:
  case MemoryLocation::Pinned: {
#ifdef WITH_USM
    std::memcpy(target, source, sizeInBytes);
#else
    assertion1(device >= 0, device);
    logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to device");
    sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
    queue.memcpy(target, source, sizeInBytes).wait();
    // queue.submit([&](sycl::handler& handle) { handle.memcpy(target, source, sizeInBytes); }).wait_and_throw();
#endif
    break;
  }
  }
}

void tarch::accelerator::
  copyFrom(const void* source, void* target, std::size_t sizeInBytes, MemoryLocation location, int device) {
  assertion(source);
  assertion(target);
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    logDebug("copyFrom()", "Copy " << sizeInBytes << " bytes from host to host");
    std::memcpy(target, source, sizeInBytes);
    break;
  case MemoryLocation::Device:
  case MemoryLocation::Pinned: {
#ifdef WITH_USM
    std::memcpy(target, source, sizeInBytes);
#else
    assertion1(device >= 0, device);
    logDebug("copyFrom()", "Copy " << sizeInBytes << " bytes from device to host");
    sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
    queue.memcpy(target, source, sizeInBytes).wait();
    // queue.submit([&](sycl::handler& handle) { handle.memcpy(target, source, sizeInBytes); }).wait_and_throw();
#endif
    break;
  }
  }
}

#endif // defined(WITH_GPU_SYCL)
