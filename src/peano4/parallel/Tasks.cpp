#include "peano4/parallel/Tasks.h"


#include "tarch/Assertions.h"
#include "tarch/multicore/Tasks.h"


tarch::logging::Log  peano4::parallel::Tasks::_log( "peano4::parallel::Tasks" );
int                  peano4::parallel::Tasks::_locationCounter(0);


int peano4::parallel::Tasks::getLocationIdentifier(const std::string&  trace) {
  logTraceInWith1Argument( "getLocationIdentifier(trace)", trace );
  _locationCounter++;
  logTraceOutWith1Argument( "getLocationIdentifier(trace)", (_locationCounter-1) );
  return _locationCounter-1;
}


#if defined(TBBInvade)
#include "tarch/multicore/Core.h"


shminvade::SHMInvade*  peano4::parallel::Tasks::_backgroundTaskInvade = nullptr;
#endif


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
    new tarch::multicore::TaskWithCopyOfFunctor(-1,getPriority(type),function),
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
    bool reschedule = task->run();
    assertionMsg( not reschedule, "wenn ein einziger Task rescheduled, dann muss er immer in den Hintergrund gehen, oder das ganze Ding geht net (Deadlock)")
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
      bool reschedule = p->run();
      assertionMsg( not reschedule, "wenn ein einziger Task rescheduled, dann muss er immer in den Hintergrund gehen, oder das ganze Set an Tasks geht den Bach runter. Es muss permutieren! (Deadlock)")
      delete p;
    }
  }
}
