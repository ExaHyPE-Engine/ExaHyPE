#include "tarch/Assertions.h"
#include "tarch/logging/Statistics.h"

#include "BooleanSemaphore.h"
#include "Lock.h"
#include "Core.h"

#include <thread>
#include <queue>
#include <set>


#include "Tasks.h"
#include "multicore.h"

#include "tarch/multicore/orchestration/StrategyFactory.h"


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

  NonblockingTasks                     nonblockingTasks;
  tarch::multicore::BooleanSemaphore   nonblockingTasksSemaphore;

  tarch::multicore::orchestration::Strategy* orchestrationStrategy = tarch::multicore::orchestration::createDefaultStrategy();

  tarch::logging::Log _log( "tarch::multicore" );

  const std::string PendingTasksStatisticsIdentifier( "tarch::multicore::pending-tasks" );
  const std::string FuseTasksStatisticsIdentifier( "tarch::multicore::fuse-tasks");
  const std::string BSPTasksStatisticsIdentifier( "tarch::multicore::bsp-tasks");

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
    if (not nonblockingTasks.empty()) {
      myTask = nonblockingTasks.back();
      nonblockingTasks.pop_back();
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
    if (not nonblockingTasks.empty()) {
      myTask = nonblockingTasks.front();
      nonblockingTasks.pop_front();
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
    if (not nonblockingTasks.empty()) {
      myTask = nonblockingTasks.front();
      nonblockingTasks.pop_front();
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

    NonblockingTasks::iterator cutIteration = nonblockingTasks.begin();
    while (
      cutIteration!=nonblockingTasks.end()
      and
      maxTasks>0
    ) {
      maxTasks--;
      cutIteration++;
    }

    extractedTasks.splice( extractedTasks.begin(), nonblockingTasks, nonblockingTasks.begin(), cutIteration );
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
    
  NonblockingTasks::iterator cutIteration = nonblockingTasks.begin();
  while (
    cutIteration!=nonblockingTasks.end()
    and
    maxTasks>0
  ) {
    maxTasks--;
    cutIteration++;
  }
    
  extractedTasks.splice( extractedTasks.begin(), nonblockingTasks, nonblockingTasks.begin(), cutIteration );
  lock.free();

  for (auto& task: extractedTasks) {
    tarch::multicore::native::spawnTask(task);
  }

  return extractedTasks.size();
}


  /**
   * ## Algorithm:
   *
   * - Lock queue and pop first task from queue.
   * - If there is a task and task's canFuse() returns true:
   *   - As long as there are further tasks in the queue and these tasks have the same task type
   *     (an integer), append these tasks to tasksOfSameType.
   * - If there has been a task and merged task list is not empty:
   *   - Call fuse on this list
   *   - Check if fuse still requires us to run prototype (first) task explicitly
   *   - Execute this task if required
   * - If there has been a task and merged task list is empty:
   *   - Execute the one task extracted from task queue
   *
   * ## Thread safety
   *
   * I assume that this routine is only called within an environment which has already
   * locked the queues. So I do not care about any thread safety here myself.
   *
   *
   * @return Number of merged/processed tasks
   */
bool tarch::multicore::internal::fusePendingTasks(std::pair<int,int> maxTasksAndDevice) {
  logDebug( "fusePendingTasks(int)", "fuse up to " << nonblockingTasks.size() << " tasks (max=" << maxTasksAndDevice.first << ")" );

  tarch::multicore::Task* myTask = nullptr;
  std::list< tarch::multicore::Task* > tasksOfSameType;

  if (not nonblockingTasks.empty()) {
    myTask = nonblockingTasks.front();
    nonblockingTasks.pop_front();
  }

  auto pp = nonblockingTasks.begin();
  while (
    myTask!=nullptr
    and
    myTask->canFuse()
    and
    pp!=nonblockingTasks.end()
    and
    (*pp)->getTaskType()==myTask->getTaskType()
    and
    tasksOfSameType.size()<maxTasksAndDevice.first-1
  ) {
    tasksOfSameType.push_back( *pp );
    pp = nonblockingTasks.erase(pp);
  }

  ::tarch::logging::Statistics::getInstance().log( FuseTasksStatisticsIdentifier, tasksOfSameType.size() );

  if (myTask!=nullptr) {
    bool stillExecuteLocally;
    if (tasksOfSameType.empty()) {
      stillExecuteLocally = true;
    }
    else {
      stillExecuteLocally = myTask->fuse(tasksOfSameType,maxTasksAndDevice.second);
    }
    if (stillExecuteLocally) {
      tarch::multicore::native::spawnTask(myTask);
    }
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


tarch::multicore::Task::Task( int id, int taskType, int priority ):
  _id(id),
  _taskType( taskType ),
  _priority( priority ) {
}


bool tarch::multicore::Task::canFuse() const {
  return false;
}


bool tarch::multicore::Task::isSmartMPITask() const {
  return false;
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
  for (auto pp: otherTasks) {
    tarch::multicore::Task* currentTask = pp;
    while (currentTask->run()) {}
    delete currentTask;
  }
  return true;
}


int tarch::multicore::Task::getTaskId() const {
  return _id;
}


bool tarch::multicore::Task::split(std::list<tarch::multicore::Task*>&) {
  return true;
}


bool tarch::multicore::Task::canSplit() const {
  return false;
}


tarch::multicore::TaskWithCopyOfFunctor::TaskWithCopyOfFunctor( int id, int taskType, int priority, const std::function<bool()>& taskFunctor ):
  Task(id, taskType, priority),
  _taskFunctor(taskFunctor)  {
}


bool tarch::multicore::TaskWithCopyOfFunctor::run() {
  return _taskFunctor();
}


tarch::multicore::TaskWithoutCopyOfFunctor::TaskWithoutCopyOfFunctor( int id, int taskType, int priority, std::function<bool()>& taskFunctor ):
  Task(id, taskType, priority),
  _taskFunctor(taskFunctor)  {
}


bool tarch::multicore::TaskWithoutCopyOfFunctor::run() {
  return _taskFunctor();
}


bool tarch::multicore::processPendingTasks(int maxTasks, bool fifo) {
  assertion(maxTasks>=0);

  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );

  #ifdef UseSmartMPI
  smartmpi::tock();
  #endif

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
      maxTasks=0;
    }
  }

  if (not result) {
    native::yield();
  }

  return result;
}


void tarch::multicore::spawnTask(Task*  task) {
  assertion( task!=nullptr );

  #ifdef UseSmartMPI
  if ( task->isSmartMPITask() ) {
    smartmpi::spawn( dynamic_cast<smartmpi::Task*>(task) );
    return;
  }
  #endif

  if ( nonblockingTasks.size()>=orchestrationStrategy->getNumberOfTasksToHoldBack() ) {
    logDebug( "spawnTasks(int)", "spawn native task" );
    native::spawnTask(task);
  }
  else {
    tarch::multicore::Lock lock(nonblockingTasksSemaphore);
    nonblockingTasks.push_back(task);

    logDebug( "spawnTask(...)", "enqueued task (#tasks=" << nonblockingTasks.size() << ")" );

    auto fusionCommand = orchestrationStrategy->getNumberOfTasksToFuseAndTargetDevice();
    if (
      fusionCommand.first>0
      and
      nonblockingTasks.size()>=fusionCommand.first
      and
      orchestrationStrategy->fuseTasksImmediatelyWhenSpawned()
    ) {
      internal::fusePendingTasks(fusionCommand);
    }
  }

  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );
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
* @see  processPendingTasks(int)
*/
void tarch::multicore::spawnAndWait(
  const std::vector< Task* >&  tasks
) {
  static tarch::logging::Log _log( "tarch::multicore" );

  if (not tasks.empty()) {
    orchestrationStrategy->startBSPSection();
    native::spawnAndWait(tasks,orchestrationStrategy);
    orchestrationStrategy->endBSPSection();

    if ( not nonblockingTasks.empty() ) {
      int numberOfTasksToProcessNow = std::max(0,static_cast<int>(nonblockingTasks.size())-orchestrationStrategy->getNumberOfTasksToHoldBack());
      internal::mapPendingTasksOntoNativeTasks(numberOfTasksToProcessNow);
    }
  }
}


void tarch::multicore::yield() {
  tarch::multicore::native::yield();
}


int tarch::multicore::getNumberOfPendingTasks() {
  return nonblockingTasks.size() + tarch::multicore::native::getNumberOfPendingTasks();
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


/**
 * This has to be the standard C++ implementation. Even if multithreading is
 * disabled, the code might logically run with multiple threads, and then we
 * have to switch the thread context from time to time.
 */
void tarch::multicore::native::yield() {
  std::this_thread::yield();
}


int tarch::multicore::native::getNumberOfPendingTasks() {
  return 0;
}

#endif
