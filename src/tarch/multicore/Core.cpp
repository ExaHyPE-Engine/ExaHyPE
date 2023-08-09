#include "tarch/multicore/Core.h"

#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Tasks.h"

#ifdef CompilerHasSysinfo
#include <sched.h>
#endif

std::string tarch::multicore::printUnmaskedThreads() {
  std::stringstream out;
  out << "(";
#ifdef CompilerHasSysinfo
  cpu_set_t mask;
  sched_getaffinity(0, sizeof(cpu_set_t), &mask);

  for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
    if (CPU_ISSET(i, &mask) != 0) {
      out << "x";
    } else {
      out << "o";
    }
  }
#else
  out << " -- not available -- ";
#endif
  out << ")";
  return out.str();
}

int tarch::multicore::getNumberOfUnmaskedThreads() {
#ifdef CompilerHasSysinfo
  cpu_set_t mask;
  sched_getaffinity(0, sizeof(cpu_set_t), &mask);

  int result = 0;
  for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
    if (CPU_ISSET(i, &mask) != 0) {
      result++;
    }
  }

  return result;
#else
  return std::thread::hardware_concurrency();
#endif
}

#ifndef SharedMemoryParallelisation
tarch::multicore::Core::Core() { internal::configureInternalTaskQueues(std::thread::hardware_concurrency()); }

tarch::multicore::Core::~Core() {}

tarch::multicore::Core& tarch::multicore::Core::getInstance() {
  static Core instance;
  return instance;
}

void tarch::multicore::Core::configure(int numberOfThreads) { internal::configureInternalTaskQueues(numberOfThreads); }

void tarch::multicore::Core::shutdown() {}

bool tarch::multicore::Core::isInitialised() const { return true; }

int tarch::multicore::Core::getNumberOfThreads() const { return 1; }

int tarch::multicore::Core::getCoreNumber() const {
#ifdef CompilerHasSysinfo
  return sched_getcpu();
#else
  //  https://stackoverflow.com/questions/33745364/sched-getcpu-equivalent-for-os-x
  return 0;
#endif
}

int tarch::multicore::Core::getThreadNumber() const { return 0; }

void tarch::multicore::Core::yield() {}

#endif
