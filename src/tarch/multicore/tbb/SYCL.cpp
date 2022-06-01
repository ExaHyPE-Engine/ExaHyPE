#include "SYCL.h"


#if defined(GPUOffloadingSYCL)

#include "SYCL.h"



namespace {
  std::map< int, sycl::queue >  syclQueues;
}


sycl::queue& tarch::multicore::getSYCLQueue( int number ) {
  assertion1( syclQueues.count(number)==1, number );
  return syclQueues[number];
}


int tarch::multicore::getNumberOfSYCLQueues() {
  return syclQueues.size();
}


void tarch::multicore::initSYCLQueues() {
  auto platforms     = sycl::platform::get_platforms();
  for (auto &platform : platforms) {
    auto devices = platform.get_devices();

    for (auto &device : devices ) {
      std::cout << "device #" << _numberOfDevices << ": " << device.get_info<sycl::info::device::name>() << std::endl;
      syclQueues.insert( std::pair< int, sycl::queue >( _numberOfDevices, sycl::queue(device) ) );
    }
  }
  syclQueues.insert( std::pair< int, sycl::queue >( -1, sycl::device(sycl::cpu_selector()) ) );
  std::cout << "default device (queue -1) maps onto CPU:" << syclQueues[-1].get_device().get_info<sycl::info::device::name>() << std::endl;
}

#endif
