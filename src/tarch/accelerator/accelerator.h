// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Device.h"

#include "tarch/multicore/multicore.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

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
    #define GPUCallableInlineMethod //inline
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

  std::string toString(MemoryLocation value);

  namespace internal {
    void* allocateRawData(
      std::size_t    size,
      MemoryLocation location,
      int            device
    );
  } // namespace internal

  /**
   * Allocates memory on the specified memory location
   *
   * The device id is an optional argument that is required for
   * memory allocation on an accelerator. Default is -1 (HostDevice).
   * For memory allocation on accelerators, passing a device id >= 0 is necessary.
   *
   *
   * @param count Number of elements to allocate memory for.
   * @param location Location on where to allocate the memory.
   * @param device (Optional) device id for memory allocation on accelerators.
  */
  template <class T = double>
  T* allocateMemory(
    std::size_t count,
    MemoryLocation location,
    int device = accelerator::Device::HostDevice
) {
    return reinterpret_cast<T*>(internal::allocateRawData(
      count * sizeof(T),
      location,
      device
    ));
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
    void* data,
    MemoryLocation location,
    int device = accelerator::Device::HostDevice
  );

  std::size_t padSizeToAlignment(std::size_t size, std::size_t alignment);

  /**
   * Alternative GPU-ready version of memset
   *
   * AMD's standard library hasn't annotated the memset call with target
   * offload for OpenMP. So we have to provide a GPU-ready version ourselves.
   * We do so if and only if the OpenMPManuallyOffloadMemset macro is set. It
   * is typically set in the CompilerSpecificSettings.h. If that macro is not
   * defined, we fall back to std::memset.
   *
   * The signature here is compatible with std::memset. The original interface
   * is
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * char* memset(char* dest, int ch, size_t count);
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * which is just not nice. It should be
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * double* memset(double* dest, double ch, size_t countNumberOfDoubles);
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * but that's not what the C++ standard gives us. So we pick the nicest
   * interface we could think of, but is still compatible.
   *
   * @param count Byte count.
  */
  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif
  double* memset(double* dest, double ch, size_t byteCount);
  #if defined(GPUOffloadingOMP)
  #pragma omp end declare target
  #endif
} // namespace tarch
