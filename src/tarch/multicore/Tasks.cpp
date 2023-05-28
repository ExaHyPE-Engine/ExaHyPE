#include "tarch/Assertions.h"
#include "tarch/logging/Statistics.h"
#include "tarch/services/ServiceRepository.h"

#include "BooleanSemaphore.h"
#include "Lock.h"
#include "Core.h"

#include <thread>
#include <queue>
#include <set>


#include "Tasks.h"
#include "multicore.h"

#include "tarch/multicore/orchestration/StrategyFactory.h"
#include "tarch/multicore/otter.h"


#ifdef UseSmartMPI
#include "smartmpi.h"
#endif


namespace {
  tarch::multicore::BooleanSemaphore  activeTasksSemaphore;
  std::set<int>                       activeTaskNumbers;

  /**
   * Temporary task queue which is used if and only if we hold back tasks
   */
  typedef std::list< tarch::multicore::Task* > NonblockingTasks;

  NonblockingTasks                     globalNonblockingTasks;
  std::vector<NonblockingTasks>        localNonblockingTasks;
  tarch::multicore::BooleanSemaphore   nonblockingTasksSemaphore;

  tarch::multicore::orchestration::Strategy* orchestrationStrategy = tarch::multicore::orchestration::createDefaultStrategy();

  tarch::logging::Log _log( "tarch::multicore" );

  const std::string BSPConcurrencyLevelStatisticsIdentifier( "tarch::multicore::bsp-concurrency-level" );
  const std::string GlobalPendingTasksStatisticsIdentifier( "tarch::multicore::global-pending-tasks" );
  const std::string ThreadLocalPendingTasksStatisticsIdentifier( "tarch::multicore::thread-local-pending-tasks" );
  const std::string FuseTasksStatisticsIdentifier( "tarch::multicore::fuse-tasks");

  /**
   * This routine processes one pending tasks.
   *
   * It has no hidden logic, i.e. it will always take one task
   * if possible. The routine calling it is reponsible to make
   * the decision whether this is appropriate.
   *
   * @return -1 if nothing found, otherwise task id
   */
  bool processOnePendingTaskLIFO() {
    tarch::multicore::Task* myTask = nullptr;

    tarch::multicore::Lock lock(nonblockingTasksSemaphore);
    if (not globalNonblockingTasks.empty()) {
      myTask = globalNonblockingTasks.back();
      globalNonblockingTasks.pop_back();
    }
    lock.free();

    if (myTask!=nullptr) {
      bool requeue = myTask->run();
      if (requeue)
        spawnTask( myTask );
      else
        delete myTask;
    }
    return myTask!=nullptr;
  }


  bool processOnePendingTaskFIFO() {
    tarch::multicore::Task* myTask = nullptr;

    tarch::multicore::Lock lock(nonblockingTasksSemaphore);
    if (not globalNonblockingTasks.empty()) {
      myTask = globalNonblockingTasks.front();
      globalNonblockingTasks.pop_front();
    }
    lock.free();

    if (myTask!=nullptr) {
      bool requeue = myTask->run();
      if (requeue)
        spawnTask( myTask );
      else
        delete myTask;
      return true;
    }
    else return false;
  }


  bool mapOnePendingTaskOntoNativeTask() {
    tarch::multicore::Task* myTask = nullptr;

    tarch::multicore::Lock lock(nonblockingTasksSemaphore);
    if (not globalNonblockingTasks.empty()) {
      myTask = globalNonblockingTasks.front();
      globalNonblockingTasks.pop_front();
    }
    lock.free();

    if (myTask!=nullptr) {
      tarch::multicore::native::spawnTask(myTask);
    }
    return myTask!=nullptr;
  }


  /**
   * @return Number of processed tasks
   */
  int processPendingTasksFIFO(int maxTasks) {
    NonblockingTasks extractedTasks;

    tarch::multicore::Lock lock(nonblockingTasksSemaphore);

    NonblockingTasks::iterator cutIteration = globalNonblockingTasks.begin();
    while (
      cutIteration!=globalNonblockingTasks.end()
      and
      maxTasks>0
    ) {
      maxTasks--;
      cutIteration++;
    }

    extractedTasks.splice( extractedTasks.begin(), globalNonblockingTasks, globalNonblockingTasks.begin(), cutIteration );
    lock.free();

    for (auto& task: extractedTasks) {
      bool requeue = task->run();
      if (requeue)
        spawnTask( task );
      else
        delete task;
    }

    return extractedTasks.size();
  }
}


/**
 * @return Number of processed tasks
 */
int tarch::multicore::internal::mapPendingTasksOntoNativeTasks(int maxTasks) {
  NonblockingTasks extractedTasks;

  tarch::multicore::Lock lock(nonblockingTasksSemaphore);
  maxTasks = std::min( static_cast<int>(globalNonblockingTasks.size()), maxTasks );
  NonblockingTasks::iterator cutIteration = globalNonblockingTasks.begin();
  std::advance( cutIteration, globalNonblockingTasks.size() - maxTasks );
  extractedTasks.splice( extractedTasks.begin(), globalNonblockingTasks, cutIteration, globalNonblockingTasks.end() );
  lock.free();

  for (NonblockingTasks::reverse_iterator task = extractedTasks.rbegin(); task != extractedTasks.rend(); task++) {
    tarch::multicore::native::spawnTask(*task);
  }

  return extractedTasks.size();
}


bool tarch::multicore::internal::fusePendingTasks(const tarch::multicore::orchestration::Strategy::FuseInstruction&  fuseInstruction) {
  logDebug( "fusePendingTasks(int)", "process " << globalNonblockingTasks.size() << " tasks subject to " << fuseInstruction.toString() );

  ::tarch::logging::Statistics::getInstance().log( GlobalPendingTasksStatisticsIdentifier, tarch::multicore::internal::getNumberOfWithholdPendingTasks() );

  tarch::multicore::Task* myTask = nullptr;
  std::list< tarch::multicore::Task* > tasksOfSameType;

  tarch::multicore::Lock lock(nonblockingTasksSemaphore);
  if (not globalNonblockingTasks.empty()) {
    myTask = globalNonblockingTasks.front();
    globalNonblockingTasks.pop_front();
  }

  if (
    myTask!=nullptr
    and
    myTask->canFuse()
  ) {
    auto pp = globalNonblockingTasks.begin();
    while (
      pp!=globalNonblockingTasks.end()
      and
      (*pp)->getTaskType()==myTask->getTaskType()
      and
      (*pp)->getTaskType()==myTask->canFuse()
      and
      tasksOfSameType.size()<fuseInstruction.maxTasks-1
    ) {
      tasksOfSameType.push_back( *pp );
      pp = globalNonblockingTasks.erase(pp);
    }
    lock.free();

    ::tarch::logging::Statistics::getInstance().log( FuseTasksStatisticsIdentifier, tasksOfSameType.size() );

    native::processFusedTask(myTask, tasksOfSameType, fuseInstruction.device);
  }
  else if (
    myTask!=nullptr
    and
    not myTask->canFuse()
  ) {
    lock.free();
    tarch::multicore::native::spawnTask(myTask);
  }

  return myTask==nullptr ? 0 : tasksOfSameType.size()+1;
}


void tarch::multicore::setOrchestration( tarch::multicore::orchestration::Strategy* realisation ) {
  assertion(orchestrationStrategy!=nullptr);
  assertion(realisation!=nullptr);

  delete orchestrationStrategy;
  orchestrationStrategy = realisation;
}


tarch::multicore::orchestration::Strategy* tarch::multicore::swapOrchestration( tarch::multicore::orchestration::Strategy* realisation ) {
  assertion(orchestrationStrategy!=nullptr);
  assertion(realisation!=nullptr);

  tarch::multicore::orchestration::Strategy* result = orchestrationStrategy;
  orchestrationStrategy = realisation;
  return result;
}


int tarch::multicore::getNumberOfReservedTaskNumbers() {
  return activeTaskNumbers.size();
}


void tarch::multicore::releaseTaskNumber(int number) {
  tarch::multicore::Lock activeTasksLock( activeTasksSemaphore );
  assertionEquals( activeTaskNumbers.count(number),1 );
  activeTaskNumbers.erase( number );
}


int tarch::multicore::reserveTaskNumber() {
  tarch::multicore::Lock lock( activeTasksSemaphore );
  int result = activeTaskNumbers.size();
  while (activeTaskNumbers.count( result )>0) {
    result+=23;
  }
  activeTaskNumbers.insert( result );
  return result;
}


bool operator<( const tarch::multicore::Task& lhs, const tarch::multicore::Task& rhs ) {
  return lhs.getPriority() < rhs.getPriority();
}


bool tarch::multicore::TaskComparison::operator() (const Task& lhs, const Task& rhs) const {
  return lhs < rhs;
}


bool tarch::multicore::TaskComparison::operator() (Task* lhs, Task* rhs) const {
  return *lhs < *rhs;
}


tarch::multicore::Task::Task( int taskType, int priority ):
  _taskType( taskType ),
  _priority( priority ) {
}


bool tarch::multicore::Task::canFuse() const {
  return _taskType!=DontFuse;
}


int tarch::multicore::Task::getPriority() const {
  return _priority;
}


void tarch::multicore::Task::setPriority( int priority ) {
  _priority = priority;
}


int tarch::multicore::Task::getTaskType() const {
  return _taskType;
}


bool tarch::multicore::Task::fuse( const std::list<Task*>& otherTasks, int device ) {
  assertion( canFuse() );
  for (auto pp: otherTasks) {
    tarch::multicore::Task* currentTask = pp;
    while (currentTask->run()) {}
    delete currentTask;
  }
  return true;
}


tarch::multicore::TaskWithCopyOfFunctor::TaskWithCopyOfFunctor( int taskType, int priority, const std::function<bool()>& taskFunctor ):
  Task(taskType, priority),
  _taskFunctor(taskFunctor)  {
}


bool tarch::multicore::TaskWithCopyOfFunctor::run() {
  return _taskFunctor();
}


tarch::multicore::TaskWithoutCopyOfFunctor::TaskWithoutCopyOfFunctor( int taskType, int priority, std::function<bool()>& taskFunctor ):
  Task(taskType, priority),
  _taskFunctor(taskFunctor)  {
}


bool tarch::multicore::TaskWithoutCopyOfFunctor::run() {
  return _taskFunctor();
}


bool tarch::multicore::processPendingTasks(int maxTasks, bool fifo) {
  assertion(maxTasks>=0);

  ::tarch::logging::Statistics::getInstance().log( GlobalPendingTasksStatisticsIdentifier, tarch::multicore::internal::getNumberOfWithholdPendingTasks() );

  internal::copyInternalTaskQueuesOverIntoGlobalQueue();

  bool  result        = false;
  while (maxTasks>0) {
    if (fifo) {
      maxTasks--;
      result |= processOnePendingTaskFIFO();
    }
    else {
      maxTasks--;
      result |= processOnePendingTaskLIFO();
    }

    if (not result and maxTasks==0) {
      Core::getInstance().yield();
    }
  }

  return result;
}


void tarch::multicore::spawnTask(Task*  task) {
  assertion( task!=nullptr );

  int threadNumber = Core::getInstance().getThreadNumber();
  assertion1( threadNumber>=0, threadNumber );
  assertion2( threadNumber<localNonblockingTasks.size(), threadNumber, localNonblockingTasks.size());

  #ifdef UseSmartMPI
  if ( task->isSmartMPITask() and smartmpi::spawn( dynamic_cast<smartmpi::Task*>(task) )) {
  }
  else
  #endif
  if ( localNonblockingTasks[threadNumber].size()>=orchestrationStrategy->getNumberOfTasksToHoldBack() ) {
    logDebug( "spawnTasks(int)", "spawn native task" );
    native::spawnTask(task);
  }
  else {
    localNonblockingTasks[threadNumber].push_back(task);
    ::tarch::logging::Statistics::getInstance().log( ThreadLocalPendingTasksStatisticsIdentifier, localNonblockingTasks[threadNumber].size() );

    logDebug( "spawnTask(...)", "enqueued task (#tasks=" << localNonblockingTasks[threadNumber].size() << ")" );

    auto fusionCommand = orchestrationStrategy->getNumberOfTasksToFuseAndTargetDevice();
    if (
      localNonblockingTasks[threadNumber].size()>=fusionCommand.minTasks
      and
      orchestrationStrategy->fuseTasksImmediatelyWhenSpawned()
    ) {
      internal::copyInternalTaskQueuesOverIntoGlobalQueue();
      internal::fusePendingTasks(fusionCommand);
    }
  }
}



void tarch::multicore::spawnAndWait(
  const std::vector< Task* >&  tasks
) {
  static tarch::logging::Log _log( "tarch::multicore" );

  if (not tasks.empty()) {
    static int nestedSpawnAndWaits = 0;
    static tarch::multicore::BooleanSemaphore nestingSemaphore;

    tarch::multicore::Lock nestingLock( nestingSemaphore, false );
    nestingLock.lock();
    nestedSpawnAndWaits++;
    nestingLock.free();

    switch (orchestrationStrategy->paralleliseForkJoinSection(nestedSpawnAndWaits, tasks.size(), tasks[0]->getTaskType())) {
      case tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunSerially:
        {
          for (auto& p: tasks) {
            while (p->run()) {
              Core::getInstance().yield();
            }
            delete p;
          }
        }
        break;
      case tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallel:
        {
          ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, static_cast<int>(tasks.size()), true );
          native::spawnAndWaitAsTaskLoop(tasks);
          ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, -static_cast<int>(tasks.size()), true );

          // AT: ideally would record the handle of the encountering task here to correctly identify child tasks
          otterSynchroniseTasks(NULL, otter_sync_children);

          internal::copyInternalTaskQueuesOverIntoGlobalQueue();
          int  numberOfPendingTasks = tarch::multicore::internal::getNumberOfWithholdPendingTasks();
          auto fusionInstruction    = orchestrationStrategy->getNumberOfTasksToFuseAndTargetDevice();

          bool successInFusing = true;
          while ( successInFusing and globalNonblockingTasks.size()>=fusionInstruction.minTasks) {
            successInFusing = tarch::multicore::internal::fusePendingTasks(fusionInstruction);
          }

          if ( not globalNonblockingTasks.empty() ) {
            int numberOfTasksToProcessNow = std::max(0,static_cast<int>(globalNonblockingTasks.size())-orchestrationStrategy->getNumberOfTasksToHoldBack());
            internal::mapPendingTasksOntoNativeTasks(numberOfTasksToProcessNow);
          }
        }
        break;
      case tarch::multicore::orchestration::Strategy::ExecutionPolicy::RunParallelAndIgnoreWithholdSubtasks:
        {
          ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, static_cast<int>(tasks.size()), true );
          native::spawnAndWaitAsTaskLoop(tasks);
          ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, -static_cast<int>(tasks.size()), true );

          // AT: ideally would record the handle of the encountering task here to correctly identify child tasks
          otterSynchroniseTasks(NULL, otter_sync_children);
        }
        break;
    }

    nestingLock.lock();
    nestedSpawnAndWaits--;
    nestingLock.free();
  }
}


void tarch::multicore::internal::configureInternalTaskQueues(int numberOfTasks) {
  localNonblockingTasks.resize(numberOfTasks);
}


void tarch::multicore::internal::copyInternalTaskQueuesOverIntoGlobalQueue() {
  tarch::multicore::Lock lock(nonblockingTasksSemaphore);

  for (int threadNumber=0; threadNumber<localNonblockingTasks.size(); threadNumber++) {
    ::tarch::logging::Statistics::getInstance().log( ThreadLocalPendingTasksStatisticsIdentifier, localNonblockingTasks[threadNumber].size() );
  }

  for (auto& p: localNonblockingTasks) {
    std::copy(
      p.begin(),
      p.end(),
      std::back_inserter(globalNonblockingTasks)
    );
    p.clear();
 }
}


int tarch::multicore::internal::getNumberOfWithholdPendingTasks() {
  return globalNonblockingTasks.size();
}


#ifndef SharedMemoryParallelisation

#include <thread>

void tarch::multicore::native::spawnTask(Task*  job) {
  bool reschedule = job->run();
  if (reschedule) {
    tarch::multicore::spawnTask(job);
  }
  else {
    delete job;
  }
}


void tarch::multicore::native::spawnAndWait(
  const std::vector< Task* >&  tasks,
  orchestration::Strategy*     strategy
) {
  for (auto& p: tasks) {
    while (p->run()) {}
    delete p;
  }
}


int tarch::multicore::internal::getNumberOfWithholdPendingTasks() {
  return 0;
}


void tarch::multicore::native::processFusedTask(Task*  myTask, const std::list< tarch::multicore::Task* >& tasksOfSameType, int device) {
  bool stillExecuteLocally = myTask->fuse(tasksOfSameType,device);
  if (stillExecuteLocally) {
    tarch::multicore::native::spawnTask(myTask);
  }
  else {
    delete myTask;
  }
}

#endif
