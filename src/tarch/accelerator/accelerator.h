// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Device.h"
#include "MemoryLocation.h"

#if defined(WITH_GPU_CUDA)
#include "cuda/Device.h" // To get access to the __device__ keyword
#endif

#if defined(WITH_GPU_KOKKOS)
#include "kokkos/Device.h" // To get access to the KOKKOS_FUNCTION keyword
#endif

#include "tarch/CompilerSpecificSettings.h"

namespace tarch {
// https://docs.nvidia.com/cuda/cuda-compiler-driver-nvcc/#nvcc-identification-macro
// defined(_NVHPC_CUDA) || defined(__NVCOMPILER_CUDA__) || defined(__CUDACC__) || defined(NVCC) || defined(__NVCC__)
#if defined(WITH_GPU_CUDA)
#define GPU_CALLABLE_METHOD __host__ __device__
GPU_CALLABLE_METHOD void gpuAbort();
#elif defined(WITH_GPU_SYCL)
#define GPU_CALLABLE_METHOD
void gpuAbort();
#elif defined(WITH_GPU_OMP_TARGET)
#define GPU_CALLABLE_METHOD
#pragma omp declare target
void gpuAbort();
#pragma omp end declare target
#elif defined(WITH_GPU_KOKKOS)
#define GPU_CALLABLE_METHOD KOKKOS_FUNCTION
#else
#define GPU_CALLABLE_METHOD
#endif

  namespace accelerator {
    void* allocateRawData(
      std::size_t    sizeInBytes,
      MemoryLocation location,
      int            device
    );

    void* reallocateRawData(
      void*          data,
      std::size_t    sizeInBytes,
      MemoryLocation location,
      int            device
    );

    void freeRawData(
      void*          data,
      MemoryLocation location,
      int            device
    );

    void copyTo(
      void*          target,
      const void*    source,
      std::size_t    sizeInBytes,
      MemoryLocation location,
      int            device
    );

    void copyFrom(
      const void*    source,
      void*          target,
      std::size_t    sizeInBytes,
      MemoryLocation location,
      int            device
    );
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
  template <class T = double>
  T* allocateMemory(
    std::size_t    count,
    MemoryLocation location,
    int            device = accelerator::Device::DefaultDevice
  ) {
    return reinterpret_cast<T*>(
      accelerator::allocateRawData(count * sizeof(T), location, device)
    );
  }

  template <class T = double>
  T* reallocateMemory(
    void*          memoryPtr,
    std::size_t    count,
    MemoryLocation location,
    int            device = accelerator::Device::DefaultDevice
  ) {
    return reinterpret_cast<T*>(
      accelerator::reallocateRawData(memoryPtr, count * sizeof(T), location, device)
    );
  }

  /**
   * Free memory
   *
   * This routine is thread-safe in the sense that it checks if data actually
   * point to something that is not nullptr. If data is the nullptr, the
   * operation degenerates to nop. This is important as we use the routine
   * within smart pointers, where we don't want to check manually if data is
   * actually nullptr or not.
   */
  void freeMemory(
    void*          memoryPtr,
    MemoryLocation location,
    int            device = accelerator::Device::DefaultDevice
  );

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

  std::size_t padSizeToAlignment(std::size_t size, std::size_t alignment);
} // namespace tarch
