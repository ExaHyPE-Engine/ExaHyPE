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

  tarch::multicore::Realisation realisation = tarch::multicore::Realisation::HoldTasksBackInLocalQueueMergeAndBackfill;

  tarch::logging::Log _log( "tarch::multicore" );

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
  const std::string BSPTasksStatisticsIdentifier( "tarch::multicore::bsp-tasks");

  /**
   * Upper limit on number of tasks that are fused into one (meta-)task
   *
   * By default, there is no upper limit on this count.
   *
   * @see configureTaskFusion(int,int)
   */
  int numberOfTasksThatShouldBeFused  = 20;


  /**
   * Maximum number of large meta tasks that are created
   *
   * If this number is exceeded, the code will not fuse any tasks anymore
   * even though there would be fuse-able tasks around. This threshold plus
   * numberOfTasksThatShouldBeFused constrain the task fusion behaviour:
   * One limits the max number of fused (large) tasks, one limits the number
   * of tasks that feed into one meta task.
   *
   * @see numberOfTasksThatShouldBeFused
   */
  int maxNumberOfFusedTasksAssemblies = std::numeric_limits<int>::max();

  /**
   * Statistics counter
   */
  int numberOfFusedTasksAssemblies    = 0;

  /**
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


  /**
   * @return Number of processed tasks
   */
  int mapPendingTasksOntoNativeTasks(int maxTasks) {
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
   * Algorithm:
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
   * @return Number of merged/processed tasks
   */
  bool fusePendingTasks(int maxTasks) {
    tarch::multicore::Task* myTask = nullptr;
    std::list< tarch::multicore::Task* > tasksOfSameType;

    tarch::multicore::Lock lock(nonblockingTasksSemaphore);
    if (not nonblockingTasks.empty()) {
      myTask = nonblockingTasks.front();
      nonblockingTasks.pop_front();
    }

    logDebug( "fusePendingTasks(int)", "got " << nonblockingTasks.size() << " (max=" << maxTasks << ")" );

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

    return myTask==nullptr ? 0 : tasksOfSameType.size()+1;
  }
}


std::string tarch::multicore::getListOfRealisations() {
  return toString(Realisation::MapOntoNativeTasks)
       + ","
       + toString(Realisation::HoldTasksBackInLocalQueue)
       + ","
       + toString(Realisation::HoldTasksBackInLocalQueueAndEventuallyMapOntoNativeTask)
       + ","
       + toString(Realisation::HoldTasksBackInLocalQueueAndBackfill)
       + ","
       + toString(Realisation::HoldTasksBackInLocalQueueAndBackfillAndEventuallyMapOntoNativeTask)
       + ","
       + toString(Realisation::HoldTasksBackInLocalQueueMergeAndBackfill)
       + ","
       + toString(Realisation::HoldTasksBackInLocalQueueMergeAndBackfillAndEventuallyMapOntoNativeTask);
}


std::string tarch::multicore::toString( Realisation realisation ) {
  switch (realisation) {
    case Realisation::MapOntoNativeTasks:
      return "map-onto-native-tasks";
    case Realisation::HoldTasksBackInLocalQueue:
      return "hold-tasks-back-in-local-queue";
    case Realisation::HoldTasksBackInLocalQueueAndEventuallyMapOntoNativeTask:
      return "hold-tasks-back-in-local-queue-and-eventually-map-onto-native-task";
    case Realisation::HoldTasksBackInLocalQueueAndBackfill:
      return "hold-tasks-back-in-local-queue-and-backfill";
    case Realisation::HoldTasksBackInLocalQueueAndBackfillAndEventuallyMapOntoNativeTask:
      return "hold-tasks-back-in-local-queue-and-backfill-and-eventually-map-onto-native-task";
    case Realisation::HoldTasksBackInLocalQueueMergeAndBackfill:
      return "hold-tasks-back-in-local-queue-merge-and-backfill";
    case Realisation::HoldTasksBackInLocalQueueMergeAndBackfillAndEventuallyMapOntoNativeTask:
      return "hold-tasks-back-in-local-queue-merge-and-backfill-and-eventually-map-onto-native-task";
  }
  return "<undef>";
}


bool tarch::multicore::parseRealisation( const std::string& realisationString ) {
  if ( realisationString.compare( toString(Realisation::MapOntoNativeTasks) )==0 ) {
    realisation = Realisation::MapOntoNativeTasks;
  }
  else if (realisationString.compare( toString(Realisation::HoldTasksBackInLocalQueue) )==0 ) {
    realisation = Realisation::HoldTasksBackInLocalQueue;
  }
  else if (realisationString.compare( toString(Realisation::HoldTasksBackInLocalQueueAndEventuallyMapOntoNativeTask) )==0 ) {
    realisation = Realisation::HoldTasksBackInLocalQueueAndEventuallyMapOntoNativeTask;
  }
  else if (realisationString.compare( toString(Realisation::HoldTasksBackInLocalQueueAndBackfill) )==0 ) {
    realisation = Realisation::HoldTasksBackInLocalQueueAndBackfill;
  }
  else if (realisationString.compare( toString(Realisation::HoldTasksBackInLocalQueueAndBackfillAndEventuallyMapOntoNativeTask) )==0 ) {
    realisation = Realisation::HoldTasksBackInLocalQueueAndBackfillAndEventuallyMapOntoNativeTask;
  }
  else if (realisationString.compare( toString(Realisation::HoldTasksBackInLocalQueueMergeAndBackfill) )==0 ) {
    realisation = Realisation::HoldTasksBackInLocalQueueMergeAndBackfill;
  }
  else if (realisationString.compare( toString(Realisation::HoldTasksBackInLocalQueueMergeAndBackfillAndEventuallyMapOntoNativeTask) )==0 ) {
    realisation = Realisation::HoldTasksBackInLocalQueueMergeAndBackfillAndEventuallyMapOntoNativeTask;
  }
  else {
    tarch::logging::Log _log( "tarch::multicore" );
    logError( "parseRealisation(std::string)", "realisation variant " << realisationString << " not known" );
    return false;
  }
  return true;
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
    int handledTasks = 0;
    switch (taskProgressionStrategy) {
      case TaskProgressionStrategy::MapOntoNativeTask:
        handledTasks = mapPendingTasksOntoNativeTasks(maxTasks);
        break;
      case TaskProgressionStrategy::BufferInQueue:
        assertion(fifo or maxTasks==1);
        if (fifo and maxTasks==1) {
          handledTasks = processOnePendingTaskFIFO();
        }
        else if (fifo) {
          handledTasks = processPendingTasksFIFO(maxTasks);
        }
        else {
          handledTasks = processOnePendingTaskLIFO() ? 1 : 0;
        } 
        break;
      case TaskProgressionStrategy::MergeTasks:
        assertion(fifo or maxTasks==1);
        if (
          numberOfFusedTasksAssemblies < maxNumberOfFusedTasksAssemblies
          and
          nonblockingTasks.size()>numberOfTasksThatShouldBeFused
        ) {
          handledTasks = fusePendingTasks(numberOfTasksThatShouldBeFused);
        }
        else if (numberOfFusedTasksAssemblies >= maxNumberOfFusedTasksAssemblies) {
          handledTasks = processPendingTasksFIFO(maxTasks);
        }
        else {
          // we do only one, so there is a chance that more and more tasks
          // drop in and we eventually can merge
          handledTasks = processOnePendingTaskFIFO() ? 1 : 0;
        }
        break;
    }

    if (handledTasks>0) {
      maxTasks -= handledTasks;
      result = true;
    }
    else maxTasks=0;
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
  static tarch::logging::Log _log( "tarch::multicore" );

  if (not tasks.empty()) {
    if (
      tarch::multicore::Core::getInstance().getNumberOfThreads()<=1
      and
      realisation!=Realisation::HoldTasksBackInLocalQueue
    ) {
      logWarning(
        "spawnAndWait()",
        "threading model is not set to " << toString(Realisation::HoldTasksBackInLocalQueue) <<
        ". However, as we have only one core, any other threading model will deadlock. Switch model therefore"
      );
      realisation = Realisation::HoldTasksBackInLocalQueue;
    }

    switch (realisation) {
      case Realisation::MapOntoNativeTasks:
        taskProgressionStrategy = TaskProgressionStrategy::MapOntoNativeTask;
        break;
      case Realisation::HoldTasksBackInLocalQueue:
      case Realisation::HoldTasksBackInLocalQueueAndEventuallyMapOntoNativeTask:
      case Realisation::HoldTasksBackInLocalQueueAndBackfill:
      case Realisation::HoldTasksBackInLocalQueueAndBackfillAndEventuallyMapOntoNativeTask:
        taskProgressionStrategy = TaskProgressionStrategy::BufferInQueue;
        break;
      case Realisation::HoldTasksBackInLocalQueueMergeAndBackfill:
      case Realisation::HoldTasksBackInLocalQueueMergeAndBackfillAndEventuallyMapOntoNativeTask:
        numberOfFusedTasksAssemblies = 0;
        taskProgressionStrategy      = TaskProgressionStrategy::MergeTasks;
        break;
    }

    native::spawnAndWait(tasks);

    if ( tarch::multicore::Core::getInstance().getNumberOfThreads()<=1 ) {
      taskProgressionStrategy = TaskProgressionStrategy::BufferInQueue;
      tarch::multicore::processPendingTasks();
    }

    switch (realisation) {
      case Realisation::MapOntoNativeTasks:
        taskProgressionStrategy = TaskProgressionStrategy::MapOntoNativeTask;
        assertion(nonblockingTasks.empty());
        break;
      case Realisation::HoldTasksBackInLocalQueue:
      case Realisation::HoldTasksBackInLocalQueueAndBackfill:
        taskProgressionStrategy = TaskProgressionStrategy::BufferInQueue;
        ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );
        break;
      case Realisation::HoldTasksBackInLocalQueueMergeAndBackfill:
        taskProgressionStrategy = TaskProgressionStrategy::MergeTasks;
        tarch::multicore::processPendingTasks( maxNumberOfFusedTasksAssemblies-numberOfFusedTasksAssemblies );
        taskProgressionStrategy = TaskProgressionStrategy::BufferInQueue;
        ::tarch::logging::Statistics::getInstance().log( PendingTasksStatisticsIdentifier, tarch::multicore::getNumberOfPendingTasks() );
        break;
      case Realisation::HoldTasksBackInLocalQueueAndEventuallyMapOntoNativeTask:
      case Realisation::HoldTasksBackInLocalQueueAndBackfillAndEventuallyMapOntoNativeTask:
        taskProgressionStrategy = TaskProgressionStrategy::MapOntoNativeTask;
        tarch::multicore::processPendingTasks();
        break;
      case Realisation::HoldTasksBackInLocalQueueMergeAndBackfillAndEventuallyMapOntoNativeTask:
        taskProgressionStrategy = TaskProgressionStrategy::MergeTasks;
        tarch::multicore::processPendingTasks( maxNumberOfFusedTasksAssemblies-numberOfFusedTasksAssemblies );
        taskProgressionStrategy = TaskProgressionStrategy::MapOntoNativeTask;
        tarch::multicore::processPendingTasks();
        break;
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
