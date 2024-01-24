#include "tarch/multicore/Tasks.h"

#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/multicore.h"
#include "tarch/services/ServiceRepository.h"
#include "tarch/multicore/sycl/Core.h"

#if defined(SharedSYCL)

namespace {
  tarch::multicore::BooleanSemaphore suspensionPointSemaphore;
} // namespace

/**
 * Native mapping of a task loop onto a SYCL loop
 *
 * @see spawnTask()
 *
 */
void tarch::multicore::native::spawnAndWaitAsTaskLoop(const std::vector<tarch::multicore::Task*>& tasks) {
  assertion(not tasks.empty());

  for (auto& p : tasks) {
    tarch::multicore::getHostSYCLQueue().submit([&](auto& h) {
      while (p->run()) {
      }
      delete p;
    });
  }

  tarch::multicore::getHostSYCLQueue().wait();
}


void tarch::multicore::native::spawnTask(Task* job, int in, int out) { tarch::multicore::native::spawnTask(job); }


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
void tarch::multicore::native::spawnTask(Task* job) {
  sycl::event e = tarch::multicore::getHostSYCLQueue().submit([&](auto& h) {
    h.host_task([=]() {
      while (job->run()) {
      }
      delete job;
    });
  });
}

/**
 * It is not clear what happens if I use tasksOfSameType directly within the task.
 * It is clear that a pure reference cannot be used within a task, as the task uses
 * firstprivate by default. Therefore, it would copy something which ceases to exist.
 * It is not clear what happens with const references, but we had some memory issues
 * with the newer Intel/LLVM versions. Therefore, I manually copy the list and pass
 * this one into fuse(). The firstprivate annotation is not required, as this is the
 * default. But I leave it in here to highlight that three variables are copied,
 * but tasksOfSameType is not copied and not used within the task.
 */
void tarch::multicore::native::processFusedTask(
  Task* myTask, const std::list<tarch::multicore::Task*>& tasksOfSameType, int device
) {
  bool stillExecuteLocally = myTask->fuse(tasksOfSameType, device);
  if (stillExecuteLocally) {
    tarch::multicore::native::spawnTask(myTask);
  } else {
    delete myTask;
  }
}


// void tarch::multicore::native::drainTaskQueues() { tarch::multicore::getHostSYCLQueue().wait(); }


void tarch::multicore::native::spawnTaskWithDependencies(Task* job, int inDependency, int taskNumber) {
  tarch::multicore::native::spawnTask(job);
}


void tarch::multicore::native::spawnTaskWithDependencies(
  Task* job, const std::vector<int>& inDependencies, int taskNumber
) {
  tarch::multicore::native::spawnTask(job);
}


void tarch::multicore::native::waitForTasks(const std::vector<int>& inDependencies) {}

void tarch::multicore::native::waitForTask(int taskNumber) {}
#endif
