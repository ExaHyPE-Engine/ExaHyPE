#include "SYCL.h"


#if defined(GPUOffloadingSYCL) or defined(SharedSYCL)

#include <map>


#include "tarch/Assertions.h"


namespace {
  std::map< int, sycl::queue >  syclQueues;
}


sycl::queue& tarch::multicore::getSYCLQueue( int number ) {
  assertion1( syclQueues.count(number)==1, number );
  return syclQueues[number];
}


int tarch::multicore::getNumberOfSYCLQueues() {
  return syclQueues.size()-1;
}


void tarch::multicore::initSYCLQueues() {
  auto platforms       = sycl::platform::get_platforms();
  int  numberOfDevices = 0;
  for (auto &platform : platforms) {
    auto devices = platform.get_devices();

    for (auto &device : devices ) {
      std::cout << "device #" << numberOfDevices << ": " << device.get_info<sycl::info::device::name>() << std::endl;
      syclQueues.insert( std::pair< int, sycl::queue >( numberOfDevices, sycl::queue(device) ) );
      numberOfDevices++;
    }
  }
  syclQueues.insert( std::pair< int, sycl::queue >( HostDefaultQueue, sycl::device(sycl::cpu_selector()) ) );
  std::cout << "default device (queue " << HostDefaultQueue << ") maps onto CPU:" << syclQueues[HostDefaultQueue].get_device().get_info<sycl::info::device::name>() << std::endl;
}

#endif
