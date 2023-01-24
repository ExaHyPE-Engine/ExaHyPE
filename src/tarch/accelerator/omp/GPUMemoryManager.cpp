#include "GPUMemoryManager.h"

#if defined(GPUOffloadingOMP)

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/accelerator/accelerator.h"

tarch::logging::Log  tarch::accelerator::omp::GPUMemoryManager::_log( "tarch::accelerator::omp::GPUMemoryManager" );

tarch::accelerator::omp::GPUMemoryManager& tarch::accelerator::omp::GPUMemoryManager::getInstance() {
  static tarch::accelerator::omp::GPUMemoryManager singleton;
  return singleton;
}

void* tarch::accelerator::omp::GPUMemoryManager::allocateRawData( int size, int device, int growth ) {
  tarch::multicore::Lock lock(_semaphore);

  assertion1(growth>0, growth);

  // Is device initialised?
  if ( _deviceMemory.count(device)==0 ) {
    _deviceMemory.insert( std::pair<int, DeviceMemory>(device,DeviceMemory()) );
  }

  // Is a free page left? If not, insert one
  if ( _deviceMemory.at(device).freePages[size].empty() ) {
    for (int i=0; i<growth; i++) {
      _deviceMemory.at(device).freePages[size].push_back(
#ifdef __NVCOMPILER_CUDA__ // Use CUDA managed memory
        // TODO: "size / sizeof(double) should not be necessary after some rework in the allocateMemory functionality."
        allocateMemory((size / sizeof(double)), MemoryLocation::ManagedSharedAcceleratorDeviceMemory)
#else
        omp_target_alloc(size, device)
#endif
      );
    }
  }

  // Grab fitting page
  void* result = _deviceMemory.at(device).freePages.at(size).back();
  _deviceMemory.at(device).freePages.at(size).pop_back();

  // Memorize page meta data, i.e. size and device
  assertion( _deviceMemory.at(device).usedPages.count(result)==0 );
  _deviceMemory.at(device).usedPages.insert( std::pair<void*,int>(result,size) );

  // Return result
  return result;
}

void tarch::accelerator::omp::GPUMemoryManager::free(void* devicePointer, int device) {
  tarch::multicore::Lock lock(_semaphore);

  assertion( _deviceMemory.count(device)==1 );
  assertion( _deviceMemory.at(device).usedPages.count(devicePointer)==1 );

  int size = _deviceMemory.at(device).usedPages.at(devicePointer);
  _deviceMemory.at(device).usedPages.erase(devicePointer);

  assertion( _deviceMemory.at(device).freePages.count(size)==1 );
  _deviceMemory.at(device).freePages.at(size).push_back( devicePointer );
}

tarch::accelerator::omp::GPUMemoryManager::~GPUMemoryManager() {
  clear();
}

void tarch::accelerator::omp::GPUMemoryManager::clear() {
  tarch::multicore::Lock lock(_semaphore);

  for (auto& deviceMemory: _deviceMemory) {
    int device = deviceMemory.first;
    logDebug( "clear()", "free data of device " << device );

    if ( not deviceMemory.second.usedPages.empty() ) {
      logError( "clear()", "used pages on device " << device << " are not empty. " << deviceMemory.second.usedPages.size() << " memory blocks still in use" );
    }

    for (auto& p: deviceMemory.second.freePages)
    for (auto& pp: p.second) {
#ifdef __NVCOMPILER_CUDA__ // Use CUDA managed memory
      freeMemory(static_cast<double *>(pp), MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
#else
      omp_target_free(pp, device);
#endif
    }
  }

  _deviceMemory.clear();
}
#endif

