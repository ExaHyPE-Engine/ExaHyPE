// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "config.h"
#include "tarch/memory/MemoryLocation.h"

#if !defined(GPUOffloadingOMP) and !defined(GPUOffloadingSYCL) and !defined(GPUOffloadingCPP) and !defined(GPUOffloadingCUDA)
#define GPUOffloadingOff
#endif

/**
 * Have to include this header, as we need access to the SYCL_EXTERNAL keyword.
 */
#include "tarch/accelerator/sycl/accelerator.h"

namespace tarch::accelerator {
#if defined(GPUOffloadingHIP)
#define GPUCallableMethod __host__ __device__
#elif defined(GPUOffloadingSYCL) or defined(SharedSYCL)
#define GPUCallableMethod SYCL_EXTERNAL
#elif defined(GPUOffloadingCPP) && defined(_NVHPC_STDPAR_GPU)
#define GPUCallableMethod __host__ __device__
#else
#define GPUCallableMethod
#endif

#if defined(GPUOffloadingHIP)
#define GPUCallableInlineMethod __host__ __device__ __forceinline__
#elif defined(GPUOffloadingSYCL) or defined(SharedSYCL)
#define GPUCallableInlineMethod inline
#elif defined(GPUOffloadingCPP) && defined(_NVHPC_STDPAR_GPU)
#define GPUCallableInlineMethod __host__ __device__ __forceinline__
#else
#define GPUCallableInlineMethod // inline
#endif

/**
 * Delegates to std::abort() if no GPU offloading is active. Otherwise it
 * degenerates to nop, as there is no abort on an accelerator. The vanilla
 * implementation can be found in accelerator.cpp. The vendor-specific
 * implementations are hold within their accelerator.cpp file.
 */
#if defined(GPUOffloadingOMP)
#pragma omp declare target
  void gpuAbort();
#pragma omp end declare target
#elif defined(GPUOffloadingSYCL) or defined(SharedSYCL)
  GPUCallableMethod void gpuAbort();
#elif defined(GPUOffloadingCPP)
  GPUCallableMethod void gpuAbort();
#else
  void gpuAbort();
#endif

  void* allocateRawData(std::size_t size, memory::MemoryLocation location, int device);
  void  freeRawData(void* data, memory::MemoryLocation location, int device);

  void copyTo(void* dst, const void* src, std::size_t count);
  void copyFrom(void* dst, const void* src, std::size_t count);
  void copyBetween(void* dst, const void* src, std::size_t count);
  void copyToAsync(void* dst, const void* src, std::size_t count, void* stream = nullptr);
  void copyFromAsync(void* dst, const void* src, std::size_t count, void* stream = nullptr);
  void copyBetweenAsync(void* dst, const void* src, std::size_t count, void* stream = nullptr);

  void copy2DTo(void* dst, std::size_t dpitch, const void* src, std::size_t spitch, std::size_t width, std::size_t height);
  void copy2DFrom(void* dst, std::size_t dpitch, const void* src, std::size_t spitch, std::size_t width, std::size_t height);

  void prefetchUnifiedMemoryTo(const void* devPtr, std::size_t count, void* stream = nullptr);

  int padSizeToAlignment(std::size_t size, int alignment);
} // namespace tarch::accelerator
