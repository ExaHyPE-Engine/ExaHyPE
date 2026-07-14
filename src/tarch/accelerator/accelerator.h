// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Device.h"
#include "MemoryLocation.h"

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)
#include "cuda/Device.h" // To get access to the __device__ keyword
#endif

#if defined(PEANO_WITH_GPU_BACKEND_HIP)
#include "hip/Device.h" // To get access to the __device__ keyword
#endif

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#include "kokkos/Device.h" // To get access to the KOKKOS_FUNCTION keyword
#endif

#include "tarch/CompilerSpecificSettings.h"

#define TARGET_DECLARE
#define TARGET_DECLARE_END
#define GPU_CALLABLE_METHOD
#define GPU_CALLABLE_LAMBDA

// https://docs.nvidia.com/cuda/cuda-compiler-driver-nvcc/#nvcc-identification-macro
// defined(_NVHPC_CUDA) || defined(__NVCOMPILER_CUDA__) || defined(__CUDACC__) || defined(NVCC) || defined(__NVCC__)
#if defined(PEANO_WITH_GPU_BACKEND_CUDA) || defined(PEANO_WITH_GPU_BACKEND_HIP)
#undef GPU_CALLABLE_METHOD
#define GPU_CALLABLE_METHOD __host__ __device__
#undef GPU_CALLABLE_LAMBDA
#define GPU_CALLABLE_LAMBDA __device__
#endif

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#undef GPU_CALLABLE_METHOD
#define GPU_CALLABLE_METHOD KOKKOS_FUNCTION
#undef GPU_CALLABLE_LAMBDA
#if defined(KOKKOS_ENABLE_CUDA) || defined(KOKKOS_ENABLE_HIP)
#define GPU_CALLABLE_LAMBDA __host__ __device__
#else
#define GPU_CALLABLE_LAMBDA
#endif
#endif

// nvcc warns (#20013) when a host+device function calls a constexpr but host-only stdlib helper
// (std::min/max/abs, std::numeric_limits). The calls are compile-time and safe; the native cuda
// backend allows them via FindCUDA's --expt-relaxed-constexpr, but that flag cannot be placed in the
// Kokkos launcher model without hitting host-only g++ units, so we relax the diagnostic here. No-op
// off nvcc.
#if defined(__CUDACC__)
#pragma nv_diag_suppress 20013
#endif

#if defined(PEANO_WITH_GPU_BACKEND_OMPT)
#undef TARGET_DECLARE
#undef TARGET_DECLARE_END
#define TARGET_DECLARE _Pragma("omp declare target")
#define TARGET_DECLARE_END _Pragma("omp end declare target")
#endif

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#define COMPILE_IF_ON_DEVICE(CODE) KOKKOS_IF_ON_DEVICE(CODE)
#define COMPILE_IF_ON_HOST(CODE) KOKKOS_IF_ON_HOST(CODE)
#else
#if defined(__CUDA_ARCH__) or defined(__HIP_DEVICE_COMPILE__) or defined(__SPIR__) or defined(__SPIRV__) or \
  defined(__SYCL_DEVICE_ONLY__) or defined(__NVPTX__) or defined(__AMDGCN__)
#define COMPILE_IF_ON_DEVICE(CODE) {CODE}
#define COMPILE_IF_ON_HOST(CODE)
#else
#define COMPILE_IF_ON_DEVICE(CODE)
#define COMPILE_IF_ON_HOST(CODE) {CODE}
#endif
#endif

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#include "kokkos/Abort.h"
#elif defined(PEANO_WITH_GPU_BACKEND_CUDA)
#include "cuda/Abort.h"
#elif defined(PEANO_WITH_GPU_BACKEND_HIP)
#include "hip/Abort.h"
#elif defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include "sycl/Abort.h"
#elif defined(PEANO_WITH_GPU_BACKEND_OMPT)
#include "ompt/Abort.h"
#endif

namespace tarch {
  namespace accelerator {
#if !defined(PEANO_WITH_GPU_BACKEND_KOKKOS) && !defined(PEANO_WITH_GPU_BACKEND_CUDA) && \
  !defined(PEANO_WITH_GPU_BACKEND_HIP) && !defined(PEANO_WITH_GPU_BACKEND_SYCL) && \
  !defined(PEANO_WITH_GPU_BACKEND_OMPT)
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

    /**
     * Set / get the implicit current device used by tarch::parallelFor and
     * tarch::parallelReduce on backends that target an accelerator.
     *
     * Storage is thread-local: each host thread carries its own selection so
     * concurrent host threads can dispatch to different GPUs without locking.
     * The default value is Device::DefaultDevice (0). The CPU OpenMP backend
     * ignores this state.
     */
    void setCurrentDevice(int device);
    int  getCurrentDevice();
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
