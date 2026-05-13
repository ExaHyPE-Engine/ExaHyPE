#include "tarch/accelerator/GPUMemoryManager.h"

#if defined(GPUOffloadingSYCL)

#include "tarch/Assertions.h"
#include "tarch/multicore/Lock.h"
#include "tarch/accelerator/accelerator.h"


tarch::logging::Log  tarch::accelerator::GPUMemoryManager::_log( "tarch::accelerator::GPUMemoryManager" );
int tarch::accelerator::GPUMemoryManager::_allocatedMemory;

tarch::accelerator::GPUMemoryManager& tarch::accelerator::GPUMemoryManager::getInstance() {
  static tarch::accelerator::GPUMemoryManager singleton;
  return singleton;
}

void* tarch::accelerator::GPUMemoryManager::allocateRawData( int size, ::sycl::queue& device ) {
  tarch::multicore::Lock lock(_semaphore);

  // Is device initialised?
  if ( _deviceMemory.count(&device)==0 ) {
    _deviceMemory.insert( std::pair<::sycl::queue*, DeviceMemory>(&device,DeviceMemory()) );
  }

  // Is a free page left? If not, insert one
  if ( _deviceMemory.at(&device).freePages[size].empty() ) {
    _deviceMemory.at(&device).freePages[size].push_back(
      ::sycl::malloc_device<void*>(size, device)
    );
  }

  // Grab fitting page
  void* result = _deviceMemory.at(&device).freePages.at(size).back();
  _deviceMemory.at(&device).freePages.at(size).pop_back();

  // Memorize page meta data, i.e. size and device
  assertion( _deviceMemory.at(&device).usedPages.count(result)==0 );
  _deviceMemory.at(&device).usedPages.insert( std::pair<void*,int>(result,size) );

  // Return result
  return result;
}

void tarch::accelerator::GPUMemoryManager::free(void* devicePointer, ::sycl::queue& device) {
  tarch::multicore::Lock lock(_semaphore);

  assertion( _deviceMemory.count(&device)==1 );
  assertion( _deviceMemory.at(&device).usedPages.count(devicePointer)==1 );

  int size = _deviceMemory.at(&device).usedPages.at(devicePointer);
  _deviceMemory.at(&device).usedPages.erase(devicePointer);

  assertion( _deviceMemory.at(&device).freePages.count(size)==1 );
  _deviceMemory.at(&device).freePages.at(size).push_back( devicePointer );
}

tarch::accelerator::GPUMemoryManager::~GPUMemoryManager() {
  clear();
}

void tarch::accelerator::GPUMemoryManager::clear() {
  tarch::multicore::Lock lock(_semaphore);

  for (auto& deviceMemory: _deviceMemory) {
    ::sycl::queue* device = deviceMemory.first;
    if ( not deviceMemory.second.usedPages.empty() ) {
      logError( "clear()", "used pages on device " << device << " are not empty. " << deviceMemory.second.usedPages.size() << " memory blocks still in use" );
    }

    for (auto& p: deviceMemory.second.freePages)
    for (auto& pp: p.second) {
      ::sycl::free(pp, *device);
    }
  }

  _deviceMemory.clear();
}

#endif
