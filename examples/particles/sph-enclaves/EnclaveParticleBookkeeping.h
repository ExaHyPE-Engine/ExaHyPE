#ifndef _PEANO4_ENCLAVE_PARTICLE_BOOKKEEPING_H_
#define _PEANO4_ENCLAVE_PARTICLE_BOOKKEEPING_H_


#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"
#include "peano4/parallel/Tasks.h"


#include <set>
#include <unordered_set>
#include <string>


namespace peano4 {
  class EnclaveParticleBookkeeping;
  class EnclaveParticleTask;
}


/**
 * Enclave bookkeeping
 *
 * The enclave bookkeeping is basically a big map which stores results of
 * enclave tasks.
 */
class peano4::EnclaveParticleBookkeeping {
  private:
    static tarch::logging::Log _log;

    const static std::string MemoryAllocationsInLookupTableIdentifier;
    const static std::string LookupMissesIdentifier;

    /**
     * Plain map onto ouput array. See lifecycle discussion of EnclaveParticleTask
     * for details.
     */
    std::unordered_set<int> _finishedTasks;

    tarch::multicore::BooleanSemaphore  _tasksSemaphore;

    EnclaveParticleBookkeeping() = default;

  public:
    static EnclaveParticleBookkeeping& getInstance();

    void spawnTask(EnclaveParticleTask* task, peano4::parallel::Tasks::TaskType priority, const std::string& location);

    /**
     * For debugging only. This routine is not thread-safe.
     */
    void dumpStatistics();

    void waitForTasksToTerminate(int number, const peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter>& taskCounters);

    /**
     * Called directly by EnclaveParticleTask.
     *
     */
    void finishedTask(int taskNumber, const peano4::datamanagement::VertexEnumerator<peanosph::enclavesph::vertexdata::VertexTaskCounter>& taskCounters);
};

#endif
