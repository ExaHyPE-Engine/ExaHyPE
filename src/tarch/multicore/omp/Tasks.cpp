// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/Task.h"
#include "Tasks.h"
#include "config.h"
#include "tarch/Assertions.h"
#include "tarch/logging/Statistics.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"
#include "tarch/multicore/multicore.h"
#include "tarch/multicore/otter.h"
#include "tarch/multicore/MultiReadSingleWriteSemaphore.h"
#include "tarch/multicore/MultiReadSingleWriteLock.h"
#include "tarch/services/ServiceRepository.h"

#if defined(SharedOMP)

namespace {
  static tarch::logging::Log _log( "tarch::multicore::native(omp)" );

  using DependencyTrackingDataType = unsigned char;

  #ifndef DefaultSizeOfMemoryRegionUsedForDependencyTracking
  constexpr int DefaultSizeOfMemoryRegionUsedForDependencyTracking = 65536;
  #endif

  /**
   * Memory region to administer dependencies
   *
   *
   * Dependency tracking in OpenMP is organised via addresses. These addresses
   * are sole helpers, i.e. the underlying memory region doesn't hold any data
   * with semantics. However, it has to be owned by the application code. To
   * ensure this is the case, we allocate some chunk only for the dependency
   * tracking and hold its baseline address in this variable.
   *
   * # Flaw
   *
   * I tried to allocate this array at startup time through
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * DependencyTrackingDataType* dependencyTrackingBaseAddress = new DependencyTrackingDataType[DefaultSizeOfMemoryRegionUsedForDependencyTracking];
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * but that failed somehow. Now, I allocate it lazyly in spawnTask().
   */
  DependencyTrackingDataType* dependencyTrackingBaseAddress = new DependencyTrackingDataType[DefaultSizeOfMemoryRegionUsedForDependencyTracking];
  //DependencyTrackingDataType* dependencyTrackingBaseAddress{nullptr};

  /**
   * Memorize size of dependency tracking. Is initially 0 as the pointer
   * dependencyTrackingBaseAddress is a nullptr.
   */
  int sizeOfMemoryRegionUsedForDependencyTracking{DefaultSizeOfMemoryRegionUsedForDependencyTracking};

  /**
   * Maximal size that we'd like to have for the memory region. Is updated
   * on-the-fly. I use this value only for reporting.
   */
  int maxSizeOfMemoryRegionUsedForDependencyTracking = DefaultSizeOfMemoryRegionUsedForDependencyTracking;

  /**
   * We need a multi-read, single-write semaphores around the dependency
   * tracking. Arbitrary many tasks can be spawned at the same time, but we
   * may ever only let it grow by one single task.
   */
  tarch::multicore::MultiReadSingleWriteSemaphore dependencyTrackingSemaphore;

  /**
   * Update the internal stats how big the whole thing should be and then
   * return a flag that indicates if we should grow the memory region.
   *
   * If the routine returns a yes, it does not mean that we can actually
   * grow the memory. We are only allowed to do this if there are no further
   * tasks pending. We check this here once provisionally, but it might
   * change until we come to the point where we actually wanna grow.
   *
   * The last or catches the initial case.
   */
  void updateMemoryStatistics(int taskNumber) {
    int requiredDependencyMemorySize = taskNumber + 1;

    #pragma omp atomic compare
    maxSizeOfMemoryRegionUsedForDependencyTracking = maxSizeOfMemoryRegionUsedForDependencyTracking<requiredDependencyMemorySize
                                                   ? requiredDependencyMemorySize
                                                   : maxSizeOfMemoryRegionUsedForDependencyTracking;
  }


  /**
   * Can I serve a certain dependency
   *
   * This routine exclusively checks if an in-dependency could exist. This does
   * not mean anything re if depending upon taskNumber might be an
   * anti-dependency.
   *
   * As we know that sizeOfMemoryRegionUsedForDependencyTracking is
   * monotonously growing, we can work with a simple read atomic.
   *
   */
  bool canServeDependencyEntry(int taskNumber) {
    return taskNumber < sizeOfMemoryRegionUsedForDependencyTracking;
  }
} // empty (internal) namespace


/**
 * This routine is not thread-safe. You have to be sure that there are no
 * race conditions and that noone else accesses any dependencies still.
 */
void tarch::multicore::omp::reserveDependencyTrackingMemory() {
  if (maxSizeOfMemoryRegionUsedForDependencyTracking>sizeOfMemoryRegionUsedForDependencyTracking) {  
    if (dependencyTrackingBaseAddress!=nullptr) {
      delete[] dependencyTrackingBaseAddress;
      dependencyTrackingBaseAddress = nullptr;
      logInfo( "canServeDependencyEntry(int)", "free preallocated memory for task dependency management" );
    }
    
    dependencyTrackingBaseAddress = new DependencyTrackingDataType[maxSizeOfMemoryRegionUsedForDependencyTracking];
    logInfo( "canServeDependencyEntry(int)", "allocated memory for task dependency management for up to " << maxSizeOfMemoryRegionUsedForDependencyTracking << " tasks" );

    sizeOfMemoryRegionUsedForDependencyTracking = maxSizeOfMemoryRegionUsedForDependencyTracking;
  }
}


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
      tasks[i]->run();
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


void tarch::multicore::native::spawnTask(
  Task*                        job,
  const std::set<TaskNumber>&  inDependencies,
  const TaskNumber&            taskNumber
) {
  updateMemoryStatistics(taskNumber);

/*
  const bool shouldAcquireWriteLock = true;
  
  @todo a multiple-read once write semaphore in the grow-mem region would help

  tarch::multicore::MultiReadSingleWriteLock lock( dependencyTrackingSemaphore, not shouldAcquireWriteLock );

  // @todo have to check again, as it might have changed meanwhile
  if (
    shouldAcquireWriteLock
    and
    dependencyTrackingBaseAddress==nullptr
  ) {
    tarch::multicore::omp::reserveDependencyTrackingMemory();
  }
*/

  // Administer in-dependencies
  std::vector<TaskNumber>  iterableExistingInDepencencies;
  iterableExistingInDepencencies.reserve( inDependencies.size()+1 );
  for (auto inTask: inDependencies) {
    if ( canServeDependencyEntry(inTask) ) {
      iterableExistingInDepencencies.push_back(inTask);
    }
  }

  if (taskNumber == NoOutDependencies) {
    logDebug( "spawnTask(...)", "fire totally independent task with " << iterableExistingInDepencencies.size() << " in dependencies" );

    OTTER_DEFINE_TASK(task, otterGetActiveTask(), otter_no_add_to_pool, __func__);
    #pragma omp task priority(job->getPriority()) depend(iterator(j=0:iterableExistingInDepencencies.size()), in: dependencyTrackingBaseAddress[iterableExistingInDepencencies[j]]) untied
    {
      OTTER_TASK_START(task);
      job->run();
      delete job;
      OTTER_TASK_END();
    }
  }
  else if ( canServeDependencyEntry(taskNumber)) {
    logDebug(
      "spawnTask(...)",
      "fire task " << taskNumber <<
      " with outgoing task dependency and " << iterableExistingInDepencencies.size() <<
      " incoming dependencies (max no=" << maxSizeOfMemoryRegionUsedForDependencyTracking <<
      ")"
    );

    OTTER_DEFINE_TASK(task, otterGetActiveTask(), otter_no_add_to_pool, __func__);
    // Please consult canServeDependencyEntry() for this critical section
    #pragma omp task priority(job->getPriority()) depend(iterator(j=0:iterableExistingInDepencencies.size()), in: dependencyTrackingBaseAddress[iterableExistingInDepencencies[j]]) depend(inout: dependencyTrackingBaseAddress[taskNumber]) untied
    {
      OTTER_TASK_START(task);
      job->run();
      delete job;
      OTTER_TASK_END();
    } // end of critical section, i.e. now memory region might change again
  }
  else {
    logDebug(
      "spawnTask(...)",
      "cannot administer outgoing task number " << taskNumber <<
      " and hence execute immediately after blocking wait for " << iterableExistingInDepencencies.size() <<
      " incoming dependencies (max no=" << maxSizeOfMemoryRegionUsedForDependencyTracking << ")"
    );
    OTTER_TASK_WAIT_START(children); //! (AT) Note - otter doesn't currently support taskwait with dependencies

    #pragma omp taskwait depend(iterator(j=0:iterableExistingInDepencencies.size()), in: dependencyTrackingBaseAddress[iterableExistingInDepencencies[j]])

    OTTER_TASK_WAIT_END();
    job->run();
    delete job;
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
  #pragma omp taskwait

  tarch::multicore::omp::reserveDependencyTrackingMemory();
}

#endif
