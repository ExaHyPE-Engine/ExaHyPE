// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/multicore/multicore.h"

#if !defined(GPUOffloadingOMP) and !defined(GPUOffloadingSYCL) and !defined(GPUOffloadingCPP)
#define GPUOffloadingOff
#endif

/**
 * Have to include this header, as I need access to the SYCL_EXTERNAL keyword.
 */
#include "tarch/accelerator/sycl/Device.h"

namespace tarch {
  /**
   * We need an annotation GPUCallableMethod for some targets like HIP. This
   * marker is to be used before the return type is written down. This is different
   * to the InlineMethod marker which is to be used after the argument list.
   */
  #if defined(GPUOffloadingHIP)
    #define GPUCallableMethod __host__ __device__
  #elif defined(GPUOffloadingSYCL)
    #define GPUCallableMethod SYCL_EXTERNAL
  #elif defined(GPUOffloadingCPP) && defined(_NVHPC_STDPAR_GPU)
    #define GPUCallableMethod __host__ __device__
  #else
    #define GPUCallableMethod
  #endif

  /**
   * Delegates to std::abort() if no GPU offloading is active. Otherwise is
   * degenerates to nop, as there is no abort on an accelerator. The vanilla
   * implementation can be found in accelerator.cpp. The vendor-specific
   * implementations are hold within their accelerator.cpp file.
   */
  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  void gpuAbort();
  #pragma omp end declare target
  #elif defined(GPUOffloadingSYCL)
  GPUCallableMethod void gpuAbort();
  #elif defined(GPUOffloadingCPP)
  GPUCallableMethod void gpuAbort();
  #else
  void gpuAbort();
  #endif

  enum class MemoryLocation {
    /**
     * Create data on the heap of the local device. That is, if you run
     * on an accelerator, use the accelerator's heap. If you invoke it
     * on the host, use the host's accelerator.
     */
    Heap,
    /**
     * To be used on host only.
     */
    ManagedSharedAcceleratorDeviceMemory
  };

  namespace internal {
    void* allocateRawData(int size, MemoryLocation location);
  }

  template <class T>
  T* allocateMemory( int size, MemoryLocation location, [[maybe_unused]] int device = -1 ) {
    return reinterpret_cast<T*>( internal::allocateRawData(size * static_cast<int>(sizeof(T)), location ) );
  }
  void freeMemory(void* data, MemoryLocation location);
}
