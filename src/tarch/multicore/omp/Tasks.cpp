// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/multicore/Task.h"

#include "multicore.h"
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
#include "tarch/compiler/CompilerSpecificSettings.h"


#if defined(SharedOMP)



namespace {
  static tarch::logging::Log _log( "tarch::multicore::native(omp)" );

  #ifndef DefaultSizeOfMemoryRegionUsedForDependencyTracking
  constexpr int DefaultSizeOfMemoryRegionUsedForDependencyTracking = 16;
  #endif

  /**
   * Initialisation value for entries in dependencyTrackingBaseAddress
   *
   * Consult docu of tarch::multicore::omp::internal for details.
   */
  constexpr tarch::multicore::omp::internal::DependencyTrackingDataType TaskNumberNotUsedYet = 0;

  /**
   * Initialisation value for entries in dependencyTrackingBaseAddress
   *
   * Consult docu of tarch::multicore::omp::internal for details.
   */
  constexpr tarch::multicore::omp::internal::DependencyTrackingDataType TaskNumberSpawned    = 1;

  /**
   * Initialisation value for entries in dependencyTrackingBaseAddress
   *
   * Consult docu of tarch::multicore::omp::internal for details.
   */
  constexpr tarch::multicore::omp::internal::DependencyTrackingDataType TaskNumberRunning    = 2;

  /**
   * Initialisation value for entries in dependencyTrackingBaseAddress
   *
   * Consult docu of tarch::multicore::omp::internal for details.
   */
  constexpr tarch::multicore::omp::internal::DependencyTrackingDataType TaskNumberTerminated = 3;

  /**
   * Memory region to administer dependencies
   *
   * Consult docu of tarch::multicore::omp::internal for details.
   */
  std::vector< tarch::multicore::omp::internal::DependencyTrackingDataType >  dependencyTrackingMemory(DefaultSizeOfMemoryRegionUsedForDependencyTracking, TaskNumberNotUsedYet);

  /**
   * We need a multi-read, single-write semaphores around the dependency
   * tracking. Arbitrary many tasks can be read and modify the dependency
   * tracking memory region at the same time, but we may only change it while
   * noone else uses it at all.
   */
  tarch::multicore::MultiReadSingleWriteSemaphore dependencyTrackingSemaphore;

  /**
   * We need to keep track of the number of tasks in flight, so we can identify
   * points where we can grow the dependency memory region.
   */
  int numberOfPendingTasks{0};

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
    return taskNumber < dependencyTrackingMemory.size();
  }


  /**
   * @todo Docu
   */
  bool hasTaskFinishedInOpenMP5( const std::vector<tarch::multicore::TaskNumber>&  inDependencies ) {
    bool result = true;
    // @todo raus
    if (not inDependencies.empty()) logInfo( "hasTaskFinishedInOpenMP5(...)", "check task " << inDependencies[0] << ": " << dependencyTrackingMemory[ inDependencies[0] ] );
    for (auto i: inDependencies) {
      if (i<dependencyTrackingMemory.size()) {
        auto* dependencyTracking = dependencyTrackingMemory.data();
        auto& dest = dependencyTracking[ i ];
        tarch::multicore::TaskNumber taskStatus;

        #pragma omp atomic read
        taskStatus = dest;

        result &= (taskStatus == TaskNumberNotUsedYet or taskStatus == TaskNumberTerminated);
      }
    }
    return result;
  }

} // empty (internal) namespace



std::string tarch::multicore::omp::internal::OpenMP5TaskStatus(DependencyTrackingDataType taskMarker) {
  switch (taskMarker) {
    case TaskNumberNotUsedYet:
      return "task-number-not-used-yet";
    case TaskNumberSpawned:
      return "task-spawned";
    case TaskNumberRunning:
      return "task-running";
    case TaskNumberTerminated:
      return "task-terminated";

  }
  return "<undef>";
}


void tarch::multicore::omp::internal::spawnTaskWithOpenMP6(
  tarch::multicore::Task*                           job,
  const std::vector<tarch::multicore::TaskNumber>&  inDependencies,
  const std::vector<tarch::multicore::TaskNumber>&  outDependency
) {
  assertion1( tarch::multicore::omp::majorOpenMPVersion()>=6, tarch::multicore::omp::majorOpenMPVersion() );
  auto* dependencyTracking = dependencyTrackingMemory.data();
  #pragma omp task priority(job->getPriority()) depend(iterator(j=0:inDependencies.size()), in: dependencyTracking[inDependencies[j]]) depend(iterator(j=0:outDependency.size()), inout: dependencyTracking[outDependency[j]]) untied transparent
  {
    OTTER_TASK_START(task);
    job->run();
    delete job;
    OTTER_TASK_END();
    #pragma omp atomic
    numberOfPendingTasks--;
  } // end of critical section, i.e. now memory region might change again
}


/**
 * Variant of spawn for all OpenMP versions under 6
 *
 * The name is slightly misleading, as we support any OpenMP version
 * between 3.x and 5 here.
 *
 *
 * ## Assumptions
 *
 * This routine makes a couple of assumptions:
 *
 * - The caller has already incremented numberOfPendingTasks by one;
 * - The dependencyTrackingBaseAddress array hosts all indices specified
 *   within inDependencies and outDependency;
 * - The semaphore dependencyTrackingSemaphore is locked with a read lock.
 */
void tarch::multicore::omp::internal::spawnTaskWithOpenMP5(
  tarch::multicore::Task*                           job,
  // @todo Muss ich aendern duerfen
  std::vector<tarch::multicore::TaskNumber>         inDependencies,
  const std::vector<tarch::multicore::TaskNumber>&  outDependency
) {
  assertion1( tarch::multicore::omp::majorOpenMPVersion()<6, tarch::multicore::omp::majorOpenMPVersion() );
  assertion1( outDependency.size()<2, outDependency.size() );

  static tarch::multicore::TaskNumber lastSpawnedTaskThatHasNotBeenReady = -1;
//  if (not hasTaskFinishedInOpenMP5(inDependencies)) {
//    logInfo( "spawnTaskWithOpenMP5(...)", "task is not ready yet, add artificial dependency on existing task " << lastSpawnedTaskThatHasNotBeenReady );
    // Might introduce self-dependency which degenerated to anti-dependency
//    inDependencies.push_back( lastSpawnedTaskThatHasNotBeenReady );
//  }



  if (not outDependency.empty()) {
    auto p = inDependencies.begin();
    while (p!=inDependencies.end()) {
      if (*p==outDependency[0]) {
        logInfo( "spawnTaskWithOpenMP5(...)", "manually wait for predecessor to terminate, as we cannot handle inout dependencies in task runtime" );

        // @todo den darf ich net verwenden:
        // waitForTask(*p);

        std::vector<tarch::multicore::TaskNumber> selfDependency(1);
        selfDependency[0] = outDependency[0];

        // Polling thread
        bool isReady;
        do {
          isReady = hasTaskFinishedInOpenMP5(selfDependency);
        } while (not isReady);

        p = inDependencies.erase(p);
      }
      else p++;
    }

//    lastSpawnedTaskThatHasNotBeenReady = outDependency[0];
    auto* dependencyTracking = dependencyTrackingMemory.data();
    auto& dest = dependencyTracking[ lastSpawnedTaskThatHasNotBeenReady ];
    #pragma omp atomic write
    dest = TaskNumberSpawned;

    logInfo( "spawnTaskWithOpenMP5(...)", "set task status " << outDependency[0] << " to spawned" );
  }

  // spawn mich selber und dann hab ich da unten die loop, ob alle ok sind

  auto* dependencyTracking = dependencyTrackingMemory.data();
  #pragma omp task priority(job->getPriority()) depend(iterator(j=0:inDependencies.size()), in: dependencyTracking[inDependencies[j]]) depend(iterator(j=0:outDependency.size()), inout: dependencyTracking[outDependency[j]]) untied
  {
    // Polling thread
    bool isReady;
    do {
      isReady = hasTaskFinishedInOpenMP5(inDependencies);
    } while (not isReady);

    if (not outDependency.empty()) {
      auto* dependencyTracking = dependencyTrackingMemory.data();
      auto& dest = dependencyTracking[ outDependency[0] ];
      #pragma omp atomic write
      dest = TaskNumberRunning;
    }

    // @todo dependency tracking
    OTTER_TASK_START(task);
    job->run();
    delete job;
    OTTER_TASK_END();

    logInfo( "spawnTaskWithOpenMP5(...)", "mark task " << outDependency[0] << " as complete" );
    if (not outDependency.empty()) {
      auto* dependencyTracking = dependencyTrackingMemory.data();
      auto& dest = dependencyTracking[ outDependency[0] ];
      #pragma omp atomic write
      dest = TaskNumberTerminated;
    }
    numberOfPendingTasks--;
    logInfo( "spawnTaskWithOpenMP5(...)", "unlocked" );
  }
}


void tarch::multicore::omp::internal::reserveDependencyTrackingMemory( int taskNumber ) {
  tarch::multicore::MultiReadSingleWriteSemaphore::Lock optimisticLock(dependencyTrackingSemaphore, tarch::multicore::MultiReadSingleWriteSemaphore::Lock::Read);
  bool mightHaveToGrow = taskNumber>dependencyTrackingMemory.size();
  optimisticLock.free();

  if (mightHaveToGrow) {
    tarch::multicore::MultiReadSingleWriteSemaphore::Lock conservativeLock(dependencyTrackingSemaphore, tarch::multicore::MultiReadSingleWriteSemaphore::Lock::Write);
    if (taskNumber>dependencyTrackingMemory.size()) {
      int volatile currentNumberOfPendingTasks = 1;

      while (currentNumberOfPendingTasks>0) {
        #pragma omp taskyield

        #pragma omp atomic read
        currentNumberOfPendingTasks = numberOfPendingTasks;
      }

      logInfo( "reserveDependencyTrackingMemory(int)", "allocated memory for task dependency management for up to " << (taskNumber+1) << " tasks (previously max " << dependencyTrackingMemory.size() << ")" );


      // @todo raus
      omp_pause_resource(omp_pause_hard, omp_get_initial_device());
/*
#pragma omp taskwait
#pragma omp critical
      {
*/
      dependencyTrackingMemory.resize( taskNumber+1, TaskNumberNotUsedYet);
//      }
    }
  }
}


/**
 * Native mapping of a task loop onto a task loop
 *
 *
 * ## Observations
 *
 * I found that quite a lot of OpenMP implementations realise the taskwait at the
 * end as busy wait. This is totally annoying as it might lead to deadlocks whenever
 * a task has a taskyield internally and/or there are more tasks than threads.
 *
 *
 * ## NVIDIA
 *
 * NVIDIA's nvc++ does not support taskloops. We have contacted their R&D and they
 * confirmed that they only implement a subset of OpenMP, and the taskloop is not
 * on the list of features that they want to support.
 *
 *
 * ## Implementation and bugs
 *
 * See discussion in the context of the other taskloop: It is important to
 * label tasks as shared even though I think copying should be fine as a copy
 * degenerates to a shallow copy.
 *
 * You are not allowed to call this routine for an empty task
 * set. The empty task set means that the taskloop construct becomes nop. The
 * subsequent taskwait hence synchronises on one logical level above within the
 * task hierarchy, which is not what we want.
 *
 *
 * ## SmartMPI (legacy)
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
 */
void tarch::multicore::native::spawnAndWaitAsTaskLoop(const std::vector<tarch::multicore::Task*>& tasks) {
  assertion(not tasks.empty());

#if defined(UseOtter)
  aacuto otter_tasks = std::vector<otter_task_context*>();
  for (int i = 0; i < static_cast<int>(tasks.size()); i++) {
    otter_tasks.push_back(otterTaskInitialise(OTTER_NULL_TASK, -1, otter_no_add_to_pool, true, OTTER_SOURCE_LOCATION(), __func__));
  }
#endif

  if (tarch::multicore::omp::majorOpenMPVersion()>=6) {
    #define OpenMP6Syntax
  }
  else {
    #undef OpenMP6Syntax
  }

  #if defined(OpenMPTaskGroup) and defined(OpenMP6Syntax)
  #pragma omp taskloop nogroup shared(tasks) grainsize(1) priority(tarch::multicore::Task::DefaultPriority) untied transparent
  #elif defined(OpenMPTaskGroup) and !defined(OpenMP6Syntax)
  #pragma omp taskloop nogroup shared(tasks) grainsize(1) priority(tarch::multicore::Task::DefaultPriority) untied
  #endif
  for (int i = 0; i < static_cast<int>(tasks.size()); i++) {
    #if !defined(OpenMPTaskGroup) and defined(OpenMP6Syntax)
    #pragma omp task priority(tarch::multicore::Task::DefaultPriority) transparent
    #elif !defined(OpenMPTaskGroup) and !defined(OpenMP6Syntax)
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
  OTTER_TASK_WAIT_END();
}


void tarch::multicore::native::spawnTask(
  Task*                        job,
  const std::set<TaskNumber>&  inDependencies,
  const TaskNumber&            taskNumber
) {
  tarch::multicore::omp::internal::reserveDependencyTrackingMemory(taskNumber);

  // @todo Docu: Has to be the very first thing!
#pragma omp atomic
numberOfPendingTasks++;

  // ensure we do not alter dependency memory while we spawn
  tarch::multicore::MultiReadSingleWriteSemaphore::Lock optimisticLock(dependencyTrackingSemaphore, tarch::multicore::MultiReadSingleWriteSemaphore::Lock::Read);

  // Administer in-dependencies
  std::vector<TaskNumber>  iterableExistingInDepencencies;
  iterableExistingInDepencencies.reserve( inDependencies.size()+1 );
  for (auto inTask: inDependencies) {
    if ( canServeDependencyEntry(inTask) ) {
      iterableExistingInDepencencies.push_back(inTask);
    }
  }

  std::vector<TaskNumber>  iterableOutDepencencies( taskNumber == NoOutDependencies ? 0 : 1 );
  if (taskNumber != NoOutDependencies) iterableOutDepencencies[0] = taskNumber;

  logInfo(
    "spawnTask(...)",
    "fire task " << taskNumber <<
    " with " << iterableOutDepencencies.size() << " outgoing task dependencies and incoming dependencies " << toString(inDependencies) <<
    " out of which " << iterableExistingInDepencencies.size() << " can be served"
  );

  OTTER_DEFINE_TASK(task, otterGetActiveTask(), otter_no_add_to_pool, __func__);

  if (tarch::multicore::omp::majorOpenMPVersion()>=6) {
    tarch::multicore::omp::internal::spawnTaskWithOpenMP6(
      job,
      iterableExistingInDepencencies,
      iterableOutDepencencies
    );
  }
  else {
    tarch::multicore::omp::internal::spawnTaskWithOpenMP5(
      job,
      iterableExistingInDepencencies,
      iterableOutDepencencies
    );
  }
}


void tarch::multicore::native::waitForTasks(const std::set<TaskNumber>& inDependencies) {
  // ensure we do not alter dependency memory while we spawn
  tarch::multicore::MultiReadSingleWriteSemaphore::Lock optimisticLock(dependencyTrackingSemaphore, tarch::multicore::MultiReadSingleWriteSemaphore::Lock::Read);

  // Administer in-dependencies
  std::vector<TaskNumber>  iterableExistingInDepencencies;
  iterableExistingInDepencencies.reserve( inDependencies.size() );
  for (auto inTask: inDependencies) {
    if ( canServeDependencyEntry(inTask) ) {
      iterableExistingInDepencencies.push_back(inTask);
    }
  }

  if (not iterableExistingInDepencencies.empty()) {
    logInfo( "waitForTasks(...)", "wait for "<< iterableExistingInDepencencies.size() << " tasks to terminate. Original in-set=" << toString(inDependencies) );

    OTTER_TASK_WAIT_START(children); //! (AT) Note - otter doesn't currently support taskwait with dependencies

    auto* dependencyTracking = dependencyTrackingMemory.data();
    #pragma omp taskwait depend(iterator(j=0:iterableExistingInDepencencies.size()), in: dependencyTracking[iterableExistingInDepencencies[j]])

    OTTER_TASK_WAIT_END();

    // @todo raus
    logInfo( "waitForTasks(...)", "native OpenMP task wait for " << toString(inDependencies) << " returned with " << dependencyTrackingMemory[iterableExistingInDepencencies[0]] );
  }
  else {
    logInfo( "waitForTasks(...)", "task handles for " << toString(inDependencies) << " do not exist" );
  }

  if (tarch::multicore::omp::majorOpenMPVersion()<6 and not iterableExistingInDepencencies.empty()) {
    logDebug( "waitForTasks(...)", "tasks " << toString(inDependencies) << " might have been spawned by non-sibling. Better check manually if they have terminated" );
    bool isReady;
    do {
      isReady = hasTaskFinishedInOpenMP5(iterableExistingInDepencencies);
    } while (not isReady);
  }
}


void tarch::multicore::native::waitForAllTasks() {
  #pragma omp taskwait
}
#endif
