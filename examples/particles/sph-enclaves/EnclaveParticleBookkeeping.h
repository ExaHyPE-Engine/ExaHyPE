#ifndef _PEANO4_ENCLAVE_PARTICLE_BOOKKEEPING_H_
#define _PEANO4_ENCLAVE_PARTICLE_BOOKKEEPING_H_


#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "peano4/parallel/Tasks.h"
#include "peano4/datamanagement/VertexEnumerator.h"

#include <array>
#include <unordered_set>
#include <unordered_map>
#include <string>


namespace peano4 {
  template <class Particle>
  class EnclaveParticleBookkeeping;

  template <class Particle>
  class EnclaveParticleTask;
}


/**
 * Enclave Particle Bookkeeping
 *
 * Based on the ExaHyPE2 Enclave Bookkeeping but adapted for PIDT applications.
 */
template <class Particle>
class peano4::EnclaveParticleBookkeeping {
  private:
    static tarch::logging::Log _log;

    const static std::string MemoryAllocationsInLookupTableIdentifier;
    const static std::string LookupMissesIdentifier;

    /**
     * Plain set to record finished tasks.
     */
    std::unordered_set<int> _finishedTasks;

    std::unordered_map<int, int> _vertexTaskCounters;

    tarch::multicore::BooleanSemaphore  _tasksSemaphore;

    EnclaveParticleBookkeeping() = default;

  public:
    static constexpr int NoEnclaveTaskNumber = -1;
    static constexpr int NoVertexIdNumber = -1;

    static EnclaveParticleBookkeeping& getInstance();

    int registerVertex();

    /**
     * Spawn a new enclave task.
     *
     * Spawn EnclaveParticleTasks through this method to correctly/thread-safely
     * increment vertex counters.
     */
    void spawnTask(EnclaveParticleTask<Particle>* task, peano4::parallel::Tasks::TaskType priority, const std::string& location);

    /**
     * Called per cell during secondary traversal.
     *
     * Waits for the task spawned in the calling cell to complete, as well as
     * the tasks spawned in the (3^D) - 1 neighbouring cells.
     */
    void waitForTasksToTerminate(int taskNumber, const std::array<int, TwoPowerD>& vertexIds);

    /**
     * Called directly by EnclaveParticleTask.
     *
     * Correctly/thread-safely decrement vertex counters.
     */
    void finishedTask(int taskNumber, const std::array<int, TwoPowerD>& vertexIds);

    /**
     * For debugging only. This routine is not thread-safe.
     */
    void dumpStatistics();
};

#include "EnclaveParticleBookkeeping.cpph"

#endif
