// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Device.h"
#include "MemoryLocation.h"

#if defined(WITH_GPU_CUDA)
#include "cuda/Device.h" // To get access to the __device__ keyword
#endif

#if defined(WITH_GPU_HIP)
#include "hip/Device.h" // To get access to the __device__ keyword
#endif

#if defined(WITH_GPU_KOKKOS)
#include "kokkos/Device.h" // To get access to the KOKKOS_FUNCTION keyword
#endif

#include "tarch/CompilerSpecificSettings.h"

#define TARGET_DECLARE
#define TARGET_DECLARE_END
#define GPU_CALLABLE_METHOD

// https://docs.nvidia.com/cuda/cuda-compiler-driver-nvcc/#nvcc-identification-macro
// defined(_NVHPC_CUDA) || defined(__NVCOMPILER_CUDA__) || defined(__CUDACC__) || defined(NVCC) || defined(__NVCC__)
#if defined(WITH_GPU_CUDA) || defined(WITH_GPU_HIP)
#undef GPU_CALLABLE_METHOD
#define GPU_CALLABLE_METHOD __host__ __device__
#endif

#if defined(WITH_GPU_KOKKOS)
#undef GPU_CALLABLE_METHOD
#define GPU_CALLABLE_METHOD KOKKOS_FUNCTION
#endif

#if defined(WITH_GPU_OMPT)
#undef TARGET_DECLARE
#undef TARGET_DECLARE_END
#define TARGET_DECLARE _Pragma("omp declare target")
#define TARGET_DECLARE_END _Pragma("omp end declare target")
#endif

namespace tarch {
  namespace accelerator {
    void* allocateHeapMemory(std::size_t sizeInBytes);
    void* allocateDeviceMemory(std::size_t sizeInBytes, int device);
    void* allocatePinnedMemory(std::size_t sizeInBytes, int device);
    void* allocateRawData(std::size_t sizeInBytes, MemoryLocation location, int device);
    void  freeHeapMemory(void* data);
    void  freeDeviceMemory(void* data, int device);
    void  freePinnedMemory(void* data, int device);
    void  freeRawData(void* data, MemoryLocation location, int device);
    void  copyToHeapMemory(void* target, const void* source, std::size_t sizeInBytes);
    void  copyToDeviceMemory(void* target, const void* source, std::size_t sizeInBytes, int device);
    void  copyToPinnedMemory(void* target, const void* source, std::size_t sizeInBytes, int device);
    void  copyTo(void* target, const void* source, std::size_t sizeInBytes, MemoryLocation location, int device);
    void  copyFromHeapMemory(const void* source, void* target, std::size_t sizeInBytes);
    void  copyFromDeviceMemory(const void* source, void* target, std::size_t sizeInBytes, int device);
    void  copyFromPinnedMemory(const void* source, void* target, std::size_t sizeInBytes, int device);
    void  copyFrom(const void* source, void* target, std::size_t sizeInBytes, MemoryLocation location, int device);
  } // namespace accelerator

  /**
   * Allocates memory on the specified memory location
   *
   * The device id is an optional argument that is required for
   * memory allocation on an accelerator. Default is 0 (DefaultDevice).
   * For memory allocation on accelerators, passing a device id >= 0 is
   * necessary.
   *
   * @param count Number of elements to allocate memory for.
   * @param location Location on where to allocate the memory.
   * @param device (Optional) device id for memory allocation on accelerators.
   */
  template <class T>
  T* allocateMemory(std::size_t count, MemoryLocation location, int device = accelerator::Device::DefaultDevice) {
    return reinterpret_cast<T*>(accelerator::allocateRawData(count * sizeof(T), location, device));
  }

  void freeMemory(void* memoryPtr, MemoryLocation location, int device = accelerator::Device::DefaultDevice);

  template <class To, class From>
  void copyTo(
    To*            to,
    const From*    from,
    std::size_t    count,
    MemoryLocation location,
    int            device = accelerator::Device::DefaultDevice
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
    int            device = accelerator::Device::DefaultDevice
  ) {
    static_assert(sizeof(From) == sizeof(To), "Data type mismatch");
    accelerator::copyFrom(from, to, count * sizeof(From), location, device);
  }
} // namespace tarch
