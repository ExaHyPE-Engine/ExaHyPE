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
  static tarch::logging::Log _log( "tarch::multicore" );
  auto platforms       = sycl::platform::get_platforms();
  int  physicalNumberOfDevices = 0;
  int  logicalNumberOfDevices  = 0;
  for (auto &platform : platforms) {
    auto devices = platform.get_devices();
    for (auto &device : devices ) {
      if ( syclQueues.count(physicalNumberOfDevices)>0 ) {
        logInfo( "initSYCLQueues(...)", "skip device #" << physicalNumberOfDevices << " (SYCL queue already set up)" );
      }
      else if (
        syclQueues.count(physicalNumberOfDevices)==0
        and
        (
          deviceList.empty()
          or
          deviceList.count(physicalNumberOfDevices)>0
        )
      ) {
        logInfo( "initSYCLQueues(...)", "init physical device #" << physicalNumberOfDevices << " as logical device " << logicalNumberOfDevices << ". Device numbers from hereon refer to logical devices" );
        syclQueues.insert( std::pair< int, sycl::queue >( logicalNumberOfDevices, sycl::queue(device) ) );
        logicalNumberOfDevices++;
      }
      else {
        #if PeanoDebug>0
          logInfo( "initSYCLQueues(...)", "skip device #" << physicalNumberOfDevices << " (masked out)" );
        #endif
      }
      physicalNumberOfDevices++;
    }
  }

  if ( syclQueues.count(tarch::multicore::Core::HostDevice)==0 ) {
    syclQueues.insert( std::pair< int, sycl::queue >( tarch::multicore::Core::HostDevice, sycl::device(sycl::cpu_selector()) ) );
    #if PeanoDebug>0
    std::cout << "default device (queue " << tarch::multicore::Core::HostDevice << ") maps onto CPU:" << syclQueues[tarch::multicore::Core::HostDevice].get_device().get_info<sycl::info::device::name>() << std::endl;
    #endif
  }
}

#endif
