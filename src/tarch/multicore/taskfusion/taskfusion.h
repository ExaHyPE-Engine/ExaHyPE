 // This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


namespace tarch {
  namespace multicore {
    namespace internal {
      /**
       * Take up to maxTasks tasks that are internally buffered and throw them
       * into the native runtime
       *
       * This routine should submit the tasks LIFO, as we assume that Peano's
       * traversals produce tasks and the follow-up traversal
       * then consumes the outcome. In the Peano world, two subsequent mesh
       * traversals run through the grid in inverted order, so submitting tasks
       * LIFO makes sense once we assume that most runtimes will kind of stick
       * to the task submission order unless there are dependencies between the
       * tasks.
       */
      int mapPendingTasksOntoNativeTasks();

      /**
       *
       * The routine searches through the global task queue and tries to
       * fuse tasks. Therefore, calling the routine is kind of pointless
       * if you haven't committed the tasks to the global queue yet. In
       * all places where you invoke the routine, it is thus important
       * that you've committed any thread-local tasks to the global queue
       * if you think that fusion should be successful.
       *
       * # Thread safety
       *
       * The method locks and unlocks the queue. So don't lock it yourself.
       *
       */
      bool fusePendingTasks();
    } // namespace internal
  }
}
