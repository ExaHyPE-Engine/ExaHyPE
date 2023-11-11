// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/multicore/multicore.h"

#if !defined(GPUOffloadingOMP) and !defined(GPUOffloadingSYCL) and !defined(GPUOffloadingCPP) and !defined(GPUOffloadingCUDA)
#define GPUOffloadingOff
#endif

/**
 * Have to include this header, as I need access to the SYCL_EXTERNAL keyword.
 */
#include "tarch/accelerator/sycl/Device.h"

namespace tarch {
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
    #define GPUCallableInlineMethod inline
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
  #elif defined(GPUOffloadingSYCL) or defined(SharedSYCL)
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
     *
     * The alignment of this routine can be controlled via the compile
     * time constant AlignmentOnHeap. It is typically set (to a default)
     * within the corresponding compiler-specific header-file in
     * tarch/compiler.
     */
    Heap,
    /**
     * To be used on host only.
     */
    ManagedSharedAcceleratorDeviceMemory
  };

  std::string toString( MemoryLocation value );


  namespace internal {
    void* allocateRawData(int size, MemoryLocation location);
  }

  template <class T=double>
  T* allocateMemory( int size, MemoryLocation location, [[maybe_unused]] int device = -1 ) {
     return reinterpret_cast<T*>( internal::allocateRawData(size * static_cast<int>(sizeof(T)), location ) );
  }
  void freeMemory(void* data, MemoryLocation location);
}
