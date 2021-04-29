#include "tarch/multicore/Core.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/multicore.h"
#include "tarch/compiler/CompilerSpecificSettings.h"


#ifdef CompilerHasSysinfo
#include <sched.h>
#endif


int tarch::multicore::getNumberOfUnmaskedThreads() {
  #ifdef CompilerHasSysinfo
  cpu_set_t mask;
  sched_getaffinity(0, sizeof(cpu_set_t), &mask);

  int result = 0;
  for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
    if ( CPU_ISSET(i, &mask)!=0 ) {
      result++;
    }
  }

  return result;
  #else
  return std::thread::hardware_concurrency();
  #endif
}


#ifndef SharedMemoryParallelisation
tarch::multicore::Core::Core() {
}


tarch::multicore::Core::~Core() {
}


tarch::multicore::Core& tarch::multicore::Core::getInstance() {
  static Core instance;
  return instance;
}

void tarch::multicore::Core::configure( int numberOfThreads ) {
}


void tarch::multicore::Core::shutdown() {
}


bool tarch::multicore::Core::isInitialised() const {
  return true;
}


int tarch::multicore::Core::getNumberOfThreads() const {
  return 1;
}


int tarch::multicore::Core::getNumberOfGPUs() const {
  return 0;
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
