// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


namespace tarch {
  namespace multicore {
    namespace omp {
      /**
       * Grow the memory region used for the dependency tracking
       *
       * Addresses that represent dependencies in OpenMP have to correspond to
       * allocated, user-owned memory. To ensure this is the case, I hold one
       * big memory block for all the dependencies.
       *
       * Unfortunately, I do not know how big this memory block has to be. So I
       * use any check if an out condition can be served as an
       * opportunity to grow the memory region which arises always if
       * there are no more tasks pending that might use the dependency tracking.
       * This happens in shouldGrowDependencyMemory().
       *
       * ## Thread safety
       *
       * No new tasks with dependencies can not be spawned while I grow the memory
       * region. This means we need a critical section in the routine around this
       * one: multiple threads can read, but if one decides to write, no other
       * thread should do anything related.
       *
       * ## Dependencies
       *
       * I originally had this idea that each task could increment/decrement a
       * counter, so I can check if tasks are still pending. If so, I'd increment.
       * But this does not work: If a task decrements the counter and signals
       * "I'm done", OpenMP might not have tidied up the underlying memory region.
       * So if I now free the memory (as I think no tasks are there anymore), this
       * might happen at exactly the point in time where OpenMP still updates its
       * out-dependencies. Therefore I loop over all potential in-dependencies
       * (many of them will be antidependencies) to ensure that OpenMP really has
       * done all of its work. A simple wait will btw not work: It only works on
       * directly children.
       *
       * ## Remark
       *
       * At this point, I do not use the routine. For some applications, it might
       * be reasonable to invoke it, but it has to happen outside.
       */
      void reserveDependencyTrackingMemory();

      /**
       * OpenMP-internal namespace
       *
       * I originally had some helper functions within the C++ code. However,
       * Peano implements this policy that all documentation should be in the
       * header (and the documentation extracts also only header info into the
       * generated HTML pages). Therefore, I decided to introduce this
       * internal namespace.
       *
       * The variables used internally by our mapping of Peano's tasking onto
       * OpenMP tasks still remain in the implementation file only.
       *
       *
       * ## Task dependencies
       *
       * Dependency tracking in OpenMP is organised via addresses. These
       * addresses are sole helpers, i.e. the underlying memory region doesn't
       * hold any data with semantics. However, it has to be owned by the
       * application code. To ensure this is the case, we allocate some chunk
       * only for the dependency tracking and hold its baseline address in this
       * variable. This chunk memory is held by the pointer
       * dependencyTrackingBaseAddress.
       *
       * The initial default size of this memory tracking is specified by
       * DefaultSizeOfMemoryRegionUsedForDependencyTracking. This is implicitly
       * the highest task number that we can handle. However, if the code
       * requests a higher number, then we pause and grow the memory region
       * before we continue.
       *
       * We maintain two integers, sizeOfMemoryRegionUsedForDependencyTracking
       * and maxSizeOfMemoryRegionUsedForDependencyTracking, to always keep
       * track how big the memory is.
       *
       *
       * ## Lesson learned: Not handling tasks
       *
       * I originally thought that we could refrain from always immediately
       * growing this memory dependency region. We could, in theory, look up
       * if we can handle a dependency (we do that anyway) and then execute
       * tasks immediately if we cannot map them onto an OpenMP dependency as
       * we don't have the memory reserved yet.
       *
       * Is a bad idea. The task fusion relies on working taskwaits to yield
       * correct results. That is, if a thread issues a taskwait on a specific
       * task, it has to trust the OpenMP layer that this task actually has
       * terminated. If the task is currently executed immediately by another
       * thread, as we don't have dependency memory, there's no way for the
       * taskwait to actually wait for the task to terminate. All tasks are
       * therefore mapped onto actual tasks as a matter of principle. If we
       * have to wait to get some memory allocations through before we issue
       * the task, then we wait.
       *
       *
       */
      namespace internal {
        using DependencyTrackingDataType = unsigned short int;

        /**
         * Translate task marker into string
         *
         * Used only for debugging reasons.
         */
        std::string OpenMP5TaskStatus(DependencyTrackingDataType);

        /**
         * Spawn task using OpenMP 6 syntax
         *
         * With OpenMP 6, we get transparent tasking. That is, we can have tasks with
         * dependencies which are not sibling tasks. Therefore, we can directly map
         * all dependencies on memory references in dependencyTrackingBaseAddress.
         *
         *
         * ## Assumptions
         *
         * This routine makes a couple of assumptions:
         *
         * - The caller has already incremented numberOfPendingTasks by one;
         * - The dependencyTrackingBaseAddress array hosts all indices specified
         *   within inDependencies and outDependency;
         * - The semaphore dependencyTrackingSemaphore is locked with a read lock.
         *
         *
         * ## OpenMP < 6
         *
         * If we use an "older" version of OpenMP, then the transparent keyword is
         * not supported. That means, the pragma with a transparent will create a
         * translation error or at least a warning. I therefore tried to embed the
         * function into a
         *
         * ~~~~~~~~~~~~~~~~~~~~~~~~~
         * if constexpr( tarch::multicore::omp::majorOpenMPVersion()>=6 ) {
         *    ...
         * }
         * ~~~~~~~~~~~~~~~~~~~~~~~~~
         *
         * section. Unfortunately, this does not help. C++ might ignore the if
         * branch, but the standard also says that the loop body has to be
         * syntactically correct, which is kind of violated. So you'll still get a
         * translation error/warning for this code if you use older OpenMP versions.
         * I therefore decided to remove the if.
         *
         *
         * @param inDependencies Sequence of task numbers which can be empty. We need
         *  a vector here (even if the udnerlying type is a set), as we use OpenMP's
         *  iterator.
         *
         * @param outDependencies Is either empty or contains exactly one element. As
         *  we use a vector, we can work with OpenMP's iterator supporting both an
         *  outgoing dependency and no dependencies at all.
         */
        void spawnTaskWithOpenMP6(
          tarch::multicore::Task*                           job,
          const std::vector<tarch::multicore::TaskNumber>&  inDependencies,
          const std::vector<tarch::multicore::TaskNumber>&  outDependency
        );

        void spawnTaskWithOpenMP5(
          tarch::multicore::Task*                           job,
          // @todo Muss ich aendern duerfen
          std::vector<tarch::multicore::TaskNumber>         inDependencies,
          const std::vector<tarch::multicore::TaskNumber>&  outDependency
        );

        /**
         * Reserve memory for dependency tracking
         *
         * OpenMP manages dependencies through memory addresses. However, to be able
         * to do so, the code has to own these addresses. This routine checks if the
         * space for the dependency tracking is big enough. If not, it grows it.
         *
         * We cannot grow the memory region while anybody still refers to them.
         * Therefore, the growing process first polls numberOfPendingTasks to be sure
         * that everybody has finished before we continue. Due to this polling and
         * the semaphores employed, this routine is thread-safe.
         */
        void reserveDependencyTrackingMemory( int taskNumber );
      }
    }
  }
}
