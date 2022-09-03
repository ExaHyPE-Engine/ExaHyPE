#include "SYCL.h"


#if defined(GPUOffloadingSYCL) or defined(SharedSYCL)

#include <map>


#include "tarch/Assertions.h"
#include "tarch/multicore/Core.h"


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


void tarch::multicore::listSYCLQueues() {
  auto platforms       = sycl::platform::get_platforms();
  int  numberOfDevices = 0;
  std::cout << "SYCL system overview:" << std::endl;
  for (auto &platform : platforms) {
    auto devices = platform.get_devices();
    for (auto &device : devices ) {
      std::cout << "device #" << numberOfDevices << ":  platform " << platform.get_info<sycl::info::platform::name>() << " / " << device.get_info<sycl::info::device::name>() << std::endl;
      numberOfDevices++;
    }
  }
}


void tarch::multicore::initSYCLQueues(const std::set<int>& deviceList) {
  auto platforms       = sycl::platform::get_platforms();
  int  numberOfDevices = 0;
  for (auto &platform : platforms) {
    auto devices = platform.get_devices();
    for (auto &device : devices ) {
      std::cout << "I try to initialise " << numberOfDevices << std::endl;
      if ( syclQueues.count(numberOfDevices)>0 ) {
        std::cout << "skip device #" << numberOfDevices << " (SYCL queue already set up)" << std::endl;
      }
      else if (
        syclQueues.count(numberOfDevices)==0
        and
        (
          deviceList.empty()
          or
          deviceList.count(numberOfDevices)>0
        )
      ) {
        std::cout << "init device #" << numberOfDevices << std::endl;
        syclQueues.insert( std::pair< int, sycl::queue >( numberOfDevices, sycl::queue(device) ) );
      }
      else {
        std::cout << "skip device #" << numberOfDevices << " (masked out)" << std::endl;
      }
      numberOfDevices++;
    }
  }

  if ( syclQueues.count(HostDefaultQueue)==0 ) {
    syclQueues.insert( std::pair< int, sycl::queue >( HostDefaultQueue, sycl::device(sycl::cpu_selector()) ) );
    std::cout << "default device (queue " << HostDefaultQueue << ") maps onto CPU:" << syclQueues[HostDefaultQueue].get_device().get_info<sycl::info::device::name>() << std::endl;
  }
}

#endif
