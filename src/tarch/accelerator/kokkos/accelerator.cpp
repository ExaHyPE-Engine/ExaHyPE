// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/accelerator/accelerator.h"

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)

#include "tarch/Assertions.h"

#include <Kokkos_Core.hpp>

void*
tarch::accelerator::allocateDeviceMemory(std::size_t sizeInBytes, [[maybe_unused]] int device) {
  void* data = Kokkos::kokkos_malloc<Kokkos::DefaultExecutionSpace::memory_space>(sizeInBytes);
  assertion(data);
  return data;
}

void*
tarch::accelerator::allocatePinnedMemory(std::size_t sizeInBytes, [[maybe_unused]] int device) {
  void* data = Kokkos::kokkos_malloc<Kokkos::SharedHostPinnedSpace>(sizeInBytes);
  assertion(data);
  return data;
}

void
tarch::accelerator::freeDeviceMemory(void* data, [[maybe_unused]] int device) {
  Kokkos::kokkos_free<Kokkos::DefaultExecutionSpace::memory_space>(data);
}

void
tarch::accelerator::freePinnedMemory(void* data, [[maybe_unused]] int device) {
  Kokkos::kokkos_free<Kokkos::SharedHostPinnedSpace>(data);
}

void
tarch::accelerator::copyToDeviceMemory(
  void*                target,
  const void*          source,
  std::size_t          sizeInBytes,
  [[maybe_unused]] int device
) {
  using SrcViewType = Kokkos::View<const unsigned char*, Kokkos::HostSpace, Kokkos::MemoryTraits<Kokkos::Unmanaged>>;
  SrcViewType srcView(static_cast<const unsigned char*>(source), sizeInBytes);

  using DstViewType =
    Kokkos::View<unsigned char*, Kokkos::DefaultExecutionSpace::memory_space, Kokkos::MemoryTraits<Kokkos::Unmanaged>>;
  DstViewType dstView(static_cast<unsigned char*>(target), sizeInBytes);

  Kokkos::deep_copy(dstView, srcView);
}

void
tarch::accelerator::copyToPinnedMemory(
  void*                target,
  const void*          source,
  std::size_t          sizeInBytes,
  [[maybe_unused]] int device
) {
  std::memcpy(target, source, sizeInBytes);
}

void
tarch::accelerator::copyFromDeviceMemory(
  const void*          source,
  void*                target,
  std::size_t          sizeInBytes,
  [[maybe_unused]] int device
) {
  using SrcViewType = Kokkos::View<
    const unsigned char*,
    Kokkos::DefaultExecutionSpace::memory_space,
    Kokkos::MemoryTraits<Kokkos::Unmanaged>
  >;
  SrcViewType srcView(static_cast<const unsigned char*>(source), sizeInBytes);

  using DstViewType = Kokkos::View<unsigned char*, Kokkos::HostSpace, Kokkos::MemoryTraits<Kokkos::Unmanaged>>;
  DstViewType dstView(static_cast<unsigned char*>(target), sizeInBytes);

  Kokkos::deep_copy(dstView, srcView);
}

void
tarch::accelerator::copyFromPinnedMemory(
  const void*          source,
  void*                target,
  std::size_t          sizeInBytes,
  [[maybe_unused]] int device
) {
  std::memcpy(target, source, sizeInBytes);
}

#endif // defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
