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

/**
 * Native mapping of a task loop onto a SYCL loop
 *
 * @see spawnTask()
 *
 */
void tarch::multicore::native::spawnAndWaitAsTaskLoop(const std::vector<tarch::multicore::Task*>& tasks) {
  assertion(not tasks.empty());

  for (auto& p : tasks) {
    static_cast<sycl::queue*>(getHostSYCLQueue())->submit([&](auto& h) {
      while (p->run()) {
      }
      delete p;
    });
  }

  static_cast<sycl::queue*>(getHostSYCLQueue())->wait();
}


/**
 * Read spawnAndWaitAsTaskLoop() first.
 *
 * ## Implementation details
 *
 * The tasks here are always spawned into the host queue. Consequently, they
 * will run on the CPU and thus should be able to invoke virtual functions and
 * access global variables. Our experiments with Intel DPC++ however suggest
 * that this is not the case and that SYCL/DPC++ refuses to translate this
 * code: you have to spawn explicitly as host task.
 *
 * @author Andrew Mallinson
 * @author Tobias Weinzierl
 *
 * @see spawnAndWaitAsTaskLoop()
 */
void tarch::multicore::native::spawnTask(
  Task*                       job,
  const std::set<TaskNumber>& /*inDependencies*/,
  const TaskNumber&           /*taskNumber*/
) {
  sycl::event e = static_cast<sycl::queue*>(getHostSYCLQueue())->submit([&](auto& h) {
    h.host_task([=]() {
      while (job->run()) {
      }
      delete job;
    });
  });
}


void tarch::multicore::native::spawnTask(
  Task*                       job,
  const TaskNumber&           /*taskNumber*/
) {
  sycl::event e = static_cast<sycl::queue*>(getHostSYCLQueue())->submit([&](auto& h) {
    h.host_task([=]() {
      while (job->run()) {
      }
      delete job;
    });
  });
}


void tarch::multicore::native::spawnTask(
  Task*                        job,
  const double*                outData
) {
  assert(outData);

  sycl::event e = static_cast<sycl::queue*>(getHostSYCLQueue())->submit([&](auto& h) {
    h.host_task([=]() {
      while (job->run()) {
      }
      delete job;
    });
  });
}


void tarch::multicore::native::spawnTask(Task* job) {
  sycl::event e = static_cast<sycl::queue*>(getHostSYCLQueue())->submit([&](auto& h) {
    h.host_task([=]() {
      while (job->run()) {
      }
      delete job;
    });
  });
}


void tarch::multicore::native::waitForTasks(const std::set<int>& /*inDependencies*/) { static_cast<sycl::queue*>(getHostSYCLQueue())->wait(); }


void tarch::multicore::native::waitForTask(const TaskNumber& /*taskNumber*/) { static_cast<sycl::queue*>(getHostSYCLQueue())->wait(); }


void tarch::multicore::native::waitForTask(const double* /*inData*/) { static_cast<sycl::queue*>(getHostSYCLQueue())->wait(); }


void tarch::multicore::native::waitForAllTasks() { static_cast<sycl::queue*>(getHostSYCLQueue())->wait(); }

#endif // defined(SharedSYCL)
