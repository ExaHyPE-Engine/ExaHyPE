#include "tarch/multicore/Core.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/multicore.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#include "tarch/multicore/sycl/SYCL.h"


#ifdef CompilerHasSysinfo
#include <sched.h>
#endif


#if defined(SharedOMP)

#include <omp.h>

#include "config.h"

#if defined(UseNVIDIA)
#include <cuda_runtime_api.h>
#include <cuda.h>
#endif


tarch::logging::Log  tarch::multicore::Core::_log( "tarch::multicore::Core" );

#if defined(GPUOffloadingOMP)
bool checkOMPOffloading() {
  bool canOffload = false;
  #pragma omp target map(tofrom : canOffload)
  {
    if (!omp_is_initial_device()) {
      canOffload = true;
    }
  }
  return canOffload;
}
#endif

tarch::multicore::Core::Core():
  _numberOfThreads( omp_get_max_threads() ),
  _numberOfDevices( 0 ) {
  internal::configureInternalTaskQueues(_numberOfThreads);

  #if defined(GPUOffloadingOMP)
  _numberOfDevices = omp_get_num_devices();
   /*if (!checkOMPOffloading()) {
    std::cerr << "tarch was built with OpenMP GPU offloading support (--with-gpu=omp). "
      "Either your chosen compiler does not support GPU offloading or your hardware does not have the required compute capability." << std::endl;
    exit(-1);
  }*/
  #endif

  #if defined(GPUOffloadingSYCL)
  if (not tarch::multicore::SYCLQueuesAreCreated() ) {
    initSYCLQueues( std::set<int>() );
  }
  _numberOfDevices = getNumberOfSYCLQueues();
  #endif
}


tarch::multicore::Core::~Core() {
}


tarch::multicore::Core& tarch::multicore::Core::getInstance() {
  static Core instance;
  return instance;
}


void tarch::multicore::Core::configure( int numberOfThreads, const std::set<int>& devicesToUse ) {
  _devicesToUse = devicesToUse;

  if ( omp_get_num_procs() != omp_get_max_threads() ) {
    logWarning( "configure(int)", "omp_get_num_procs reports " << omp_get_num_procs() << " while omp_get_max_threads reports " << omp_get_max_threads() << ". Take smaller value to avoid overbooking" );
  }
  int maxThreads = std::min(omp_get_num_procs(), omp_get_max_threads());

  if (numberOfThreads!=UseDefaultNumberOfThreads) {
    if ( maxThreads!=numberOfThreads ) {
      logWarning( "configure(int)", "number of threads configured (" << numberOfThreads << ") does not match system thread level of " << maxThreads << ". OpenMP may ignore manual thread count reset");
    }

    omp_set_num_threads(numberOfThreads);
    _numberOfThreads = numberOfThreads;
    logInfo( "configure(int)", "manually reset number of threads used to " << numberOfThreads );
  }
  else {
    omp_set_num_threads(maxThreads);
    _numberOfThreads = omp_get_max_threads();
  }

  if (_numberOfThreads>getNumberOfUnmaskedThreads()) {
    logWarning( "configure(int)", "number of configured threads (" << _numberOfThreads << ") is bigger than available unmasked threads (" << getNumberOfUnmaskedThreads() << ")" );
    logWarning( "configure(int)", "unmasked threads: " << printUnmaskedThreads() );
  }

  internal::configureInternalTaskQueues(_numberOfThreads);
}


void tarch::multicore::Core::shutdown() {
}


bool tarch::multicore::Core::isInitialised() const {
  return true;
}


int tarch::multicore::Core::getNumberOfThreads() const {
  return _numberOfThreads;
}


int tarch::multicore::Core::getNumberOfGPUs() const {
  return _numberOfDevices;
}


int tarch::multicore::Core::getThreadNumber() const {
  return omp_get_thread_num();
}


int tarch::multicore::Core::getCoreNumber() const {
  #ifdef CompilerHasSysinfo
  return sched_getcpu();
  #else
  //  https://stackoverflow.com/questions/33745364/sched-getcpu-equivalent-for-os-x
  return 0;
  #endif
}


#endif
