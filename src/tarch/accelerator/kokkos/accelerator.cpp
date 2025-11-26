// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(WITH_GPU_KOKKOS)

#include "tarch/Assertions.h"
#include "tarch/accelerator/MemoryBlocks.h"

#include <Kokkos_Core.hpp>

namespace {
  tarch::logging::Log _log("tarch::accelerator::kokkos");
} // namespace

void* tarch::accelerator::allocateRawData(std::size_t sizeInBytes, MemoryLocation location, int device) {
  void* data = nullptr;
  assertion2(sizeInBytes > 0, sizeInBytes, toString(location));

  switch (location) {
  case MemoryLocation::Heap:
    data = Kokkos::kokkos_malloc<Kokkos::HostSpace>(sizeInBytes);
    // data = allocateRawDataOnHeap(sizeInBytes);
    break;
  case MemoryLocation::Device:
#ifdef WITH_USM
    data = Kokkos::kokkos_malloc<Kokkos::SharedSpace>(sizeInBytes);
#else
    data = Kokkos::kokkos_malloc<Kokkos::DefaultExecutionSpace::memory_space>(sizeInBytes);
#endif
    break;
  case MemoryLocation::Pinned:
    data = Kokkos::kokkos_malloc<Kokkos::SharedHostPinnedSpace>(sizeInBytes);
    break;
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
    Kokkos::kokkos_free<Kokkos::HostSpace>(data);
    // freeRawDataOnHeap(data);
    break;
  case MemoryLocation::Device:
#ifdef WITH_USM
    Kokkos::kokkos_free<Kokkos::SharedSpace>(data);
#else
    Kokkos::kokkos_free<Kokkos::DefaultExecutionSpace::memory_space>(data);
#endif
    break;
  case MemoryLocation::Pinned:
    Kokkos::kokkos_free<Kokkos::SharedHostPinnedSpace>(data);
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
  case MemoryLocation::Device: {
    logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to device");
#ifdef WITH_USM
    std::memcpy(target, source, sizeInBytes);
#else
    using SrcViewType = Kokkos::View<const unsigned char*, Kokkos::HostSpace, Kokkos::MemoryTraits<Kokkos::Unmanaged>>;
    SrcViewType srcView(static_cast<const unsigned char*>(source), sizeInBytes);

    using DstViewType = Kokkos::
      View<unsigned char*, Kokkos::DefaultExecutionSpace::memory_space, Kokkos::MemoryTraits<Kokkos::Unmanaged>>;
    DstViewType dstView(static_cast<unsigned char*>(target), sizeInBytes);

    Kokkos::deep_copy(dstView, srcView);
#endif
    break;
  }
  case MemoryLocation::Pinned:
    logDebug("copyTo()", "Copy " << sizeInBytes << " bytes from host to device");
    std::memcpy(target, source, sizeInBytes);
    break;
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
  case MemoryLocation::Device: {
    logDebug("copyFrom()", "Copy " << sizeInBytes << " bytes from device to host");
#ifdef WITH_USM
    std::memcpy(target, source, sizeInBytes);
#else
    using SrcViewType = Kokkos::
      View<const unsigned char*, Kokkos::DefaultExecutionSpace::memory_space, Kokkos::MemoryTraits<Kokkos::Unmanaged>>;
    SrcViewType srcView(static_cast<const unsigned char*>(source), sizeInBytes);

    using DstViewType = Kokkos::View<unsigned char*, Kokkos::HostSpace, Kokkos::MemoryTraits<Kokkos::Unmanaged>>;
    DstViewType dstView(static_cast<unsigned char*>(target), sizeInBytes);

    Kokkos::deep_copy(dstView, srcView);
#endif
    break;
  }
  case MemoryLocation::Pinned:
    logDebug("copyFrom()", "Copy " << sizeInBytes << " bytes from device to host");
    std::memcpy(target, source, sizeInBytes);
    break;
  }
}

#endif // defined(WITH_GPU_KOKKOS)
