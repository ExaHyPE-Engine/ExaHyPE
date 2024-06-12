// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
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
  static tarch::logging::Log _log( "tarch::multicore::native(omp)" );

  int numberOfSpawnedTasks = 0;

  using DependencyTrackingDataType = unsigned char;

  /**
   * Memory region to administer dependencies
   *
   *
   * Dependency tracking in OpenMP is organised via addresses. These addresses
   * are sole helpers, i.e. the underlying memory region doesn't hold any data
   * with semantics. However, it has to be owned by the application code. To
   * ensure this is the case, we allocate some chunk only for the dependency
   * tracking and hold its baseline address in this variable.
   */
  DependencyTrackingDataType* dependencyTrackingBaseAddress{nullptr};
  int sizeOfMemoryRegionUsedForDependencyTracking{0};
  int maxSizeOfMemoryRegionUsedForDependencyTracking{0};

  /**
   *
   */
  bool canServeDependencyEntry(int taskNumber) {
    bool result = false;
    #pragma omp critical (dependency_checks)
    {
      maxSizeOfMemoryRegionUsedForDependencyTracking = std::max( maxSizeOfMemoryRegionUsedForDependencyTracking, taskNumber+1 );
      if (
        numberOfSpawnedTasks==0
        and
        maxSizeOfMemoryRegionUsedForDependencyTracking > sizeOfMemoryRegionUsedForDependencyTracking
        and
        dependencyTrackingBaseAddress!=nullptr
      ) {
        delete[] dependencyTrackingBaseAddress;
        dependencyTrackingBaseAddress = nullptr;
        logDebug( "canServeDependencyEntry(int)", "free preallocated memory for task dependency management" );
      }

      if ( dependencyTrackingBaseAddress==nullptr ) {
        maxSizeOfMemoryRegionUsedForDependencyTracking += 2;
        sizeOfMemoryRegionUsedForDependencyTracking     = maxSizeOfMemoryRegionUsedForDependencyTracking;
        dependencyTrackingBaseAddress = new DependencyTrackingDataType[sizeOfMemoryRegionUsedForDependencyTracking];
        logDebug( "canServeDependencyEntry(int)", "allocated memory for task dependency management for up to " << sizeOfMemoryRegionUsedForDependencyTracking << " tasks" );
      }

      result = taskNumber < sizeOfMemoryRegionUsedForDependencyTracking;
    }
    return result;
  }
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
 * TODO: Finish documentation here
 */
#if defined(OpenMPHighThroughputWaiting)
void tarch::multicore::native::spawnAndWaitAsTaskLoop(const std::vector<tarch::multicore::Task*>& tasks) {
  assertion(not tasks.empty());

  int numberOfLoopTasks = tasks.size(); // Bookkeeping of how many loop tasks
                                        // we have in the system.

  #if defined(OpenMPTaskGroup)
  #pragma omp taskloop nogroup shared(tasks) grainsize(1) priority(tarch::multicore::Task::DefaultPriority) shared(numberOfLoopTasks) untied
  #endif
  for (int i = 0; i < static_cast<int>(tasks.size()); i++) {
    #if !defined(OpenMPTaskGroup)
    #pragma omp task priority(tarch::multicore::Task::DefaultPriority) shared(numberOfLoopTasks) untied
    #endif
    {
      while (tasks[i]->run()) {
        tarch::multicore::Core::getInstance().yield();
      }
      delete tasks[i];

      int currentNumberOfLoopTasks;                   // Reduce the bookkeeping
      #pragma omp atomic capture                      // counter and create my
      currentNumberOfLoopTasks = --numberOfLoopTasks; // own loop termination
                                                      // criterion.

      while (currentNumberOfLoopTasks > 0) {
        if (processPendingTasks(1, false)) { // I use FILO to increase cache hit probability
          #pragma omp atomic read
          currentNumberOfLoopTasks = numberOfLoopTasks;
        }
        else {
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
    } // end of artificial bracket which is only important if
      // taskloop is not properly supported, i.e. if !defined(OpenMPTaskGroup)
  }

  #pragma omp taskwait // Wait for all elements from tasks to complete
                       // do not wait for the children of tasks
                       // but do children if you have time (this does not
                       // happen however).

  assertionEquals(numberOfLoopTasks, 0);
}
#else
void tarch::multicore::native::spawnAndWaitAsTaskLoop(const std::vector<tarch::multicore::Task*>& tasks) {
  assertion(not tasks.empty());

#if defined(UseOtter)
  auto otter_tasks = std::vector<otter_task_context*>();
  for (int i = 0; i < static_cast<int>(tasks.size()); i++) {
    otter_tasks.push_back(otterTaskInitialise(OTTER_NULL_TASK, -1, otter_no_add_to_pool, true, OTTER_SOURCE_LOCATION(), __func__));
  }
#endif

#if defined(OpenMPTaskGroup)
#pragma omp taskloop nogroup shared(tasks) grainsize(1) priority(tarch::multicore::Task::DefaultPriority) untied
#endif
  for (int i = 0; i < static_cast<int>(tasks.size()); i++) {
#if !defined(OpenMPTaskGroup)
#pragma omp task priority(tarch::multicore::Task::DefaultPriority)
#endif
    {
      OTTER_TASK_START(otter_tasks[i]);
      while (tasks[i]->run()) {
        tarch::multicore::Core::getInstance().yield();
      }
      delete tasks[i];
      OTTER_TASK_END();
    }
  }

  OTTER_TASK_WAIT_START(children);
#pragma omp taskwait // Wait for all elements from tasks to complete
                     // do not wait for the children of tasks
                     // but do children if you have time (this does not
                     // happen however).
  OTTER_TASK_WAIT_END();
}
#endif


int tarch::multicore::native::getNumberOfPendingTasks() { return numberOfSpawnedTasks; }


void tarch::multicore::native::spawnTask(
  Task*                        job,
  const std::set<TaskNumber>&  inDependencies,
  const TaskNumber&            taskNumber
) {
  // Administer in-dependencies incl self-dep
  std::vector<TaskNumber>  iterableExistingInDepencencies;
  iterableExistingInDepencencies.reserve( inDependencies.size()+1 );
  for (auto inTask: inDependencies) {
    if ( canServeDependencyEntry(inTask) ) {
      iterableExistingInDepencencies.push_back(inTask);
    }
  }
  if (canServeDependencyEntry(taskNumber)) {
    iterableExistingInDepencencies.push_back(taskNumber);
  }

  if (taskNumber == NoOutDependencies) {
    #pragma omp atomic
    numberOfSpawnedTasks++;

    logDebug( "spawnTask(...)", "fire totally independent task with " << iterableExistingInDepencencies.size() << " in dependencies" );
    OTTER_DEFINE_TASK(task, otterGetActiveTask(), otter_no_add_to_pool, __func__);
    #pragma omp task priority(job->getPriority()) depend(iterator(j=0:iterableExistingInDepencencies.size()), in: dependencyTrackingBaseAddress[iterableExistingInDepencencies[j]]) untied
    {
      OTTER_TASK_START(task);
      while (job->run()) {
        #pragma omp taskyield
      }
      delete job;

      #pragma omp atomic
      numberOfSpawnedTasks--;
      OTTER_TASK_END();
    }
  }
  else if ( canServeDependencyEntry(taskNumber)) {
    logDebug(
      "spawnTask(...)",
      "fire task " << taskNumber <<
      " with outgoing task dependency and " << iterableExistingInDepencencies.size() <<
      " incoming dependencies (max no=" << maxSizeOfMemoryRegionUsedForDependencyTracking <<
      ", #spawned tasks=" << numberOfSpawnedTasks <<
      ")"
    );
    #pragma omp atomic
    numberOfSpawnedTasks++;

    OTTER_DEFINE_TASK(task, otterGetActiveTask(), otter_no_add_to_pool, __func__);
    #pragma omp task priority(job->getPriority()) depend(iterator(j=0:iterableExistingInDepencencies.size()), in: dependencyTrackingBaseAddress[iterableExistingInDepencencies[j]]) depend(out: dependencyTrackingBaseAddress[taskNumber]) untied
    {
      OTTER_TASK_START(task);
      while (job->run()) {
        OTTER_TASK_YIELD_START();
        #pragma omp taskyield
        OTTER_TASK_YIELD_END();
      }
      delete job;

      #pragma omp atomic
      numberOfSpawnedTasks--;
      OTTER_TASK_END();
    }
  }
  else {
    logDebug(
      "spawnTask(...)",
      "cannot administer outgoing task number " << taskNumber <<
      " and hence execute immediately after blocking wait for " << iterableExistingInDepencencies.size() <<
      " incoming dependencies (max no=" << maxSizeOfMemoryRegionUsedForDependencyTracking <<
      ", #spawned tasks=" << numberOfSpawnedTasks <<
      ")"
    );
    OTTER_TASK_WAIT_START(children); //! (AT) Note - otter doesn't currently support taskwait with dependencies
    #pragma omp taskwait depend(iterator(j=0:iterableExistingInDepencencies.size()), in: dependencyTrackingBaseAddress[iterableExistingInDepencencies[j]])
    OTTER_TASK_WAIT_END();
    while (job->run()) {
      OTTER_TASK_YIELD_START();
      #pragma omp taskyield
      OTTER_TASK_YIELD_END();
    }
    delete job;
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
  #if !defined(__NVCOMPILER)
  OTTER_DEFINE_TASK(task, otterGetActiveTask(), otter_no_add_to_pool, __func__);
  #pragma omp task firstprivate(myTask, copyOfTasksOfSameType, device)
  #endif
  {
    #if !defined(__NVCOMPILER)
    OTTER_TASK_START(task);
    #endif
    bool stillExecuteLocally = myTask->fuse(copyOfTasksOfSameType, device);
    if (stillExecuteLocally) {
      tarch::multicore::native::spawnTask(myTask, std::set<TaskNumber>(), NoOutDependencies);
    } else {
      delete myTask;
    }
    #if !defined(__NVCOMPILER)
    OTTER_TASK_END();
    #endif
  }
}


void tarch::multicore::native::waitForTasks(const std::set<TaskNumber>& inDependencies) {
  // Administer in-dependencies
  std::vector<TaskNumber>  iterableExistingInDepencencies;
  iterableExistingInDepencencies.reserve( inDependencies.size() );
  for (auto inTask: inDependencies) {
    if ( canServeDependencyEntry(inTask) ) {
      iterableExistingInDepencencies.push_back(inTask);
    }
  }

  if (not iterableExistingInDepencencies.empty()) {
    logDebug( "waitForTasks(...)", "wait for "<< iterableExistingInDepencencies.size() << " tasks to terminate: " << iterableExistingInDepencencies[0] );
    OTTER_TASK_WAIT_START(children); //! (AT) Note - otter doesn't currently support taskwait with dependencies
    #pragma omp taskwait depend(iterator(j=0:iterableExistingInDepencencies.size()), in: dependencyTrackingBaseAddress[iterableExistingInDepencencies[j]])
    OTTER_TASK_WAIT_END();
  }
}


void tarch::multicore::native::waitForAllTasks() {
  OTTER_TASK_WAIT_START(children);
  #pragma omp taskwait
  OTTER_TASK_WAIT_END();
}

#endif
