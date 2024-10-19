// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org.
#pragma once


#include "config.h"


#if !defined(SharedTBBExtension) and !defined(TBB_USE_TASK_GROUP_PREVIEW) and defined(SharedTBB)


#include <functional>
#include <set>
#include <tbb/task_arena.h>
#include <oneapi/tbb/task_group.h>
#include <oneapi/tbb/task_arena.h>
#include <oneapi/tbb/concurrent_hash_map.h>
#include <oneapi/tbb/concurrent_queue.h>


namespace tarch {
  namespace multicore {
    namespace tbb {
      class TaskHandleRepository;
    }
  }
}

/**
 * Simple utility class around dynamic task graphs to work with integer task numbers
 *
 * It handles the management of the nodes, such that users can work with
 * integer task numbers instead. The only interesting thing here is that
 * users can obviously "overwrite" tasks, i.e. create a new task with the
 * same number over and over again. Usually, the newest task is the ruling
 * one, i.e. hides all previous ones. However, when you add a task dependency
 * for a task to itself, this bookkeeping interprets this as: Hey, I have a
 * task with number A and now create a new task with number A, but that one
 * should only run once the first one has completed.
 */
class tarch::multicore::tbb::TaskHandleRepository {
    public:
      using task_handle_number = int;

      TaskHandleRepository();
      ~TaskHandleRepository();
      /**
       * Factor mechanism
       *
       * Gives you a new node. If there's already a task with that number,
       * then this task is backed up in the data structure 
       * _shadowed_node_container. This way, we can add in-out dependencies
       * (the task with the same number is an in-dependency for the newly
       * created task). To add this dependency is the job of the user, i.e.
       * this routine just ensures that the backup/shadow container is 
       * properly befilled.
       */
      oneapi::tbb::task_handle& include_handle(
        oneapi::tbb::task_handle&&      handle,
        task_handle_number              number
      );

      oneapi::tbb::task_handle& get_node(const task_handle_number& number);

      /**
       * Alternative to number-based routine
       *
       * This routine is slightly less powerful, as you cannot create any links
       * to previous tasks with the same number: By the time you have created a
       * task with a number that you pipe in here with new_task_node, you have
       * no access to previous tasks which did carry the same number.
       */
      void add_dependency( oneapi::tbb::task_handle&  new_task_node, const task_handle_number&  in_dependency);

      /**
       * Add dependency
       *
       * If in_dependency equals new_task_node, then we assume that it refers
       * to a previously submitted task with the same number. If no such task
       * is known, the add_dependency creates an anti-dependency. We simply
       * ignore it.
       */
      void add_dependency( const task_handle_number&  new_task_node, const task_handle_number&  in_dependency);

      /**
       * Wrapper around multiple add_dependency() calls
       */
      void add_dependencies( oneapi::tbb::task_handle&  new_task_node, const std::set< task_handle_number >&  in_dependencies );
      void add_dependencies( const task_handle_number&  new_task_node, const std::set< task_handle_number >&  in_dependencies );

    private:
      using HashMap = oneapi::tbb::concurrent_hash_map<task_handle_number, oneapi::tbb::task_handle*>;
      HashMap  _handle_container;

      /**
       * Holds those nodes which have been reused already.
       */
      HashMap  _shadowed_handle_container;
  };
}

#endif
