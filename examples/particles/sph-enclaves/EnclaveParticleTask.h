#ifndef _PEANO_ENCLAVE_PARTICLE_TASK_H_
#define _PEANO_ENCLAVE_PARTICLE_TASK_H_

#include "tarch/multicore/Tasks.h"
#include "peano4/datamanagement/VertexEnumerator.h"
#include "globaldata/Particle.h"
#include "vertexdata/VertexTaskCounter.h"

#include <functional>
#include <forward_list>

namespace peano4 {
  class EnclaveParticleTask;
  class EnclaveParticleBookkeeping;
}

class peano4::EnclaveParticleTask: public tarch::multicore::Task {
  private:
    friend class EnclaveParticleBookkeeping;

    static tarch::logging::Log _log;

    const std::forward_list<enclavesph::globaldata::Particle*> _activeParticles;
    const std::forward_list<enclavesph::globaldata::Particle*> _localParticles;
    const peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter> _taskCounters;

    std::function<void(
      const std::forward_list<enclavesph::globaldata::Particle*>& activeParticles,
      const std::forward_list<enclavesph::globaldata::Particle*>& localParticles
    )> _functor;

  public:
    EnclaveParticleTask(
      const std::forward_list<enclavesph::globaldata::Particle*>& activeParticles,
      const std::forward_list<enclavesph::globaldata::Particle*>& localParticles,
      const peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter>& taskCounters,
      std::function<void(
        const std::forward_list<enclavesph::globaldata::Particle*>& activeParticles,
        const std::forward_list<enclavesph::globaldata::Particle*>& localParticles
      )> functor
    );

    EnclaveParticleTask(const EnclaveParticleTask& other) = delete;
    EnclaveParticleTask(const EnclaveParticleTask&& other) = delete;

    virtual ~EnclaveParticleTask() = default;

    bool run() override;

    // nop
    void prefetch() override;
};

#endif
