// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_PARALLEL_TASKS_H_
#define _PEANO4_PARALLEL_TASKS_H_


#include "tarch/logging/Log.h"
#include "tarch/multicore/Tasks.h"


#include <functional>


namespace peano4 {
  namespace parallel {
    class Tasks;
  }
}


/**
 * Task Set
 *
 * This is my very simple wrapper of task parallelism in Peano. The class
 * provides only one type of constructor. This constructor is passed a set of
 * tasks. The constructor executes all of them in parallel (if this is the
 * user's wish) and returns. Consequently, this wrapper is the right choice iff
 * all the task are totally independent of each other.
 *
 * The standard variants do not copy the task you hand in and I expect the
 * functors to be handed in as references.
 *
 * <h2> Task priorities </h2>
 *
 * Tasks in Peano can have different priorities. Within the Peano core code, I
 * don't use these priorities, but work with logical names (task types). One
 * further job of this wrapper thus is to map the logical task types onto
 * numerical priorities.
 *
 * <h2> Autotuning </h2>
 *
 * Finally, the class expects an identifier that allows this wrapper to
 * autotune. Autotuning in the present context always means `should I invoke
 * some operation as task or rather do stuff sequentially'.
 *
 * @author Tobias Weinzierl
 */
class peano4::parallel::Tasks {
  public:
    enum class TaskType {
      /**
       * Default
       */
      Task,
      HighPriority,
      LowPriority,
      /**
       * For debugging only
       */
      Sequential
    };
  private:
    static tarch::logging::Log  _log;

    static int                  _locationCounter;

    /**
     * Find out whether to launch tasks issued by location in parallel.
     */
    bool taskForLocationShouldBeIssuedAsTask( int location, int taskCount ) const;

    int getPriority( TaskType type ) const;
  public:
    /**
     * Codes create an identifier (int) per parallel region through this
     * operation. This identifier then can be used internally for autotuning.
     */
    static int getLocationIdentifier(const std::string&  trace);

    /**
     * Spawn One Task
     *
     * This routine spans a task with no wait semantics, i.e. the constructor
     * returns immediately.
     *
     * @param location          See getLocationIdentifier()
     * @param waitForCompletion
     */
    Tasks(
      std::function<bool()>  task,
      TaskType               type,
      int                    location
    );


    /**
     * Alternative to other Tasks constructor. Ownership goes to Tasks
     * class, i.e. you don't have to delete it.
     */
    Tasks(
      tarch::multicore::Task*  task,
      TaskType                 type,
      int                      location
    );


    /**
     * Task wrapper to issue a whole set of tasks in one rush. While the
     * other tasks all return immediately (no-wait semantics), this
     * constructor can either issue tasks one by one or launch them
     * with wait semantics.
     *
     * @param location From where is this constructor called from. This allows
     *   the routine to autotune, i.e. to decide whether an independent task
     *   should be launched. See getLocationIdentifier().
     */
    Tasks(
      const std::vector< tarch::multicore::Task* >& tasks,
      TaskType                 type,
      int                      location,
      bool                     waitForCompletion
    );
};


#endif
