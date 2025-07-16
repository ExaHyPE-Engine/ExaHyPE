// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/Core.h"

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Task.h"

#ifdef CompilerHasSysinfo
#include <sched.h>
#endif

tarch::logging::Log tarch::multicore::Core::_log(
  "tarch::multicore::Core"
);

std::string tarch::multicore::printUnmaskedThreads() {
  std::stringstream out;
  out << "(";
#ifdef CompilerHasSysinfo
  cpu_set_t mask;
  sched_getaffinity(0, sizeof(cpu_set_t), &mask);

  for (auto i = 0u; i < std::thread::hardware_concurrency(); i++) {
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
  for (auto i = 0u; i < std::thread::hardware_concurrency(); i++) {
    if (CPU_ISSET(i, &mask) != 0) {
      result++;
    }
  }

  return result;
#else
  return std::thread::hardware_concurrency();
#endif
}

tarch::multicore::Core& tarch::multicore::Core::getInstance() {
  static Core instance;
  return instance;
}

#ifndef SharedMemoryParallelisation

bool tarch::multicore::Core::init() {
  _numberOfThreads = 1;
  return isInitialised();
}

void tarch::multicore::Core::shutdown() {}

bool tarch::multicore::Core::isInitialised() const { return true; }

int tarch::multicore::Core::getNumberOfThreads() const { return _numberOfThreads; }

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
