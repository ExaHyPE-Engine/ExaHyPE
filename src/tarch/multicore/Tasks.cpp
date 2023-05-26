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

/*
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
*/

//  for (auto& task: extractedTasks) {
  for (NonblockingTasks::reverse_iterator task = extractedTasks.rbegin(); task != extractedTasks.rend(); task++) {
    tarch::multicore::native::spawnTask(*task);
  }

  return extractedTasks.size();
}


/**
 * ## Statistics
 *
 * We do log the number of thread-local tasks, but also found out that these are
 * often tiny. So if you sample it, it might not come through.
 */
bool tarch::multicore::internal::processThreadLocalTask(int threadNumber) {
  ::tarch::logging::Statistics::getInstance().log( ThreadLocalPendingTasksStatisticsIdentifier, localNonblockingTasks[threadNumber].size() );

  tarch::multicore::Task* myTask = localNonblockingTasks[threadNumber].front();
  localNonblockingTasks[threadNumber].pop_front();

  bool requeue = myTask->run();
  if (requeue)
    spawnTask( myTask );
  else
    delete myTask;

  return true;
}


/**
 * ## Algorithm:
 *
 * - Lock queue and pop first task from queue.
 * - If there is a task and task's canFuse() returns true:
 *   - As long as there are further tasks in the queue and these tasks have the same task type
 *     (an integer), append these tasks to tasksOfSameType.
 * - If there has been a task: Call native::processFusedTask() on the task and the list (which might be empty)
 *
 * ## Thread safety
 *
 * I assume that this routine is only called within an environment which has already
 * locked the queues. So I do not care about any thread safety here myself.
 *
 *
 * @return Number of merged/processed tasks
 */
bool tarch::multicore::internal::fusePendingTasks(const tarch::multicore::orchestration::Strategy::FuseInstruction&  fuseInstruction) {
  logDebug( "fusePendingTasks(int)", "process " << globalNonblockingTasks.size() << " tasks subject to " << fuseInstruction.toString() );

  ::tarch::logging::Statistics::getInstance().log( GlobalPendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks(-1) );

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

  ::tarch::logging::Statistics::getInstance().log( GlobalPendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks(-1) );

  bool  result        = false;
  while (maxTasks>0) {
    if (fifo and processOnePendingTaskFIFO() ) {
      maxTasks--;
      result = true;
    }
    else if (not fifo and processOnePendingTaskLIFO() ) {
      maxTasks--;
      result = true;
    }
    else {
      native::deferRunningThreadAndContinueWithOtherTasks();
      maxTasks=0;
    }
  }

  return result;
}


void tarch::multicore::spawnTask(Task*  task) {
  assertion( task!=nullptr );

  #ifdef UseSmartMPI
  if ( task->isSmartMPITask() and smartmpi::spawn( dynamic_cast<smartmpi::Task*>(task) )) {
    return;
  }
  #endif

  int threadNumber = Core::getInstance().getThreadNumber();
  assertion1( threadNumber>=0, threadNumber );
  assertion2( threadNumber<localNonblockingTasks.size(), threadNumber, localNonblockingTasks.size());

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
      internal::copyInternalTaskQueuesOverIntoGlobalQueue(threadNumber);
      internal::fusePendingTasks(fusionCommand);
    }
  }
}



/**
* Process a set of tasks and wait for their completion
*
* @todo stimmt so nimmer
*
* Run over a set of task and wait until they are complete. Each of these tasks
* can spawn further tasks. We do not have to wait for these guys.
*
* Logically, this is a plain task loop where we wait only for the direct
* children only. Technically, we need however something
* more sophisticated.
*
* - OpenMP implementations fail to "backfill" idling threads with OpenMP tasks
*   when we hit a taskwait, and we know that the tasks within the passed
*   container are highly ill-balanced.
* - A lot of OpenMP tools do not support OpenMP's taskloop properly.
* - The tasks within tasks spawn a lot of further tiny tasks. All OpenMP
*   implementations
*
* The strategy is thus straightforward: Normally, we map OpenMP tasks directly
* onto OpenMP tasks. If we hit this routine, we however start to buffer all
* the tiny tasks that are created by the big tasks in the container in a local
* queue. We release the tiny ones eventually after the big tasks
* have terminated. At the same time, we map the each big task onto a proper
* OpenMP tasks which checks towards the end of its lifetime whether its
* colleagues have finished, too. If this is not the case, they process a few
* of these pending tiny tasks and check again.
*
* <h2> Problem context </h2>
*
* The taskwait pragma allows the scheduler to process other tasks. This way,
* it should keep cores busy all the time. However, several groups have
* reported that this is not the case. See in particular
*
* Jones, Christopher Duncan (Fermilab): Using OpenMP for HEP Framework Algorithm Scheduling
* http://cds.cern.ch/record/2712271
*
* The presentation slides can be found at https://zenodo.org/record/3598796#.X6eVv8fgqV4.
*
* This documentation clarifies that some OpenMP runtimes do (busy) waits within
* the taskwait construct to be able to continue immediately. They do not
* process other tasks meanwhile. We want to support a variaty of runtimes -
* GCC for example is important for us due to its NVIDIA GPU support = and
* we therefore do supply the layered implementation. Also, we need a, again
* for our GPU/dynamic LB stuff, a variant where we have scheduler control.
*
* Our own ExaHyPE 2 POP review came to the same conclusion.
*
*
* <h2> Implementation </h2>
*
* I originally thought we could just work with parallel fors. However, this
* does not work, as we don't want to spawn our own parallel region here. We
* expect the overarching main loop to be a parallel region. This main things
* has to be embedded into a single/master statement, so it is only executed
* on one thread. As a result, by the time we hit spawnAndWait, we are working
* within a single environment. If we spawn parallel for within a single
* environment, OpenMP will complain. It won't work, as we operate within
* single. So I rely here a sequential loop which produces tasks.
*
* The implementation is straightforward: We use a counter that we set to the
* number of work items or number of threads; whichever is bigger. Per loop
* iteration, we produce one task. Every task decrements this counter upon completion.
* That is: each task knows towards the end of its lifetime how many other
* tasks are still up and running.
*
* @image html layered-openmp-steps.png
*
*
* ## Statistics
*
* It is important to know how many BSP sections are active at a point. So I disable
* any sampling here.
*
* @see  processPendingTasks(int)
*/
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

    switch (orchestrationStrategy->getExecutionPolicyForForkJoinSection(nestedSpawnAndWaits, tasks.size(), tasks[0]->getTaskType())) {
      case tarch::multicore::orchestration::Strategy::ExecutionPolicy::EmbedWithoutSubTasks:
        {
          for (auto& p: tasks) {
            while (p->run()) {
              native::deferRunningThreadAndContinueWithOtherTasks();
            }
            delete p;
          }
        }
        break;
      case tarch::multicore::orchestration::Strategy::ExecutionPolicy::MinimiseLatency:
        {
          ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, static_cast<int>(tasks.size()), true );
          native::spawnAndWaitAsTaskLoop(tasks);
          ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, -static_cast<int>(tasks.size()), true );
        }
        break;
      case tarch::multicore::orchestration::Strategy::ExecutionPolicy::MaximiseThroughput:
        {
          ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, static_cast<int>(tasks.size()), true );
          orchestrationStrategy->startBSPSection();
          native::spawnAndWaitAsExplicitTasksWithPolling(tasks,orchestrationStrategy);
          orchestrationStrategy->endBSPSection();
          ::tarch::logging::Statistics::getInstance().inc( BSPConcurrencyLevelStatisticsIdentifier, -static_cast<int>(tasks.size()), true );

          // AT: ideally would record the handle of the encountering task here to correctly identify child tasks
          otterSynchroniseTasks(NULL, otter_sync_children);

          internal::copyInternalTaskQueuesOverIntoGlobalQueue(-1);

          const auto fusionInstruction = orchestrationStrategy->getNumberOfTasksToFuseAndTargetDevice();
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
    }

    nestingLock.lock();
    nestedSpawnAndWaits--;
    nestingLock.free();
  }
}


bool tarch::multicore::internal::progressTasksWithinSpawnAndWaitAsExplicitTasksWithPolling(
 tarch::multicore::orchestration::Strategy*     strategy
) {
  const auto fusion       = strategy->getNumberOfTasksToFuseAndTargetDevice();
  const int  threadNumber = tarch::multicore::Core::getInstance().getThreadNumber();

  tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
  int numberOfPendingTasks = tarch::multicore::getNumberOfPendingTasks(threadNumber);

  bool processedSomeTasks = false;
  if (
    numberOfPendingTasks>0
    and
    fusion.minTasks!=std::numeric_limits<int>::max()
  ) {
    tarch::multicore::internal::copyInternalTaskQueuesOverIntoGlobalQueue(threadNumber);
    tarch::multicore::internal::fusePendingTasks(fusion);
    processedSomeTasks = true;
  }
  else if (numberOfPendingTasks>0) {
    processedSomeTasks = tarch::multicore::internal::processThreadLocalTask(threadNumber);
  }
  else if (
    tarch::multicore::getNumberOfPendingTasks(-1)>0
    and
    fusion.minTasks!=std::numeric_limits<int>::max()
  ) {
    processedSomeTasks |= tarch::multicore::internal::fusePendingTasks(fusion);
    if (not processedSomeTasks) {
      const int numberOfTasksToProcess = std::max(1,tarch::multicore::getNumberOfPendingTasks(-1)/2);
      processedSomeTasks = tarch::multicore::processPendingTasks( numberOfTasksToProcess );
    }
  }
  else if (
    tarch::multicore::getNumberOfPendingTasks(-1)>0
  ) {
    const int numberOfTasksToProcess = std::max(1,tarch::multicore::getNumberOfPendingTasks(-1)/2);
    processedSomeTasks = tarch::multicore::processPendingTasks( numberOfTasksToProcess );
  }
  else {
    assertion( tarch::multicore::getNumberOfPendingTasks(-1)==0 );

    for (int threadNumber=0; threadNumber<tarch::multicore::Core::getInstance().getThreadNumber(); threadNumber++) {
      tarch::multicore::internal::copyInternalTaskQueuesOverIntoGlobalQueue(threadNumber);
    }

    processedSomeTasks = tarch::multicore::getNumberOfPendingTasks(-1) > 0;
  }

  return processedSomeTasks;
}


void tarch::multicore::internal::configureInternalTaskQueues(int numberOfTasks) {
  localNonblockingTasks.resize(numberOfTasks);
}


void tarch::multicore::internal::copyInternalTaskQueuesOverIntoGlobalQueue(int threadNumber) {
  if (threadNumber<0) {
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
  else if ( not localNonblockingTasks[threadNumber].empty() ) {
    tarch::multicore::Lock lock(nonblockingTasksSemaphore);
    ::tarch::logging::Statistics::getInstance().log( ThreadLocalPendingTasksStatisticsIdentifier, localNonblockingTasks[threadNumber].size() );
    std::copy(
      localNonblockingTasks[threadNumber].begin(),
      localNonblockingTasks[threadNumber].end(),
      std::back_inserter(globalNonblockingTasks)
    );
    localNonblockingTasks[threadNumber].clear();
  }
}


int tarch::multicore::getNumberOfPendingTasks(int threadNumber) {
  return threadNumber<0
    ? globalNonblockingTasks.size() + tarch::multicore::native::getNumberOfPendingTasks()
    : localNonblockingTasks[threadNumber].size();
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


int tarch::multicore::native::getNumberOfPendingTasks() {
  return 0;
}


void tarch::multicore::native::deferRunningThreadAndContinueWithOtherTasks() {
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
