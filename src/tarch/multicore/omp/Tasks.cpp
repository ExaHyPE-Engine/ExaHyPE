#include "config.h"

#if defined(UseOtter)
#define OTTER_TASK_GRAPH_ENABLE_USER
#endif

#include "tarch/Assertions.h"
#include "tarch/logging/Statistics.h"

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/otter.h"
#include "tarch/services/ServiceRepository.h"
#include "tarch/mpi/Rank.h"

#ifdef UseSmartMPI
#include "smartmpi.h"
#endif

#if defined(SharedOMP)

namespace {
  const int StandardPriority           = 16;

  const int BackgroundConsumerPriority = 1;

  int numberOfSpawnedTasks             = 0;
}


/**
 * Native mapping of a task loop onto a task loop
 *
 * I found that quite a lot of OpenMP implementations realise the taskwait at the
 * end as busy wait. This is totally annoying as it might lead to deadlocks whenever
 * a task has a taskyield internally and/or there are more tasks than threads.
 *
 * ## NVIDIA
 *
 * NVIDIA's nvc++ does not support taskloops. We have contacted their R&D and they
 * confirmed that they only implement a subset of OpenMP, and the taskloop is not
 * on the list of features that they want to support.
 *
 * ## Implementation (bugs)
 *
 * See discussion in the context of the other taskloop: It is important to
 * label tasks as shared even though I think copying should be fine as a copy
 * degenerates to a shallow copy.
 *
 * Please study the remarks in tarch::multicore::spawnAndWait() which discusses
 * flaws of the OpenMP realisation on some systems.
 *
 * Please note that you are not allowed to call this routine for an empty task
 * set. The empty task set means that the taskloop construct becomes nop. The
 * subsequent taskwait hence synchronises on one logical level above within the
 * task hierarchy, which is not what we want.
 */
void tarch::multicore::native::spawnAndWaitAsTaskLoop(
  const std::vector< tarch::multicore::Task* >&  tasks
) {
  assertion( not tasks.empty() );

  // TODO(AT): re-insert annotations here? i.e. step_task -> traversal_task[s]?

  #if defined(OpenMPTaskGroup)
  #pragma omp taskloop nogroup shared(tasks) priority(StandardPriority) untied
  #endif
  for (int i=0; i<static_cast<int>(tasks.size()); i++) {
    #if !defined(OpenMPTaskGroup)
    #pragma omp task
    #endif
    {
      while (tasks[i]->run()) {
        tarch::multicore::Core::getInstance().yield();
      }
      delete tasks[i];
    }
  }

  #ifdef UseSmartMPI
  tarch::timing::Watch taskWaitTime("spawnAndWaitAsTaskLoop", "taskwait", false);
  #endif

  #pragma omp taskwait // wait for all elements from tasks to complete
                       // do not wait for the children of tasks
                       // but do children if you have time (this does not
                       // happen however)

  #ifdef UseSmartMPI
  taskWaitTime.stop();
  smartmpi::reportWaitTime(taskWaitTime.getCPUTime(),tarch::mpi::Rank::getInstance().getRank());
  #endif
}




int tarch::multicore::native::getNumberOfPendingTasks() {
  return numberOfSpawnedTasks;
}


void tarch::multicore::native::spawnTask(Task*  job) {
  #pragma omp atomic
  numberOfSpawnedTasks++;

  #pragma omp task priority(BackgroundConsumerPriority) untied
  {
    while (job->run()) {
      #pragma omp taskyield
    }
    delete job;

    #pragma omp atomic
    numberOfSpawnedTasks--;
  }
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
 *
 * ## NVIDIA
 *
 * With NVIDIA/nvc++, I get the error
 *
 * <pre>
NVC++-S-0155-A possibly throwing copy constructor for a task firstprivate variable is not supported  (tarch/multicore/omp/Tasks.cpp: 289)
NVC++/x86-64 Linux 22.3-0: compilation completed with severe errors
 * </pre>
 *
 * if I try to map fuse onto a task of its own.
 *
 */
void tarch::multicore::native::processFusedTask(Task*  myTask, const std::list< tarch::multicore::Task* >& tasksOfSameType, int device) {
  std::list< tarch::multicore::Task* > copyOfTasksOfSameType = tasksOfSameType;
  #if !defined(__NVCOMPILER)
  #pragma omp task firstprivate(myTask,copyOfTasksOfSameType,device)
  #endif
  {
    bool stillExecuteLocally = myTask->fuse(copyOfTasksOfSameType,device);
    if (stillExecuteLocally) {
      tarch::multicore::native::spawnTask(myTask);
    }
    else {
      delete myTask;
    }
  }
}


/**
 * There is no draining of the CPU queues in OpenMP, but we can invoke a
 * taskyield which issues a scheduling point and hence gives OpenMP the
 * opportunity to disrupt the local execution and instead progress pending
 * tasks.
 */
void tarch::multicore::native::drainTaskQueues() {
  #pragma omp taskyield
}


#endif

