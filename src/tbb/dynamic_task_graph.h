// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org. It
// supplements something to oneTBB which has been removed and which I need:
// dynamic tasking. Therefore I add it to the tbb namespace rather than
// Peano's tarch.
#pragma once



#include <functional>
#include <set>
#include <tbb/task_arena.h>
#include <oneapi/tbb/task_group.h>
#include <oneapi/tbb/spin_mutex.h>
#include <oneapi/tbb/spin_rw_mutex.h>
#include <oneapi/tbb/concurrent_unordered_set.h>


namespace tbb {
  class dynamic_task_graph_node;
  class dynamic_task_graph_spawned_node;
  class dynamic_task_graph;
  class dynamic_task_graph_node_repository;
}


/**
 * @page tbb_extension TBB extensions
 *
 * TBB's current release version lacks support for two important features
 * that we need in several extensions built on top of Peano:
 *
 * - ranges of arbitrary dimension which facilitate/prioritise vectorisation; and
 * - dynamic task graphs
 *
 * We add both features through a "manual" tbb add-on. We also try to bring
 * those changes into the UXL foundation standard, but it is not clear when
 * these attempts will be successful.
 *
 * Therefore, this extension is always shipped with Peano, and it is
 * automatically built and added once you translate Peano with
 * ``--with-multithreading=tbb_extension``. The outcome will end
 * up in a library called tbb_extension.a, tbb_extension_debug.a or
 * tbb_extension_trace.a. This means that you have to add
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * --with-multithreading=tbb_extension LDFLAGS=-Lsrc/tbb LIBS=-ltbb_extension
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * to your build. Peano's autotools configuration will do this for you.
 *
 *
 * ## Compatibility
 *
 * You can also enable certain features individually. To do so, you have to
 * compile with tbb_exension, but then you can manually (via -D flags) define
 *
 * - TBB_USE_TASK_GROUP_PREVIEW to try out TBB's dynamic task group handling
 * - TBB_PREVIEW_BLOCKED_RANGE_ND to try out TBB's n-dimensional ranges.
 *
 *
 * ## Code usage
 *
 * Users create an instance of dynamic_task_graph. After that, they
 * create an arbitrary number of objects of type dynamic_task_graph_node.
 * They have in-dependencies, i.e. each instance of dynamic_task_graph_node
 * can have many other instances of dynamic_task_graph_node which
 * feed into it and have to be completed before it starts to run.
 *
 * Once we submit a node to the task graph through put(), it becomes itself invariant,
 * i.e. you cannot add further dependencies anymore. When
 * it is invariant, you can still copy the task node around or make it
 * feed into other tasks. You can also
 * forget about it. The underlying task is submitted and therefore will
 * stay alive.
 *
 *
 * ## Code design
 *
 * The instances of dynamic_task_graph_node form a DAG with reverse
 * dependencies, i.e. each object holds a set of tasks it depends upon.
 *
 * The put() submits the task. Internally, the submission process now
 * builds up an object graph over instances of dynamic_task_graph_spawned_node.
 * The spawned nodes invert the dependencies: We construct the task graph
 * over in-dependencies. When we submit, these in-dependencies are translated
 * into out-dependencies, i.e. each task knows now exactly which follow-up
 * tasks to inform upon completion.
 *
 * The rationale here is simple: Once a task finishes, we want to inform
 * all follow-up tasks that this dependency is cleared. If all in-dependencies
 * are done, the task is ready and can be released into oneTBB's scheduler.
 * We never want to poll the tasks to see which ones are ready to go.
 *
 *
 * ## Implementation and memory management
 *
 * Each dynamic_task_graph_node on the user side is attached a
 * dynamic_task_graph_spawned_node once it is submitted. It is important to
 * host these submitted task graph nodes on the heap, as they need
 * to host a mutex. So we use a shared pointer here (as we can also copy
 * the user-facing task descriptions).
 *
 * Once we put a node into the graph, the dynamic_task_graph instance also
 * memorises the shared pointer to the underlying dynamic_task_graph_spawned_node
 * object. Therefore, even if the user forgets about all the user-facing
 * instances of dynamic_task_graph_node, dynamic_task_graph will still keep
 * the actually submitted nodes alive.
 *
 * Submitted task graph might change their status quo into COMPLETE. In this
 * case, we still hold them in the task graph until our garbage collection is
 * kicked off and eliminates those entries to completed tasks. As we work with
 * smart pointers, this is likley the point where the object is actually
 * removed from the heap.
 *
 * The task graph also holds a set of submitted tasks which are still alive or
 * have completed yet not been removed. In return, some completed tasks might
 * literally been removed from the set of tasks already. Nevertheless, users
 * might still hold the task objects and define in-dependencies over them.
 * These are trivially already fulfilled. In this case, we don't even insert
 * out-dependencies in put() anymore, but we have to check if the in-tasks
 * referred to are still among our active tasks.
 *
 */
namespace tbb {
  /**
   * Very simple dynamic task graph interface
   *
   * Each task graph is tied to an arena and maintains a set of graphs nodes
   * aka tasks with dependencies between them. Immediately after tasks become
   * ready, the task graph spawns them into the arena.
   *
   * ## Implementation
   *
   * The task graph holds a big hash map mapping individual task numbers of
   * type dynamic_task_graph_node_number onto dynamic_task_graph_node which
   * in turn hold the actual task graph. The entries are added to the list
   * when we construct a node. They are removed after the ask has terminated.
   *
   * @author Tobias Weinzierl
   */
  class dynamic_task_graph {
      public:
        friend class dynamic_task_graph_spawned_node;

        /**
         * Construct task graph over arena
         *
         * @param arena Arena to be used once the task becomes ready. It is the
         *   user's responsibility to ensure that the arena is valid.
         */
        dynamic_task_graph(
          ::oneapi::tbb::task_group&      arena
        );

        /**
         * Put a new node into the task graph
         *
         * The ownership of the node is passed over to the task graph. You
         * don't have to delete it yourself. This is different to oneAPI's
         * flow graph, where we can work with references, as we rely on the
         * fact that nodes stay alive until we call wait_for_all. Here,
         * everything is dynamic and the task might or might not be executed
         * immediately or be referenced later on.
         *
         * Once you have submitted a node, it becomes read-only, i.e. you
         * cannot add any further in-dependencies to it. But you can still copy
         * it or you can even delete the object. The ownership of the
         * underlying "real" task now resides within the dynamic_task_graph
         * object.
         *
         * Internally, the put will create a second object of the type
         * dynamic_task_graph_spawned_node, and then it will translate all
         * dependencies into links between instances of dynamic_task_graph_spawned_node.
         * However, the directions here are inverted: Users model their task
         * dependencies as in-dependencies. When they submit tasks, the
         * relations are translated into out-dependencies, i.e. each task
         * knows which tasks are follow-up tasks and need to be notified once
         * we are ready. We translate in-dependencies into a forward-flow model.
         *
         * As everything is dynamic here, it could be that we refer with an
         * in-dependency to a task that's already completed. It might even
         * have been removed completely from the set of spawned tasks. In this
         * case, we can safely ignore these in-dependency. We do not even have
         * to increment the dependency counter.
         *
         *
         * ## Implementation
         *
         * 1. Create a new spawned task. This is a shared pointer.
         * 2. Loop over all in-dependencies. For each one for which we find an
         *    entry in the graph, first of all lock it. If it has already
         *    terminated, unlock the semaphore and delete it. This is kind of
         *    an on-the-fly garbage collection. Otherwise, the task obviously
         *    has not yet finished, so we increase the number of our
         *    in-dependencies to mark the new task has to complete before we
         *    start ourselves, and we add ourselves to the other task's
         *    out dependencies.
         * 3. Insert task into set of pending tasks.
         * 4. Release semaphore on new task, so all other tasks which feed into
         *    this one can inform it about their termination from now on.
         * 5. Reduce the task's in-counter by one. The constructor of
         *    dynamic_task_graph_spawned_node always increments this counter
         *    artificially by one to avoid that it is released too soon.
         *
         * We always have to increment the task dependency counter before we
         * actually insert the new task, as the predecessor migth literally
         * just now be about to finish and hence immediately unlock our own
         * task while we still construct its input dependencies. By incrementing
         * first, we are on the safe side that our task will not gallop ahead.
         *
         */
        void put(dynamic_task_graph_node& node );

        /**
         * Wait for one specific node
         *
         * ## Realisation
         *
         * Very simplistic implementation: We know that the task has been
         * submitted, so its spawned task's pointer should be in the repo.
         * If it is not there (anymore), it has finished. It also could
         * still be in there yet have the state complete. So we run into a
         * while loop over the repo. As long as the task is still bookmarked
         * there, we yield.
         *
         * @param node Task to wait for. This task has to be submitted.
         *   Otherwise, we trigger an assertion.
         *
         * @see wait_for_all()
         */
        void wait( dynamic_task_graph_node node );

        /**
         * Wait for set of nodes
         */
        void wait( const std::set<dynamic_task_graph_node>& node );

        /**
         * Wait for all tasks to finish
         *
         * This is a brute force method which has to be used with care. It
         * strongly synchronises, and it is not clear if it can deadlock if
         * multiple threads invoke it at the same time.
         *
         * The implementation is close to trivial:
         *
         * - We hand over the task responsibility to the task group, so the
         *   task group can process some of its ready tasks.
         * - We remove all of those tasks which are now flagged as ready.
         * - We jump back to the start (iteration) if there are still pending
         *   tasks in the queue.
         *
         * It seems that wait() on a task group returns immediately if there
         * are no further tasks pending in there. This is very reasonable yet
         * might mean that there are still tasks being processed at this point.
         * They might in return not have released any successor tasks, i.e.
         * there might still be some tasks in the queue that are marked as
         * submitted and have not yet been released as their in dependency
         * counter has not been decreased yet. Therefore, we have to loop.
         *
         */
        void wait_for_all();

        /**
         * Number of tasks in graph
         *
         * This includes all running and pending tasks.
         */
        std::size_t size() const;
      private:
        ::oneapi::tbb::task_group&      _task_group;

        /**
         * I originally used
         * 
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~
         * using SpawnedTaskMutex = ::oneapi::tbb::spin_mutex;
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~
         * 
         * but it is absolutely crucial that the lock for the
         * spawned tasks does not become a bottleneck.
         */
        using SpawnedTaskMutex = ::oneapi::tbb::spin_rw_mutex;
        SpawnedTaskMutex       _spawned_task_mutex;

        /**
         * See documentation of dynamic_task_graph_spawned_node for a
         * discussion why we have to use smart pointers here.
         * 
         * Just to reiterate the core message why we need this: A user
         * creates an instance of a task, i.e. dynamic_task_graph_node.
         * Once submitted, this node holds a (smart) pointer to a task,
         * and then is allowed to throw the node away. So we have to 
         * safe the submitted task!
         */
        using SpawnedTasksContainer = std::set< std::shared_ptr<dynamic_task_graph_spawned_node> >;

        /**
         * Spawned tasks known to system
         */
        SpawnedTasksContainer _spawned_tasks;

        /**
         * Remove all completed task
         *
         * Almost a garbage collection over the tasks that are complete. This
         * makes follow up traversals of the _spawned_tasks container faster.
         * The routine is a helper for the waits.
         */
        void clean_up_completed_tasks();
  };
}

