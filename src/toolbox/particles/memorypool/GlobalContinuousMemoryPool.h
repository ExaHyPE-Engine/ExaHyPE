// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "tarch/multicore/BooleanSemaphore.h"


namespace toolbox {
  namespace particles {
    namespace memorypool {
      template <class T>
      class GlobalContinuousMemoryPool;
    } // namespace memorypool
  }   // namespace particles
} // namespace toolbox


/**
 * Memory pool offering continuous global memory for a particle species
 *
 * Consult the @ref toolbox_particles_memorypool "generic description of memory pools"
 * before you continue to read this documentation. This class is used by every
 * "vertex" object, i.e. each vertex holds one instance of
 * GlobalContinuousMemoryPool. It's field container is used to point to all the
 * particles scattered over the memory after we've created the particles.
 * However, whenever we call gather(), we take all of these particles from the
 * heap, put them into one continuous container and then make the pointers
 * within container point to those guys.
 *
 * Originally, I had planned to work with one big memory chunk indeed, and then
 * to subdivide this memory into chunks to serve the memory requests.
 * Unfortunately, such an approach is doomed to fail: We don't know the
 * particle distribution a priori. So we have to grow the memory dynamically.
 * However, once a vertex points to a continuous memory location for its
 * particle, these particles may not move under no circumstances. This
 * however is unavoidable if we want to work with one huge memory chunk. At
 * the same time, every gather() is guaranteed to return to continue(). So we
 * cannot return something like "sorry, wasn't able to reserve that continuous
 * chunk of memory". The only solution to this problem is, atm, to make the
 * global continuous memory model a strict extension of
 * VertexWiseContinousMemory:
 *
 * We hold a global memory pool with pages that we try to befill with the
 * memory requests. If this does not succeed, we fall back to
 * VertexWiseContinousMemory's strategy, i.e. hand out a new continuous piece
 * of memory on the heap. However, we learn from such mistakes. The class keeps
 * books of requests that it has not been able to host locally and then grows
 * its memory upon the next opportunity accordingly. Next time, it should be
 * able to serve all requests.
 *
 * The global memory is a static object of type GlobalMemory. It is a wrapper
 * around an std::vector which holds the data. So this vector can grow upon
 * demand. On top of this vector, we maintain a list of tuples (int,booleans).
 * They divide the memory into pages, whereas the first index defines the size
 * of the pages. Per page, we also take notes if this page is occupied at the
 * moment.
 *
 * Overall, this memory pool is more complex than the its vertex counterpart.
 * It also might consume slightly more memory in total, as it might
 * overestimate the total memory footprint. However, there are a few
 * advantages, too:
 *
 * - Memory is less scattered compared to other approaches, as we try to store
 *   everything in one huge chunk of memory.
 * - We don't need a lot of allocs and frees anymore, as most of the memory
 *   requests can be served from one huge chunk of memory.
 *
 * It is very important that you read through the @ref toolbox_particles_memorypool "implications for the particle
 * sorting" that arise if you use a global memory pool.
 */
template <class T>
class toolbox::particles::memorypool::GlobalContinuousMemoryPool {
public:
  typedef std::list<T*> Container;

  /**
   * List of pointers. If data is scattered, that's our "only" link to the
   * particles on the heap. Otherwise, it indexes a continuous sequence of
   * particles from one page.
   */
  Container container;

  GlobalContinuousMemoryPool();

  /**
   * Scatter the data
   *
   * If the data is not gathered yet, nothing is to be done. container
   * already points to particles all over the heap. However, if
   * _globalMemoryPage points to a valid page, we put all the particles
   * of this page onto the heap, make the entries of _container point
   * to these heap locations, and then mark the page as freed. Eventually,
   * we call the garbage collection.
   */
  void scatter();

  /**
   *
   */
  typename Container::iterator scatterAndUpdateIterator(const typename Container::iterator& p);

  /**
   * Gather the particle
   *
   * If we invoke this routine on a gathered memory pool, it becomes nop
   * (no operation).
   * Otherwise, the routine's implementation is rather straightforward:
   *
   * - Allocate one big chunk of memory
   * - Run over the list of pointers. This run-through has to be done with
   *   a reference, as we will change the pointers on our way. So we use
   *   auto& as type.
   * - Copy the current iterator element over into the big chunk of memory
   *   and increment the counter in there.
   * - Delete the original piece of data on the global heap.
   * - Make the list pointer point to the new fragment within the memory.
   *
   * Please note that it makes no sense to gather an empty set, so we
   * explicitly take that into account as well.
   */
  void gather();

  /**
   * Is the vertex data gathered
   *
   * This routine returns false if the underlying container is empty.
   */
  bool isGathered() const;

  /**
   * Replace particle
   *
   * Takes the particle identified via p and copies the content of newCopy
   * over.
   *
   * If our data is gathered, we copy stuff over. After that, newCopy is
   * deleted. If our data is scattered, we can just delete the original one
   * and push back the new copy.
   */
  void replace(typename Container::iterator p, T* newCopy);

  /**
   * Clear dataset tied to one vertex.
   *
   * This routine is used by shallow clears(). It does not actually free any
   * data, but basically just resets the local vertex and unties it to any
   * data it might reference to.
   */
  void clearAndReset();

  /**
   * Recommend complete scatter
   *
   * A memory pool can recommend a complete scatter. The memory pool should do
   * this if the memory is managed becomes too scattered, or any pre-allocated
   * memory region is too small.
   */
  static bool requestCompleteScatter();

private:
  static tarch::logging::Log _log;

  static constexpr int UndefinedMemoryPage = -1;

  /**
   * Represents the global memory
   */
  struct GlobalMemory {
    struct Page {
      int  startIndex;
      int  size;
      bool used;
    };
    std::vector<T>    data;
    std::vector<Page> pages;
    /**
     * Number of entries that we would have liked to have
     */
    int                                additionalEntriesRequested;
    tarch::multicore::BooleanSemaphore semaphore;

    /**
     * Take last page's start index, add its size and you get the result.
     */
    int totalUsedSize() const;

    GlobalMemory(int initialSize);

    /**
     * Add a new page of size size
     *
     * This routine is not thread-safe, i.e. we rely on the calling code to
     * have ensured that either the semaphore is locked or race conditions
     * are not happening anyway.
     */
    int addPage(int size);

    /**
     * Free a page
     *
     * Set the page pageNumber to unused (boolean flag). After that, study the
     * last page. If this one is empty - that is, if the current freePage() has
     * freed the last page - remove it completely. This is done iteratively, as
     * the last free could have "unlocked" a whole sequence of pages. By
     * deleting pages at the end, we give Peano the opportunity to create new,
     * larger or smaller pages.
     *
     * Finally, we study if all pages now have been freed. If this is the case,
     * we have the unique opportunity to grow the total memory using
     * information about the requests that we haven't been able to serve
     * before.
     */
    void freePage(int pageNumber);
  };

  /**
   * This class attribute represents the global data space for all of the
   * particles. I can initialise the global space with a certain size and
   * hence ensure that the first few particles fit in. However, it seems that
   * this is not clever: It is better to let the storage scheme use scattered
   * heap accesses in the first time step. After that, we know exactly how
   * much space we need and can allocate the heap accordingly.
   */
  static GlobalMemory _globalMemory;

  /**
   * Is nullptr as long as data is not gathered. We either store
   * this pointer or we make _globalMemoryPage hold something
   * meaningful.
   */
  T* _gatheredDataPointer;

  /**
   * Equals UndefinedPoolReference as long as data is not gathered.
   */
  int _globalMemoryPage;
};


#include "GlobalContinuousMemoryPool.cpph"
