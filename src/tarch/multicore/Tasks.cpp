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


namespace {
  tarch::multicore::BooleanSemaphore  activeTasksSemaphore;
  std::set<int>                       activeTaskNumbers;

  /**
   * Temporary task queue which is used if and only if we hold back tasks
   */
  std::list< tarch::multicore::Task* > nonblockingTasks;
  tarch::multicore::BooleanSemaphore   nonblockingTasksSemaphore;

  tarch::multicore::Realisation realisation = tarch::multicore::Realisation::MapOntoNativeTasks;

  /**
   * Determines how to handle the tasks dumped into nonblockingTasks.
   *
   * @see tarch::multicore::spawnAndWait()
   */
  enum class TaskProgressionStrategy {
    /**
     * Put stuff into queue if it is spawned. If we progress, grab the first one and
     * handle it.
     */
    BufferInQueue,
    /**
     * Immediately spawn as a new task
     */
    MapOntoNativeTask,
    /**
     * If we spawn a task, put it into a queue and return. If we progress, look at
     * the queue. If it is big enough, fuse the tasks. Otherwise, process the first
     * task from the queue.
     */
    MergeTasks
  };

  /**
   * This flag/strategy is toggled by tarch::multicore::spawnAndWait(). The
   * flag determines how
   * tarch::multicore::processPendingTasks(int) processes the tasks, and it
   * also controls how spawnTask() works.
   *
   * @see tarch::multicore::spawnAndWait()
   * @see tarch::multicore::processPendingTasks(int)
   */
  TaskProgressionStrategy  taskProgressionStrategy = TaskProgressionStrategy::MapOntoNativeTask;

  const std::string PendingTasksStatisticsIdentifier( "tarch::multicore::pending-tasks" );
  const std::string MergeTasksStatisticsIdentifier( "tarch::multicore::merge-tasks");

  int numberOfTasksThatShouldBeFused  = std::numeric_limits<int>::max();
  int maxNumberOfFusedTasksAssemblies = std::numeric_limits<int>::max();
  int numberOfFusedTasksAssemblies    = 0;

  /**
   * @return -1 if nothing found, otherwise task id
   */
  int processOnePendingTaskLIFO() {
    tarch::multicore::Task* myTask = nullptr;

    tarch::multicore::Lock lock(nonblockingTasksSemaphore);
    if (not nonblockingTasks.empty()) {
      myTask = nonblockingTasks.back();
      nonblockingTasks.pop_back();
    }
    lock.free();

    int result = -1;
    if (myTask!=nullptr) {
      result = myTask->getTaskId();
      bool requeue = myTask->run();
      if (requeue)
        spawnTask( myTask );
      else
        delete myTask;
    }
    return result;
  }


  bool processOnePendingTaskFIFO() {
    tarch::multicore::Task* myTask = nullptr;

    tarch::multicore::Lock lock(nonblockingTasksSemaphore);
    if (not nonblockingTasks.empty()) {
      myTask = nonblockingTasks.front();
      nonblockingTasks.pop_front();
    }
    lock.free();

    int result = -1;
    if (myTask!=nullptr) {
      result = myTask->getTaskId();
      bool requeue = myTask->run();
      if (requeue)
        spawnTask( myTask );
      else
        delete myTask;
    }
    return result;
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


  bool fusePendingTasks(int maxTasks) {
    tarch::multicore::Task* myTask = nullptr;
    std::list< tarch::multicore::Task* > tasksOfSameType;

    tarch::multicore::Lock lock(nonblockingTasksSemaphore);
    if (not nonblockingTasks.empty()) {
      myTask = nonblockingTasks.front();
      nonblockingTasks.pop_front();
    }

    auto pp = nonblockingTasks.begin();
    while (
      myTask->canFuse()
      and
      pp!=nonblockingTasks.end()
      and
      (*pp)->getTaskType()==myTask->getTaskType()
      and
      tasksOfSameType.size()<maxTasks
    ) {
      tasksOfSameType.push_back( *pp );
      pp = nonblockingTasks.erase(pp);
    }
    lock.free();

    ::tarch::logging::Statistics::getInstance().log( MergeTasksStatisticsIdentifier, tasksOfSameType.size() );

    if (myTask!=nullptr) {
      bool stillExecuteLocally;
      if (tasksOfSameType.empty()) {
        stillExecuteLocally = true;
      }
      else {
        stillExecuteLocally = myTask->fuse(tasksOfSameType);
        numberOfFusedTasksAssemblies++;
      }
      if (stillExecuteLocally) {
        tarch::multicore::native::spawnTask(myTask);
      }
    }

    return myTask!=nullptr;
  }
}


void tarch::multicore::configureTaskFusion( int maxNumberOfFusedAssemblies, int maxSizeOfFusedTaskSet ) {
  numberOfTasksThatShouldBeFused  = maxSizeOfFusedTaskSet;
  maxNumberOfFusedTasksAssemblies = maxNumberOfFusedAssemblies;
}


void tarch::multicore::setRealisation( Realisation realisation_ ) {
  realisation = realisation_;
}


tarch::multicore::Realisation tarch::multicore::getRealisation() {
  return realisation;
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


int tarch::multicore::Task::getPriority() const {
  return _priority;
}


void tarch::multicore::Task::setPriority( int priority ) {
  _priority = priority;
}


int tarch::multicore::Task::getTaskType() const {
  return _taskType;
}


bool tarch::multicore::Task::fuse( const std::list<Task*>& otherTasks ) {
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



/**
* Process a few tasks from my backlog of tasks
*
* This routine loops through the pending tasks until either maxTasks have been
* processed (we decrement this counter by one per loop iteration) or no tasks
* are left anymore. In the latter case, we set maxTasks to 0 and thus terminate
* the loop.
*
* Both the way we pick tasks from the queue and extract them from there and the
* way we process the tasks depends on the current state of our task processing
* strategy. This is encoded via the enum taskProgressionStrategy.
*/
bool tarch::multicore::processPendingTasks(int maxTasks) {
  assertion(maxTasks>=0);

  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );

  bool  result        = false;
  while (maxTasks>0) {
    bool handledATask = false;
    switch (taskProgressionStrategy) {
      case TaskProgressionStrategy::BufferInQueue:
        handledATask = processOnePendingTaskFIFO();
        break;
      case TaskProgressionStrategy::MapOntoNativeTask:
        handledATask = mapOnePendingTaskOntoNativeTask();
        break;
      case TaskProgressionStrategy::MergeTasks:
        if (
          numberOfFusedTasksAssemblies < maxNumberOfFusedTasksAssemblies
          and
          nonblockingTasks.size()>numberOfTasksThatShouldBeFused
        ) {
          handledATask =  fusePendingTasks(numberOfTasksThatShouldBeFused);
        }
        else {
          handledATask = processOnePendingTaskFIFO();
        }
        break;
    }

    if (handledATask) {
      maxTasks--;
      result = true;
    }
    else maxTasks=0;
  }

  return result;
}


/**
* Spawns a single task in a non-blocking fashion
*
* @see tarch::multicore::spawnAndWait()
* @see processPendingTasks(int)
*/
void tarch::multicore::spawnTask(Task*  task) {
  switch (taskProgressionStrategy) {
    case TaskProgressionStrategy::MapOntoNativeTask:
      native::spawnTask(task);
      break;
    case TaskProgressionStrategy::BufferInQueue:
    case TaskProgressionStrategy::MergeTasks:
    {
      tarch::multicore::Lock lock(nonblockingTasksSemaphore);
      nonblockingTasks.push_back(task);
    }
    break;
  }

  ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );
}



/**
* Process a set of tasks and wait for their completion
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
  if (not tasks.empty()) {
    switch (realisation) {
      case Realisation::MapOntoNativeTasks:
        taskProgressionStrategy = TaskProgressionStrategy::MapOntoNativeTask;
        break;
      case Realisation::HoldTasksBackInLocalQueue:
      case Realisation::HoldTasksBackInLocalQueueAndBackfill:
      case Realisation::HoldTasksBackInLocalQueueBackfillAndRelease:
        taskProgressionStrategy = TaskProgressionStrategy::BufferInQueue;
        break;
      case Realisation::HoldTasksBackInLocalQueueMergeBackfillAndRelease:
        numberOfFusedTasksAssemblies = 0;
        taskProgressionStrategy      = TaskProgressionStrategy::MergeTasks;
        break;
    }

    native::spawnAndWait(tasks);

    switch (realisation) {
      case Realisation::MapOntoNativeTasks:
        assertion(nonblockingTasks.empty());
        break;
      case Realisation::HoldTasksBackInLocalQueue:
      case Realisation::HoldTasksBackInLocalQueueAndBackfill:
        {
          taskProgressionStrategy = TaskProgressionStrategy::BufferInQueue;
          while (not nonblockingTasks.empty()) {
            tarch::multicore::processPendingTasks( nonblockingTasks.size() );
          }
        }
      break;
      case Realisation::HoldTasksBackInLocalQueueBackfillAndRelease:
        {
          taskProgressionStrategy = TaskProgressionStrategy::MapOntoNativeTask;
          while (not nonblockingTasks.empty()) {
            tarch::multicore::processPendingTasks( nonblockingTasks.size() );
          }
        }
        break;
      case Realisation::HoldTasksBackInLocalQueueMergeBackfillAndRelease:
        {
          taskProgressionStrategy = TaskProgressionStrategy::MergeTasks;
          tarch::multicore::processPendingTasks( maxNumberOfFusedTasksAssemblies-numberOfFusedTasksAssemblies );

          taskProgressionStrategy = TaskProgressionStrategy::MapOntoNativeTask;
          while (not nonblockingTasks.empty()) {
            tarch::multicore::processPendingTasks( nonblockingTasks.size() );
          }
        }
        break;
    }
  }

  taskProgressionStrategy = TaskProgressionStrategy::MapOntoNativeTask;
}


void tarch::multicore::yield() {
  tarch::multicore::native::yield();
}


/**
* Process one particular task
*
* If we search for a particular task, we typically should search
* LIFO. Therefore, I change the general processing pattern and
* continue.
*/
bool tarch::multicore::processTask(int number) {
  int lastTaskProcessed = -1;
  assertion(number>=0);
  while (
    not nonblockingTasks.empty()
    and
    lastTaskProcessed!=number
  ) {
    lastTaskProcessed = processOnePendingTaskLIFO();
  }
  return lastTaskProcessed==number;
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
  const std::vector< Task* >&  tasks
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
