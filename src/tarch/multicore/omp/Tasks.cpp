#include "tarch/multicore/Tasks.h"

#include "config.h"
#include "tarch/Assertions.h"
#include "tarch/logging/Statistics.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/otter.h"
#include "tarch/services/ServiceRepository.h"


#if defined(SharedOMP)

namespace {
  int numberOfSpawnedTasks = 0;

  /**
   * Dependency tracking in OpenMP is organised via addresses. These addresses
   * are sole helpers, i.e. the underlying memory region doesn't have to be
   * owned by the application code. So we basically use an arbitrary address
   * and then use this as baseline for any dependency, taking into account that
   * dependencies are all enumerated continuously.
   *
   * The pointer has to be an integer pointer (or any built-in type), as we
   * want to do some pointer arithmetics with it. This would not work with a
   * void pointer.
   */
  int* dependencyTrackingBaseAddress = &numberOfSpawnedTasks;
} // namespace


// #define OpenMPHighThroughputWaiting

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
 *
 *
 *
 * ## SmartMPI
 *
 * I used to insert the fragment
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * #ifdef UseSmartMPI
 * tarch::timing::Watch taskWaitTime("spawnAndWaitAsTaskLoop", "taskwait", false);
 * #endif
 *
 * #pragma omp taskwait
 *
 * #ifdef UseSmartMPI
 * taskWaitTime.stop();
 * smartmpi::reportWaitTime(taskWaitTime.getCPUTime(), tarch::mpi::Rank::getInstance().getRank());
 * #endif
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * but as we don't use SmartMPI anymore atm, I'm not sure anymore if this is a
 * good solution, so I removed it for the time being to make the code more
 * readable.
 *
 *
 * ## High throughput waiting
 *
 * I have a second variant of the code which I call "high-throughput waiting".
 * This version runs risk to
 */
#if defined(OpenMPHighThroughputWaiting)
void tarch::multicore::native::spawnAndWaitAsTaskLoop(const std::vector<tarch::multicore::Task*>& tasks) {
  assertion(not tasks.empty());

  int numberOfLoopTasks = tasks.size(); // bookkeeping of how many loop tasks
                                        // we have in the system

#if defined(OpenMPTaskGroup)
#pragma omp taskloop nogroup shared(tasks) grainsize(1) priority(tarch::multicore::Task::DefaultPriority) shared(numberOfLoopTasks) untied
#endif
  for (int i = 0; i < static_cast<int>(tasks.size()); i++) {
#if !defined(OpenMPTaskGroup)
#pragma omp task priority(tarch::multicore::Task::DefaultPriority) shared(numberOfLoopTasks)
#endif
    {
      while (tasks[i]->run()) {
        tarch::multicore::Core::getInstance().yield();
      }
      delete tasks[i];
    }

    int currentNumberOfLoopTasks;                   // reduce the bookkeeping
#pragma omp atomic capture                          // counter and create my
    currentNumberOfLoopTasks = --numberOfLoopTasks; // own loop termination
                                                    // criterion

    while (currentNumberOfLoopTasks > 0) {
      if (processPendingTasks(1, false)) { // I use FILO to increase cache hit probability
#pragma omp atomic read
        currentNumberOfLoopTasks = numberOfLoopTasks;
      } else {
        currentNumberOfLoopTasks = 0; // This is not nice. It means that a
                                      // fast task might finish while ready
                                      // tasks literally become a available a
                                      // second later. However, without his
                                      // statement, another loop task might
                                      // never be able to run, as the busy
                                      // polling keeps the thread idle. We
                                      // starve.
      }
    }
  }

#pragma omp taskwait // wait for all elements from tasks to complete
                     // do not wait for the children of tasks
                     // but do children if you have time (this does not
                     // happen however)

  assertionEquals(numberOfLoopTasks, 0);
}
#else
void tarch::multicore::native::spawnAndWaitAsTaskLoop(const std::vector<tarch::multicore::Task*>& tasks) {
  assertion(not tasks.empty());

#if defined(OpenMPTaskGroup)
#pragma omp taskloop nogroup shared(tasks) grainsize(1) priority(tarch::multicore::Task::DefaultPriority) untied
#endif
  for (int i = 0; i < static_cast<int>(tasks.size()); i++) {
#if !defined(OpenMPTaskGroup)
#pragma omp task priority(tarch::multicore::Task::DefaultPriority)
#endif
    {
      while (tasks[i]->run()) {
        tarch::multicore::Core::getInstance().yield();
      }
      delete tasks[i];
    }
  }

#pragma omp taskwait // wait for all elements from tasks to complete
                     // do not wait for the children of tasks
                     // but do children if you have time (this does not
                     // happen however)
}
#endif


int tarch::multicore::native::getNumberOfPendingTasks() { return numberOfSpawnedTasks; }



void tarch::multicore::native::spawnTask(Task* job, const std::set<TaskNumber>& inDependencies, const TaskNumber& taskNumber) {
  int* outDependencyPointer = dependencyTrackingBaseAddress + taskNumber;

  #if !defined(OpenMPDependenciesInTaskWait)
  if (taskNumber != NoDependencies) {
    #pragma omp task untied depend(in: outDependencyPointer[:1])
    {
      // empty task
    }
    #pragma omp taskwait 
  }
  #else
  if (taskNumber != NoDependencies) {
    #pragma omp taskwait depend(in : *outDependencyPointer)
  }
  #endif

  #pragma omp atomic
  numberOfSpawnedTasks++;

  if (taskNumber == NoDependencies and inDependencies.empty()) {
    #pragma omp task priority(job->getPriority()) untied
    {
      while (job->run()) {
        #pragma omp taskyield
      }
      delete job;

      #pragma omp atomic
      numberOfSpawnedTasks--;
    }
  } else if (taskNumber != NoDependencies and inDependencies.empty()) {
    // A nicer syntax would be 
    //
    // #pragma omp task priority(job->getPriority()) untied depend(out : *outDependencyPointer)
    //
    // but some toolchains don't support that synax.
    #pragma omp task priority(job->getPriority()) untied depend(out : outDependencyPointer[:1])
    {
      while (job->run()) {
        #pragma omp taskyield
      }
      delete job;

      #pragma omp atomic
      numberOfSpawnedTasks--;
    }
  } else if (taskNumber == NoDependencies and not inDependencies.empty()) {
    int helperDependency;
    for (auto& inDependencyTaskNumber: inDependencies) {
       int* inDependencyPointer = dependencyTrackingBaseAddress + inDependencyTaskNumber;
       #pragma omp task priority(job->getPriority()) untied depend(in: inDependencyPointer[:1]) depend(out:helperDependency)
       {  
         // empty task
       }
    }
    #pragma omp task priority(job->getPriority()) untied depend(in: helperDependency)
    {
      while (job->run()) {
        #pragma omp taskyield
      }
      delete job;

      #pragma omp atomic
      numberOfSpawnedTasks--;
    }
  } else if (taskNumber != NoDependencies and not inDependencies.empty()) {
    //
    // See above
    //
    int helperDependency;
    for (auto& inDependencyTaskNumber: inDependencies) {
      int* inDependencyPointer = dependencyTrackingBaseAddress + inDependencyTaskNumber;
      #pragma omp task priority(job->getPriority()) untied depend(in: inDependencyPointer[:1]) depend(out:helperDependency)
      {  
        // empty task
      }
    }
    #pragma omp task priority(job->getPriority()) untied depend(in: helperDependency) depend(out: outDependencyPointer[:1])
    {
      while (job->run()) {
        #pragma omp taskyield
      }
      delete job;

      #pragma omp atomic
      numberOfSpawnedTasks--;
    }
  } else {
    assertionMsg(false, "should never be called");
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
NVC++-S-0155-A possibly throwing copy constructor for a task firstprivate variable is not supported
(tarch/multicore/omp/Tasks.cpp: 289) NVC++/x86-64 Linux 22.3-0: compilation completed with severe errors
 * </pre>
 *
 * if I try to map fuse onto a task of its own.
 *
 */
void tarch::multicore::native::processFusedTask(Task* myTask, const std::list<tarch::multicore::Task*>& tasksOfSameType, int device) {
  std::list<tarch::multicore::Task*> copyOfTasksOfSameType = tasksOfSameType;
  // #if !defined(__NVCOMPILER)
#pragma omp task firstprivate(myTask, copyOfTasksOfSameType, device)
  // #endif
  {
    bool stillExecuteLocally = myTask->fuse(copyOfTasksOfSameType, device);
    if (stillExecuteLocally) {
      tarch::multicore::native::spawnTask(myTask, std::set<TaskNumber>(), NoDependencies);
    } else {
      delete myTask;
    }
  }
}


void tarch::multicore::native::waitForTasks(const std::set<TaskNumber>& inDependencies) {
  if (not inDependencies.empty()) {
    //
    // very primiate workaround, where we construct empty funneling-in tasks
    //
    int helperDependency;
    for (auto& inDependencyTaskNumber: inDependencies) {
      int* inDependencyPointer = dependencyTrackingBaseAddress + inDependencyTaskNumber;
      #pragma omp task untied depend(in: inDependencyPointer[:1]) depend(out:helperDependency)
      {  
        // empty task
      }
    }
    #pragma omp task untied depend(in: helperDependency)
    {}
    #pragma omp taskwait
  }
}


void tarch::multicore::native::waitForAllTasks() {
  #pragma omp taskwait
}

#endif
