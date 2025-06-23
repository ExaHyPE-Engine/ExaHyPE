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
    }
  }
}
