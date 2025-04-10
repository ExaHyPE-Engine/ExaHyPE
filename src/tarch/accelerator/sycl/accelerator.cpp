// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(GPUOffloadingSYCL)

#include "Device.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"

#pragma push_macro("Dimensions")
#undef Dimensions
#include <sycl/sycl.hpp>
#pragma pop_macro("Dimensions")

#include <cstring>

namespace {
  tarch::logging::Log _log("tarch::accelerator::sycl");
} // namespace

void tarch::gpuAbort() {}

void* tarch::accelerator::allocateRawData(
  std::size_t    sizeInBytes,
  MemoryLocation location,
  int            device
) {
  void* data = nullptr;
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    assertion1(device == Device::HostDevice, device);
    data = std::aligned_alloc( // sycl::malloc_host
      AlignmentOnHeap,
      tarch::padSizeToAlignment(sizeInBytes, AlignmentOnHeap)
    );
    break;
  case MemoryLocation::Device: {
    assertion1(device >= 0, device);
    sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
#ifdef UseUSM
    data = sycl::malloc_shared(sizeInBytes, queue);
#else
    data = sycl::malloc_device(sizeInBytes, queue);
#endif
    break;
  }}

  assertion(data);

  {
    tarch::multicore::Lock lock(Device::getInstance().getMemoryBlockSemaphore());
    Device::getInstance().getMemoryBlockTable()[data] = sizeInBytes;
  }

  return data;
}

void tarch::accelerator::freeRawData(
  void*          data,
  MemoryLocation location,
  int            device
) {
  if (!data) {
    return;
  }

  {
    tarch::multicore::Lock lock(Device::getInstance().getMemoryBlockSemaphore());
    auto itr = Device::getInstance().getMemoryBlockTable().find(data);
    if (itr == Device::getInstance().getMemoryBlockTable().end()) {
      logDebug("freeRawData(...)", "Attempt to free invalid pointer")
      return;
    }
    Device::getInstance().getMemoryBlockTable().erase(itr);
  }

  switch (location) {
    case MemoryLocation::Heap:
      assertion1(device == Device::HostDevice, device);
      std::free(data); // sycl::free
      break;
    case MemoryLocation::Device:
      assertion1(device >= 0, device);
      sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
      sycl::free(data, queue);
      break;
  }
}

void tarch::accelerator::copyTo(
  void*          target,
  const void*    source,
  std::size_t    sizeInBytes,
  MemoryLocation location,
  int            device
) {
  assertion(source);
  assertion(target);
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    logDebug("copyTo(...)", "Copy " << sizeInBytes << " bytes from host to host");
    std::memcpy(target, source, sizeInBytes);
    break;
  case MemoryLocation::Device: {
#ifdef UseUSM
    std::memcpy(target, source, sizeInBytes);
#else
    assertion1(device >= 0, device);
    logDebug("copyTo(...)", "Copy " << sizeInBytes << " bytes from host to device");
    sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
    queue.memcpy(target, source, sizeInBytes).wait();
    //queue.submit([&](sycl::handler& handle) { handle.memcpy(target, source, sizeInBytes); }).wait_and_throw();
#endif
    break;
  }}
}

void tarch::accelerator::copyFrom(
  const void*    source,
  void*          target,
  std::size_t    sizeInBytes,
  MemoryLocation location,
  int            device
) {
  assertion(source);
  assertion(target);
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    logDebug("copyFrom(...)", "Copy " << sizeInBytes << " bytes from host to host");
    std::memcpy(target, source, sizeInBytes);
    break;
  case MemoryLocation::Device: {
#ifdef UseUSM
    std::memcpy(target, source, sizeInBytes);
#else
    assertion1(device >= 0, device);
    logDebug("copyFrom(...)", "Copy " << sizeInBytes << " bytes from device to host");
    sycl::queue& queue = *static_cast<sycl::queue*>(getSYCLQueue(device));
    queue.memcpy(target, source, sizeInBytes).wait();
    //queue.submit([&](sycl::handler& handle) { handle.memcpy(target, source, sizeInBytes); }).wait_and_throw();
#endif
    break;
  }}
}

#endif // defined(GPUOffloadingSYCL)
