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
  constexpr int DefaultSizeOfMemoryRegionUsedForDependencyTracking = 16000;
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
  tarch::multicore::omp::internal::DependencyTrackingDataType*  dependencyTrackingMemory = new tarch::multicore::omp::internal::DependencyTrackingDataType[DefaultSizeOfMemoryRegionUsedForDependencyTracking](TaskNumberNotUsedYet);

  int dependencyTrackingMemorySize{DefaultSizeOfMemoryRegionUsedForDependencyTracking};

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


bool tarch::multicore::omp::internal::hasTaskFinishedInOpenMP5( const std::vector<tarch::multicore::TaskNumber>&  inDependencies ) {
  bool result = true;

  for (auto i: inDependencies) {
    if (i<dependencyTrackingMemorySize) {
      auto& dest = dependencyTrackingMemory[ i ];
      tarch::multicore::TaskNumber taskStatus;

      #pragma omp atomic read
      taskStatus = dest;

      result &= (taskStatus == TaskNumberNotUsedYet or taskStatus == TaskNumberTerminated);
    }
  }

  return result;
}


void tarch::multicore::omp::internal::spawnTaskWithOpenMP6(
  tarch::multicore::Task*                           job,
  const std::vector<tarch::multicore::TaskNumber>&  inDependencies,
  const std::vector<tarch::multicore::TaskNumber>&  outDependency
) {
  assertion1( tarch::multicore::omp::majorOpenMPVersion()>=6, tarch::multicore::omp::majorOpenMPVersion() );

  if (tarch::multicore::omp::majorOpenMPVersion()>=6) {
/*
    #define OpenMP6Syntax
  }
  else {
    #undef OpenMP6Syntax
  }
*/

    #pragma omp task priority(job->getPriority()) depend(iterator(j=0:inDependencies.size()), in: dependencyTrackingMemory[inDependencies[j]]) depend(iterator(j=0:outDependency.size()), inout: dependencyTrackingMemory[outDependency[j]]) untied transparent
    {
      OTTER_TASK_START(task);
      job->run();
      delete job;
      OTTER_TASK_END();
      #pragma omp atomic
      numberOfPendingTasks--;
    }
  }
}


void tarch::multicore::omp::internal::spawnTaskWithOpenMP5(
  tarch::multicore::Task*                           job,
  std::vector<tarch::multicore::TaskNumber>         inDependencies,
  const std::vector<tarch::multicore::TaskNumber>&  outDependency
) {
  assertion1( tarch::multicore::omp::majorOpenMPVersion()<6, tarch::multicore::omp::majorOpenMPVersion() );
  assertion1( outDependency.size()<2, outDependency.size() );

  if (not outDependency.empty()) {
    auto p = inDependencies.begin();
    while (p!=inDependencies.end()) {
      if (*p==outDependency[0]) {
        logDebug( "spawnTaskWithOpenMP5(...)", "manually wait for predecessor to terminate, as we cannot handle inout dependencies in task runtime" );

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

    auto& dest = dependencyTrackingMemory[ outDependency[0] ];
    #pragma omp atomic write
    dest = TaskNumberSpawned;

    logDebug( "spawnTaskWithOpenMP5(...)", "set task status " << outDependency[0] << " to spawned" );
  }

  // #pragma omp task priority(job->getPriority()) depend(iterator(j=0:inDependencies.size()), in: dependencyTrackingMemory[inDependencies[j]]) depend(iterator(j=0:outDependency.size()), out: dependencyTrackingMemory[outDependency[j]]) firstprivate(inDependencies,outDependency,job) shared(numberOfPendingTasks,dependencyTrackingMemory) untied
  #pragma omp task priority(job->getPriority()) depend(iterator(j=0:inDependencies.size()), in: dependencyTrackingMemory[inDependencies[j]]) depend(iterator(j=0:outDependency.size()), out: dependencyTrackingMemory[outDependency[j]]) untied
  {
    // Polling thread
    bool isReady;
    do {
      isReady = hasTaskFinishedInOpenMP5(inDependencies);
    } while (not isReady);

    auto& dest = dependencyTrackingMemory[ outDependency[0] ];

    assertion1( dest==TaskNumberSpawned, OpenMP5TaskStatus(dest) );

    if (not outDependency.empty()) {
      #pragma omp atomic write
      dest = TaskNumberRunning;
    }

    OTTER_TASK_START(task);
    job->run();
    delete job;
    OTTER_TASK_END();

    logDebug( "spawnTaskWithOpenMP5(...)", "mark task " << outDependency[0] << " as complete" );
    if (not outDependency.empty()) {
      #pragma omp atomic write
      dest = TaskNumberTerminated;
    }

    #pragma omp atomic
    numberOfPendingTasks--;
  }
}


void tarch::multicore::omp::internal::reserveDependencyTrackingMemory( int taskNumber ) {
  tarch::multicore::MultiReadSingleWriteSemaphore::Lock optimisticLock(dependencyTrackingSemaphore, tarch::multicore::MultiReadSingleWriteSemaphore::Lock::Read);
  bool mightHaveToGrow = taskNumber>=dependencyTrackingMemorySize;
  optimisticLock.free();

  if (mightHaveToGrow) {
    tarch::multicore::MultiReadSingleWriteSemaphore::Lock conservativeLock(dependencyTrackingSemaphore, tarch::multicore::MultiReadSingleWriteSemaphore::Lock::Write);
    if (taskNumber>=dependencyTrackingMemorySize) {
      int volatile currentNumberOfPendingTasks = 1;

      while (currentNumberOfPendingTasks>0) {
        #pragma omp taskyield

        #pragma omp atomic read
        currentNumberOfPendingTasks = numberOfPendingTasks;
      }

      int oldSize = dependencyTrackingMemorySize;

      #pragma omp atomic write
      dependencyTrackingMemorySize = (taskNumber+1)*2;

      logInfo( "reserveDependencyTrackingMemory(int)", "allocated memory for task dependency management for up to " << (dependencyTrackingMemorySize+1) << " tasks (previously max " << oldSize << ")" );

      dependencyTrackingMemory = new tarch::multicore::omp::internal::DependencyTrackingDataType[dependencyTrackingMemorySize](TaskNumberNotUsedYet);
    }
  }
}


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

  #pragma omp atomic
  numberOfPendingTasks++;

  tarch::multicore::MultiReadSingleWriteSemaphore::Lock optimisticLock(dependencyTrackingSemaphore, tarch::multicore::MultiReadSingleWriteSemaphore::Lock::Read);

  // Administer in-dependencies
  std::vector<TaskNumber>  iterableExistingInDepencencies;
  iterableExistingInDepencencies.reserve( inDependencies.size()+1 );
  for (auto inTask: inDependencies) {
    if ( tarch::multicore::omp::internal::canServeDependencyEntry(inTask) ) {
      iterableExistingInDepencencies.push_back(inTask);
    }
  }

  std::vector<TaskNumber>  iterableOutDepencencies( taskNumber == NoOutDependencies ? 0 : 1 );
  if (taskNumber != NoOutDependencies) iterableOutDepencencies[0] = taskNumber;

  logDebug(
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
    if ( tarch::multicore::omp::internal::canServeDependencyEntry(inTask) ) {
      iterableExistingInDepencencies.push_back(inTask);
    }
  }

  if (not iterableExistingInDepencencies.empty()) {
    logDebug( "waitForTasks(...)", "wait for "<< iterableExistingInDepencencies.size() << " tasks to terminate. Original in-set=" << toString(inDependencies) );

    OTTER_TASK_WAIT_START(children); //! (AT) Note - otter doesn't currently support taskwait with dependencies

    #pragma omp taskwait depend(iterator(j=0:iterableExistingInDepencencies.size()), in: dependencyTrackingMemory[iterableExistingInDepencencies[j]])

    OTTER_TASK_WAIT_END();
  }

  if (tarch::multicore::omp::majorOpenMPVersion()<6 and not iterableExistingInDepencencies.empty()) {
    logDebug( "waitForTasks(...)", "tasks " << toString(inDependencies) << " might have been spawned by non-sibling. Better check manually if they have terminated" );
    bool isReady;
    do {
      isReady = tarch::multicore::omp::internal::hasTaskFinishedInOpenMP5(iterableExistingInDepencencies);
    } while (not isReady);
  }
}


void tarch::multicore::native::waitForAllTasks() {
  #pragma omp taskwait
}


bool tarch::multicore::omp::internal::canServeDependencyEntry(int taskNumber) {
  return taskNumber < dependencyTrackingMemorySize;
}

#endif
