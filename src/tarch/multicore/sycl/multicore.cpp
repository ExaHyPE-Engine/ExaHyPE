// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/multicore.h"

#if defined(SharedSYCL)

#include "Core.h"

#include "tarch/Assertions.h"
#include "tarch/multicore/Task.h"

#pragma push_macro("Dimensions")
#undef Dimensions
#include <sycl/sycl.hpp>
#pragma pop_macro("Dimensions")

namespace {
  static tarch::logging::Log _log("tarch::multicore::sycl");
} // namespace

void tarch::multicore::native::spawnTasks(const std::vector<Task*>& tasks) {
  assertion(not tasks.empty());
  logDebug("spawnTasks()", "Spawn " << tasks.size() << " tasks as taskloop");

  for (auto& p : tasks) {
    static_cast<sycl::queue*>(getHostSYCLQueue())->submit([&](auto& h) {
      while (p->run()) {
      }
      delete p;
    });
  }

  static_cast<sycl::queue*>(getHostSYCLQueue())->wait();
}

void tarch::multicore::native::spawnTask(Task* task, const void* const, const void* const) {
  sycl::event e = static_cast<sycl::queue*>(getHostSYCLQueue())->submit([&](auto& h) {
    h.host_task([=]() {
      while (task->run()) {
      }
      delete task;
    });
  });
  e.wait();
}

void tarch::multicore::native::spawnTask(Task* task, const void* const) {
  sycl::event e = static_cast<sycl::queue*>(getHostSYCLQueue())->submit([&](auto& h) {
    h.host_task([=]() {
      while (task->run()) {
      }
      delete task;
    });
  });
  e.wait();
}

void tarch::multicore::native::spawnTask(Task* task) {
  sycl::event e = static_cast<sycl::queue*>(getHostSYCLQueue())->submit([&](auto& h) {
    h.host_task([=]() {
      while (task->run()) {
      }
      delete task;
    });
  });
  e.wait();
}

void tarch::multicore::native::waitForTask(const void* const) {}

void tarch::multicore::native::waitForAllTasks() {
  static_cast<sycl::queue*>(getHostSYCLQueue())->wait();
}

#endif // defined(SharedSYCL)
