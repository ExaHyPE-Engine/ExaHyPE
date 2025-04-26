// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch {
  namespace multicore {
    /**
     * Forward declaration
     */
    class Task;

    namespace taskfusion {
      /**
       * Translate a single task into a sequence of (fusable tasks)
       *
       * First ensure that there's a queue for this task type, and then
       * insert the task into the queue.
       */
      void translateFusableTaskIntoTaskSequence(Task* task);

      /**
       * If there was no decision on when to fuse tasks in translateFusableTaskIntoTaskSequence()
       * based on a chosen strategy, we fuse all tasks that are still pending.
       */
      void processPendingTasksThatCanBeFused();
    } // namespace taskfusion
  } // namespace multicore
} // namespace tarch
