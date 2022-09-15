// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/multicore.h"

#if !defined( _TARCH_MULTICORE_OMP_GPU_MEMORY_MANAGER_H_) && defined(GPUOffloadingOMP)
#define _TARCH_MULTICORE_OMP_GPU_MEMORY_MANAGER_H_


#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/logging/Log.h"


namespace tarch {
  namespace multicore {
    namespace omp {
      class GPUMemoryManager;
    }
  }
}



/**
 * Singleton that manages persistent memory on the GPUs
 *
 * The idea is that this is basically a heap rep for the GPU and that code
 * requiring GPU memory does not allocate this memory but instead grabs it
 * from here. As it is not freed afterwards, we avoid overheads that arise
 * from frequent re-allocation.
 *
 * It seems that memory allocation on GPUs is very slow with most OMP runtimes.
 * Therefore, it is reasonable to manage the GPU memory in the user space.
 */
class tarch::multicore::omp::GPUMemoryManager {
  private:
    static tarch::logging::Log          _log;

    tarch::multicore::BooleanSemaphore  _semaphore;

    GPUMemoryManager() = default;

    struct MemoryBlock {
      /**
       * Pointer to some device data. The pointer is not to be used on the
       * host.
       */
      void*    data;

      /**
       * Size in bytes.
       */
      int      size;

      int      device;
    };
  public:
    GPUMemoryManager( const GPUMemoryManager& copy ) = delete;
    virtual ~GPUMemoryManager() = default;

    static GPUMemoryManager& getInstance();

    /**
     * @param size Count of block, i.e. number of doubles
     *
     * @return This pointer is a device pointer, i.e. do not use it on the
     *   host.
     */
    template< typename T >
    T* allocate( int size ) {
      return reinterpret_cast<T*>( allocateRawData(size * static_cast<int>(sizeof(T))) );
    }

    void*   allocateRawData( int size );

    void free(void* devicePointer);
};


#endif
