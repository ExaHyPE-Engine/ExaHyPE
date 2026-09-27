// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/MemoryLocation.h"

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#include "tarch/accelerator/kokkos/Abort.h"
#elif defined(PEANO_WITH_GPU_BACKEND_CUDA)
#include "tarch/accelerator/cuda/Abort.h"
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
#include "tarch/accelerator/hip/Abort.h"
#elif defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include "tarch/accelerator/sycl/Abort.h"
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
#include "tarch/accelerator/ompt/Abort.h"
#endif

namespace tarch::accelerator {
#if not defined(PEANO_WITH_GPU_BACKEND_KOKKOS) and not defined(PEANO_WITH_GPU_BACKEND_CUDA) \
  and not defined(PEANO_WITH_GPU_BACKEND_HIP) and not defined(PEANO_WITH_GPU_BACKEND_SYCL) \
  and not defined(PEANO_WITH_GPU_BACKEND_OMPT)
  TARGET_DECLARE
  inline GPU_CALLABLE_METHOD void abort(const char* message) {
    std::fputs(message, stderr);
    std::fflush(stderr);
    std::_Exit(EXIT_FAILURE);
  }
  TARGET_DECLARE_END
#endif

  void* allocateHeapMemory(std::size_t sizeInBytes);
  void* allocateDeviceMemory(std::size_t sizeInBytes, int device);
  void* allocatePinnedMemory(std::size_t sizeInBytes, int device);
  void* allocateRawData(std::size_t sizeInBytes, MemoryLocation location, int device);
  void freeHeapMemory(void* data);
  void freeDeviceMemory(void* data, int device);
  void freePinnedMemory(void* data, int device);
  void freeRawData(void* data, MemoryLocation location, int device);
  void copyToHeapMemory(void* target, const void* source, std::size_t sizeInBytes);
  void copyToDeviceMemory(void* target, const void* source, std::size_t sizeInBytes, int device);
  void copyToPinnedMemory(void* target, const void* source, std::size_t sizeInBytes, int device);
  void copyTo(void* target, const void* source, std::size_t sizeInBytes, MemoryLocation location, int device);
  void copyFromHeapMemory(const void* source, void* target, std::size_t sizeInBytes);
  void copyFromDeviceMemory(const void* source, void* target, std::size_t sizeInBytes, int device);
  void copyFromPinnedMemory(const void* source, void* target, std::size_t sizeInBytes, int device);
  void copyFrom(const void* source, void* target, std::size_t sizeInBytes, MemoryLocation location, int device);

  void setCurrentDevice(int device);
  int getCurrentDevice();

  void reportMissingTaskloopSupportOnce();
} // namespace tarch::accelerator

namespace tarch {
  /**
   * Allocates memory for count elements of T at the given memory location. An allocation on an
   * accelerator lands on the current device unless device names another one.
   */
  template <class T>
  T* allocateMemory(std::size_t count, MemoryLocation location, int device = accelerator::getCurrentDevice()) {
    return reinterpret_cast<T*>(accelerator::allocateRawData(count * sizeof(T), location, device));
  }

  void freeMemory(void* memoryPtr, MemoryLocation location, int device = accelerator::getCurrentDevice());

  template <class To, class From>
  void copyTo(
    To*            to,
    const From*    from,
    std::size_t    count,
    MemoryLocation location,
    int            device = accelerator::getCurrentDevice()
  ) {
    static_assert(sizeof(To) == sizeof(From), "Data type mismatch");
    accelerator::copyTo(to, from, count * sizeof(To), location, device);
  }

  template <class From, class To>
  void copyFrom(
    const From*    from,
    To*            to,
    std::size_t    count,
    MemoryLocation location,
    int            device = accelerator::getCurrentDevice()
  ) {
    static_assert(sizeof(From) == sizeof(To), "Data type mismatch");
    accelerator::copyFrom(from, to, count * sizeof(From), location, device);
  }
} // namespace tarch
