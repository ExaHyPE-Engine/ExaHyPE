#include "peano4/parallel/Tasks.h"


#include "tarch/Assertions.h"
#include "tarch/multicore/Tasks.h"
#include "tarch/multicore/Lock.h"
#include "tarch/multicore/BooleanSemaphore.h"


tarch::logging::Log  peano4::parallel::Tasks::_log( "peano4::parallel::Tasks" );
int                  peano4::parallel::Tasks::_locationCounter(0);
int                  peano4::parallel::Tasks::_taskTypeCounter(0);


tarch::multicore::BooleanSemaphore peano4::parallel::Tasks::_tasksema;


int peano4::parallel::Tasks::getLocationIdentifier(const std::string&  trace) {
  logTraceInWith1Argument( "getLocationIdentifier(trace)", trace );

  tarch::multicore::Lock myLock( _tasksema );
  int result = _locationCounter;
  _locationCounter++;
  myLock.free();

  logTraceOutWith1Argument( "getLocationIdentifier(trace)", result );
  return result;
}


int peano4::parallel::Tasks::getTaskType(const std::string&  className, bool useLock) {
   int result(-1);
   if (useLock)
   {
      tarch::multicore::Lock myLock( _tasksema );
      result = _taskTypeCounter;
      _taskTypeCounter++;
      myLock.free();
   }
   else
   {
      result = _taskTypeCounter;
      _taskTypeCounter++;
   }

  return result;//_taskTypeCounter;
}


bool peano4::parallel::Tasks::taskForLocationShouldBeIssuedAsTask( int location, int taskCount ) const {
  return true;
}


int peano4::parallel::Tasks::getPriority( TaskType type ) {
  const int DefaultPriority = 65536;

  static int LIFOCounter = 0;

  switch (type) {
    case TaskType::Task:
      return DefaultPriority;
    case TaskType::HighPriority:
      return DefaultPriority * 2;
    case TaskType::LowPriorityLIFO:
      {
        if (tarch::multicore::getNumberOfPendingTasks()<32) {
          LIFOCounter = 0;
        }
        LIFOCounter++;
        return DefaultPriority / 2 + LIFOCounter;
      }
    case TaskType::LowPriorityFIFO:
      return DefaultPriority / 2;
    case TaskType::Sequential:
      assertionMsg( false, "you should not ask for a priority for a sequential task" );
      return DefaultPriority;
  }
  return -1;
}


peano4::parallel::Tasks::Tasks(
  std::function<bool ()>  function,
  TaskType                type,
  int                     location
):
  Tasks(
    new tarch::multicore::TaskWithCopyOfFunctor(-1,-location,getPriority(type),function),
    type,
    location
  ) {
}


peano4::parallel::Tasks::Tasks(
  tarch::multicore::Task*  task,
  TaskType                 type,
  int                      location
) {
  const bool parallelise = type!=TaskType::Sequential
                       and taskForLocationShouldBeIssuedAsTask( location, 1 );

  if (parallelise) {
    task->setPriority( getPriority(type) );
    tarch::multicore::spawnTask( task );
  }
  else {
    #if PeanoDebug>1
    bool reschedule = task->run();
    assertionMsg( not reschedule, "if a single task reschedules, it should go to background, i.e. not return true, as we otherwise deadlock" )
    #else
    task->run();
    #endif
    delete task;
  }
}


peano4::parallel::Tasks::Tasks(
  const std::vector< tarch::multicore::Task* >& tasks,
  TaskType                 type,
  int                      location,
  bool                     waitForCompletion
) {
  const bool parallelise = type!=TaskType::Sequential and taskForLocationShouldBeIssuedAsTask( location, tasks.size() );

  if (parallelise and waitForCompletion) {
    tarch::multicore::spawnAndWait(tasks);
  }
  else if (parallelise and not waitForCompletion) {
    const int  priority    = getPriority(type);
    for (auto& p: tasks) {
      p->setPriority( priority );
      tarch::multicore::spawnTask( p );
    }
  }
  else {
    for (auto& p: tasks) {
      #if PeanoDebug>1
      bool reschedule = p->run();
      assertionMsg( not reschedule, "if a single task reschedules, it should go to background, i.e. not return true, as we otherwise deadlock" )
      #else
      p->run();
      #endif
      delete p;
    }
  }
}
