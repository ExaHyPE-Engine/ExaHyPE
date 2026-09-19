// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/Device.h"
#include "tarch/accelerator/MemoryLocation.h"

#if defined(PEANO_WITH_GPU_BACKEND_CUDA)
#include "tarch/accelerator/cuda/Device.h" // To get access to the __device__ keyword
#endif

#if defined(PEANO_WITH_GPU_BACKEND_HIP)
#include "tarch/accelerator/hip/Device.h" // To get access to the __device__ keyword
#endif

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#include "tarch/accelerator/kokkos/Device.h" // To get access to the KOKKOS_FUNCTION keyword
#endif

#if defined(PEANO_WITH_GPU_BACKEND_SYCL)
#include "tarch/accelerator/sycl/Device.h" // To get access to the ACPP_UNIVERSAL_TARGET attribute
#endif

#include "tarch/CompilerSpecificSettings.h"

#define TARGET_DECLARE
#define TARGET_DECLARE_END
#define GPU_CALLABLE_METHOD
#define GPU_CALLABLE_LAMBDA

// AdaptiveCpp compiles a function that a kernel reaches through the SYCL library headers only
// with this attribute, and DPC++ needs no attribute.
#if defined(PEANO_WITH_GPU_BACKEND_SYCL) and defined(ACPP_UNIVERSAL_TARGET)
#undef GPU_CALLABLE_METHOD
#define GPU_CALLABLE_METHOD ACPP_UNIVERSAL_TARGET
#endif

#if defined(PEANO_WITH_GPU_BACKEND_CUDA) or defined(PEANO_WITH_GPU_BACKEND_HIP)
#undef GPU_CALLABLE_METHOD
#define GPU_CALLABLE_METHOD __host__ __device__
#undef GPU_CALLABLE_LAMBDA
#define GPU_CALLABLE_LAMBDA __device__
#endif

#if defined(PEANO_WITH_GPU_BACKEND_KOKKOS)
#undef GPU_CALLABLE_METHOD
#define GPU_CALLABLE_METHOD KOKKOS_FUNCTION
#undef GPU_CALLABLE_LAMBDA
#if defined(KOKKOS_ENABLE_CUDA) or defined(KOKKOS_ENABLE_HIP)
#define GPU_CALLABLE_LAMBDA __host__ __device__
#else
#define GPU_CALLABLE_LAMBDA
#endif
#endif

// nvcc warns when a host and device template runs a host-only lambda or constexpr helper on the host.
#if defined(__CUDACC__) and not defined(__clang__)
#pragma nv_diag_suppress 20011
#pragma nv_diag_suppress 20013
#pragma nv_diag_suppress 20014
#pragma nv_diag_suppress 20015
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
#if defined(__CUDA_ARCH__) or defined(__HIP_DEVICE_COMPILE__) or defined(__SPIR__) or defined(__SPIRV__) \
  or defined(__SYCL_DEVICE_ONLY__) or defined(__NVPTX__) or defined(__AMDGCN__)
#define COMPILE_IF_ON_DEVICE(CODE) {CODE}
#define COMPILE_IF_ON_HOST(CODE)
#else
#define COMPILE_IF_ON_DEVICE(CODE)
#define COMPILE_IF_ON_HOST(CODE) {CODE}
#endif
#endif

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
  void* allocateZeroedHeapMemory(std::size_t sizeInBytes);
  void* allocateDeviceMemory(std::size_t sizeInBytes, int device);
  void* allocatePinnedMemory(std::size_t sizeInBytes, int device);
  void* allocateRawData(std::size_t sizeInBytes, MemoryLocation location, int device);
  void  freeHeapMemory(void* data);
  void  freeZeroedHeapMemory(void* data, std::size_t sizeInBytes);
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

  void setCurrentDevice(int device);
  int  getCurrentDevice();

  void reportMissingTaskloopSupportOnce();
} // namespace tarch::accelerator

namespace tarch {
  /**
   * Allocates memory for count elements of T at the given memory location. An allocation on an
   * accelerator needs the id of that device.
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
